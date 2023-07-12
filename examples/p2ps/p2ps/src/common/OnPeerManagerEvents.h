#ifndef ONPEERMANAGEREVENTS_H
#define ONPEERMANAGEREVENTS_H
#include "../../webrtc_headers.h"

namespace PCS {

class OnPeerManagerEvents {
public:
    ~OnPeerManagerEvents() = default;

    virtual void OnAddTrack(std::string peerid,
                            rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                            const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
                                streams) = 0;
    virtual void OnRemoveTrack(std::string peerid,
                               rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) = 0;
    virtual void OnDataChannel(std::string peerid,
                               rtc::scoped_refptr<webrtc::DataChannelInterface> channel) = 0;
    virtual void OnIceCandidate(std::string peerid,const webrtc::IceCandidateInterface* candidate) = 0;

    virtual void OnCreateSuccess(bool offer,std::string peerid,webrtc::SessionDescriptionInterface* desc) = 0;
    virtual void OnCreateFailure(bool offer,std::string peerid,webrtc::RTCError error) = 0;

};
}
#endif // ONPEERMANAGEREVENTS_H
