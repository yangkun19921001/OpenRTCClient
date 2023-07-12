#ifndef PEERCONNECTIONOBSERVERIMP_H
#define PEERCONNECTIONOBSERVERIMP_H

#include "../../webrtc_headers.h"
#include "OnPeerManagerEvents.h"

namespace PCS {


class PeerConnectionObserverImpl :public webrtc::PeerConnectionObserver{

public:
    PeerConnectionObserverImpl(std::string remoteid,OnPeerManagerEvents *events){
        this->user_id_ = remoteid;
        this->on_peer_manager_event_ = events;
    };
    ~PeerConnectionObserverImpl(){}

public:
    //
    // PeerConnectionObserver implementation.
    //

    void OnSignalingChange(
        webrtc::PeerConnectionInterface::SignalingState new_state) override {

    }
    void OnAddTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
        const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
            streams) override {
        if(this->on_peer_manager_event_)this->on_peer_manager_event_->OnAddTrack(getUserId(),receiver,streams);
    };
    void OnRemoveTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override{
        if(this->on_peer_manager_event_)this->on_peer_manager_event_->OnRemoveTrack(getUserId(),receiver);

    };
    void OnDataChannel(
        rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {
        if(this->on_peer_manager_event_)this->on_peer_manager_event_->OnDataChannel(getUserId(),channel);
    }
    void OnRenegotiationNeeded() override {}
    void OnIceConnectionChange(
        webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
    void OnIceGatheringChange(
        webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override{
        if(this->on_peer_manager_event_)this->on_peer_manager_event_->OnIceCandidate(getUserId(),candidate);
    };
    void OnIceConnectionReceivingChange(bool receiving) override {}
    std::string getUserId(){
        return user_id_;
    }

private:
    std::string user_id_;
    OnPeerManagerEvents *on_peer_manager_event_ = nullptr;
};

}

#endif // PEERCONNECTIONOBSERVERIMP_H
