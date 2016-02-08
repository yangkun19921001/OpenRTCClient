/*
 *  Copyright (c) 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/media/devices/macdevicemanager.h"

#include <CoreAudio/CoreAudio.h>
#include <QuickTime/QuickTime.h>

#include "webrtc/base/logging.h"
#include "webrtc/base/stringutils.h"
#include "webrtc/base/thread.h"
#include "webrtc/media/base/mediacommon.h"

class DeviceWatcherImpl;

namespace cricket {

DeviceManagerInterface* DeviceManagerFactory::Create() {
  return new MacDeviceManager();
}

class MacDeviceWatcher : public DeviceWatcher {
 public:
  explicit MacDeviceWatcher(DeviceManagerInterface* dm);
  virtual ~MacDeviceWatcher();
  virtual bool Start();
  virtual void Stop();

 private:
  DeviceManagerInterface* manager_;
  DeviceWatcherImpl* impl_;
};

static const char* kFilteredAudioDevicesName[] = {
    NULL,
};
// TODO(tommyw): Try to get hold of a copy of Final Cut to understand why we
//               crash while scanning their components on OS X.
static const char* const kFilteredVideoDevicesName[] =  {
    "DVCPRO HD",               // Final cut
    "Sonix SN9C201p",          // Crashes in OpenAComponent and CloseComponent
    NULL,
};
static const UInt32 kAudioDeviceNameLength = 64;
// Obj-C functions defined in macdevicemanagermm.mm
// TODO(ronghuawu): have a shared header for these function defines.
extern DeviceWatcherImpl* CreateDeviceWatcherCallback(
    DeviceManagerInterface* dm);
extern void ReleaseDeviceWatcherCallback(DeviceWatcherImpl* impl);
extern bool GetAVFoundationVideoDevices(std::vector<Device>* out);
static bool GetAudioDeviceIDs(bool inputs, std::vector<AudioDeviceID>* out);
static bool GetAudioDeviceName(AudioDeviceID id, bool input, std::string* out);

MacDeviceManager::MacDeviceManager() {
  set_watcher(new MacDeviceWatcher(this));
}

MacDeviceManager::~MacDeviceManager() {
}

bool MacDeviceManager::GetVideoCaptureDevices(std::vector<Device>* devices) {
  devices->clear();
  if (!GetAVFoundationVideoDevices(devices)) {
    return false;
  }
  return FilterDevices(devices, kFilteredVideoDevicesName);
}

bool MacDeviceManager::GetAudioDevices(bool input,
                                       std::vector<Device>* devs) {
  devs->clear();
  std::vector<AudioDeviceID> dev_ids;
  bool ret = GetAudioDeviceIDs(input, &dev_ids);
  if (!ret) {
    return false;
  }
  for (size_t i = 0; i < dev_ids.size(); ++i) {
    std::string name;
    if (GetAudioDeviceName(dev_ids[i], input, &name)) {
      devs->push_back(Device(name, dev_ids[i]));
    }
  }
  return FilterDevices(devs, kFilteredAudioDevicesName);
}

static bool GetAudioDeviceIDs(bool input,
                              std::vector<AudioDeviceID>* out_dev_ids) {
  UInt32 propsize;
  OSErr err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,
                                           &propsize, NULL);
  if (0 != err) {
    LOG(LS_ERROR) << "Couldn't get information about property, "
                  << "so no device list acquired.";
    return false;
  }

  size_t num_devices = propsize / sizeof(AudioDeviceID);
  rtc::scoped_ptr<AudioDeviceID[]> device_ids(
      new AudioDeviceID[num_devices]);

  err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,
                                 &propsize, device_ids.get());
  if (0 != err) {
    LOG(LS_ERROR) << "Failed to get device ids, "
                  << "so no device listing acquired.";
    return false;
  }

  for (size_t i = 0; i < num_devices; ++i) {
    AudioDeviceID an_id = device_ids[i];
    // find out the number of channels for this direction
    // (input/output) on this device -
    // we'll ignore anything with no channels.
    err = AudioDeviceGetPropertyInfo(an_id, 0, input,
                                     kAudioDevicePropertyStreams,
                                     &propsize, NULL);
    if (0 == err) {
      unsigned num_channels = propsize / sizeof(AudioStreamID);
      if (0 < num_channels) {
        out_dev_ids->push_back(an_id);
      }
    } else {
      LOG(LS_ERROR) << "No property info for stream property for device id "
                    << an_id << "(is_input == " << input
                    << "), so not including it in the list.";
    }
  }

  return true;
}

static bool GetAudioDeviceName(AudioDeviceID id,
                               bool input,
                               std::string* out_name) {
  UInt32 nameLength = kAudioDeviceNameLength;
  char name[kAudioDeviceNameLength + 1];
  OSErr err = AudioDeviceGetProperty(id, 0, input,
                                     kAudioDevicePropertyDeviceName,
                                     &nameLength, name);
  if (0 != err) {
    LOG(LS_ERROR) << "No name acquired for device id " << id;
    return false;
  }

  *out_name = name;
  return true;
}

MacDeviceWatcher::MacDeviceWatcher(DeviceManagerInterface* manager)
    : DeviceWatcher(manager),
      manager_(manager),
      impl_(NULL) {
}

MacDeviceWatcher::~MacDeviceWatcher() {
}

bool MacDeviceWatcher::Start() {
  if (!impl_) {
    impl_ = CreateDeviceWatcherCallback(manager_);
  }
  return impl_ != NULL;
}

void MacDeviceWatcher::Stop() {
  if (impl_) {
    ReleaseDeviceWatcherCallback(impl_);
    impl_ = NULL;
  }
}

};  // namespace cricket
