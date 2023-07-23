#ifndef CAMERACAPTURERTRACKSOURCE_H
#define CAMERACAPTURERTRACKSOURCE_H


#include "CameraCapturer.h"

class CameraCapturerTrackSource : public webrtc::VideoTrackSource {
public:
    static rtc::scoped_refptr<CameraCapturerTrackSource> Create(int width,int height,int fps) {
        const size_t kWidth = width;
        const size_t kHeight = height;
        const size_t kFps = fps;
        std::unique_ptr<CameraCapturer> capturer;
        std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
            webrtc::VideoCaptureFactory::CreateDeviceInfo());
        if (!info) {
            return nullptr;
        }
        int num_devices = info->NumberOfDevices();
        for (int i = 1; i < num_devices; ++i) {
            capturer = absl::WrapUnique(
                CameraCapturer::Create(kWidth, kHeight, kFps, i));
            if (capturer) {
                return new rtc::RefCountedObject<CameraCapturerTrackSource>(
                    std::move(capturer));
            }
        }

        return nullptr;
    }

    static rtc::scoped_refptr<CameraCapturerTrackSource> Create(int width,int height,int fps,int deviceId) {
        const size_t kWidth = width;
        const size_t kHeight = height;
        const size_t kFps = fps;
        std::unique_ptr<CameraCapturer> capturer;
        capturer = absl::WrapUnique(
            CameraCapturer::Create(kWidth, kHeight, kFps, deviceId));
        if (capturer) {
            return new rtc::RefCountedObject<CameraCapturerTrackSource>(
                std::move(capturer));
        }

        return nullptr;
    }

    void Destroy(){
        if (capturer_) {
            capturer_->Destroy();
            capturer_.reset();
        }
    }

protected:
    explicit CameraCapturerTrackSource(
        std::unique_ptr<CameraCapturer> capturer)
        : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

private:
    rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
        return capturer_.get();
    }
    std::unique_ptr<CameraCapturer> capturer_;
};

#endif // CAMERACAPTURERTRACKSOURCE_H
