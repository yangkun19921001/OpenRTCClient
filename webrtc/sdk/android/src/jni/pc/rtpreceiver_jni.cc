/*
 *  Copyright 2017 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/api/rtpreceiverinterface.h"
#include "webrtc/sdk/android/src/jni/jni_helpers.h"
#include "webrtc/sdk/android/src/jni/pc/java_native_conversion.h"
#include "webrtc/sdk/android/src/jni/pc/rtpreceiverobserver_jni.h"

namespace webrtc_jni {

JOW(jlong, RtpReceiver_nativeGetTrack)
(JNIEnv* jni, jclass, jlong j_rtp_receiver_pointer, jlong j_track_pointer) {
  return jlongFromPointer(
      reinterpret_cast<webrtc::RtpReceiverInterface*>(j_rtp_receiver_pointer)
          ->track()
          .release());
}

JOW(jboolean, RtpReceiver_nativeSetParameters)
(JNIEnv* jni, jclass, jlong j_rtp_receiver_pointer, jobject j_parameters) {
  if (IsNull(jni, j_parameters)) {
    return false;
  }
  webrtc::RtpParameters parameters;
  JavaToNativeRtpParameters(jni, j_parameters, &parameters);
  return reinterpret_cast<webrtc::RtpReceiverInterface*>(j_rtp_receiver_pointer)
      ->SetParameters(parameters);
}

JOW(jobject, RtpReceiver_nativeGetParameters)
(JNIEnv* jni, jclass, jlong j_rtp_receiver_pointer) {
  webrtc::RtpParameters parameters =
      reinterpret_cast<webrtc::RtpReceiverInterface*>(j_rtp_receiver_pointer)
          ->GetParameters();
  return NativeToJavaRtpParameters(jni, parameters);
}

JOW(jstring, RtpReceiver_nativeId)
(JNIEnv* jni, jclass, jlong j_rtp_receiver_pointer) {
  return JavaStringFromStdString(
      jni,
      reinterpret_cast<webrtc::RtpReceiverInterface*>(j_rtp_receiver_pointer)
          ->id());
}

JOW(void, RtpReceiver_free)
(JNIEnv* jni, jclass, jlong j_rtp_receiver_pointer) {
  reinterpret_cast<webrtc::RtpReceiverInterface*>(j_rtp_receiver_pointer)
      ->Release();
}

JOW(jlong, RtpReceiver_nativeSetObserver)
(JNIEnv* jni, jclass, jlong j_rtp_receiver_pointer, jobject j_observer) {
  RtpReceiverObserverJni* rtpReceiverObserver =
      new RtpReceiverObserverJni(jni, j_observer);
  reinterpret_cast<webrtc::RtpReceiverInterface*>(j_rtp_receiver_pointer)
      ->SetObserver(rtpReceiverObserver);
  return jlongFromPointer(rtpReceiverObserver);
}

JOW(void, RtpReceiver_nativeUnsetObserver)
(JNIEnv* jni, jclass, jlong j_rtp_receiver_pointer, jlong j_observer_pointer) {
  reinterpret_cast<webrtc::RtpReceiverInterface*>(j_rtp_receiver_pointer)
      ->SetObserver(nullptr);
  RtpReceiverObserverJni* observer =
      reinterpret_cast<RtpReceiverObserverJni*>(j_observer_pointer);
  if (observer) {
    delete observer;
  }
}

}  // namespace webrtc_jni
