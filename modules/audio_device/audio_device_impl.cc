/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/audio_device/audio_device_impl.h"

#include "modules/audio_device/audio_device_config.h"
#include "modules/audio_device/audio_device_generic.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/refcount.h"
#include "rtc_base/refcountedobject.h"
#include "system_wrappers/include/metrics.h"

#if defined(_WIN32)
#if defined(WEBRTC_WINDOWS_CORE_AUDIO_BUILD)
#include "audio_device_core_win.h"
#endif
#elif defined(WEBRTC_ANDROID)
#include <stdlib.h>
#include "modules/audio_device/android/audio_device_template.h"
#include "modules/audio_device/android/audio_manager.h"
#include "modules/audio_device/android/audio_record_jni.h"
#include "modules/audio_device/android/audio_track_jni.h"
#include "modules/audio_device/android/opensles_player.h"
#include "modules/audio_device/android/opensles_recorder.h"
#elif defined(WEBRTC_LINUX)
#if defined(LINUX_ALSA)
#include "audio_device_alsa_linux.h"
#endif
#if defined(LINUX_PULSE)
#include "audio_device_pulse_linux.h"
#endif
#elif defined(WEBRTC_IOS)
#include "audio_device_ios.h"
#elif defined(WEBRTC_MAC)
#include "audio_device_mac.h"
#endif
#if defined(WEBRTC_DUMMY_FILE_DEVICES)
#include "modules/audio_device/dummy/file_audio_device_factory.h"
#endif
#include "modules/audio_device/dummy/audio_device_dummy.h"
#include "modules/audio_device/dummy/file_audio_device.h"

#define CHECKinitialized_() \
  {                         \
    if (!initialized_) {    \
      return -1;            \
    };                      \
  }

#define CHECKinitialized__BOOL() \
  {                              \
    if (!initialized_) {         \
      return false;              \
    };                           \
  }

namespace webrtc {

// static
// TODO(henrika): remove id parameter when all clients are updated.
rtc::scoped_refptr<AudioDeviceModule> AudioDeviceModule::Create(
    const int32_t id,
    const AudioLayer audio_layer) {
  LOG(INFO) << __FUNCTION__;
  // Create the generic reference counted (platform independent) implementation.
  rtc::scoped_refptr<AudioDeviceModuleImpl> audioDevice(
      new rtc::RefCountedObject<AudioDeviceModuleImpl>(audio_layer));

  // Ensure that the current platform is supported.
  if (audioDevice->CheckPlatform() == -1) {
    return nullptr;
  }

  // Create the platform-dependent implementation.
  if (audioDevice->CreatePlatformSpecificObjects() == -1) {
    return nullptr;
  }

  // Ensure that the generic audio buffer can communicate with the platform
  // specific parts.
  if (audioDevice->AttachAudioBuffer() == -1) {
    return nullptr;
  }

  return audioDevice;
}

AudioDeviceModuleImpl::AudioDeviceModuleImpl(const AudioLayer audioLayer)
    : audio_layer_(audioLayer) {
  LOG(INFO) << __FUNCTION__;
}

int32_t AudioDeviceModuleImpl::CheckPlatform() {
  LOG(INFO) << __FUNCTION__;
  // Ensure that the current platform is supported
  PlatformType platform(kPlatformNotSupported);
#if defined(_WIN32)
  platform = kPlatformWin32;
  LOG(INFO) << "current platform is Win32";
#elif defined(WEBRTC_ANDROID)
  platform = kPlatformAndroid;
  LOG(INFO) << "current platform is Android";
#elif defined(WEBRTC_LINUX)
  platform = kPlatformLinux;
  LOG(INFO) << "current platform is Linux";
#elif defined(WEBRTC_IOS)
  platform = kPlatformIOS;
  LOG(INFO) << "current platform is IOS";
#elif defined(WEBRTC_MAC)
  platform = kPlatformMac;
  LOG(INFO) << "current platform is Mac";
#endif
  if (platform == kPlatformNotSupported) {
    LOG(LERROR) << "current platform is not supported => this module will self "
                   "destruct!";
    return -1;
  }
  platform_type_ = platform;
  return 0;
}

int32_t AudioDeviceModuleImpl::CreatePlatformSpecificObjects() {
  LOG(INFO) << __FUNCTION__;
// Dummy ADM implementations if build flags are set.
#if defined(WEBRTC_DUMMY_AUDIO_BUILD)
  audio_device_.reset(new AudioDeviceDummy());
  LOG(INFO) << "Dummy Audio APIs will be utilized";
#elif defined(WEBRTC_DUMMY_FILE_DEVICES)
  audio_device_.reset(FileAudioDeviceFactory::CreateFileAudioDevice());
  if (audio_device_) {
    LOG(INFO) << "Will use file-playing dummy device.";
  } else {
    // Create a dummy device instead.
    audio_device_.reset(new AudioDeviceDummy());
    LOG(INFO) << "Dummy Audio APIs will be utilized";
  }

// Real (non-dummy) ADM implementations.
#else
  AudioLayer audio_layer(PlatformAudioLayer());
// Windows ADM implementation.
#if defined(WEBRTC_WINDOWS_CORE_AUDIO_BUILD)
  if ((audio_layer == kWindowsCoreAudio) ||
      (audio_layer == kPlatformDefaultAudio)) {
    LOG(INFO) << "Attempting to use the Windows Core Audio APIs...";
    if (AudioDeviceWindowsCore::CoreAudioIsSupported()) {
      audio_device_.reset(new AudioDeviceWindowsCore());
      LOG(INFO) << "Windows Core Audio APIs will be utilized";
    }
  }
#endif  // defined(WEBRTC_WINDOWS_CORE_AUDIO_BUILD)

#if defined(WEBRTC_ANDROID)
  // Create an Android audio manager.
  audio_manager_android_.reset(new AudioManager());
  // Select best possible combination of audio layers.
  if (audio_layer == kPlatformDefaultAudio) {
    if (audio_manager_android_->IsLowLatencyPlayoutSupported() &&
        audio_manager_android_->IsLowLatencyRecordSupported()) {
      // Use OpenSL ES for both playout and recording.
      audio_layer = kAndroidOpenSLESAudio;
    } else if (audio_manager_android_->IsLowLatencyPlayoutSupported() &&
               !audio_manager_android_->IsLowLatencyRecordSupported()) {
      // Use OpenSL ES for output on devices that only supports the
      // low-latency output audio path.
      audio_layer = kAndroidJavaInputAndOpenSLESOutputAudio;
    } else {
      // Use Java-based audio in both directions when low-latency output is
      // not supported.
      audio_layer = kAndroidJavaAudio;
    }
  }
  AudioManager* audio_manager = audio_manager_android_.get();
  if (audio_layer == kAndroidJavaAudio) {
    // Java audio for both input and output audio.
    audio_device_.reset(new AudioDeviceTemplate<AudioRecordJni, AudioTrackJni>(
        audio_layer, audio_manager));
  } else if (audio_layer == kAndroidOpenSLESAudio) {
    // OpenSL ES based audio for both input and output audio.
    audio_device_.reset(
        new AudioDeviceTemplate<OpenSLESRecorder, OpenSLESPlayer>(
            audio_layer, audio_manager));
  } else if (audio_layer == kAndroidJavaInputAndOpenSLESOutputAudio) {
    // Java audio for input and OpenSL ES for output audio (i.e. mixed APIs).
    // This combination provides low-latency output audio and at the same
    // time support for HW AEC using the AudioRecord Java API.
    audio_device_.reset(new AudioDeviceTemplate<AudioRecordJni, OpenSLESPlayer>(
        audio_layer, audio_manager));
  } else {
    // Invalid audio layer.
    audio_device_.reset(nullptr);
  }
// END #if defined(WEBRTC_ANDROID)

// Linux ADM implementation.
#elif defined(WEBRTC_LINUX)
  if ((audio_layer == kLinuxPulseAudio) ||
      (audio_layer == kPlatformDefaultAudio)) {
#if defined(LINUX_PULSE)
    LOG(INFO) << "Attempting to use Linux PulseAudio APIs...";
    // Linux PulseAudio implementation.
    audio_device_.reset(new AudioDeviceLinuxPulse());
    LOG(INFO) << "Linux PulseAudio APIs will be utilized";
#endif
#if defined(LINUX_PULSE)
#endif
  } else if (audio_layer == kLinuxAlsaAudio) {
#if defined(LINUX_ALSA)
    // Linux ALSA implementation.
    audio_device_.reset(new AudioDeviceLinuxALSA());
    LOG(INFO) << "Linux ALSA APIs will be utilized.";
#endif
  }
#endif  // #if defined(WEBRTC_LINUX)

// iOS ADM implementation.
#if defined(WEBRTC_IOS)
  if (audio_layer == kPlatformDefaultAudio) {
    audio_device_.reset(new AudioDeviceIOS());
    LOG(INFO) << "iPhone Audio APIs will be utilized.";
  }
// END #if defined(WEBRTC_IOS)

// Mac OS X ADM implementation.
#elif defined(WEBRTC_MAC)
  if (audio_layer == kPlatformDefaultAudio) {
    audio_device_.reset(new AudioDeviceMac());
    LOG(INFO) << "Mac OS X Audio APIs will be utilized.";
  }
#endif  // WEBRTC_MAC

  // Dummy ADM implementation.
  if (audio_layer == kDummyAudio) {
    audio_device_.reset(new AudioDeviceDummy());
    LOG(INFO) << "Dummy Audio APIs will be utilized.";
  }
#endif  // if defined(WEBRTC_DUMMY_AUDIO_BUILD)

  if (!audio_device_) {
    LOG(LS_ERROR)
        << "Failed to create the platform specific ADM implementation.";
    return -1;
  }
  return 0;
}

int32_t AudioDeviceModuleImpl::AttachAudioBuffer() {
  LOG(INFO) << __FUNCTION__;
  audio_device_->AttachAudioBuffer(&audio_device_buffer_);
  return 0;
}

AudioDeviceModuleImpl::~AudioDeviceModuleImpl() {
  LOG(INFO) << __FUNCTION__;
}

int32_t AudioDeviceModuleImpl::ActiveAudioLayer(AudioLayer* audioLayer) const {
  LOG(INFO) << __FUNCTION__;
  AudioLayer activeAudio;
  if (audio_device_->ActiveAudioLayer(activeAudio) == -1) {
    return -1;
  }
  *audioLayer = activeAudio;
  return 0;
}

int32_t AudioDeviceModuleImpl::Init() {
  LOG(INFO) << __FUNCTION__;
  if (initialized_)
    return 0;
  RTC_CHECK(audio_device_);
  AudioDeviceGeneric::InitStatus status = audio_device_->Init();
  RTC_HISTOGRAM_ENUMERATION(
      "WebRTC.Audio.InitializationResult", static_cast<int>(status),
      static_cast<int>(AudioDeviceGeneric::InitStatus::NUM_STATUSES));
  if (status != AudioDeviceGeneric::InitStatus::OK) {
    LOG(LS_ERROR) << "Audio device initialization failed.";
    return -1;
  }
  initialized_ = true;
  return 0;
}

int32_t AudioDeviceModuleImpl::Terminate() {
  LOG(INFO) << __FUNCTION__;
  if (!initialized_)
    return 0;
  if (audio_device_->Terminate() == -1) {
    return -1;
  }
  initialized_ = false;
  return 0;
}

bool AudioDeviceModuleImpl::Initialized() const {
  LOG(INFO) << __FUNCTION__ << ": " << initialized_;
  return initialized_;
}

int32_t AudioDeviceModuleImpl::InitSpeaker() {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  return audio_device_->InitSpeaker();
}

int32_t AudioDeviceModuleImpl::InitMicrophone() {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  return audio_device_->InitMicrophone();
}

int32_t AudioDeviceModuleImpl::SpeakerVolumeIsAvailable(bool* available) {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  bool isAvailable = false;
  if (audio_device_->SpeakerVolumeIsAvailable(isAvailable) == -1) {
    return -1;
  }
  *available = isAvailable;
  LOG(INFO) << "output: " << isAvailable;
  return 0;
}

int32_t AudioDeviceModuleImpl::SetSpeakerVolume(uint32_t volume) {
  LOG(INFO) << __FUNCTION__ << "(" << volume << ")";
  CHECKinitialized_();
  return audio_device_->SetSpeakerVolume(volume);
}

int32_t AudioDeviceModuleImpl::SpeakerVolume(uint32_t* volume) const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  uint32_t level = 0;
  if (audio_device_->SpeakerVolume(level) == -1) {
    return -1;
  }
  *volume = level;
  LOG(INFO) << "output: " << *volume;
  return 0;
}

bool AudioDeviceModuleImpl::SpeakerIsInitialized() const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized__BOOL();
  bool isInitialized = audio_device_->SpeakerIsInitialized();
  LOG(INFO) << "output: " << isInitialized;
  return isInitialized;
}

bool AudioDeviceModuleImpl::MicrophoneIsInitialized() const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized__BOOL();
  bool isInitialized = audio_device_->MicrophoneIsInitialized();
  LOG(INFO) << "output: " << isInitialized;
  return isInitialized;
}

int32_t AudioDeviceModuleImpl::MaxSpeakerVolume(uint32_t* maxVolume) const {
  CHECKinitialized_();
  uint32_t maxVol = 0;
  if (audio_device_->MaxSpeakerVolume(maxVol) == -1) {
    return -1;
  }
  *maxVolume = maxVol;
  return 0;
}

int32_t AudioDeviceModuleImpl::MinSpeakerVolume(uint32_t* minVolume) const {
  CHECKinitialized_();
  uint32_t minVol = 0;
  if (audio_device_->MinSpeakerVolume(minVol) == -1) {
    return -1;
  }
  *minVolume = minVol;
  return 0;
}

int32_t AudioDeviceModuleImpl::SpeakerMuteIsAvailable(bool* available) {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  bool isAvailable  = false;
  if (audio_device_->SpeakerMuteIsAvailable(isAvailable) == -1) {
    return -1;
  }
  *available = isAvailable;
  LOG(INFO) << "output: " << isAvailable;
  return 0;
}

int32_t AudioDeviceModuleImpl::SetSpeakerMute(bool enable) {
  LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
  CHECKinitialized_();
  return audio_device_->SetSpeakerMute(enable);
}

int32_t AudioDeviceModuleImpl::SpeakerMute(bool* enabled) const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  bool muted = false;
  if (audio_device_->SpeakerMute(muted) == -1) {
    return -1;
  }
  *enabled = muted;
  LOG(INFO) << "output: " << muted;
  return 0;
}

int32_t AudioDeviceModuleImpl::MicrophoneMuteIsAvailable(bool* available) {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  bool isAvailable = false;
  if (audio_device_->MicrophoneMuteIsAvailable(isAvailable) == -1) {
    return -1;
  }
  *available = isAvailable;
  LOG(INFO) << "output: " << isAvailable;
  return 0;
}

int32_t AudioDeviceModuleImpl::SetMicrophoneMute(bool enable) {
  LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
  CHECKinitialized_();
  return (audio_device_->SetMicrophoneMute(enable));
}

int32_t AudioDeviceModuleImpl::MicrophoneMute(bool* enabled) const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  bool muted = false;
  if (audio_device_->MicrophoneMute(muted) == -1) {
    return -1;
  }
  *enabled = muted;
  LOG(INFO) << "output: " << muted;
  return 0;
}

int32_t AudioDeviceModuleImpl::MicrophoneVolumeIsAvailable(bool* available) {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  bool isAvailable = false;
  if (audio_device_->MicrophoneVolumeIsAvailable(isAvailable) == -1) {
    return -1;
  }
  *available = isAvailable;
  LOG(INFO) << "output: " << isAvailable;
  return 0;
}

int32_t AudioDeviceModuleImpl::SetMicrophoneVolume(uint32_t volume) {
  LOG(INFO) << __FUNCTION__ << "(" << volume << ")";
  CHECKinitialized_();
  return (audio_device_->SetMicrophoneVolume(volume));
}

int32_t AudioDeviceModuleImpl::MicrophoneVolume(uint32_t* volume) const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  uint32_t level = 0;
  if (audio_device_->MicrophoneVolume(level) == -1) {
    return -1;
  }
  *volume = level;
  LOG(INFO) << "output: " << *volume;
  return 0;
}

int32_t AudioDeviceModuleImpl::StereoRecordingIsAvailable(
    bool* available) const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  bool isAvailable = false;
  if (audio_device_->StereoRecordingIsAvailable(isAvailable) == -1) {
    return -1;
  }
  *available = isAvailable;
  LOG(INFO) << "output: " << isAvailable;
  return 0;
}

int32_t AudioDeviceModuleImpl::SetStereoRecording(bool enable) {
  LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
  CHECKinitialized_();
  if (audio_device_->RecordingIsInitialized()) {
    LOG(WARNING) << "recording in stereo is not supported";
    return -1;
  }
  if (audio_device_->SetStereoRecording(enable) == -1) {
    LOG(WARNING) << "failed to change stereo recording";
    return -1;
  }
  int8_t nChannels(1);
  if (enable) {
    nChannels = 2;
  }
  audio_device_buffer_.SetRecordingChannels(nChannels);
  return 0;
}

int32_t AudioDeviceModuleImpl::StereoRecording(bool* enabled) const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  bool stereo = false;
  if (audio_device_->StereoRecording(stereo) == -1) {
    return -1;
  }
  *enabled = stereo;
  LOG(INFO) << "output: " << stereo;
  return 0;
}

int32_t AudioDeviceModuleImpl::SetRecordingChannel(const ChannelType channel) {
  if (channel == kChannelBoth) {
    LOG(INFO) << __FUNCTION__ << "(both)";
  } else if (channel == kChannelLeft) {
    LOG(INFO) << __FUNCTION__ << "(left)";
  } else {
    LOG(INFO) << __FUNCTION__ << "(right)";
  }
  CHECKinitialized_();
  bool stereo = false;
  if (audio_device_->StereoRecording(stereo) == -1) {
    LOG(WARNING) << "recording in stereo is not supported";
    return -1;
  }
  return audio_device_buffer_.SetRecordingChannel(channel);
}

int32_t AudioDeviceModuleImpl::RecordingChannel(ChannelType* channel) const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  ChannelType chType;
  if (audio_device_buffer_.RecordingChannel(chType) == -1) {
    return -1;
  }
  *channel = chType;
  if (*channel == kChannelBoth) {
    LOG(INFO) << "output: both";
  } else if (*channel == kChannelLeft) {
    LOG(INFO) << "output: left";
  } else {
    LOG(INFO) << "output: right";
  }
  return 0;
}

int32_t AudioDeviceModuleImpl::StereoPlayoutIsAvailable(bool* available) const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  bool isAvailable = false;
  if (audio_device_->StereoPlayoutIsAvailable(isAvailable) == -1) {
    return -1;
  }
  *available = isAvailable;
  LOG(INFO) << "output: " << isAvailable;
  return 0;
}

int32_t AudioDeviceModuleImpl::SetStereoPlayout(bool enable) {
  LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
  CHECKinitialized_();
  if (audio_device_->PlayoutIsInitialized()) {
    LOG(LERROR)
        << "unable to set stereo mode while playing side is initialized";
    return -1;
  }
  if (audio_device_->SetStereoPlayout(enable)) {
    LOG(WARNING) << "stereo playout is not supported";
    return -1;
  }
  int8_t nChannels(1);
  if (enable) {
    nChannels = 2;
  }
  audio_device_buffer_.SetPlayoutChannels(nChannels);
  return 0;
}

int32_t AudioDeviceModuleImpl::StereoPlayout(bool* enabled) const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  bool stereo = false;
  if (audio_device_->StereoPlayout(stereo) == -1) {
    return -1;
  }
  *enabled = stereo;
  LOG(INFO) << "output: " << stereo;
  return 0;
}

int32_t AudioDeviceModuleImpl::SetAGC(bool enable) {
  LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
  CHECKinitialized_();
  return (audio_device_->SetAGC(enable));
}

bool AudioDeviceModuleImpl::AGC() const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized__BOOL();
  return audio_device_->AGC();
}

int32_t AudioDeviceModuleImpl::PlayoutIsAvailable(bool* available) {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  bool isAvailable = false;
  if (audio_device_->PlayoutIsAvailable(isAvailable) == -1) {
    return -1;
  }
  *available = isAvailable;
  LOG(INFO) << "output: " << isAvailable;
  return 0;
}

int32_t AudioDeviceModuleImpl::RecordingIsAvailable(bool* available) {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  bool isAvailable = false;
  if (audio_device_->RecordingIsAvailable(isAvailable) == -1) {
    return -1;
  }
  *available = isAvailable;
  LOG(INFO) << "output: " << isAvailable;
  return 0;
}

int32_t AudioDeviceModuleImpl::MaxMicrophoneVolume(uint32_t* maxVolume) const {
  CHECKinitialized_();
  uint32_t maxVol(0);
  if (audio_device_->MaxMicrophoneVolume(maxVol) == -1) {
    return -1;
  }
  *maxVolume = maxVol;
  return 0;
}

int32_t AudioDeviceModuleImpl::MinMicrophoneVolume(uint32_t* minVolume) const {
  CHECKinitialized_();
  uint32_t minVol(0);
  if (audio_device_->MinMicrophoneVolume(minVol) == -1) {
    return -1;
  }
  *minVolume = minVol;
  return 0;
}

int16_t AudioDeviceModuleImpl::PlayoutDevices() {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  uint16_t nPlayoutDevices = audio_device_->PlayoutDevices();
  LOG(INFO) << "output: " << nPlayoutDevices;
  return (int16_t)(nPlayoutDevices);
}

int32_t AudioDeviceModuleImpl::SetPlayoutDevice(uint16_t index) {
  LOG(INFO) << __FUNCTION__ << "(" << index << ")";
  CHECKinitialized_();
  return audio_device_->SetPlayoutDevice(index);
}

int32_t AudioDeviceModuleImpl::SetPlayoutDevice(WindowsDeviceType device) {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  return audio_device_->SetPlayoutDevice(device);
}

int32_t AudioDeviceModuleImpl::PlayoutDeviceName(
    uint16_t index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize]) {
  LOG(INFO) << __FUNCTION__ << "(" << index << ", ...)";
  CHECKinitialized_();
  if (name == NULL) {
    return -1;
  }
  if (audio_device_->PlayoutDeviceName(index, name, guid) == -1) {
    return -1;
  }
  if (name != NULL) {
    LOG(INFO) << "output: name = " << name;
  }
  if (guid != NULL) {
    LOG(INFO) << "output: guid = " << guid;
  }
  return 0;
}

int32_t AudioDeviceModuleImpl::RecordingDeviceName(
    uint16_t index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize]) {
  LOG(INFO) << __FUNCTION__ << "(" << index << ", ...)";
  CHECKinitialized_();
  if (name == NULL) {
    return -1;
  }
  if (audio_device_->RecordingDeviceName(index, name, guid) == -1) {
    return -1;
  }
  if (name != NULL) {
    LOG(INFO) << "output: name = " << name;
  }
  if (guid != NULL) {
    LOG(INFO) << "output: guid = " << guid;
  }
  return 0;
}

int16_t AudioDeviceModuleImpl::RecordingDevices() {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  uint16_t nRecordingDevices = audio_device_->RecordingDevices();
  LOG(INFO) << "output: " << nRecordingDevices;
  return (int16_t)nRecordingDevices;
}

int32_t AudioDeviceModuleImpl::SetRecordingDevice(uint16_t index) {
  LOG(INFO) << __FUNCTION__ << "(" << index << ")";
  CHECKinitialized_();
  return audio_device_->SetRecordingDevice(index);
}

int32_t AudioDeviceModuleImpl::SetRecordingDevice(WindowsDeviceType device) {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  return audio_device_->SetRecordingDevice(device);
}

int32_t AudioDeviceModuleImpl::InitPlayout() {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  if (PlayoutIsInitialized()) {
    return 0;
  }
  int32_t result = audio_device_->InitPlayout();
  LOG(INFO) << "output: " << result;
  RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.InitPlayoutSuccess",
                        static_cast<int>(result == 0));
  return result;
}

int32_t AudioDeviceModuleImpl::InitRecording() {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  if (RecordingIsInitialized()) {
    return 0;
  }
  int32_t result = audio_device_->InitRecording();
  LOG(INFO) << "output: " << result;
  RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.InitRecordingSuccess",
                        static_cast<int>(result == 0));
  return result;
}

bool AudioDeviceModuleImpl::PlayoutIsInitialized() const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized__BOOL();
  return audio_device_->PlayoutIsInitialized();
}

bool AudioDeviceModuleImpl::RecordingIsInitialized() const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized__BOOL();
  return audio_device_->RecordingIsInitialized();
}

int32_t AudioDeviceModuleImpl::StartPlayout() {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  if (Playing()) {
    return 0;
  }
  audio_device_buffer_.StartPlayout();
  int32_t result = audio_device_->StartPlayout();
  LOG(INFO) << "output: " << result;
  RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StartPlayoutSuccess",
                        static_cast<int>(result == 0));
  return result;
}

int32_t AudioDeviceModuleImpl::StopPlayout() {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  int32_t result = audio_device_->StopPlayout();
  audio_device_buffer_.StopPlayout();
  LOG(INFO) << "output: " << result;
  RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StopPlayoutSuccess",
                        static_cast<int>(result == 0));
  return result;
}

bool AudioDeviceModuleImpl::Playing() const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized__BOOL();
  return audio_device_->Playing();
}

int32_t AudioDeviceModuleImpl::StartRecording() {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  if (Recording()) {
    return 0;
  }
  audio_device_buffer_.StartRecording();
  int32_t result = audio_device_->StartRecording();
  LOG(INFO) << "output: " << result;
  RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StartRecordingSuccess",
                        static_cast<int>(result == 0));
  return result;
}

int32_t AudioDeviceModuleImpl::StopRecording() {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  int32_t result = audio_device_->StopRecording();
  audio_device_buffer_.StopRecording();
  LOG(INFO) << "output: " << result;
  RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StopRecordingSuccess",
                        static_cast<int>(result == 0));
  return result;
}

bool AudioDeviceModuleImpl::Recording() const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized__BOOL();
  return audio_device_->Recording();
}

int32_t AudioDeviceModuleImpl::RegisterAudioCallback(
    AudioTransport* audioCallback) {
  LOG(INFO) << __FUNCTION__;
  return audio_device_buffer_.RegisterAudioCallback(audioCallback);
}

int32_t AudioDeviceModuleImpl::PlayoutDelay(uint16_t* delayMS) const {
  CHECKinitialized_();
  uint16_t delay = 0;
  if (audio_device_->PlayoutDelay(delay) == -1) {
    LOG(LERROR) << "failed to retrieve the playout delay";
    return -1;
  }
  *delayMS = delay;
  return 0;
}

int32_t AudioDeviceModuleImpl::SetRecordingSampleRate(
    const uint32_t samplesPerSec) {
  LOG(INFO) << __FUNCTION__ << "(" << samplesPerSec << ")";
  CHECKinitialized_();
  if (audio_device_->SetRecordingSampleRate(samplesPerSec) != 0) {
    return -1;
  }
  return 0;
}

int32_t AudioDeviceModuleImpl::RecordingSampleRate(
    uint32_t* samplesPerSec) const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  int32_t sampleRate = audio_device_buffer_.RecordingSampleRate();
  if (sampleRate == -1) {
    LOG(LERROR) << "failed to retrieve the sample rate";
    return -1;
  }
  *samplesPerSec = sampleRate;
  LOG(INFO) << "output: " << *samplesPerSec;
  return 0;
}

int32_t AudioDeviceModuleImpl::SetPlayoutSampleRate(
    const uint32_t samplesPerSec) {
  LOG(INFO) << __FUNCTION__ << "(" << samplesPerSec << ")";
  CHECKinitialized_();
  if (audio_device_->SetPlayoutSampleRate(samplesPerSec) != 0) {
    return -1;
  }
  return 0;
}

int32_t AudioDeviceModuleImpl::PlayoutSampleRate(
    uint32_t* samplesPerSec) const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  int32_t sampleRate = audio_device_buffer_.PlayoutSampleRate();
  if (sampleRate == -1) {
    LOG(LERROR) << "failed to retrieve the sample rate";
    return -1;
  }
  *samplesPerSec = sampleRate;
  LOG(INFO) << "output: " << *samplesPerSec;
  return 0;
}

int32_t AudioDeviceModuleImpl::SetLoudspeakerStatus(bool enable) {
  LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
  CHECKinitialized_();
  if (audio_device_->SetLoudspeakerStatus(enable) != 0) {
    return -1;
  }
  return 0;
}

int32_t AudioDeviceModuleImpl::GetLoudspeakerStatus(bool* enabled) const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized_();
  int32_t ok = 0;
  if (audio_device_->GetLoudspeakerStatus(*enabled) != 0) {
    ok = -1;
  }
  LOG(INFO) << "output: " << ok;
  return ok;
}

bool AudioDeviceModuleImpl::BuiltInAECIsAvailable() const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized__BOOL();
  bool isAvailable = audio_device_->BuiltInAECIsAvailable();
  LOG(INFO) << "output: " << isAvailable;
  return isAvailable;
}

int32_t AudioDeviceModuleImpl::EnableBuiltInAEC(bool enable) {
  LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
  CHECKinitialized_();
  int32_t ok = audio_device_->EnableBuiltInAEC(enable);
  LOG(INFO) << "output: " << ok;
  return ok;
}

bool AudioDeviceModuleImpl::BuiltInAGCIsAvailable() const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized__BOOL();
  bool isAvailable = audio_device_->BuiltInAGCIsAvailable();
  LOG(INFO) << "output: " << isAvailable;
  return isAvailable;
}

int32_t AudioDeviceModuleImpl::EnableBuiltInAGC(bool enable) {
  LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
  CHECKinitialized_();
  int32_t ok = audio_device_->EnableBuiltInAGC(enable);
  LOG(INFO) << "output: " << ok;
  return ok;
}

bool AudioDeviceModuleImpl::BuiltInNSIsAvailable() const {
  LOG(INFO) << __FUNCTION__;
  CHECKinitialized__BOOL();
  bool isAvailable = audio_device_->BuiltInNSIsAvailable();
  LOG(INFO) << "output: " << isAvailable;
  return isAvailable;
}

int32_t AudioDeviceModuleImpl::EnableBuiltInNS(bool enable) {
  LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
  CHECKinitialized_();
  int32_t ok = audio_device_->EnableBuiltInNS(enable);
  LOG(INFO) << "output: " << ok;
  return ok;
}

#if defined(WEBRTC_IOS)
int AudioDeviceModuleImpl::GetPlayoutAudioParameters(
    AudioParameters* params) const {
  LOG(INFO) << __FUNCTION__;
  int r = audio_device_->GetPlayoutAudioParameters(params);
  LOG(INFO) << "output: " << r;
  return r;
}

int AudioDeviceModuleImpl::GetRecordAudioParameters(
    AudioParameters* params) const {
  LOG(INFO) << __FUNCTION__;
  int r = audio_device_->GetRecordAudioParameters(params);
  LOG(INFO) << "output: " << r;
  return r;
}
#endif  // WEBRTC_IOS

AudioDeviceModuleImpl::PlatformType AudioDeviceModuleImpl::Platform() const {
  LOG(INFO) << __FUNCTION__;
  return platform_type_;
}

AudioDeviceModule::AudioLayer AudioDeviceModuleImpl::PlatformAudioLayer()
    const {
  LOG(INFO) << __FUNCTION__;
  return audio_layer_;
}

}  // namespace webrtc
