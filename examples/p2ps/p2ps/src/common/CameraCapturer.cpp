#include "CameraCapturer.h"

namespace PCS {

CameraCapturer::CameraCapturer() : vcm_(nullptr) {}

bool CameraCapturer::Init(size_t width,
                       size_t height,
                       size_t target_fps,
                       size_t capture_device_index) {
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> device_info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());

    char device_name[256];
    char unique_name[256];
    if (device_info->GetDeviceName(static_cast<uint32_t>(capture_device_index),
                                   device_name, sizeof(device_name), unique_name,
                                   sizeof(unique_name)) != 0) {
        Destroy();
        return false;
    }

    vcm_ = webrtc::VideoCaptureFactory::Create(unique_name);
    if (!vcm_) {
        return false;
    }
    vcm_->RegisterCaptureDataCallback(this);

    device_info->GetCapability(vcm_->CurrentDeviceName(), 0, capability_);

    capability_.width = static_cast<int32_t>(width);
    capability_.height = static_cast<int32_t>(height);
    capability_.maxFPS = static_cast<int32_t>(target_fps);
    capability_.videoType = webrtc::VideoType::kI420;

    if (vcm_->StartCapture(capability_) != 0) {
        Destroy();
        return false;
    }
    RTC_CHECK(vcm_->CaptureStarted());

    return true;
}

CameraCapturer* CameraCapturer::Create(size_t width,
                                 size_t height,
                                 size_t target_fps,
                                 size_t capture_device_index) {
    std::unique_ptr<CameraCapturer> vcm_capturer(new CameraCapturer());
    if (!vcm_capturer->Init(width, height, target_fps, capture_device_index)) {
        RTC_LOG(LS_WARNING) << "Failed to create CameraCapture(w = " << width
                            << ", h = " << height << ", fps = " << target_fps
                            << ")";
        return nullptr;
    }
    return vcm_capturer.release();
}

void CameraCapturer::Destroy() {
    if (!vcm_)
        return;

    vcm_->StopCapture();
    vcm_->DeRegisterCaptureDataCallback();
    // Release reference to VCM.
    vcm_ = nullptr;
}

CameraCapturer::~CameraCapturer() {
    Destroy();
}

void CameraCapturer::OnFrame(const webrtc::VideoFrame& frame) {
    VideoCapturer::OnFrame(frame);
}
}
