/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_TOOLS_EVENT_LOG_VISUALIZER_ANALYZER_H_
#define WEBRTC_TOOLS_EVENT_LOG_VISUALIZER_ANALYZER_H_

#include <vector>

#include "webrtc/call/rtc_event_log_parser.h"
#include "webrtc/tools/event_log_visualizer/plot_base.h"

namespace webrtc {
namespace plotting {

class EventLogAnalyzer {
 public:
  // The EventLogAnalyzer keeps a reference to the ParsedRtcEventLog for the
  // duration of its lifetime. The ParsedRtcEventLog must not be destroyed or
  // modified while the EventLogAnalyzer is being used.
  explicit EventLogAnalyzer(const ParsedRtcEventLog& log);

  void CreatePacketGraph(PacketDirection desired_direction, Plot* plot);

  void CreatePlayoutGraph(Plot* plot);

  void CreateSequenceNumberGraph(Plot* plot);

  void CreateDelayChangeGraph(Plot* plot);

  void CreateAccumulatedDelayChangeGraph(Plot* plot);

  void CreateTotalBitrateGraph(PacketDirection desired_direction, Plot* plot);

  void CreateStreamBitrateGraph(PacketDirection desired_direction, Plot* plot);

 private:
  const ParsedRtcEventLog& parsed_log_;

  // A list of SSRCs we are interested in analysing.
  // If left empty, all SSRCs will be considered relevant.
  std::vector<uint32_t> desired_ssrc_;

  // Window and step size used for calculating moving averages, e.g. bitrate.
  // The generated data points will be |step_| microseconds apart.
  // Only events occuring at most |window_duration_| microseconds before the
  // current data point will be part of the average.
  uint64_t window_duration_;
  uint64_t step_;

  // First and last events of the log.
  uint64_t begin_time_;
  uint64_t end_time_;
};

}  // namespace plotting
}  // namespace webrtc

#endif  // WEBRTC_TOOLS_EVENT_LOG_VISUALIZER_ANALYZER_H_
