/*
 *  Copyright 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import "WebRTC/RTCVideoCodecH264.h"

#include <vector>

#import "RTCVideoCodec+Private.h"
#import "WebRTC/RTCVideoCodec.h"
#import "WebRTC/RTCVideoFrame.h"
#import "WebRTC/RTCVideoFrameBuffer.h"

#include "webrtc/rtc_base/timeutils.h"
#include "webrtc/sdk/objc/Framework/Classes/Video/objc_frame_buffer.h"
#include "webrtc/sdk/objc/Framework/Classes/VideoToolbox/decoder.h"
#include "webrtc/sdk/objc/Framework/Classes/VideoToolbox/encoder.h"
#include "webrtc/system_wrappers/include/field_trial.h"

const size_t kDefaultPayloadSize = 1440;

const char kHighProfileExperiment[] = "WebRTC-H264HighProfile";

// These thresholds deviate from the default h264 QP thresholds, as they have been found to work
// better on devices that support VideoToolbox
const int kLowH264QpThreshold = 28;
const int kHighH264QpThreshold = 39;

bool IsHighProfileEnabled() {
  return webrtc::field_trial::IsEnabled(kHighProfileExperiment);
}

// H264 specific settings.
@implementation RTCCodecSpecificInfoH264

@synthesize packetizationMode = _packetizationMode;

- (webrtc::CodecSpecificInfo)nativeCodecSpecificInfo {
  webrtc::CodecSpecificInfo codecSpecificInfo;
  codecSpecificInfo.codecType = webrtc::kVideoCodecH264;
  codecSpecificInfo.codec_name = "H264";
  codecSpecificInfo.codecSpecific.H264.packetization_mode =
      (webrtc::H264PacketizationMode)_packetizationMode;

  return codecSpecificInfo;
}

@end

namespace {

class H264VideoToolboxEncodeCompleteCallback : public webrtc::EncodedImageCallback {
 public:
  Result OnEncodedImage(const webrtc::EncodedImage &encoded_image,
                        const webrtc::CodecSpecificInfo *codec_specific_info,
                        const webrtc::RTPFragmentationHeader *fragmentation) {
    RTCEncodedImage *image = [[RTCEncodedImage alloc] initWithNativeEncodedImage:encoded_image];

    RTCCodecSpecificInfoH264 *info = [[RTCCodecSpecificInfoH264 alloc] init];
    info.packetizationMode =
        (RTCH264PacketizationMode)codec_specific_info->codecSpecific.H264.packetization_mode;

    RTCRtpFragmentationHeader *header =
        [[RTCRtpFragmentationHeader alloc] initWithNativeFragmentationHeader:fragmentation];

    callback(image, info, header);
    return Result(Result::OK, 0);
  }

  RTCVideoEncoderCallback callback;
};

class H264VideoToolboxDecodeCompleteCallback : public webrtc::DecodedImageCallback {
 public:
  int32_t Decoded(webrtc::VideoFrame &decodedImage) {
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> video_frame_buffer =
        decodedImage.video_frame_buffer();
    id<RTCVideoFrameBuffer> rtcFrameBuffer;
    rtc::scoped_refptr<webrtc::ObjCFrameBuffer> objc_frame_buffer(
        static_cast<webrtc::ObjCFrameBuffer *>(video_frame_buffer.get()));
    rtcFrameBuffer = (id<RTCVideoFrameBuffer>)objc_frame_buffer->wrapped_frame_buffer();

    RTCVideoFrame *videoFrame = [[RTCVideoFrame alloc]
        initWithBuffer:rtcFrameBuffer
              rotation:static_cast<RTCVideoRotation>(decodedImage.rotation())
           timeStampNs:decodedImage.timestamp_us() * rtc::kNumNanosecsPerMicrosec];
    videoFrame.timeStamp = decodedImage.timestamp();

    callback(videoFrame);

    return 0;
  }

  RTCVideoDecoderCallback callback;
};

}  // namespace

// Encoder.
@implementation RTCVideoEncoderH264 {
  webrtc::H264VideoToolboxEncoder *_videoToolboxEncoder;
  H264VideoToolboxEncodeCompleteCallback *_toolboxCallback;
}

- (instancetype)initWithCodecInfo:(RTCVideoCodecInfo *)codecInfo {
  if (self = [super init]) {
    cricket::VideoCodec codec = [codecInfo nativeVideoCodec];
    _videoToolboxEncoder = new webrtc::H264VideoToolboxEncoder(codec);
  }
  return self;
}

- (void)setCallback:(RTCVideoEncoderCallback)callback {
  if (!_toolboxCallback) _toolboxCallback = new H264VideoToolboxEncodeCompleteCallback();
  _toolboxCallback->callback = callback;
  _videoToolboxEncoder->RegisterEncodeCompleteCallback(_toolboxCallback);
}

- (void)destroy {
  delete _videoToolboxEncoder;
  _videoToolboxEncoder = nullptr;
  delete _toolboxCallback;
  _toolboxCallback = nullptr;
}

-(void)dealloc {
  // Make sure C++ objects have been properly cleaned up before this
  // is dealloc'd.
  RTC_DCHECK(!_videoToolboxEncoder);
  RTC_DCHECK(!_toolboxCallback);
}

- (NSInteger)startEncodeWithSettings:(RTCVideoEncoderSettings *)settings
                       numberOfCores:(int)numberOfCores {
  std::unique_ptr<webrtc::VideoCodec> codecSettings = [settings createNativeVideoEncoderSettings];
  return _videoToolboxEncoder->InitEncode(
      codecSettings.release(), numberOfCores, kDefaultPayloadSize);
}

- (NSInteger)releaseEncoder {
  return _videoToolboxEncoder->Release();
}

- (NSInteger)encode:(RTCVideoFrame *)frame
    codecSpecificInfo:(id<RTCCodecSpecificInfo>)info
           frameTypes:(NSArray<NSNumber *> *)frameTypes {
  rtc::scoped_refptr<webrtc::VideoFrameBuffer> frameBuffer =
      new rtc::RefCountedObject<webrtc::ObjCFrameBuffer>(frame.buffer);
  webrtc::VideoFrame videoFrame(frameBuffer,
                                (webrtc::VideoRotation)frame.rotation,
                                frame.timeStampNs / rtc::kNumNanosecsPerMicrosec);
  videoFrame.set_timestamp(frame.timeStamp);

  // Handle types than can be converted into one of webrtc::CodecSpecificInfo's hard coded cases.
  webrtc::CodecSpecificInfo codecSpecificInfo;
  if ([info isKindOfClass:[RTCCodecSpecificInfoH264 class]]) {
    codecSpecificInfo = [(RTCCodecSpecificInfoH264 *)info nativeCodecSpecificInfo];
  }

  std::vector<webrtc::FrameType> nativeFrameTypes;
  for (NSNumber *frameType in frameTypes) {
    RTCFrameType rtcFrameType = (RTCFrameType)frameType.unsignedIntegerValue;
    nativeFrameTypes.push_back((webrtc::FrameType)rtcFrameType);
  }

  return _videoToolboxEncoder->Encode(videoFrame, &codecSpecificInfo, &nativeFrameTypes);
}

- (BOOL)setBitrate:(uint32_t)bitrateKbit framerate:(uint32_t)framerate {
  return _videoToolboxEncoder->SetRates(bitrateKbit, framerate) == WEBRTC_VIDEO_CODEC_OK;
}

- (RTCVideoEncoderQpThresholds *)scalingSettings {
  return [[RTCVideoEncoderQpThresholds alloc] initWithThresholdsLow:kLowH264QpThreshold
                                                               high:kHighH264QpThreshold];
}

@end

// Decoder.
@implementation RTCVideoDecoderH264 {
  webrtc::H264VideoToolboxDecoder *_videoToolboxDecoder;
  H264VideoToolboxDecodeCompleteCallback *_toolboxCallback;
}

- (instancetype)init {
  if (self = [super init]) {
    cricket::VideoCodec codec(cricket::kH264CodecName);
    _videoToolboxDecoder = new webrtc::H264VideoToolboxDecoder();
  }
  return self;
}

- (NSInteger)startDecodeWithSettings:(RTCVideoEncoderSettings *)settings
                       numberOfCores:(int)numberOfCores {
  std::unique_ptr<webrtc::VideoCodec> codecSettings = [settings createNativeVideoEncoderSettings];
  return _videoToolboxDecoder->InitDecode(codecSettings.release(), numberOfCores);
}

- (void)setCallback:(RTCVideoDecoderCallback)callback {
  if (!_toolboxCallback) _toolboxCallback = new H264VideoToolboxDecodeCompleteCallback();
  _toolboxCallback->callback = callback;
  _videoToolboxDecoder->RegisterDecodeCompleteCallback(_toolboxCallback);
}

- (NSInteger)releaseDecoder {
  return _videoToolboxDecoder->Release();
}

- (void)destroy {
  delete _videoToolboxDecoder;
  _videoToolboxDecoder = nullptr;
  delete _toolboxCallback;
  _toolboxCallback = nullptr;
}

-(void)dealloc {
  // Make sure C++ objects have been properly cleaned up before this
  // is dealloc'd.
  RTC_DCHECK(!_videoToolboxDecoder);
  RTC_DCHECK(!_toolboxCallback);
}

- (NSInteger)decode:(RTCEncodedImage *)encodedImage
          missingFrames:(BOOL)missingFrames
    fragmentationHeader:(RTCRtpFragmentationHeader *)fragmentationHeader
      codecSpecificInfo:(__nullable id<RTCCodecSpecificInfo>)info
           renderTimeMs:(int64_t)renderTimeMs {
  webrtc::EncodedImage image = [encodedImage nativeEncodedImage];

  // Handle types than can be converted into one of webrtc::CodecSpecificInfo's hard coded cases.
  webrtc::CodecSpecificInfo codecSpecificInfo;
  if ([info isKindOfClass:[RTCCodecSpecificInfoH264 class]]) {
    codecSpecificInfo = [(RTCCodecSpecificInfoH264 *)info nativeCodecSpecificInfo];
  }

  std::unique_ptr<webrtc::RTPFragmentationHeader> header =
      [fragmentationHeader createNativeFragmentationHeader];

  return _videoToolboxDecoder->Decode(
      image, missingFrames, header.release(), &codecSpecificInfo, renderTimeMs);
}

@end

// Encoder factory.
@implementation RTCVideoEncoderFactoryH264

- (NSArray<RTCVideoCodecInfo *> *)supportedCodecs {
  NSMutableArray<RTCVideoCodecInfo *> *codecs = [NSMutableArray array];
  NSString *codecName = [NSString stringWithUTF8String:cricket::kH264CodecName];

  if (IsHighProfileEnabled()) {
    NSDictionary<NSString *, NSString *> *constrainedHighParams = @{
      @"profile-level-id" : @"640c1f",  // Level 3.1 Constrained High.
      @"level-asymmetry-allowed" : @"1",
      @"packetization-mode" : @"1",
    };
    RTCVideoCodecInfo *constrainedHighInfo =
        [[RTCVideoCodecInfo alloc] initWithPayload:0
                                              name:codecName
                                        parameters:constrainedHighParams];
    [codecs addObject:constrainedHighInfo];
  }

  NSDictionary<NSString *, NSString *> *constrainedBaselineParams = @{
    @"profile-level-id" : @"42e01f",  // Level 3.1 Constrained Baseline.
    @"level-asymmetry-allowed" : @"1",
    @"packetization-mode" : @"1",
  };
  RTCVideoCodecInfo *constrainedBaselineInfo =
      [[RTCVideoCodecInfo alloc] initWithPayload:0
                                            name:codecName
                                      parameters:constrainedBaselineParams];
  [codecs addObject:constrainedBaselineInfo];

  return [codecs copy];
}

- (id<RTCVideoEncoder>)createEncoder:(RTCVideoCodecInfo *)info {
  return [[RTCVideoEncoderH264 alloc] initWithCodecInfo:info];
}

@end

// Decoder factory.
@implementation RTCVideoDecoderFactoryH264

- (id<RTCVideoDecoder>)createDecoder:(RTCVideoCodecInfo *)info {
  return [[RTCVideoDecoderH264 alloc] init];
}

- (NSArray<RTCVideoCodecInfo *> *)supportedCodecs {
  NSString *codecName = [NSString stringWithUTF8String:cricket::kH264CodecName];
  return @[ [[RTCVideoCodecInfo alloc] initWithPayload:0 name:codecName parameters:@{}] ];
}

@end
