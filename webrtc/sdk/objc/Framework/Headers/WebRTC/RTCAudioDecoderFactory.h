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

namespace rtc
{
  template <class T>
  class scoped_refptr;
};

namespace webrtc
{
  class AudioDecoderFactory;
};

NS_ASSUME_NONNULL_BEGIN

@protocol RTCAudioDecoderFactory<NSObject>

@property(nonatomic, readonly) rtc::scoped_refptr<webrtc::AudioDecoderFactory>
    nativeAudioDecoderFactory;

@end

NS_ASSUME_NONNULL_END
