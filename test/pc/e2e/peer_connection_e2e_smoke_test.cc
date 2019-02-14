/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <cstdint>
#include <memory>

#include "absl/memory/memory.h"
#include "call/simulated_network.h"
#include "rtc_base/async_invoker.h"
#include "rtc_base/fake_network.h"
#include "test/gtest.h"
#include "test/pc/e2e/analyzer/video/default_video_quality_analyzer.h"
#include "test/pc/e2e/api/create_peerconnection_quality_test_fixture.h"
#include "test/pc/e2e/api/peerconnection_quality_test_fixture.h"
#include "test/scenario/network/network_emulation.h"
#include "test/scenario/network/network_emulation_manager.h"
#include "test/testsupport/file_utils.h"

namespace webrtc {
namespace test {
namespace {

std::unique_ptr<rtc::NetworkManager> CreateFakeNetworkManager(
    std::vector<EndpointNode*> endpoints) {
  auto network_manager = absl::make_unique<rtc::FakeNetworkManager>();
  for (auto* endpoint : endpoints) {
    network_manager->AddInterface(
        rtc::SocketAddress(endpoint->GetPeerLocalAddress(), /*port=*/0));
  }
  return network_manager;
}

}  // namespace

TEST(PeerConnectionE2EQualityTestSmokeTest, RunWithEmulatedNetwork) {
  using Params = PeerConnectionE2EQualityTestFixture::Params;
  using RunParams = PeerConnectionE2EQualityTestFixture::RunParams;
  using VideoGeneratorType =
      PeerConnectionE2EQualityTestFixture::VideoGeneratorType;
  using Analyzers = PeerConnectionE2EQualityTestFixture::Analyzers;
  using VideoConfig = PeerConnectionE2EQualityTestFixture::VideoConfig;
  using AudioConfig = PeerConnectionE2EQualityTestFixture::AudioConfig;
  using InjectableComponents =
      PeerConnectionE2EQualityTestFixture::InjectableComponents;

  auto alice_params = absl::make_unique<Params>();
  VideoConfig alice_video_config;
  alice_video_config.width = 1280;
  alice_video_config.height = 720;
  alice_video_config.fps = 30;
  alice_video_config.stream_label = "alice-video";
  alice_video_config.generator = VideoGeneratorType::kDefault;

  alice_params->video_configs.push_back(alice_video_config);
  alice_params->audio_config = AudioConfig{
      AudioConfig::Mode::kGenerated,
      /*input_file_name=*/absl::nullopt,
      /*input_dump_file_name=*/absl::nullopt,
      /*output_dump_file_name=*/absl::nullopt, cricket::AudioOptions()};

  // Setup emulated network
  NetworkEmulationManager network_emulation_manager;

  EmulatedNetworkNode* alice_node =
      network_emulation_manager.CreateEmulatedNode(
          absl::make_unique<SimulatedNetwork>(BuiltInNetworkBehaviorConfig()));
  EmulatedNetworkNode* bob_node = network_emulation_manager.CreateEmulatedNode(
      absl::make_unique<SimulatedNetwork>(BuiltInNetworkBehaviorConfig()));
  EndpointNode* alice_endpoint =
      network_emulation_manager.CreateEndpoint(rtc::IPAddress(1));
  EndpointNode* bob_endpoint =
      network_emulation_manager.CreateEndpoint(rtc::IPAddress(2));
  network_emulation_manager.CreateRoute(alice_endpoint, {alice_node},
                                        bob_endpoint);
  network_emulation_manager.CreateRoute(bob_endpoint, {bob_node},
                                        alice_endpoint);

  rtc::Thread* alice_network_thread =
      network_emulation_manager.CreateNetworkThread({alice_endpoint});
  rtc::Thread* bob_network_thread =
      network_emulation_manager.CreateNetworkThread({bob_endpoint});

  // Setup components. We need to provide rtc::NetworkManager compatible with
  // emulated network layer.
  auto alice_components =
      absl::make_unique<InjectableComponents>(alice_network_thread);
  alice_components->pc_dependencies->network_manager =
      CreateFakeNetworkManager({alice_endpoint});
  auto bob_components =
      absl::make_unique<InjectableComponents>(bob_network_thread);
  bob_components->pc_dependencies->network_manager =
      CreateFakeNetworkManager({bob_endpoint});

  // Create analyzers.
  auto analyzers = absl::make_unique<Analyzers>();
  analyzers->video_quality_analyzer =
      absl::make_unique<DefaultVideoQualityAnalyzer>("smoke_test");
  auto* video_analyzer = static_cast<DefaultVideoQualityAnalyzer*>(
      analyzers->video_quality_analyzer.get());

  auto fixture =
      CreatePeerConnectionE2EQualityTestFixture(std::move(analyzers));
  fixture->Run(std::move(alice_components), std::move(alice_params),
               std::move(bob_components), absl::make_unique<Params>(),
               RunParams{TimeDelta::seconds(5)});

  // 150 = 30fps * 5s. On some devices pipeline can be too slow, so it can
  // happen, that frames will stuck in the middle, so we actually can't force
  // real constraints here, so lets just check, that at least 1 frame passed
  // whole pipeline.
  EXPECT_GE(video_analyzer->GetGlobalCounters().captured, 150);
  EXPECT_GE(video_analyzer->GetGlobalCounters().pre_encoded, 1);
  EXPECT_GE(video_analyzer->GetGlobalCounters().encoded, 1);
  EXPECT_GE(video_analyzer->GetGlobalCounters().received, 1);
  EXPECT_GE(video_analyzer->GetGlobalCounters().decoded, 1);
  EXPECT_GE(video_analyzer->GetGlobalCounters().rendered, 1);
}

}  // namespace test
}  // namespace webrtc
