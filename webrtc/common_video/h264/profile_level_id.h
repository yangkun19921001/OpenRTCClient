/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_COMMON_VIDEO_H264_PROFILE_LEVEL_ID_H_
#define WEBRTC_COMMON_VIDEO_H264_PROFILE_LEVEL_ID_H_

#include "webrtc/base/optional.h"

namespace webrtc {
namespace H264 {

enum Profile {
  kProfileConstrainedBaseline,
  kProfileBaseline,
  kProfileMain,
  kProfileConstrainedHigh,
  kProfileHigh,
};

// All values are equal to ten times the level number, except level 1b which is
// special.
enum Level {
  kLevel1_b = 0,
  kLevel1 = 10,
  kLevel1_1 = 11,
  kLevel1_2 = 12,
  kLevel1_3 = 13,
  kLevel2 = 20,
  kLevel2_1 = 21,
  kLevel2_2 = 22,
  kLevel3 = 30,
  kLevel3_1 = 31,
  kLevel3_2 = 32,
  kLevel4 = 40,
  kLevel4_1 = 41,
  kLevel4_2 = 42,
  kLevel5 = 50,
  kLevel5_1 = 51,
  kLevel5_2 = 52
};

struct ProfileLevelId {
  Profile profile;
  Level level;
};

// Parse profile level id that is represented as a string of 3 hex bytes.
// Nothing will be returned if the string is not a recognized H264
// profile level id.
rtc::Optional<ProfileLevelId> ParseProfileLevelId(const char* str);

}  // namespace H264
}  // namespace webrtc

#endif  // WEBRTC_COMMON_VIDEO_H264_PROFILE_LEVEL_ID_H_
