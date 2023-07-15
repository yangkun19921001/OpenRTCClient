#ifndef PEERMANAGER_H
#define PEERMANAGER_H

#include "../../webrtc_headers.h"
#include <map>
#include "PeerConnectionObserverImp.h"
#include "CreateSessionDescriptionObserverImpl.h"
#include "OnPeerManagerEvents.h"
#include "CameraCapturerTrackSource.h"
#include "SetSessionDescriptionObserverImpl.h"
namespace PCS {

struct Peer
{
    Peer() {}
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_conn_inter_;
    std::unique_ptr<PeerConnectionObserverImpl> peer_conn_obser_impl_;
    std::unique_ptr<CreateSessionDescriptionObserImpl> create_offer_sess_des_impl_;
    std::unique_ptr<CreateSessionDescriptionObserImpl> create_answer_sess_des_impl_;
};


const char kStreamId[] = "ARDAMS";
const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
/**
 *
 * PeerClient 管理
 * @brief The PeerManager class
 */
class PeerManager {
public:
    PeerManager();
    ~PeerManager();

    /*创建 PeerConnectionFactoryInterface 实例*/
    bool createPeerConnectionFactory(const std::string &localPeerId);
    /**设置本地采集机到帧回调*/
    void setLocalVideoTrackCallback(std::function<void(std::string,int,int,rtc::scoped_refptr<webrtc::VideoTrackInterface>)> localVideoTrack);
    /* 创建 PeerConnectionInterface 实例，并将其添加到 map 中*/
    bool createPeerConnection(const std::string& peerId,
                              const webrtc::PeerConnectionInterface::RTCConfiguration& config,
                               OnPeerManagerEvents* ets);

    /* 对指定的 PeerConnectionInterface 实例创建 offer*/
    void createOffer(const std::string& peerId, OnPeerManagerEvents* ets);

    // 对指定的 PeerConnectionInterface 实例创建 answer
    void createAnswer(const std::string& peerId, OnPeerManagerEvents* ets);

    // 删除指定的 PeerConnectionInterface 实例
    void removePeerConnection(const std::string& peerId);
    //设置本地的 SDP
    void setLocalDescription(const std::string& peerId,
                             webrtc::SessionDescriptionInterface* desc_ptr);
    //设置远端的 sdp 信息
    void setRemoteDescription(const std::string& peerId,
                             webrtc::SessionDescriptionInterface* desc_ptr);

    //添加本地音视频轨
    void addTracks(const std::string &localPeerId);

    void handleOffer(std::string peerId,std::string sdp, OnPeerManagerEvents* ets);
    void handleAnswer(std::string peerId,std::string sdp, OnPeerManagerEvents* ets);
    void handleCandidate(std::string peerId,std::unique_ptr<webrtc::IceCandidateInterface> candidate);
    void release();

private:
    // 用于管理 PeerConnectionInterface 实例的 map，每个实例都对应一个远程参与者
    //std::map<std::string, rtc::scoped_refptr<webrtc::PeerConnectionInterface>> peer_connections_;
    std::map<std::string ,std::unique_ptr<Peer>> peers_;
    std::map<std::string, std::unique_ptr<PeerConnectionObserverImpl>> peer_connection_observers_;

    // PeerConnectionFactoryInterface 实例，用于创建 PeerConnectionInterface 实例
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
        peer_connection_factory_;
    std::unique_ptr<rtc::Thread> signaling_thread_,network_thread_,worker_thread_;
    std::unique_ptr<rtc::Thread> main_thread_;
    std::function<void(std::string,int,int,rtc::scoped_refptr<webrtc::VideoTrackInterface>)> local_video_track_;
    rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track_;
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_;
};

}

#endif // PEERMANAGER_H
