/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_RTP_RTCP_SOURCE_RTCP_TRANSCEIVER_CONFIG_H_
#define MODULES_RTP_RTCP_SOURCE_RTCP_TRANSCEIVER_CONFIG_H_

#include <string>

namespace webrtc {
class ReceiveStatisticsProvider;
class Transport;

struct RtcpTransceiverConfig {
  RtcpTransceiverConfig();
  RtcpTransceiverConfig(const RtcpTransceiverConfig&);
  RtcpTransceiverConfig& operator=(const RtcpTransceiverConfig&);
  ~RtcpTransceiverConfig();

  // Logs the error and returns false if configuration miss key objects or
  // is inconsistant. May log warnings.
  bool Validate() const;

  // Used to prepend all log messages. Can be empty.
  std::string debug_id;

  // Ssrc to use as default sender ssrc, e.g. for transport-wide feedbacks.
  uint32_t feedback_ssrc = 1;

  // Cname of the local particiapnt.
  std::string cname;

  // Maximum packet size outgoing transport accepts.
  size_t max_packet_size = 1200;

  // Transport to send rtcp packets to. Should be set.
  Transport* outgoing_transport = nullptr;

  // Minimum period to send receiver reports and attached messages.
  int min_periodic_report_ms = 1000;

  // Rtcp report block generator for outgoing receiver reports.
  ReceiveStatisticsProvider* receive_statistics = nullptr;
};

}  // namespace webrtc

#endif  // MODULES_RTP_RTCP_SOURCE_RTCP_TRANSCEIVER_CONFIG_H_
