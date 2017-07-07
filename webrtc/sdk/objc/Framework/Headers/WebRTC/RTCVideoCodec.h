/*
 *  Copyright 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import <Foundation/Foundation.h>

#import <WebRTC/RTCMacros.h>

@class RTCVideoFrame;

NS_ASSUME_NONNULL_BEGIN

/** Represents an encoded frame's type. */
typedef NS_ENUM(NSUInteger, RTCFrameType) {
  EmptyFrame,
  VideoFrameKey,
  VideoFrameDelta,
};

/** Represents an encoded frame. Corresponds to webrtc::EncodedImage. */
RTC_EXPORT
@interface RTCEncodedImage : NSObject

@property(nonatomic, retain) NSData *buffer;
@property(nonatomic, assign) int encodedWidth;
@property(nonatomic, assign) int encodedHeight;
@property(nonatomic, assign) uint32_t timeStamp;
@property(nonatomic, assign) long captureTimeMs;
@property(nonatomic, assign) long ntpTimeMs;
@property(nonatomic, assign) BOOL isTimingFrame;
@property(nonatomic, assign) long encodeStartMs;
@property(nonatomic, assign) long encodeFinishMs;
@property(nonatomic, assign) RTCFrameType frameType;
@property(nonatomic, assign) int rotation;
@property(nonatomic, assign) BOOL completeFrame;
@property(nonatomic, retain) NSNumber *qp;

@end

/** Information for header. Corresponds to webrtc::RTPFragmentationHeader. */
RTC_EXPORT
@interface RTCRtpFragmentationHeader : NSObject

@property(nonatomic, retain) NSArray<NSNumber *> *fragmentationOffset;
@property(nonatomic, retain) NSArray<NSNumber *> *fragmentationLength;
@property(nonatomic, retain) NSArray<NSNumber *> *fragmentationTimeDiff;
@property(nonatomic, retain) NSArray<NSNumber *> *fragmentationPlType;

@end

/** Implement this protocol to pass codec specific info from the encoder.
 *  Corresponds to webrtc::CodecSpecificInfo.
 */
RTC_EXPORT
@protocol RTCCodecSpecificInfo <NSObject>

@end

/** Callback block for encoder. */
typedef void (^RTCVideoEncoderCallback)(RTCEncodedImage *frame,
                                        id<RTCCodecSpecificInfo> info,
                                        RTCRtpFragmentationHeader *header);

/** Callback block for decoder. */
typedef void (^RTCVideoDecoderCallback)(RTCVideoFrame *frame);

/** Settings for encoder. Corresponds to webrtc::VideoCodec. */
RTC_EXPORT
@interface RTCVideoEncoderSettings : NSObject

@property(nonatomic, retain) NSString *name;

@property(nonatomic, assign) unsigned short width;
@property(nonatomic, assign) unsigned short height;

@property(nonatomic, assign) unsigned int startBitrate;  // kilobits/sec.
@property(nonatomic, assign) unsigned int maxBitrate;
@property(nonatomic, assign) unsigned int minBitrate;
@property(nonatomic, assign) unsigned int targetBitrate;

@property(nonatomic, assign) uint32_t maxFramerate;

@property(nonatomic, assign) unsigned int qpMax;

@end

/** Holds information to identify a codec. Corresponds to cricket::VideoCodec. */
RTC_EXPORT
@interface RTCVideoCodecInfo : NSObject

- (instancetype)initWithPayload:(int)payload
                           name:(NSString *)name
                     parameters:(NSDictionary<NSString *, NSString *> *)parameters;

@property(nonatomic, readonly) int payload;
@property(nonatomic, readonly) NSString *name;
@property(nonatomic, readonly) NSDictionary<NSString *, NSString *> *parameters;

@end

/** Protocol for encoder implementations. */
RTC_EXPORT
@protocol RTCVideoEncoder <NSObject>

- (instancetype)initWithCodecInfo:(RTCVideoCodecInfo *)codecInfo;
- (void)setCallback:(RTCVideoEncoderCallback)callback;
- (void)initEncodeWithSettings:(RTCVideoEncoderSettings *)settings numberOfCores:(int)numberOfCores;
- (void)releaseEncode;
- (void)encode:(RTCVideoFrame *)frame
    codecSpecificInfo:(id<RTCCodecSpecificInfo>)info
           frameTypes:(NSArray<NSNumber *> *)frameTypes;
- (BOOL)setBitrate:(uint32_t)bitrateKbit framerate:(uint32_t)framerate;

@end

/** Protocol for decoder implementations. */
RTC_EXPORT
@protocol RTCVideoDecoder <NSObject>

- (void)setCallback:(RTCVideoDecoderCallback)callback;
- (int)initDecodeWithSettings:(RTCVideoEncoderSettings *)settings numberOfCores:(int)numberOfCores;
- (int32_t)releaseDecode;
- (int)decode:(RTCEncodedImage *)encodedImage
          missingFrames:(BOOL)missingFrames
    fragmentationHeader:(RTCRtpFragmentationHeader *)fragmentationHeader
      codecSpecificInfo:(__nullable id<RTCCodecSpecificInfo>)info
           renderTimeMs:(int64_t)renderTimeMs;

@end

NS_ASSUME_NONNULL_END
