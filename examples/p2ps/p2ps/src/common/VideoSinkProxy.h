#ifndef VIDEOSINKPROXY_H
#define VIDEOSINKPROXY_H

#include "../../../webrtc_headers.h""

namespace PCS {

class VideoSinkProxy : public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
public:
    VideoSinkProxy(webrtc::VideoTrackInterface* track_to_render);
    virtual ~VideoSinkProxy();

public:
    // VideoSinkInterface implementation
    void OnFrame(const webrtc::VideoFrame& frame) override;
};

}


#endif // VIDEOSINKPROXY_H
