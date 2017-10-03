/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef LOGGING_RTC_EVENT_LOG_RTC_EVENT_LOG_H_
#define LOGGING_RTC_EVENT_LOG_RTC_EVENT_LOG_H_

#include <memory>
#include <string>

#include "api/array_view.h"
// TODO(eladalon): Get rid of this later in the CL-stack.
#include "api/rtpparameters.h"
#include "common_types.h"  // NOLINT(build/include)
// TODO(eladalon): This is here because of ProbeFailureReason; remove this
// dependency along with the deprecated LogProbeResultFailure().
#include "logging/rtc_event_log/events/rtc_event_probe_result_failure.h"
// TODO(eladalon): Remove this in an upcoming CL, that will modularize the
// log output into its own class.
#include "rtc_base/platform_file.h"

namespace webrtc {

namespace rtclog {
class EventStream;  // Storage class automatically generated from protobuf.
// TODO(eladalon): Get rid of this when deprecated methods are removed.
struct StreamConfig;
}  // namespace rtclog

class Clock;
// TODO(eladalon): The following may be removed when the deprecated methods
// are removed.
struct AudioEncoderRuntimeConfig;
class RtpPacketReceived;
class RtpPacketToSend;
enum class BandwidthUsage;
enum PacketDirection { kIncomingPacket = 0, kOutgoingPacket };

class RtcEventLog {
 public:
  // TODO(eladalon): Two stages are upcoming.
  // 1. Extend this to actually support the new encoding.
  // 2. Get rid of the legacy encoding, allowing us to get rid of this enum.
  enum class EncodingType { Legacy };

  virtual ~RtcEventLog() {}

  // Factory method to create an RtcEventLog object.
  // TODO(eladalon): Get rid of the default value after internal projects fixed.
  static std::unique_ptr<RtcEventLog> Create(
      EncodingType encoding_type = EncodingType::Legacy);
  // TODO(nisse): webrtc::Clock is deprecated. Delete this method and
  // above forward declaration of Clock when
  // webrtc/system_wrappers/include/clock.h is deleted.
  // TODO(eladalon): Get rid of the default value after internal projects fixed.
  static std::unique_ptr<RtcEventLog> Create(
      const Clock* clock,
      EncodingType encoding_type = EncodingType::Legacy) {
    return Create(encoding_type);
  }

  // Create an RtcEventLog object that does nothing.
  static std::unique_ptr<RtcEventLog> CreateNull();

  // Starts logging a maximum of max_size_bytes bytes to the specified file.
  // If the file already exists it will be overwritten.
  // If max_size_bytes <= 0, logging will be active until StopLogging is called.
  // The function has no effect and returns false if we can't start a new log
  // e.g. because we are already logging or the file cannot be opened.
  virtual bool StartLogging(const std::string& file_name,
                            int64_t max_size_bytes) = 0;

  // Same as above. The RtcEventLog takes ownership of the file if the call
  // is successful, i.e. if it returns true.
  virtual bool StartLogging(rtc::PlatformFile platform_file,
                            int64_t max_size_bytes) = 0;

  // Deprecated. Pass an explicit file size limit.
  RTC_DEPRECATED bool StartLogging(const std::string& file_name) {
    return StartLogging(file_name, 10000000);
  }

  // Deprecated. Pass an explicit file size limit.
  RTC_DEPRECATED bool StartLogging(rtc::PlatformFile platform_file) {
    return StartLogging(platform_file, 10000000);
  }

  // Stops logging to file and waits until the file has been closed, after
  // which it would be permissible to read and/or modify it.
  virtual void StopLogging() = 0;

  // Log an RTC event (the type of event is determined by the subclass).
  virtual void Log(std::unique_ptr<RtcEvent> event) = 0;

  // Logs configuration information for a video receive stream.
  RTC_DEPRECATED virtual void LogVideoReceiveStreamConfig(
      const rtclog::StreamConfig& config) = 0;

  // Logs configuration information for a video send stream.
  RTC_DEPRECATED virtual void LogVideoSendStreamConfig(
      const rtclog::StreamConfig& config) = 0;

  // Logs configuration information for an audio receive stream.
  RTC_DEPRECATED virtual void LogAudioReceiveStreamConfig(
      const rtclog::StreamConfig& config) = 0;

  // Logs configuration information for an audio send stream.
  RTC_DEPRECATED virtual void LogAudioSendStreamConfig(
      const rtclog::StreamConfig& config) = 0;

  RTC_DEPRECATED virtual void LogRtpHeader(PacketDirection direction,
                                           const uint8_t* header,
                                           size_t packet_length) {}

  RTC_DEPRECATED virtual void LogRtpHeader(PacketDirection direction,
                                           const uint8_t* header,
                                           size_t packet_length,
                                           int probe_cluster_id) {}

  // Logs the header of an incoming RTP packet. |packet_length|
  // is the total length of the packet, including both header and payload.
  RTC_DEPRECATED virtual void LogIncomingRtpHeader(
      const RtpPacketReceived& packet) = 0;

  // Logs the header of an incoming RTP packet. |packet_length|
  // is the total length of the packet, including both header and payload.
  RTC_DEPRECATED virtual void LogOutgoingRtpHeader(
      const RtpPacketToSend& packet,
      int probe_cluster_id) = 0;

  RTC_DEPRECATED virtual void LogRtcpPacket(PacketDirection direction,
                                            const uint8_t* header,
                                            size_t packet_length) {}

  // Logs an incoming RTCP packet.
  RTC_DEPRECATED virtual void LogIncomingRtcpPacket(
      rtc::ArrayView<const uint8_t> packet) = 0;

  // Logs an outgoing RTCP packet.
  RTC_DEPRECATED virtual void LogOutgoingRtcpPacket(
      rtc::ArrayView<const uint8_t> packet) = 0;

  // Logs an audio playout event.
  RTC_DEPRECATED virtual void LogAudioPlayout(uint32_t ssrc) = 0;

  // Logs a bitrate update from the bandwidth estimator based on packet loss.
  RTC_DEPRECATED virtual void LogLossBasedBweUpdate(int32_t bitrate_bps,
                                                    uint8_t fraction_loss,
                                                    int32_t total_packets) = 0;

  // Logs a bitrate update from the bandwidth estimator based on delay changes.
  RTC_DEPRECATED virtual void LogDelayBasedBweUpdate(
      int32_t bitrate_bps,
      BandwidthUsage detector_state) = 0;

  // Logs audio encoder re-configuration driven by audio network adaptor.
  RTC_DEPRECATED virtual void LogAudioNetworkAdaptation(
      const AudioEncoderRuntimeConfig& config) = 0;

  // Logs when a probe cluster is created.
  RTC_DEPRECATED virtual void LogProbeClusterCreated(int id,
                                                     int bitrate_bps,
                                                     int min_probes,
                                                     int min_bytes) = 0;

  // Logs the result of a successful probing attempt.
  RTC_DEPRECATED virtual void LogProbeResultSuccess(int id,
                                                    int bitrate_bps) = 0;

  // Logs the result of an unsuccessful probing attempt.
  RTC_DEPRECATED virtual void LogProbeResultFailure(
      int id,
      ProbeFailureReason failure_reason) = 0;
};

// No-op implementation is used if flag is not set, or in tests.
class RtcEventLogNullImpl : public RtcEventLog {
 public:
  bool StartLogging(const std::string& file_name,
                    int64_t max_size_bytes) override {
    return false;
  }
  bool StartLogging(rtc::PlatformFile platform_file,
                    int64_t max_size_bytes) override {
    return false;
  }
  void StopLogging() override {}
  void Log(std::unique_ptr<RtcEvent> event) override {}
  void LogVideoReceiveStreamConfig(
      const rtclog::StreamConfig& config) override {}
  void LogVideoSendStreamConfig(const rtclog::StreamConfig& config) override {}
  void LogAudioReceiveStreamConfig(
      const rtclog::StreamConfig& config) override {}
  void LogAudioSendStreamConfig(const rtclog::StreamConfig& config) override {}
  void LogIncomingRtpHeader(const RtpPacketReceived& packet) override {}
  void LogOutgoingRtpHeader(const RtpPacketToSend& packet,
                            int probe_cluster_id) override {}
  void LogIncomingRtcpPacket(rtc::ArrayView<const uint8_t> packet) override {}
  void LogOutgoingRtcpPacket(rtc::ArrayView<const uint8_t> packet) override {}
  void LogAudioPlayout(uint32_t ssrc) override {}
  void LogLossBasedBweUpdate(int32_t bitrate_bps,
                             uint8_t fraction_loss,
                             int32_t total_packets) override {}
  void LogDelayBasedBweUpdate(int32_t bitrate_bps,
                              BandwidthUsage detector_state) override {}
  void LogAudioNetworkAdaptation(
      const AudioEncoderRuntimeConfig& config) override {}
  void LogProbeClusterCreated(int id,
                              int bitrate_bps,
                              int min_probes,
                              int min_bytes) override{};
  void LogProbeResultSuccess(int id, int bitrate_bps) override{};
  void LogProbeResultFailure(int id,
                             ProbeFailureReason failure_reason) override{};
};

}  // namespace webrtc

#endif  // LOGGING_RTC_EVENT_LOG_RTC_EVENT_LOG_H_
