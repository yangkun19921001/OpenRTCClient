/*
 *  Copyright (c) 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "media/engine/convert_legacy_video_factory.h"

#include <utility>
#include <vector>

#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "media/base/h264_profile_level_id.h"
#include "media/engine/internaldecoderfactory.h"
#include "media/engine/internalencoderfactory.h"
#include "media/engine/scopedvideodecoder.h"
#include "media/engine/scopedvideoencoder.h"
#include "media/engine/simulcast_encoder_adapter.h"
#include "media/engine/videodecodersoftwarefallbackwrapper.h"
#include "media/engine/videoencodersoftwarefallbackwrapper.h"
#include "media/engine/vp8_encoder_simulcast_proxy.h"
#include "media/engine/webrtcvideodecoderfactory.h"
#include "media/engine/webrtcvideoencoderfactory.h"
#include "rtc_base/checks.h"
#include "rtc_base/ptr_util.h"

namespace cricket {

namespace {

bool IsSameFormat(const webrtc::SdpVideoFormat& format1,
                  const webrtc::SdpVideoFormat& format2) {
  // If different names (case insensitive), then not same formats.
  if (!CodecNamesEq(format1.name, format2.name))
    return false;
  // For every format besides H264, comparing names is enough.
  if (!CodecNamesEq(format1.name.c_str(), kH264CodecName))
    return true;
  // Compare H264 profiles.
  const rtc::Optional<webrtc::H264::ProfileLevelId> profile_level_id =
      webrtc::H264::ParseSdpProfileLevelId(format1.parameters);
  const rtc::Optional<webrtc::H264::ProfileLevelId> other_profile_level_id =
      webrtc::H264::ParseSdpProfileLevelId(format2.parameters);
  // Compare H264 profiles, but not levels.
  return profile_level_id && other_profile_level_id &&
         profile_level_id->profile == other_profile_level_id->profile;
}

bool IsFormatSupported(
    const std::vector<webrtc::SdpVideoFormat>& supported_formats,
    const webrtc::SdpVideoFormat& format) {
  for (const webrtc::SdpVideoFormat& supported_format : supported_formats) {
    if (IsSameFormat(format, supported_format))
      return true;
  }
  return false;
}

class EncoderAdapter : public webrtc::VideoEncoderFactory {
 public:
  explicit EncoderAdapter(
      std::unique_ptr<WebRtcVideoEncoderFactory> external_encoder_factory)
      : internal_encoder_factory_(new InternalEncoderFactory()),
        external_encoder_factory_(std::move(external_encoder_factory)) {}

  webrtc::VideoEncoderFactory::CodecInfo QueryVideoEncoder(
      const webrtc::SdpVideoFormat& format) const {
    const VideoCodec codec(format);
    if (external_encoder_factory_ != nullptr &&
        FindMatchingCodec(external_encoder_factory_->supported_codecs(),
                          codec)) {
      // Format is supported by the external factory.
      const webrtc::VideoCodecType codec_type =
          webrtc::PayloadStringToCodecType(codec.name);
      webrtc::VideoEncoderFactory::CodecInfo info;
      info.has_internal_source =
          external_encoder_factory_->EncoderTypeHasInternalSource(codec_type);
      info.is_hardware_accelerated = true;
      return info;
    }

    // Format must be one of the internal formats.
    RTC_DCHECK(FindMatchingCodec(internal_encoder_factory_->supported_codecs(),
                                 codec));
    webrtc::VideoEncoderFactory::CodecInfo info;
    info.has_internal_source = false;
    info.is_hardware_accelerated = false;
    return info;
  }

  std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(
      const webrtc::SdpVideoFormat& format) {
    const VideoCodec codec(format);

    // Try creating internal encoder.
    std::unique_ptr<webrtc::VideoEncoder> internal_encoder;
    if (FindMatchingCodec(internal_encoder_factory_->supported_codecs(),
                          codec)) {
      internal_encoder =
          CodecNamesEq(format.name.c_str(), kVp8CodecName)
              ? rtc::MakeUnique<webrtc::VP8EncoderSimulcastProxy>(
                    internal_encoder_factory_.get())
              : std::unique_ptr<webrtc::VideoEncoder>(
                    internal_encoder_factory_->CreateVideoEncoder(codec));
    }

    // Try creating external encoder.
    std::unique_ptr<webrtc::VideoEncoder> external_encoder;
    if (external_encoder_factory_ != nullptr &&
        FindMatchingCodec(external_encoder_factory_->supported_codecs(),
                          codec)) {
      external_encoder = CodecNamesEq(format.name.c_str(), kVp8CodecName)
                             ? rtc::MakeUnique<webrtc::SimulcastEncoderAdapter>(
                                   external_encoder_factory_.get())
                             : CreateScopedVideoEncoder(
                                   external_encoder_factory_.get(), codec);
    }

    if (internal_encoder && external_encoder) {
      // Both internal SW encoder and external HW encoder available - create
      // fallback encoder.
      return rtc::MakeUnique<webrtc::VideoEncoderSoftwareFallbackWrapper>(
          std::move(internal_encoder), std::move(external_encoder));
    }
    return external_encoder ? std::move(external_encoder)
                            : std::move(internal_encoder);
  }

  std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const {
    std::vector<VideoCodec> codecs =
        InternalEncoderFactory().supported_codecs();

    // Add external codecs.
    if (external_encoder_factory_ != nullptr) {
      const std::vector<VideoCodec>& external_codecs =
          external_encoder_factory_->supported_codecs();
      for (const VideoCodec& codec : external_codecs) {
        // Don't add same codec twice.
        if (!FindMatchingCodec(codecs, codec))
          codecs.push_back(codec);
      }
    }

    std::vector<webrtc::SdpVideoFormat> formats;
    for (const VideoCodec& codec : codecs) {
      formats.push_back(webrtc::SdpVideoFormat(codec.name, codec.params));
    }

    return formats;
  }

 private:
  const std::unique_ptr<WebRtcVideoEncoderFactory> internal_encoder_factory_;
  const std::unique_ptr<WebRtcVideoEncoderFactory> external_encoder_factory_;
};

class DecoderAdapter : public webrtc::VideoDecoderFactory {
 public:
  explicit DecoderAdapter(
      std::unique_ptr<WebRtcVideoDecoderFactory> external_decoder_factory)
      : external_decoder_factory_(std::move(external_decoder_factory)) {}

  std::unique_ptr<webrtc::VideoDecoder> CreateVideoDecoder(
      const webrtc::SdpVideoFormat& format) override {
    std::unique_ptr<webrtc::VideoDecoder> internal_decoder;
    webrtc::InternalDecoderFactory internal_decoder_factory;
    if (IsFormatSupported(internal_decoder_factory.GetSupportedFormats(),
                          format)) {
      internal_decoder = internal_decoder_factory.CreateVideoDecoder(format);
    }

    const VideoCodec codec(format);
    const VideoDecoderParams params = {};
    if (external_decoder_factory_ != nullptr) {
      std::unique_ptr<webrtc::VideoDecoder> external_decoder =
          CreateScopedVideoDecoder(external_decoder_factory_.get(), codec,
                                   params);
      if (external_decoder) {
        if (!internal_decoder)
          return external_decoder;
        // Both external and internal decoder available - create fallback
        // wrapper.
        return std::unique_ptr<webrtc::VideoDecoder>(
            new webrtc::VideoDecoderSoftwareFallbackWrapper(
                std::move(internal_decoder), std::move(external_decoder)));
      }
    }

    return internal_decoder;
  }

  std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override {
    // This is not implemented for the legacy decoder factory.
    RTC_NOTREACHED();
    return std::vector<webrtc::SdpVideoFormat>();
  }

 private:
  const std::unique_ptr<WebRtcVideoDecoderFactory> external_decoder_factory_;
};

}  // namespace

std::unique_ptr<webrtc::VideoEncoderFactory> ConvertVideoEncoderFactory(
    std::unique_ptr<WebRtcVideoEncoderFactory> external_encoder_factory) {
  return std::unique_ptr<webrtc::VideoEncoderFactory>(
      new EncoderAdapter(std::move(external_encoder_factory)));
}

std::unique_ptr<webrtc::VideoDecoderFactory> ConvertVideoDecoderFactory(
    std::unique_ptr<WebRtcVideoDecoderFactory> external_decoder_factory) {
  return std::unique_ptr<webrtc::VideoDecoderFactory>(
      new DecoderAdapter(std::move(external_decoder_factory)));
}

}  // namespace cricket
