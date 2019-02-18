/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef TEST_PC_E2E_PEER_CONNECTION_QUALITY_TEST_H_
#define TEST_PC_E2E_PEER_CONNECTION_QUALITY_TEST_H_

#include <memory>
#include <string>
#include <vector>

#include "pc/test/frame_generator_capturer_video_track_source.h"
#include "rtc_base/thread.h"
#include "system_wrappers/include/clock.h"
#include "test/pc/e2e/analyzer/video/single_process_encoded_image_id_injector.h"
#include "test/pc/e2e/analyzer/video/video_quality_analyzer_injection_helper.h"
#include "test/pc/e2e/api/peerconnection_quality_test_fixture.h"
#include "test/pc/e2e/test_peer.h"
#include "test/testsupport/video_frame_writer.h"

namespace webrtc {
namespace test {

class PeerConnectionE2EQualityTest
    : public PeerConnectionE2EQualityTestFixture {
 public:
  using Params = PeerConnectionE2EQualityTestFixture::Params;
  using Analyzers = PeerConnectionE2EQualityTestFixture::Analyzers;
  using InjectableComponents =
      PeerConnectionE2EQualityTestFixture::InjectableComponents;
  using VideoGeneratorType =
      PeerConnectionE2EQualityTestFixture::VideoGeneratorType;
  using RunParams = PeerConnectionE2EQualityTestFixture::RunParams;
  using VideoConfig = PeerConnectionE2EQualityTestFixture::VideoConfig;

  PeerConnectionE2EQualityTest(std::unique_ptr<Analyzers> analyzers);

  ~PeerConnectionE2EQualityTest() override = default;

  void Run(std::unique_ptr<InjectableComponents> alice_components,
           std::unique_ptr<Params> alice_params,
           std::unique_ptr<InjectableComponents> bob_components,
           std::unique_ptr<Params> bob_params,
           RunParams run_params) override;

 private:
  // Sets video stream labels that are not specified in VideoConfigs to unique
  // generated values.
  void SetMissedVideoStreamLabels(std::vector<Params*> params);
  // Validate peer's parameters, also ensure uniqueness of all video stream
  // labels.
  void ValidateParams(std::vector<Params*> params);
  void SetupVideoSink(rtc::scoped_refptr<RtpTransceiverInterface> transceiver,
                      std::vector<VideoConfig> remote_video_configs);
  // Have to be run on the signaling thread.
  void SetupCallOnSignalingThread();
  void TearDownCallOnSignalingThread();
  std::vector<rtc::scoped_refptr<FrameGeneratorCapturerVideoTrackSource>>
  AddMedia(TestPeer* peer);
  std::vector<rtc::scoped_refptr<FrameGeneratorCapturerVideoTrackSource>>
  AddVideo(TestPeer* peer);
  std::unique_ptr<FrameGenerator> CreateFrameGenerator(
      const VideoConfig& video_config);
  void AddAudio(TestPeer* peer);
  void SetupCall();
  void StartVideo(
      const std::vector<
          rtc::scoped_refptr<FrameGeneratorCapturerVideoTrackSource>>& sources);
  void TearDownCall();
  VideoFrameWriter* MaybeCreateVideoWriter(
      absl::optional<std::string> file_name,
      const VideoConfig& config);

  Clock* const clock_;
  std::unique_ptr<VideoQualityAnalyzerInjectionHelper>
      video_quality_analyzer_injection_helper_;
  std::unique_ptr<SingleProcessEncodedImageIdInjector>
      encoded_image_id_controller_;

  std::unique_ptr<TestPeer> alice_;
  std::unique_ptr<TestPeer> bob_;

  std::vector<rtc::scoped_refptr<FrameGeneratorCapturerVideoTrackSource>>
      alice_video_sources_;
  std::vector<rtc::scoped_refptr<FrameGeneratorCapturerVideoTrackSource>>
      bob_video_sources_;
  std::vector<std::unique_ptr<VideoFrameWriter>> video_writers_;
  std::vector<std::unique_ptr<rtc::VideoSinkInterface<VideoFrame>>>
      output_video_sinks_;
};

}  // namespace test
}  // namespace webrtc

#endif  // TEST_PC_E2E_PEER_CONNECTION_QUALITY_TEST_H_
