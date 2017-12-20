/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
spect *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_AUDIO_PROCESSING_AEC3_MAIN_FILTER_UPDATE_GAIN_H_
#define MODULES_AUDIO_PROCESSING_AEC3_MAIN_FILTER_UPDATE_GAIN_H_

#include <memory>
#include <vector>

#include "modules/audio_processing/aec3/adaptive_fir_filter.h"
#include "modules/audio_processing/aec3/aec3_common.h"
#include "modules/audio_processing/aec3/echo_path_variability.h"
#include "modules/audio_processing/aec3/render_signal_analyzer.h"
#include "modules/audio_processing/aec3/subtractor_output.h"
#include "rtc_base/constructormagic.h"

namespace webrtc {

class ApmDataDumper;

// Provides functionality for computing the adaptive gain for the main filter.
class MainFilterUpdateGain {
 public:
  MainFilterUpdateGain(float leakage_converged,
                       float leakage_diverged,
                       float noise_gate_power,
                       float error_floor);
  ~MainFilterUpdateGain();

  // Takes action in the case of a known echo path change.
  void HandleEchoPathChange(const EchoPathVariability& echo_path_variability);

  // Computes the gain.
  void Compute(const std::array<float, kFftLengthBy2Plus1>& render_power,
               const RenderSignalAnalyzer& render_signal_analyzer,
               const SubtractorOutput& subtractor_output,
               const AdaptiveFirFilter& filter,
               bool saturated_capture_signal,
               FftData* gain_fft);

 private:
  static int instance_count_;
  std::unique_ptr<ApmDataDumper> data_dumper_;
  const float leakage_converged_;
  const float leakage_diverged_;
  const float noise_gate_power_;
  const float error_floor_;
  std::array<float, kFftLengthBy2Plus1> H_error_;
  size_t poor_excitation_counter_;
  size_t call_counter_ = 0;
  RTC_DISALLOW_COPY_AND_ASSIGN(MainFilterUpdateGain);
};

}  // namespace webrtc

#endif  // MODULES_AUDIO_PROCESSING_AEC3_MAIN_FILTER_UPDATE_GAIN_H_
