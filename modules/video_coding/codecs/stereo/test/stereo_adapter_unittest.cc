/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "api/test/mock_video_decoder_factory.h"
#include "api/test/mock_video_encoder_factory.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "modules/video_coding/codecs/stereo/include/stereo_decoder_adapter.h"
#include "modules/video_coding/codecs/stereo/include/stereo_encoder_adapter.h"
#include "modules/video_coding/codecs/test/video_codec_test.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"

using testing::_;
using testing::Return;

namespace webrtc {

class TestStereoAdapter : public VideoCodecTest {
 public:
  TestStereoAdapter()
      : decoder_factory_(new webrtc::MockVideoDecoderFactory),
        encoder_factory_(new webrtc::MockVideoEncoderFactory) {}

 protected:
  VideoDecoder* CreateDecoder() override {
    return new StereoDecoderAdapter(decoder_factory_.get());
  }

  VideoEncoder* CreateEncoder() override {
    return new StereoEncoderAdapter(encoder_factory_.get());
  }

  VideoCodec codec_settings() override {
    VideoCodec codec_settings;
    codec_settings.codecType = webrtc::kVideoCodecVP9;
    codec_settings.VP9()->numberOfTemporalLayers = 1;
    codec_settings.VP9()->numberOfSpatialLayers = 1;
    return codec_settings;
  }

 private:
  void SetUp() override {
    EXPECT_CALL(*decoder_factory_, Die());
    VideoDecoder* decoder1 = VP9Decoder::Create();
    VideoDecoder* decoder2 = VP9Decoder::Create();
    EXPECT_CALL(*decoder_factory_, CreateVideoDecoderProxy(_))
        .WillOnce(Return(decoder1))
        .WillOnce(Return(decoder2));

    EXPECT_CALL(*encoder_factory_, Die());
    VideoEncoder* encoder1 = VP9Encoder::Create();
    VideoEncoder* encoder2 = VP9Encoder::Create();
    EXPECT_CALL(*encoder_factory_, CreateVideoEncoderProxy(_))
        .WillOnce(Return(encoder1))
        .WillOnce(Return(encoder2));

    VideoCodecTest::SetUp();
  }

  const std::unique_ptr<webrtc::MockVideoDecoderFactory> decoder_factory_;
  const std::unique_ptr<webrtc::MockVideoEncoderFactory> encoder_factory_;
};

// TODO(emircan): Currently VideoCodecTest tests do a complete setup
// step that goes beyond constructing |decoder_|. Simplify these tests to do
// less.
TEST_F(TestStereoAdapter, ConstructAndDestructDecoder) {
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, decoder_->Release());
}

TEST_F(TestStereoAdapter, ConstructAndDestructEncoder) {
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, encoder_->Release());
}

TEST_F(TestStereoAdapter, EncodeDecodeI420Frame) {
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            encoder_->Encode(*input_frame_, nullptr, nullptr));
  EncodedImage encoded_frame;
  CodecSpecificInfo codec_specific_info;
  ASSERT_TRUE(WaitForEncodedFrame(&encoded_frame, &codec_specific_info));
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            decoder_->Decode(encoded_frame, false, nullptr));
  std::unique_ptr<VideoFrame> decoded_frame;
  rtc::Optional<uint8_t> decoded_qp;
  ASSERT_TRUE(WaitForDecodedFrame(&decoded_frame, &decoded_qp));
  ASSERT_TRUE(decoded_frame);
  EXPECT_GT(I420PSNR(input_frame_.get(), decoded_frame.get()), 36);
}

}  // namespace webrtc
