/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/media/engine/webrtcvideoframe.h"

#include "libyuv/convert.h"
#include "webrtc/base/logging.h"
#include "webrtc/media/base/videocapturer.h"
#include "webrtc/media/base/videocommon.h"
#include "webrtc/video_frame.h"

using webrtc::kYPlane;
using webrtc::kUPlane;
using webrtc::kVPlane;

namespace cricket {

WebRtcVideoFrame::WebRtcVideoFrame():
    time_stamp_ns_(0),
    rotation_(webrtc::kVideoRotation_0) {}

WebRtcVideoFrame::WebRtcVideoFrame(
    const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer,
    int64_t time_stamp_ns,
    webrtc::VideoRotation rotation)
    : video_frame_buffer_(buffer),
      time_stamp_ns_(time_stamp_ns),
      rotation_(rotation) {
}

WebRtcVideoFrame::~WebRtcVideoFrame() {}

bool WebRtcVideoFrame::Init(uint32_t format,
                            int w,
                            int h,
                            int dw,
                            int dh,
                            uint8_t* sample,
                            size_t sample_size,
                            int64_t time_stamp_ns,
                            webrtc::VideoRotation rotation) {
  return Reset(format, w, h, dw, dh, sample, sample_size,
               time_stamp_ns, rotation,
               true /*apply_rotation*/);
}

bool WebRtcVideoFrame::Init(const CapturedFrame* frame, int dw, int dh,
                            bool apply_rotation) {
  return Reset(frame->fourcc, frame->width, frame->height, dw, dh,
               static_cast<uint8_t*>(frame->data), frame->data_size,
               frame->time_stamp,
               frame->rotation, apply_rotation);
}

bool WebRtcVideoFrame::InitToBlack(int w, int h,
                                   int64_t time_stamp_ns) {
  InitToEmptyBuffer(w, h, time_stamp_ns);
  return SetToBlack();
}

size_t WebRtcVideoFrame::GetWidth() const {
  return video_frame_buffer_ ? video_frame_buffer_->width() : 0;
}

size_t WebRtcVideoFrame::GetHeight() const {
  return video_frame_buffer_ ? video_frame_buffer_->height() : 0;
}

const uint8_t* WebRtcVideoFrame::GetYPlane() const {
  return video_frame_buffer_ ? video_frame_buffer_->data(kYPlane) : nullptr;
}

const uint8_t* WebRtcVideoFrame::GetUPlane() const {
  return video_frame_buffer_ ? video_frame_buffer_->data(kUPlane) : nullptr;
}

const uint8_t* WebRtcVideoFrame::GetVPlane() const {
  return video_frame_buffer_ ? video_frame_buffer_->data(kVPlane) : nullptr;
}

uint8_t* WebRtcVideoFrame::GetYPlane() {
  return video_frame_buffer_ ? video_frame_buffer_->MutableData(kYPlane)
                             : nullptr;
}

uint8_t* WebRtcVideoFrame::GetUPlane() {
  return video_frame_buffer_ ? video_frame_buffer_->MutableData(kUPlane)
                             : nullptr;
}

uint8_t* WebRtcVideoFrame::GetVPlane() {
  return video_frame_buffer_ ? video_frame_buffer_->MutableData(kVPlane)
                             : nullptr;
}

int32_t WebRtcVideoFrame::GetYPitch() const {
  return video_frame_buffer_ ? video_frame_buffer_->stride(kYPlane) : 0;
}

int32_t WebRtcVideoFrame::GetUPitch() const {
  return video_frame_buffer_ ? video_frame_buffer_->stride(kUPlane) : 0;
}

int32_t WebRtcVideoFrame::GetVPitch() const {
  return video_frame_buffer_ ? video_frame_buffer_->stride(kVPlane) : 0;
}

bool WebRtcVideoFrame::IsExclusive() const {
  return video_frame_buffer_->HasOneRef();
}

void* WebRtcVideoFrame::GetNativeHandle() const {
  return video_frame_buffer_ ? video_frame_buffer_->native_handle() : nullptr;
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer>
WebRtcVideoFrame::GetVideoFrameBuffer() const {
  return video_frame_buffer_;
}

VideoFrame* WebRtcVideoFrame::Copy() const {
  WebRtcVideoFrame* new_frame = new WebRtcVideoFrame(
      video_frame_buffer_, time_stamp_ns_, rotation_);
  return new_frame;
}

bool WebRtcVideoFrame::MakeExclusive() {
  RTC_DCHECK(video_frame_buffer_->native_handle() == nullptr);
  if (IsExclusive())
    return true;

  // Not exclusive already, need to copy buffer.
  rtc::scoped_refptr<webrtc::VideoFrameBuffer> new_buffer =
      new rtc::RefCountedObject<webrtc::I420Buffer>(
          video_frame_buffer_->width(), video_frame_buffer_->height(),
          video_frame_buffer_->stride(kYPlane),
          video_frame_buffer_->stride(kUPlane),
          video_frame_buffer_->stride(kVPlane));

  if (!CopyToPlanes(
          new_buffer->MutableData(kYPlane), new_buffer->MutableData(kUPlane),
          new_buffer->MutableData(kVPlane), new_buffer->stride(kYPlane),
          new_buffer->stride(kUPlane), new_buffer->stride(kVPlane))) {
    return false;
  }

  video_frame_buffer_ = new_buffer;
  return true;
}

size_t WebRtcVideoFrame::ConvertToRgbBuffer(uint32_t to_fourcc,
                                            uint8_t* buffer,
                                            size_t size,
                                            int stride_rgb) const {
  RTC_CHECK(video_frame_buffer_);
  RTC_CHECK(video_frame_buffer_->native_handle() == nullptr);
  return VideoFrame::ConvertToRgbBuffer(to_fourcc, buffer, size, stride_rgb);
}

bool WebRtcVideoFrame::Reset(uint32_t format,
                             int w,
                             int h,
                             int dw,
                             int dh,
                             uint8_t* sample,
                             size_t sample_size,
                             int64_t time_stamp_ns,
                             webrtc::VideoRotation rotation,
                             bool apply_rotation) {
  if (!Validate(format, w, h, sample, sample_size)) {
    return false;
  }
  // Translate aliases to standard enums (e.g., IYUV -> I420).
  format = CanonicalFourCC(format);

  // Set up a new buffer.
  // TODO(fbarchard): Support lazy allocation.
  int new_width = dw;
  int new_height = dh;
  // If rotated swap width, height.
  if (apply_rotation && (rotation == 90 || rotation == 270)) {
    new_width = dh;
    new_height = dw;
  }

  InitToEmptyBuffer(new_width, new_height,
                    time_stamp_ns);
  rotation_ = apply_rotation ? webrtc::kVideoRotation_0 : rotation;

  int horiz_crop = ((w - dw) / 2) & ~1;
  // ARGB on Windows has negative height.
  // The sample's layout in memory is normal, so just correct crop.
  int vert_crop = ((abs(h) - dh) / 2) & ~1;
  // Conversion functions expect negative height to flip the image.
  int idh = (h < 0) ? -dh : dh;
  int r = libyuv::ConvertToI420(
      sample, sample_size,
      GetYPlane(), GetYPitch(),
      GetUPlane(), GetUPitch(),
      GetVPlane(), GetVPitch(),
      horiz_crop, vert_crop,
      w, h,
      dw, idh,
      static_cast<libyuv::RotationMode>(
          apply_rotation ? rotation : webrtc::kVideoRotation_0),
      format);
  if (r) {
    LOG(LS_ERROR) << "Error parsing format: " << GetFourccName(format)
                  << " return code : " << r;
    return false;
  }
  return true;
}

VideoFrame* WebRtcVideoFrame::CreateEmptyFrame(
    int w, int h,
    int64_t time_stamp_ns) const {
  WebRtcVideoFrame* frame = new WebRtcVideoFrame();
  frame->InitToEmptyBuffer(w, h, time_stamp_ns);
  return frame;
}

void WebRtcVideoFrame::InitToEmptyBuffer(int w, int h,
                                         int64_t time_stamp_ns) {
  video_frame_buffer_ = new rtc::RefCountedObject<webrtc::I420Buffer>(w, h);
  time_stamp_ns_ = time_stamp_ns;
  rotation_ = webrtc::kVideoRotation_0;
}

const VideoFrame* WebRtcVideoFrame::GetCopyWithRotationApplied() const {
  // If the frame is not rotated, the caller should reuse this frame instead of
  // making a redundant copy.
  if (GetVideoRotation() == webrtc::kVideoRotation_0) {
    return this;
  }

  // If the video frame is backed up by a native handle, it resides in the GPU
  // memory which we can't rotate here. The assumption is that the renderers
  // which uses GPU to render should be able to rotate themselves.
  RTC_DCHECK(!GetNativeHandle());

  if (rotated_frame_) {
    return rotated_frame_.get();
  }

  int width = static_cast<int>(GetWidth());
  int height = static_cast<int>(GetHeight());

  int rotated_width = width;
  int rotated_height = height;
  if (GetVideoRotation() == webrtc::kVideoRotation_90 ||
      GetVideoRotation() == webrtc::kVideoRotation_270) {
    rotated_width = height;
    rotated_height = width;
  }

  rotated_frame_.reset(CreateEmptyFrame(rotated_width, rotated_height,
                                        GetTimeStamp()));

  // TODO(guoweis): Add a function in webrtc_libyuv.cc to convert from
  // VideoRotation to libyuv::RotationMode.
  int ret = libyuv::I420Rotate(
      GetYPlane(), GetYPitch(), GetUPlane(), GetUPitch(), GetVPlane(),
      GetVPitch(), rotated_frame_->GetYPlane(), rotated_frame_->GetYPitch(),
      rotated_frame_->GetUPlane(), rotated_frame_->GetUPitch(),
      rotated_frame_->GetVPlane(), rotated_frame_->GetVPitch(), width, height,
      static_cast<libyuv::RotationMode>(GetVideoRotation()));
  if (ret == 0) {
    return rotated_frame_.get();
  }
  return nullptr;
}

}  // namespace cricket
