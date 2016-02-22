/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_processing/intelligibility/intelligibility_enhancer.h"

#include <math.h>
#include <stdlib.h>
#include <algorithm>
#include <limits>
#include <numeric>

#include "webrtc/base/checks.h"
#include "webrtc/common_audio/include/audio_util.h"
#include "webrtc/common_audio/window_generator.h"

namespace webrtc {

namespace {

const size_t kErbResolution = 2;
const int kWindowSizeMs = 16;
const int kChunkSizeMs = 10;  // Size provided by APM.
const float kClipFreqKhz = 0.2f;
const float kKbdAlpha = 1.5f;
const float kLambdaBot = -1.0f;      // Extreme values in bisection
const float kLambdaTop = -10e-18f;  // search for lamda.
const float kVoiceProbabilityThreshold = 0.02f;
// Number of chunks after voice activity which is still considered speech.
const size_t kSpeechOffsetDelay = 80;
const float kDecayRate = 0.98f;              // Power estimation decay rate.
const float kMaxRelativeGainChange = 0.04f;  // Maximum relative change in gain.
const float kRho = 0.0004f;  // Default production and interpretation SNR.

// Returns dot product of vectors |a| and |b| with size |length|.
float DotProduct(const float* a, const float* b, size_t length) {
  float ret = 0.f;
  for (size_t i = 0; i < length; ++i) {
    ret = fmaf(a[i], b[i], ret);
  }
  return ret;
}

// Computes the power across ERB bands from the power spectral density |pow|.
// Stores it in |result|.
void MapToErbBands(const float* pow,
                   const std::vector<std::vector<float>>& filter_bank,
                   float* result) {
  for (size_t i = 0; i < filter_bank.size(); ++i) {
    RTC_DCHECK_GT(filter_bank[i].size(), 0u);
    result[i] = DotProduct(&filter_bank[i][0], pow, filter_bank[i].size());
  }
}

}  // namespace

IntelligibilityEnhancer::TransformCallback::TransformCallback(
    IntelligibilityEnhancer* parent)
    : parent_(parent) {
}

void IntelligibilityEnhancer::TransformCallback::ProcessAudioBlock(
    const std::complex<float>* const* in_block,
    size_t in_channels,
    size_t frames,
    size_t /* out_channels */,
    std::complex<float>* const* out_block) {
  RTC_DCHECK_EQ(parent_->freqs_, frames);
  for (size_t i = 0; i < in_channels; ++i) {
    parent_->ProcessClearBlock(in_block[i], out_block[i]);
  }
}

IntelligibilityEnhancer::IntelligibilityEnhancer(int sample_rate_hz,
                                                 size_t num_render_channels)
    : freqs_(RealFourier::ComplexLength(
          RealFourier::FftOrder(sample_rate_hz * kWindowSizeMs / 1000))),
      chunk_length_(static_cast<size_t>(sample_rate_hz * kChunkSizeMs / 1000)),
      bank_size_(GetBankSize(sample_rate_hz, kErbResolution)),
      sample_rate_hz_(sample_rate_hz),
      num_render_channels_(num_render_channels),
      clear_power_estimator_(freqs_, kDecayRate),
      noise_power_estimator_(
          new intelligibility::PowerEstimator<float>(freqs_, kDecayRate)),
      filtered_clear_pow_(new float[bank_size_]),
      filtered_noise_pow_(new float[bank_size_]),
      center_freqs_(new float[bank_size_]),
      render_filter_bank_(CreateErbBank(freqs_)),
      gains_eq_(new float[bank_size_]),
      gain_applier_(freqs_, kMaxRelativeGainChange),
      temp_render_out_buffer_(chunk_length_, num_render_channels_),
      render_callback_(this),
      audio_s16_(chunk_length_),
      chunks_since_voice_(kSpeechOffsetDelay),
      is_speech_(false) {
  RTC_DCHECK_LE(kRho, 1.f);

  memset(filtered_clear_pow_.get(), 0,
         bank_size_ * sizeof(filtered_clear_pow_[0]));
  memset(filtered_noise_pow_.get(), 0,
         bank_size_ * sizeof(filtered_noise_pow_[0]));

  const size_t erb_index = static_cast<size_t>(
      ceilf(11.17f * logf((kClipFreqKhz + 0.312f) / (kClipFreqKhz + 14.6575f)) +
            43.f));
  start_freq_ = std::max(static_cast<size_t>(1), erb_index * kErbResolution);

  size_t window_size = static_cast<size_t>(1 << RealFourier::FftOrder(freqs_));
  std::vector<float> kbd_window(window_size);
  WindowGenerator::KaiserBesselDerived(kKbdAlpha, window_size, &kbd_window[0]);
  render_mangler_.reset(new LappedTransform(
      num_render_channels_, num_render_channels_, chunk_length_, &kbd_window[0],
      window_size, window_size / 2, &render_callback_));
}

void IntelligibilityEnhancer::SetCaptureNoiseEstimate(
    std::vector<float> noise) {
  if (capture_filter_bank_.size() != bank_size_ ||
      capture_filter_bank_[0].size() != noise.size()) {
    capture_filter_bank_ = CreateErbBank(noise.size());
    noise_power_estimator_.reset(
        new intelligibility::PowerEstimator<float>(noise.size(), kDecayRate));
  }
  noise_power_estimator_->Step(&noise[0]);
}

void IntelligibilityEnhancer::ProcessRenderAudio(float* const* audio,
                                                 int sample_rate_hz,
                                                 size_t num_channels) {
  RTC_CHECK_EQ(sample_rate_hz_, sample_rate_hz);
  RTC_CHECK_EQ(num_render_channels_, num_channels);
  is_speech_ = IsSpeech(audio[0]);
  render_mangler_->ProcessChunk(audio, temp_render_out_buffer_.channels());
  for (size_t i = 0; i < num_render_channels_; ++i) {
    memcpy(audio[i], temp_render_out_buffer_.channels()[i],
           chunk_length_ * sizeof(**audio));
  }
}

void IntelligibilityEnhancer::ProcessClearBlock(
    const std::complex<float>* in_block,
    std::complex<float>* out_block) {
  if (is_speech_) {
    clear_power_estimator_.Step(in_block);
  }
  const std::vector<float>& clear_power = clear_power_estimator_.power();
  const std::vector<float>& noise_power = noise_power_estimator_->power();
  MapToErbBands(&clear_power[0], render_filter_bank_,
                filtered_clear_pow_.get());
  MapToErbBands(&noise_power[0], capture_filter_bank_,
                filtered_noise_pow_.get());
  SolveForGainsGivenLambda(kLambdaTop, start_freq_, gains_eq_.get());
  const float power_target =
      std::accumulate(&clear_power[0], &clear_power[0] + freqs_, 0.f);
  const float power_top =
      DotProduct(gains_eq_.get(), filtered_clear_pow_.get(), bank_size_);
  SolveForGainsGivenLambda(kLambdaBot, start_freq_, gains_eq_.get());
  const float power_bot =
      DotProduct(gains_eq_.get(), filtered_clear_pow_.get(), bank_size_);
  if (power_target >= power_bot && power_target <= power_top) {
    SolveForLambda(power_target, power_bot, power_top);
    UpdateErbGains();
  }  // Else experiencing power underflow, so do nothing.
  gain_applier_.Apply(in_block, out_block);
}

void IntelligibilityEnhancer::SolveForLambda(float power_target,
                                             float power_bot,
                                             float power_top) {
  const float kConvergeThresh = 0.001f;  // TODO(ekmeyerson): Find best values
  const int kMaxIters = 100;             // for these, based on experiments.

  const float reciprocal_power_target =
      1.f / (power_target + std::numeric_limits<float>::epsilon());
  float lambda_bot = kLambdaBot;
  float lambda_top = kLambdaTop;
  float power_ratio = 2.f;  // Ratio of achieved power to target power.
  int iters = 0;
  while (std::fabs(power_ratio - 1.f) > kConvergeThresh && iters <= kMaxIters) {
    const float lambda = lambda_bot + (lambda_top - lambda_bot) / 2.f;
    SolveForGainsGivenLambda(lambda, start_freq_, gains_eq_.get());
    const float power =
        DotProduct(gains_eq_.get(), filtered_clear_pow_.get(), bank_size_);
    if (power < power_target) {
      lambda_bot = lambda;
    } else {
      lambda_top = lambda;
    }
    power_ratio = std::fabs(power * reciprocal_power_target);
    ++iters;
  }
}

void IntelligibilityEnhancer::UpdateErbGains() {
  // (ERB gain) = filterbank' * (freq gain)
  float* gains = gain_applier_.target();
  for (size_t i = 0; i < freqs_; ++i) {
    gains[i] = 0.f;
    for (size_t j = 0; j < bank_size_; ++j) {
      gains[i] = fmaf(render_filter_bank_[j][i], gains_eq_[j], gains[i]);
    }
  }
}

size_t IntelligibilityEnhancer::GetBankSize(int sample_rate,
                                            size_t erb_resolution) {
  float freq_limit = sample_rate / 2000.f;
  size_t erb_scale = static_cast<size_t>(ceilf(
      11.17f * logf((freq_limit + 0.312f) / (freq_limit + 14.6575f)) + 43.f));
  return erb_scale * erb_resolution;
}

std::vector<std::vector<float>> IntelligibilityEnhancer::CreateErbBank(
    size_t num_freqs) {
  std::vector<std::vector<float>> filter_bank(bank_size_);
  size_t lf = 1, rf = 4;

  for (size_t i = 0; i < bank_size_; ++i) {
    float abs_temp = fabsf((i + 1.f) / static_cast<float>(kErbResolution));
    center_freqs_[i] = 676170.4f / (47.06538f - expf(0.08950404f * abs_temp));
    center_freqs_[i] -= 14678.49f;
  }
  float last_center_freq = center_freqs_[bank_size_ - 1];
  for (size_t i = 0; i < bank_size_; ++i) {
    center_freqs_[i] *= 0.5f * sample_rate_hz_ / last_center_freq;
  }

  for (size_t i = 0; i < bank_size_; ++i) {
    filter_bank[i].resize(num_freqs);
  }

  for (size_t i = 1; i <= bank_size_; ++i) {
    static const size_t kOne = 1;  // Avoids repeated static_cast<>s below.
    size_t lll =
        static_cast<size_t>(round(center_freqs_[std::max(kOne, i - lf) - 1] *
                                  num_freqs / (0.5f * sample_rate_hz_)));
    size_t ll = static_cast<size_t>(round(center_freqs_[std::max(kOne, i) - 1] *
                                   num_freqs / (0.5f * sample_rate_hz_)));
    lll = std::min(num_freqs, std::max(lll, kOne)) - 1;
    ll = std::min(num_freqs, std::max(ll, kOne)) - 1;

    size_t rrr = static_cast<size_t>(
        round(center_freqs_[std::min(bank_size_, i + rf) - 1] * num_freqs /
              (0.5f * sample_rate_hz_)));
    size_t rr = static_cast<size_t>(
        round(center_freqs_[std::min(bank_size_, i + 1) - 1] * num_freqs /
              (0.5f * sample_rate_hz_)));
    rrr = std::min(num_freqs, std::max(rrr, kOne)) - 1;
    rr = std::min(num_freqs, std::max(rr, kOne)) - 1;

    float step = ll == lll ? 0.f : 1.f / (ll - lll);
    float element = 0.f;
    for (size_t j = lll; j <= ll; ++j) {
      filter_bank[i - 1][j] = element;
      element += step;
    }
    step = rr == rrr ? 0.f : 1.f / (rrr - rr);
    element = 1.f;
    for (size_t j = rr; j <= rrr; ++j) {
      filter_bank[i - 1][j] = element;
      element -= step;
    }
    for (size_t j = ll; j <= rr; ++j) {
      filter_bank[i - 1][j] = 1.f;
    }
  }

  for (size_t i = 0; i < num_freqs; ++i) {
    float sum = 0.f;
    for (size_t j = 0; j < bank_size_; ++j) {
      sum += filter_bank[j][i];
    }
    for (size_t j = 0; j < bank_size_; ++j) {
      filter_bank[j][i] /= sum;
    }
  }
  return filter_bank;
}

void IntelligibilityEnhancer::SolveForGainsGivenLambda(float lambda,
                                                       size_t start_freq,
                                                       float* sols) {
  bool quadratic = (kRho < 1.f);
  const float* pow_x0 = filtered_clear_pow_.get();
  const float* pow_n0 = filtered_noise_pow_.get();

  for (size_t n = 0; n < start_freq; ++n) {
    sols[n] = 1.f;
  }

  // Analytic solution for optimal gains. See paper for derivation.
  for (size_t n = start_freq - 1; n < bank_size_; ++n) {
    float alpha0, beta0, gamma0;
    gamma0 = 0.5f * kRho * pow_x0[n] * pow_n0[n] +
             lambda * pow_x0[n] * pow_n0[n] * pow_n0[n];
    beta0 = lambda * pow_x0[n] * (2 - kRho) * pow_x0[n] * pow_n0[n];
    if (quadratic) {
      alpha0 = lambda * pow_x0[n] * (1 - kRho) * pow_x0[n] * pow_x0[n];
      sols[n] =
          (-beta0 - sqrtf(beta0 * beta0 - 4 * alpha0 * gamma0)) /
          (2 * alpha0 + std::numeric_limits<float>::epsilon());
    } else {
      sols[n] = -gamma0 / beta0;
    }
    sols[n] = fmax(0, sols[n]);
  }
}

bool IntelligibilityEnhancer::IsSpeech(const float* audio) {
  FloatToS16(audio, chunk_length_, &audio_s16_[0]);
  vad_.ProcessChunk(&audio_s16_[0], chunk_length_, sample_rate_hz_);
  if (vad_.last_voice_probability() > kVoiceProbabilityThreshold) {
    chunks_since_voice_ = 0;
  } else if (chunks_since_voice_ < kSpeechOffsetDelay) {
    ++chunks_since_voice_;
  }
  return chunks_since_voice_ < kSpeechOffsetDelay;
}

}  // namespace webrtc
