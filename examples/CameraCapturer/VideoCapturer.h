#ifndef VIDEOCAPTURE_H
#define VIDEOCAPTURE_H

#include "webrtc_headers.h"
#include <stddef.h>

#include <memory>


class VideoCapturer: public rtc::VideoSourceInterface<webrtc::VideoFrame> {
public:
    class FramePreprocessor {
    public:
        virtual ~FramePreprocessor() = default;

        virtual webrtc::VideoFrame Preprocess(const webrtc::VideoFrame& frame) = 0;
    };

    ~VideoCapturer() override;

    void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                         const rtc::VideoSinkWants& wants) override;
    void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;
    void SetFramePreprocessor(std::unique_ptr<FramePreprocessor> preprocessor) {
        webrtc::MutexLock lock(&lock_);
        preprocessor_ = std::move(preprocessor);
    }

protected:
    void OnFrame(const webrtc::VideoFrame& frame);
    rtc::VideoSinkWants GetSinkWants();

private:
    void UpdateVideoAdapter();
    webrtc::VideoFrame MaybePreprocess(const webrtc::VideoFrame& frame);

   webrtc:: Mutex lock_;
    std::unique_ptr<FramePreprocessor> preprocessor_ RTC_GUARDED_BY(lock_);
    rtc::VideoBroadcaster broadcaster_;
    cricket::VideoAdapter video_adapter_;
};



#endif // VIDEOCAPTURE_H
