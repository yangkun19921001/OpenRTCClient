/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/congestion_controller/delay_based_bwe.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "webrtc/base/checks.h"
#include "webrtc/base/constructormagic.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/thread_annotations.h"
#include "webrtc/logging/rtc_event_log/rtc_event_log.h"
#include "webrtc/modules/congestion_controller/include/congestion_controller.h"
#include "webrtc/modules/pacing/paced_sender.h"
#include "webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "webrtc/modules/remote_bitrate_estimator/test/bwe_test_logging.h"
#include "webrtc/system_wrappers/include/field_trial.h"
#include "webrtc/system_wrappers/include/metrics.h"
#include "webrtc/typedefs.h"

namespace {
constexpr int kTimestampGroupLengthMs = 5;
constexpr int kAbsSendTimeFraction = 18;
constexpr int kAbsSendTimeInterArrivalUpshift = 8;
constexpr int kInterArrivalShift =
    kAbsSendTimeFraction + kAbsSendTimeInterArrivalUpshift;
constexpr double kTimestampToMs =
    1000.0 / static_cast<double>(1 << kInterArrivalShift);
// This ssrc is used to fulfill the current API but will be removed
// after the API has been changed.
constexpr uint32_t kFixedSsrc = 0;
constexpr int kInitialRateWindowMs = 500;
constexpr int kRateWindowMs = 150;

// Parameters for linear least squares fit of regression line to noisy data.
constexpr size_t kDefaultTrendlineWindowSize = 20;
constexpr double kDefaultTrendlineSmoothingCoeff = 0.9;
constexpr double kDefaultTrendlineThresholdGain = 4.0;

constexpr int kMaxConsecutiveFailedLookups = 5;

const char kBweSparseUpdateExperiment[] = "WebRTC-BweSparseUpdateExperiment";

bool BweSparseUpdateExperimentIsEnabled() {
  std::string experiment_string =
      webrtc::field_trial::FindFullName(kBweSparseUpdateExperiment);
  return experiment_string == "Enabled";
}

class PacketFeedbackComparator {
 public:
  inline bool operator()(const webrtc::PacketFeedback& lhs,
                         const webrtc::PacketFeedback& rhs) {
    if (lhs.arrival_time_ms != rhs.arrival_time_ms)
      return lhs.arrival_time_ms < rhs.arrival_time_ms;
    if (lhs.send_time_ms != rhs.send_time_ms)
      return lhs.send_time_ms < rhs.send_time_ms;
    return lhs.sequence_number < rhs.sequence_number;
  }
};

void SortPacketFeedbackVector(const std::vector<webrtc::PacketFeedback>& input,
                              std::vector<webrtc::PacketFeedback>* output) {
  auto pred = [](const webrtc::PacketFeedback& packet_feedback) {
    return packet_feedback.arrival_time_ms !=
           webrtc::PacketFeedback::kNotReceived;
  };
  std::copy_if(input.begin(), input.end(), std::back_inserter(*output), pred);
  std::sort(output->begin(), output->end(), PacketFeedbackComparator());
}
}  // namespace

namespace webrtc {

DelayBasedBwe::BitrateEstimator::BitrateEstimator()
    : sum_(0),
      current_win_ms_(0),
      prev_time_ms_(-1),
      bitrate_estimate_(-1.0f),
      bitrate_estimate_var_(50.0f) {}

void DelayBasedBwe::BitrateEstimator::Update(int64_t now_ms, int bytes) {
  int rate_window_ms = kRateWindowMs;
  // We use a larger window at the beginning to get a more stable sample that
  // we can use to initialize the estimate.
  if (bitrate_estimate_ < 0.f)
    rate_window_ms = kInitialRateWindowMs;
  float bitrate_sample = UpdateWindow(now_ms, bytes, rate_window_ms);
  if (bitrate_sample < 0.0f)
    return;
  if (bitrate_estimate_ < 0.0f) {
    // This is the very first sample we get. Use it to initialize the estimate.
    bitrate_estimate_ = bitrate_sample;
    return;
  }
  // Define the sample uncertainty as a function of how far away it is from the
  // current estimate.
  float sample_uncertainty =
      10.0f * std::abs(bitrate_estimate_ - bitrate_sample) / bitrate_estimate_;
  float sample_var = sample_uncertainty * sample_uncertainty;
  // Update a bayesian estimate of the rate, weighting it lower if the sample
  // uncertainty is large.
  // The bitrate estimate uncertainty is increased with each update to model
  // that the bitrate changes over time.
  float pred_bitrate_estimate_var = bitrate_estimate_var_ + 5.f;
  bitrate_estimate_ = (sample_var * bitrate_estimate_ +
                       pred_bitrate_estimate_var * bitrate_sample) /
                      (sample_var + pred_bitrate_estimate_var);
  bitrate_estimate_var_ = sample_var * pred_bitrate_estimate_var /
                          (sample_var + pred_bitrate_estimate_var);
}

float DelayBasedBwe::BitrateEstimator::UpdateWindow(int64_t now_ms,
                                                    int bytes,
                                                    int rate_window_ms) {
  // Reset if time moves backwards.
  if (now_ms < prev_time_ms_) {
    prev_time_ms_ = -1;
    sum_ = 0;
    current_win_ms_ = 0;
  }
  if (prev_time_ms_ >= 0) {
    current_win_ms_ += now_ms - prev_time_ms_;
    // Reset if nothing has been received for more than a full window.
    if (now_ms - prev_time_ms_ > rate_window_ms) {
      sum_ = 0;
      current_win_ms_ %= rate_window_ms;
    }
  }
  prev_time_ms_ = now_ms;
  float bitrate_sample = -1.0f;
  if (current_win_ms_ >= rate_window_ms) {
    bitrate_sample = 8.0f * sum_ / static_cast<float>(rate_window_ms);
    current_win_ms_ -= rate_window_ms;
    sum_ = 0;
  }
  sum_ += bytes;
  return bitrate_sample;
}

rtc::Optional<uint32_t> DelayBasedBwe::BitrateEstimator::bitrate_bps() const {
  if (bitrate_estimate_ < 0.f)
    return rtc::Optional<uint32_t>();
  return rtc::Optional<uint32_t>(bitrate_estimate_ * 1000);
}

DelayBasedBwe::DelayBasedBwe(RtcEventLog* event_log, const Clock* clock)
    : event_log_(event_log),
      clock_(clock),
      inter_arrival_(),
      trendline_estimator_(),
      detector_(),
      receiver_incoming_bitrate_(),
      last_seen_packet_ms_(-1),
      uma_recorded_(false),
      probe_bitrate_estimator_(event_log),
      trendline_window_size_(kDefaultTrendlineWindowSize),
      trendline_smoothing_coeff_(kDefaultTrendlineSmoothingCoeff),
      trendline_threshold_gain_(kDefaultTrendlineThresholdGain),
      consecutive_delayed_feedbacks_(0),
      last_logged_bitrate_(0),
      last_logged_state_(BandwidthUsage::kBwNormal),
      in_sparse_update_experiment_(BweSparseUpdateExperimentIsEnabled()) {
  LOG(LS_INFO) << "Using Trendline filter for delay change estimation.";
  network_thread_.DetachFromThread();
}

DelayBasedBwe::Result DelayBasedBwe::IncomingPacketFeedbackVector(
    const std::vector<PacketFeedback>& packet_feedback_vector) {
  RTC_DCHECK(network_thread_.CalledOnValidThread());

  std::vector<PacketFeedback> sorted_packet_feedback_vector;
  SortPacketFeedbackVector(packet_feedback_vector,
                           &sorted_packet_feedback_vector);
  // TOOD(holmer): An empty feedback vector here likely means that
  // all acks were too late and that the send time history had
  // timed out. We should reduce the rate when this occurs.
  if (sorted_packet_feedback_vector.empty()) {
    LOG(LS_WARNING) << "Very late feedback received.";
    return DelayBasedBwe::Result();
  }

  if (!uma_recorded_) {
    RTC_HISTOGRAM_ENUMERATION(kBweTypeHistogram,
                              BweNames::kSendSideTransportSeqNum,
                              BweNames::kBweNamesMax);
    uma_recorded_ = true;
  }
  bool overusing = false;
  bool delayed_feedback = true;
  for (const auto& packet_feedback : sorted_packet_feedback_vector) {
    if (packet_feedback.send_time_ms < 0)
      continue;
    delayed_feedback = false;
    IncomingPacketFeedback(packet_feedback);
    if (!in_sparse_update_experiment_)
      overusing |= (detector_.State() == BandwidthUsage::kBwOverusing);
  }
  if (in_sparse_update_experiment_)
    overusing = (detector_.State() == BandwidthUsage::kBwOverusing);
  if (delayed_feedback) {
    ++consecutive_delayed_feedbacks_;
    if (consecutive_delayed_feedbacks_ >= kMaxConsecutiveFailedLookups) {
      consecutive_delayed_feedbacks_ = 0;
      return OnLongFeedbackDelay(
          sorted_packet_feedback_vector.back().arrival_time_ms);
    }
  } else {
    consecutive_delayed_feedbacks_ = 0;
    return MaybeUpdateEstimate(overusing);
  }
  return Result();
}

DelayBasedBwe::Result DelayBasedBwe::OnLongFeedbackDelay(
    int64_t arrival_time_ms) {
  // Estimate should always be valid since a start bitrate always is set in the
  // Call constructor. An alternative would be to return an empty Result here,
  // or to estimate the throughput based on the feedback we received.
  RTC_DCHECK(rate_control_.ValidEstimate());
  rate_control_.SetEstimate(rate_control_.LatestEstimate() / 2,
                            arrival_time_ms);
  Result result;
  result.updated = true;
  result.probe = false;
  result.target_bitrate_bps = rate_control_.LatestEstimate();
  LOG(LS_WARNING) << "Long feedback delay detected, reducing BWE to "
                  << result.target_bitrate_bps;
  return result;
}

void DelayBasedBwe::IncomingPacketFeedback(
    const PacketFeedback& packet_feedback) {
  int64_t now_ms = clock_->TimeInMilliseconds();

  receiver_incoming_bitrate_.Update(packet_feedback.arrival_time_ms,
                                    packet_feedback.payload_size);
  Result result;
  // Reset if the stream has timed out.
  if (last_seen_packet_ms_ == -1 ||
      now_ms - last_seen_packet_ms_ > kStreamTimeOutMs) {
    inter_arrival_.reset(
        new InterArrival((kTimestampGroupLengthMs << kInterArrivalShift) / 1000,
                         kTimestampToMs, true));
    trendline_estimator_.reset(new TrendlineEstimator(
        trendline_window_size_, trendline_smoothing_coeff_,
        trendline_threshold_gain_));
  }
  last_seen_packet_ms_ = now_ms;

  uint32_t send_time_24bits =
      static_cast<uint32_t>(
          ((static_cast<uint64_t>(packet_feedback.send_time_ms)
            << kAbsSendTimeFraction) +
           500) /
          1000) &
      0x00FFFFFF;
  // Shift up send time to use the full 32 bits that inter_arrival works with,
  // so wrapping works properly.
  uint32_t timestamp = send_time_24bits << kAbsSendTimeInterArrivalUpshift;

  uint32_t ts_delta = 0;
  int64_t t_delta = 0;
  int size_delta = 0;
  if (inter_arrival_->ComputeDeltas(timestamp, packet_feedback.arrival_time_ms,
                                    now_ms, packet_feedback.payload_size,
                                    &ts_delta, &t_delta, &size_delta)) {
    double ts_delta_ms = (1000.0 * ts_delta) / (1 << kInterArrivalShift);
    trendline_estimator_->Update(t_delta, ts_delta_ms,
                                 packet_feedback.arrival_time_ms);
    detector_.Detect(trendline_estimator_->trendline_slope(), ts_delta_ms,
                     trendline_estimator_->num_of_deltas(),
                     packet_feedback.arrival_time_ms);
  }
  if (packet_feedback.pacing_info.probe_cluster_id !=
      PacedPacketInfo::kNotAProbe) {
    probe_bitrate_estimator_.HandleProbeAndEstimateBitrate(packet_feedback);
  }
}

DelayBasedBwe::Result DelayBasedBwe::MaybeUpdateEstimate(bool overusing) {
  Result result;
  int64_t now_ms = clock_->TimeInMilliseconds();

  rtc::Optional<uint32_t> acked_bitrate_bps =
      receiver_incoming_bitrate_.bitrate_bps();
  rtc::Optional<int> probe_bitrate_bps =
      probe_bitrate_estimator_.FetchAndResetLastEstimatedBitrateBps();
  // Currently overusing the bandwidth.
  if (overusing) {
    if (acked_bitrate_bps &&
        rate_control_.TimeToReduceFurther(now_ms, *acked_bitrate_bps)) {
      result.updated = UpdateEstimate(now_ms, acked_bitrate_bps, overusing,
                                      &result.target_bitrate_bps);
    }
  } else {
    if (probe_bitrate_bps) {
      rate_control_.SetEstimate(*probe_bitrate_bps, now_ms);
      result.probe = true;
    }
    result.updated = UpdateEstimate(now_ms, acked_bitrate_bps, overusing,
                                    &result.target_bitrate_bps);
  }
  if (result.updated) {
    BWE_TEST_LOGGING_PLOT(1, "target_bitrate_bps", now_ms,
                          result.target_bitrate_bps);
    if (event_log_ && (result.target_bitrate_bps != last_logged_bitrate_ ||
                       detector_.State() != last_logged_state_)) {
      event_log_->LogDelayBasedBweUpdate(result.target_bitrate_bps,
                                         detector_.State());
      last_logged_bitrate_ = result.target_bitrate_bps;
      last_logged_state_ = detector_.State();
    }
  }
  return result;
}

bool DelayBasedBwe::UpdateEstimate(int64_t now_ms,
                                   rtc::Optional<uint32_t> acked_bitrate_bps,
                                   bool overusing,
                                   uint32_t* target_bitrate_bps) {
  // TODO(terelius): RateControlInput::noise_var is deprecated and will be
  // removed. In the meantime, we set it to zero.
  const RateControlInput input(
      overusing ? BandwidthUsage::kBwOverusing : detector_.State(),
      acked_bitrate_bps, 0);
  *target_bitrate_bps = rate_control_.Update(&input, now_ms);
  return rate_control_.ValidEstimate();
}

void DelayBasedBwe::OnRttUpdate(int64_t avg_rtt_ms, int64_t max_rtt_ms) {
  rate_control_.SetRtt(avg_rtt_ms);
}

bool DelayBasedBwe::LatestEstimate(std::vector<uint32_t>* ssrcs,
                                   uint32_t* bitrate_bps) const {
  // Currently accessed from both the process thread (see
  // ModuleRtpRtcpImpl::Process()) and the configuration thread (see
  // Call::GetStats()). Should in the future only be accessed from a single
  // thread.
  RTC_DCHECK(ssrcs);
  RTC_DCHECK(bitrate_bps);
  if (!rate_control_.ValidEstimate())
    return false;

  *ssrcs = {kFixedSsrc};
  *bitrate_bps = rate_control_.LatestEstimate();
  return true;
}

void DelayBasedBwe::SetStartBitrate(int start_bitrate_bps) {
  LOG(LS_WARNING) << "BWE Setting start bitrate to: " << start_bitrate_bps;
  rate_control_.SetStartBitrate(start_bitrate_bps);
}

void DelayBasedBwe::SetMinBitrate(int min_bitrate_bps) {
  // Called from both the configuration thread and the network thread. Shouldn't
  // be called from the network thread in the future.
  rate_control_.SetMinBitrate(min_bitrate_bps);
}

int64_t DelayBasedBwe::GetExpectedBwePeriodMs() const {
  return rate_control_.GetExpectedBandwidthPeriodMs();
}
}  // namespace webrtc
