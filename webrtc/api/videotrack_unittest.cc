/*
 *  Copyright 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <string>

#include "webrtc/api/remotevideocapturer.h"
#include "webrtc/api/test/fakevideotrackrenderer.h"
#include "webrtc/api/videosource.h"
#include "webrtc/api/videotrack.h"
#include "webrtc/base/gunit.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/media/base/fakemediaengine.h"
#include "webrtc/media/engine/webrtcvideoframe.h"
#include "webrtc/pc/channelmanager.h"

using webrtc::FakeVideoTrackRenderer;
using webrtc::VideoSource;
using webrtc::VideoTrack;
using webrtc::VideoTrackInterface;

namespace {

class WebRtcVideoTestFrame : public cricket::WebRtcVideoFrame {
 public:
  using cricket::WebRtcVideoFrame::SetRotation;
};

}  // namespace

class VideoTrackTest : public testing::Test {
 public:
  VideoTrackTest() {
    static const char kVideoTrackId[] = "track_id";

    channel_manager_.reset(new cricket::ChannelManager(
        new cricket::FakeMediaEngine(), rtc::Thread::Current()));
    EXPECT_TRUE(channel_manager_->Init());
    video_track_ = VideoTrack::Create(
        kVideoTrackId,
        VideoSource::Create(channel_manager_.get(),
                            new webrtc::RemoteVideoCapturer(), NULL, true));
  }

 protected:
  rtc::scoped_ptr<cricket::ChannelManager> channel_manager_;
  rtc::scoped_refptr<VideoTrackInterface> video_track_;
};

// Test adding renderers to a video track and render to them by providing
// frames to the source.
TEST_F(VideoTrackTest, RenderVideo) {
  // FakeVideoTrackRenderer register itself to |video_track_|
  rtc::scoped_ptr<FakeVideoTrackRenderer> renderer_1(
      new FakeVideoTrackRenderer(video_track_.get()));

  rtc::VideoSinkInterface<cricket::VideoFrame>* renderer_input =
      video_track_->GetSink();
  ASSERT_FALSE(renderer_input == NULL);

  cricket::WebRtcVideoFrame frame;
  frame.InitToBlack(123, 123, 0);
  renderer_input->OnFrame(frame);
  EXPECT_EQ(1, renderer_1->num_rendered_frames());

  EXPECT_EQ(123, renderer_1->width());
  EXPECT_EQ(123, renderer_1->height());

  // FakeVideoTrackRenderer register itself to |video_track_|
  rtc::scoped_ptr<FakeVideoTrackRenderer> renderer_2(
      new FakeVideoTrackRenderer(video_track_.get()));

  renderer_input->OnFrame(frame);

  EXPECT_EQ(123, renderer_1->width());
  EXPECT_EQ(123, renderer_1->height());
  EXPECT_EQ(123, renderer_2->width());
  EXPECT_EQ(123, renderer_2->height());

  EXPECT_EQ(2, renderer_1->num_rendered_frames());
  EXPECT_EQ(1, renderer_2->num_rendered_frames());

  video_track_->RemoveRenderer(renderer_1.get());
  renderer_input->OnFrame(frame);

  EXPECT_EQ(2, renderer_1->num_rendered_frames());
  EXPECT_EQ(2, renderer_2->num_rendered_frames());
}

// Test that disabling the track results in blacked out frames.
TEST_F(VideoTrackTest, DisableTrackBlackout) {
  rtc::scoped_ptr<FakeVideoTrackRenderer> renderer(
      new FakeVideoTrackRenderer(video_track_.get()));

  rtc::VideoSinkInterface<cricket::VideoFrame>* renderer_input =
      video_track_->GetSink();
  ASSERT_FALSE(renderer_input == NULL);

  cricket::WebRtcVideoFrame frame;
  frame.InitToBlack(100, 200, 0);
  // Make it not all-black
  frame.GetUPlane()[0] = 0;

  renderer_input->OnFrame(frame);
  EXPECT_EQ(1, renderer->num_rendered_frames());
  EXPECT_FALSE(renderer->black_frame());
  EXPECT_EQ(100, renderer->width());
  EXPECT_EQ(200, renderer->height());

  video_track_->set_enabled(false);
  renderer_input->OnFrame(frame);
  EXPECT_EQ(2, renderer->num_rendered_frames());
  EXPECT_TRUE(renderer->black_frame());
  EXPECT_EQ(100, renderer->width());
  EXPECT_EQ(200, renderer->height());

  video_track_->set_enabled(true);
  renderer_input->OnFrame(frame);
  EXPECT_EQ(3, renderer->num_rendered_frames());
  EXPECT_FALSE(renderer->black_frame());
  EXPECT_EQ(100, renderer->width());
  EXPECT_EQ(200, renderer->height());
}
