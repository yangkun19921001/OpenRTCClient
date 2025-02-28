#ifndef CAMERACAPTURE_H
#define CAMERACAPTURE_H

#include "VideoCapturer.h"

namespace PCS {
class CameraCapturer : public VideoCapturer,
                      public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
public:
    static CameraCapturer* Create(size_t width,
                               size_t height,
                               size_t target_fps,
                               size_t capture_device_index);
    virtual ~CameraCapturer();

    void OnFrame(const webrtc::VideoFrame& frame) override;

public:
    void Destroy();
private:
    CameraCapturer();
    bool Init(size_t width,
              size_t height,
              size_t target_fps,
              size_t capture_device_index);


    rtc::scoped_refptr<webrtc::VideoCaptureModule> vcm_;
    webrtc::VideoCaptureCapability capability_;
    std::unique_ptr<rtc::Thread>  vcm_capturer_;
};
}


#endif // CAMERACAPTURE_H
