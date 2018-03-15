/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <stdio.h>

#include <memory>

#include "api/optional.h"
#include "api/video/i420_buffer.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "modules/video_coding/codecs/test/video_codec_unittest.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp8/temporal_layers.h"
#include "modules/video_coding/utility/vp8_header_parser.h"
#include "test/frame_utils.h"
#include "test/gtest.h"
#include "test/testsupport/fileutils.h"
#include "test/video_codec_settings.h"

namespace webrtc {

namespace {
constexpr uint32_t kInitialTimestampRtp = 123;
constexpr int64_t kTestNtpTimeMs = 456;
constexpr int64_t kInitialTimestampMs = 789;
constexpr uint32_t kTimestampIncrement = 3000;
constexpr int kNumCores = 1;
constexpr size_t kMaxPayloadSize = 1440;
constexpr int kDefaultMinPixelsPerFrame = 320 * 180;
constexpr int kWidth = 172;
constexpr int kHeight = 144;

void Calc16ByteAlignedStride(int width, int* stride_y, int* stride_uv) {
  *stride_y = 16 * ((width + 15) / 16);
  *stride_uv = 16 * ((width + 31) / 32);
}
}  // namespace

class TestVp8Impl : public VideoCodecUnitTest {
 protected:
  std::unique_ptr<VideoEncoder> CreateEncoder() override {
    return VP8Encoder::Create();
  }

  std::unique_ptr<VideoDecoder> CreateDecoder() override {
    return VP8Decoder::Create();
  }

  VideoCodec codec_settings() override {
    VideoCodec codec_settings;
    webrtc::test::CodecSettings(kVideoCodecVP8, &codec_settings);
    codec_settings.VP8()->denoisingOn = true;
    codec_settings.VP8()->frameDroppingOn = false;
    codec_settings.VP8()->automaticResizeOn = false;
    codec_settings.VP8()->complexity = kComplexityNormal;
    codec_settings.VP8()->tl_factory = &tl_factory_;
    return codec_settings;
  }

  void SetupInputFrame() {
    // Using a QCIF image (aligned stride (u,v planes) > width).
    // Processing only one frame.
    FILE* file = fopen(test::ResourcePath("paris_qcif", "yuv").c_str(), "rb");
    ASSERT_TRUE(file != nullptr);
    rtc::scoped_refptr<I420BufferInterface> compact_buffer(
        test::ReadI420Buffer(kWidth, kHeight, file));
    ASSERT_TRUE(compact_buffer);

    // Setting aligned stride values.
    int stride_uv;
    int stride_y;
    Calc16ByteAlignedStride(kWidth, &stride_y, &stride_uv);
    EXPECT_EQ(stride_y, 176);
    EXPECT_EQ(stride_uv, 96);
    rtc::scoped_refptr<I420Buffer> stride_buffer(
        I420Buffer::Create(kWidth, kHeight, stride_y, stride_uv, stride_uv));

    // No scaling in our case, just a copy, to add stride to the image.
    stride_buffer->ScaleFrom(*compact_buffer);

    input_frame_.reset(new VideoFrame(stride_buffer, kInitialTimestampRtp,
                                      kInitialTimestampMs, kVideoRotation_0));
    fclose(file);
  }

  void EncodeAndWaitForFrame(EncodedImage* encoded_frame,
                             CodecSpecificInfo* codec_specific_info) {
    EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
              encoder_->Encode(*input_frame_, nullptr, nullptr));
    ASSERT_TRUE(WaitForEncodedFrame(encoded_frame, codec_specific_info));
    VerifyQpParser(*encoded_frame);
    EXPECT_STREQ("libvpx", codec_specific_info->codec_name);
    EXPECT_EQ(kVideoCodecVP8, codec_specific_info->codecType);
    EXPECT_EQ(0u, codec_specific_info->codecSpecific.VP8.simulcastIdx);
  }

  void EncodeAndExpectFrameWith(int16_t picture_id,
                                int tl0_pic_idx,
                                uint8_t temporal_idx) {
    EncodedImage encoded_frame;
    CodecSpecificInfo codec_specific_info;
    EncodeAndWaitForFrame(&encoded_frame, &codec_specific_info);
    EXPECT_EQ(picture_id % (1 << 15),
              codec_specific_info.codecSpecific.VP8.pictureId);
    EXPECT_EQ(tl0_pic_idx % (1 << 8),
              codec_specific_info.codecSpecific.VP8.tl0PicIdx);
    EXPECT_EQ(temporal_idx, codec_specific_info.codecSpecific.VP8.temporalIdx);
  }

  void VerifyQpParser(const EncodedImage& encoded_frame) const {
    int qp;
    EXPECT_GT(encoded_frame._length, 0u);
    ASSERT_TRUE(vp8::GetQp(encoded_frame._buffer, encoded_frame._length, &qp));
    EXPECT_EQ(encoded_frame.qp_, qp) << "Encoder QP != parsed bitstream QP.";
  }

  TemporalLayersFactory tl_factory_;
};

TEST_F(TestVp8Impl, SetRateAllocation) {
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, encoder_->Release());

  const int kBitrateBps = 300000;
  BitrateAllocation bitrate_allocation;
  bitrate_allocation.SetBitrate(0, 0, kBitrateBps);
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_UNINITIALIZED,
            encoder_->SetRateAllocation(bitrate_allocation,
                                        codec_settings_.maxFramerate));
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            encoder_->SetRateAllocation(bitrate_allocation,
                                        codec_settings_.maxFramerate));
}

TEST_F(TestVp8Impl, EncodeFrameAndRelease) {
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, encoder_->Release());
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));

  EncodedImage encoded_frame;
  CodecSpecificInfo codec_specific_info;
  EncodeAndWaitForFrame(&encoded_frame, &codec_specific_info);

  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, encoder_->Release());
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_UNINITIALIZED,
            encoder_->Encode(*input_frame_, nullptr, nullptr));
}

TEST_F(TestVp8Impl, InitDecode) {
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, decoder_->Release());
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            decoder_->InitDecode(&codec_settings_, kNumCores));
}

TEST_F(TestVp8Impl, OnEncodedImageReportsInfo) {
  SetupInputFrame();
  EncodedImage encoded_frame;
  CodecSpecificInfo codec_specific_info;
  EncodeAndWaitForFrame(&encoded_frame, &codec_specific_info);

  EXPECT_EQ(kInitialTimestampRtp, encoded_frame._timeStamp);
  EXPECT_EQ(kInitialTimestampMs, encoded_frame.capture_time_ms_);
  EXPECT_EQ(kWidth, static_cast<int>(encoded_frame._encodedWidth));
  EXPECT_EQ(kHeight, static_cast<int>(encoded_frame._encodedHeight));
}

// We only test the encoder here, since the decoded frame rotation is set based
// on the CVO RTP header extension in VCMDecodedFrameCallback::Decoded.
// TODO(brandtr): Consider passing through the rotation flag through the decoder
// in the same way as done in the encoder.
TEST_F(TestVp8Impl, EncodedRotationEqualsInputRotation) {
  input_frame_->set_rotation(kVideoRotation_0);

  EncodedImage encoded_frame;
  CodecSpecificInfo codec_specific_info;
  EncodeAndWaitForFrame(&encoded_frame, &codec_specific_info);
  EXPECT_EQ(kVideoRotation_0, encoded_frame.rotation_);

  input_frame_->set_rotation(kVideoRotation_90);
  EncodeAndWaitForFrame(&encoded_frame, &codec_specific_info);
  EXPECT_EQ(kVideoRotation_90, encoded_frame.rotation_);
}

TEST_F(TestVp8Impl, DecodedQpEqualsEncodedQp) {
  EncodedImage encoded_frame;
  CodecSpecificInfo codec_specific_info;
  EncodeAndWaitForFrame(&encoded_frame, &codec_specific_info);

  // First frame should be a key frame.
  encoded_frame._frameType = kVideoFrameKey;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            decoder_->Decode(encoded_frame, false, nullptr));
  std::unique_ptr<VideoFrame> decoded_frame;
  rtc::Optional<uint8_t> decoded_qp;
  ASSERT_TRUE(WaitForDecodedFrame(&decoded_frame, &decoded_qp));
  ASSERT_TRUE(decoded_frame);
  ASSERT_TRUE(decoded_qp);
  EXPECT_GT(I420PSNR(input_frame_.get(), decoded_frame.get()), 36);
  EXPECT_EQ(encoded_frame.qp_, *decoded_qp);
}

TEST_F(TestVp8Impl, ChecksSimulcastSettings) {
  codec_settings_.numberOfSimulcastStreams = 2;
  // Reslutions are not scaled by 2, temporal layers do not match.
  codec_settings_.simulcastStream[0] = {kWidth, kHeight, 2, 4000,
                                        3000,   2000,    80};
  codec_settings_.simulcastStream[1] = {kWidth, kHeight, 3, 4000,
                                        3000,   2000,    80};
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));
  codec_settings_.numberOfSimulcastStreams = 3;
  // Reslutions are not scaled by 2.
  codec_settings_.simulcastStream[0] = {kWidth / 2, kHeight / 2, 1, 4000,
                                        3000,       2000,        80};
  codec_settings_.simulcastStream[1] = {kWidth / 2, kHeight / 2, 1, 4000,
                                        3000,       2000,        80};
  codec_settings_.simulcastStream[2] = {kWidth, kHeight, 1, 4000,
                                        3000,   2000,    80};
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));
  // Reslutions are not scaled by 2.
  codec_settings_.simulcastStream[0] = {kWidth, kHeight, 1, 4000,
                                        3000,   2000,    80};
  codec_settings_.simulcastStream[1] = {kWidth, kHeight, 1, 4000,
                                        3000,   2000,    80};
  codec_settings_.simulcastStream[2] = {kWidth, kHeight, 1, 4000,
                                        3000,   2000,    80};
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));
  // Temporal layers do not match.
  codec_settings_.simulcastStream[0] = {kWidth / 4, kHeight / 4, 1, 4000,
                                        3000,       2000,        80};
  codec_settings_.simulcastStream[1] = {kWidth / 2, kHeight / 2, 2, 4000,
                                        3000,       2000,        80};
  codec_settings_.simulcastStream[2] = {kWidth, kHeight, 3, 4000,
                                        3000,   2000,    80};
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));
  // Resolutions do not match codec config.
  codec_settings_.simulcastStream[0] = {
      kWidth / 4 + 1, kHeight / 4 + 1, 1, 4000, 3000, 2000, 80};
  codec_settings_.simulcastStream[1] = {
      kWidth / 2 + 2, kHeight / 2 + 2, 1, 4000, 3000, 2000, 80};
  codec_settings_.simulcastStream[2] = {kWidth + 4, kHeight + 4, 1, 4000,
                                        3000,       2000,        80};
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));
  // Everything fine: scaling by 2, top resolution matches video, temporal
  // settings are the same for all layers.
  codec_settings_.simulcastStream[0] = {kWidth / 4, kHeight / 4, 1, 4000,
                                        3000,       2000,        80};
  codec_settings_.simulcastStream[1] = {kWidth / 2, kHeight / 2, 1, 4000,
                                        3000,       2000,        80};
  codec_settings_.simulcastStream[2] = {kWidth, kHeight, 1, 4000,
                                        3000,   2000,    80};
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));
}

#if defined(WEBRTC_ANDROID)
#define MAYBE_AlignedStrideEncodeDecode DISABLED_AlignedStrideEncodeDecode
#else
#define MAYBE_AlignedStrideEncodeDecode AlignedStrideEncodeDecode
#endif
TEST_F(TestVp8Impl, MAYBE_AlignedStrideEncodeDecode) {
  SetupInputFrame();
  EncodedImage encoded_frame;
  CodecSpecificInfo codec_specific_info;
  EncodeAndWaitForFrame(&encoded_frame, &codec_specific_info);

  // First frame should be a key frame.
  encoded_frame._frameType = kVideoFrameKey;
  encoded_frame.ntp_time_ms_ = kTestNtpTimeMs;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            decoder_->Decode(encoded_frame, false, nullptr));

  std::unique_ptr<VideoFrame> decoded_frame;
  rtc::Optional<uint8_t> decoded_qp;
  ASSERT_TRUE(WaitForDecodedFrame(&decoded_frame, &decoded_qp));
  ASSERT_TRUE(decoded_frame);
  // Compute PSNR on all planes (faster than SSIM).
  EXPECT_GT(I420PSNR(input_frame_.get(), decoded_frame.get()), 36);
  EXPECT_EQ(kInitialTimestampRtp, decoded_frame->timestamp());
  EXPECT_EQ(kTestNtpTimeMs, decoded_frame->ntp_time_ms());
}

#if defined(WEBRTC_ANDROID)
#define MAYBE_DecodeWithACompleteKeyFrame DISABLED_DecodeWithACompleteKeyFrame
#else
#define MAYBE_DecodeWithACompleteKeyFrame DecodeWithACompleteKeyFrame
#endif
TEST_F(TestVp8Impl, MAYBE_DecodeWithACompleteKeyFrame) {
  EncodedImage encoded_frame;
  CodecSpecificInfo codec_specific_info;
  EncodeAndWaitForFrame(&encoded_frame, &codec_specific_info);

  // Setting complete to false -> should return an error.
  encoded_frame._completeFrame = false;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_ERROR,
            decoder_->Decode(encoded_frame, false, nullptr));
  // Setting complete back to true. Forcing a delta frame.
  encoded_frame._frameType = kVideoFrameDelta;
  encoded_frame._completeFrame = true;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_ERROR,
            decoder_->Decode(encoded_frame, false, nullptr));
  // Now setting a key frame.
  encoded_frame._frameType = kVideoFrameKey;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            decoder_->Decode(encoded_frame, false, nullptr));
  std::unique_ptr<VideoFrame> decoded_frame;
  rtc::Optional<uint8_t> decoded_qp;
  ASSERT_TRUE(WaitForDecodedFrame(&decoded_frame, &decoded_qp));
  ASSERT_TRUE(decoded_frame);
  EXPECT_GT(I420PSNR(input_frame_.get(), decoded_frame.get()), 36);
}

TEST_F(TestVp8Impl, EncoderWith2TemporalLayersRetainsRtpStateAfterRelease) {
  codec_settings_.VP8()->numberOfTemporalLayers = 2;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));

  // Temporal layer 0.
  EncodedImage encoded_frame;
  CodecSpecificInfo codec_specific_info;
  EncodeAndWaitForFrame(&encoded_frame, &codec_specific_info);

  EXPECT_EQ(0, codec_specific_info.codecSpecific.VP8.temporalIdx);
  const int16_t picture_id = codec_specific_info.codecSpecific.VP8.pictureId;
  const int tl0_pic_idx = codec_specific_info.codecSpecific.VP8.tl0PicIdx;
  // Temporal layer 1.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);

  EncodeAndExpectFrameWith(picture_id + 1, tl0_pic_idx + 0, 1);
  // Temporal layer 0.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 2, tl0_pic_idx + 1, 0);
  // Temporal layer 1.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 3, tl0_pic_idx + 1, 1);

  // Reinit.
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, encoder_->Release());
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));

  // Temporal layer 0.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 4, tl0_pic_idx + 2, 0);
  // Temporal layer 1.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 5, tl0_pic_idx + 2, 1);
  // Temporal layer 0.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 6, tl0_pic_idx + 3, 0);
  // Temporal layer 1.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 7, tl0_pic_idx + 3, 1);
}

TEST_F(TestVp8Impl, EncoderWith3TemporalLayersRetainsRtpStateAfterRelease) {
  codec_settings_.VP8()->numberOfTemporalLayers = 3;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));

  EncodedImage encoded_frame;
  CodecSpecificInfo codec_specific_info;
  EncodeAndWaitForFrame(&encoded_frame, &codec_specific_info);

  // Temporal layer 0.
  EXPECT_EQ(0, codec_specific_info.codecSpecific.VP8.temporalIdx);
  const int16_t picture_id = codec_specific_info.codecSpecific.VP8.pictureId;
  const int tl0_pic_idx = codec_specific_info.codecSpecific.VP8.tl0PicIdx;
  // Temporal layer 2.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 1, tl0_pic_idx + 0, 2);
  // Temporal layer 1.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 2, tl0_pic_idx + 0, 1);
  // Temporal layer 2.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 3, tl0_pic_idx + 0, 2);

  // Reinit.
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, encoder_->Release());
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));

  // Temporal layer 0.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 4, tl0_pic_idx + 1, 0);
  // Temporal layer 2.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 5, tl0_pic_idx + 1, 2);
  // Temporal layer 1.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 6, tl0_pic_idx + 1, 1);
  // Temporal layer 2.
  input_frame_->set_timestamp(input_frame_->timestamp() + kTimestampIncrement);
  EncodeAndExpectFrameWith(picture_id + 7, tl0_pic_idx + 1, 2);
}

TEST_F(TestVp8Impl, ScalingDisabledIfAutomaticResizeOff) {
  codec_settings_.VP8()->frameDroppingOn = true;
  codec_settings_.VP8()->automaticResizeOn = false;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));

  VideoEncoder::ScalingSettings settings = encoder_->GetScalingSettings();
  EXPECT_FALSE(settings.thresholds.has_value());
}

TEST_F(TestVp8Impl, ScalingEnabledIfAutomaticResizeOn) {
  codec_settings_.VP8()->frameDroppingOn = true;
  codec_settings_.VP8()->automaticResizeOn = true;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            encoder_->InitEncode(&codec_settings_, kNumCores, kMaxPayloadSize));

  VideoEncoder::ScalingSettings settings = encoder_->GetScalingSettings();
  EXPECT_TRUE(settings.thresholds.has_value());
  EXPECT_EQ(kDefaultMinPixelsPerFrame, settings.min_pixels_per_frame);
}

}  // namespace webrtc
