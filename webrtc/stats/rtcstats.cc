/*
 *  Copyright 2016 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/api/rtcstats.h"

#include "webrtc/base/stringencode.h"

namespace webrtc {

namespace {

// Produces "{ a, b, c }". Works for non-vector |RTCStatsMemberInterface::Type|
// types.
template<typename T>
std::string VectorToString(const std::vector<T>& vector) {
  if (vector.empty())
    return "{}";
  std::ostringstream oss;
  oss << "{ " << rtc::ToString<T>(vector[0]);
  for (size_t i = 1; i < vector.size(); ++i) {
    oss << ", " << rtc::ToString<T>(vector[i]);
  }
  oss << " }";
  return oss.str();
}

// Produces "{ \"a\", \"b\", \"c\" }". Works for vectors of both const char* and
// std::string element types.
template<typename T>
std::string VectorOfStringsToString(const std::vector<T>& strings) {
  if (strings.empty())
    return "{}";
  std::ostringstream oss;
  oss << "{ \"" << rtc::ToString<T>(strings[0]) << '\"';
  for (size_t i = 1; i < strings.size(); ++i) {
    oss << ", \"" << rtc::ToString<T>(strings[i]) << '\"';
  }
  oss << " }";
  return oss.str();
}

}  // namespace

std::string RTCStats::ToString() const {
  std::ostringstream oss;
  oss << type() << " {\n  id: \"" << id_ << "\"\n  timestamp: "
      << timestamp_us_ << '\n';
  for (const RTCStatsMemberInterface* member : Members()) {
    oss << "  " << member->name() << ": ";
    if (member->is_defined()) {
      if (member->is_string())
        oss << '"' << member->ValueToString() << "\"\n";
      else
        oss << member->ValueToString() << '\n';
    } else {
      oss << "undefined\n";
    }
  }
  oss << '}';
  return oss.str();
}

std::vector<const RTCStatsMemberInterface*> RTCStats::Members() const {
  return MembersOfThisObjectAndAncestors(0);
}

std::vector<const RTCStatsMemberInterface*>
RTCStats::MembersOfThisObjectAndAncestors(
    size_t additional_capacity) const {
  std::vector<const RTCStatsMemberInterface*> members;
  members.reserve(additional_capacity);
  return members;
}

#define WEBRTC_DEFINE_RTCSTATSMEMBER(T, type, is_seq, is_str, to_str)          \
  template<>                                                                   \
  const RTCStatsMemberInterface::Type RTCStatsMember<T>::kType =               \
      RTCStatsMemberInterface::type;                                           \
  template<>                                                                   \
  bool RTCStatsMember<T>::is_sequence() const { return is_seq; }               \
  template<>                                                                   \
  bool RTCStatsMember<T>::is_string() const { return is_str; }                 \
  template<>                                                                   \
  std::string RTCStatsMember<T>::ValueToString() const {                       \
    RTC_DCHECK(is_defined_);                                                   \
    return to_str;                                                             \
  }

WEBRTC_DEFINE_RTCSTATSMEMBER(int32_t, kInt32, false, false,
                             rtc::ToString(value_));
WEBRTC_DEFINE_RTCSTATSMEMBER(uint32_t, kUint32, false, false,
                             rtc::ToString(value_));
WEBRTC_DEFINE_RTCSTATSMEMBER(int64_t, kInt64, false, false,
                             rtc::ToString(value_));
WEBRTC_DEFINE_RTCSTATSMEMBER(uint64_t, kUint64, false, false,
                             rtc::ToString(value_));
WEBRTC_DEFINE_RTCSTATSMEMBER(double, kDouble, false, false,
                             rtc::ToString(value_));
WEBRTC_DEFINE_RTCSTATSMEMBER(const char*, kStaticString, false, true,
                             value_);
WEBRTC_DEFINE_RTCSTATSMEMBER(std::string, kString, false, true,
                             value_);
WEBRTC_DEFINE_RTCSTATSMEMBER(
    std::vector<int32_t>, kSequenceInt32, true, false,
    VectorToString(value_));
WEBRTC_DEFINE_RTCSTATSMEMBER(
    std::vector<uint32_t>, kSequenceUint32, true, false,
    VectorToString(value_));
WEBRTC_DEFINE_RTCSTATSMEMBER(
    std::vector<int64_t>, kSequenceInt64, true, false,
    VectorToString(value_));
WEBRTC_DEFINE_RTCSTATSMEMBER(
    std::vector<uint64_t>, kSequenceUint64, true, false,
    VectorToString(value_));
WEBRTC_DEFINE_RTCSTATSMEMBER(
    std::vector<double>, kSequenceDouble, true, false,
    VectorToString(value_));
WEBRTC_DEFINE_RTCSTATSMEMBER(
    std::vector<const char*>, kSequenceStaticString, true, false,
    VectorOfStringsToString(value_));
WEBRTC_DEFINE_RTCSTATSMEMBER(
    std::vector<std::string>, kSequenceString, true, false,
    VectorOfStringsToString(value_));

}  // namespace webrtc
