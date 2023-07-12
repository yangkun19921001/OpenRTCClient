#ifndef RTCROOMMANAGER_H
#define RTCROOMMANAGER_H

#include "OnSignalEventListener.h"
#include "SocketIoSignalClientImpl.h"
#include "PeerManager.h"
#include "OnRoomStateChangeCallback.h"
#include "OnPeerManagerEvents.h"
#include "Message.h"

namespace PCS{

class RTCRoomManager : public OnSignalEventListener,public OnPeerManagerEvents {
public:
    RTCRoomManager();
    virtual ~RTCRoomManager();

    //连接服务器
    void connect(const std::string url,OnRoomStateChangeCallback* callback);
    void setLocalTrackCallback(std::function<void(std::string,int,int,rtc::scoped_refptr<webrtc::VideoTrackInterface>)> localVideoTrack);
    //加入房间
    void join(const std::string roomId);
    //离开房间
    void leave(const std::string roomId);
    //销毁
    void release();

    //处理 ui 传递过来的 消息
    void onUIMessage(Message msg);




private:
    //实现连接成功的处理代码
    void onConnectSuccessful() override;
    //实现正在连接的处理代码
    void onConnecting() override ;
    //实现连接错误的处理代码
    void onConnectError(const std::string& error) override ;
    //实现已加入房间的处理代码
    void onJoined(const std::string& room, const std::string& id, const std::vector<std::string>& otherClientIds) override;
    //实现离开房间的处理代码
    void onLeaved(const std::string& room, const std::string& id) override ;
    //实现接收到消息的处理代码
    void onMessage(const std::string& from, const std::string& to, const std::string& message) override ;


     void OnAddTrack(std::string peerid,
                            rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                            const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
                                streams)override;
     void OnRemoveTrack(std::string peerid,
                               rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
     void OnDataChannel(std::string peerid,
                               rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;
     void OnIceCandidate(std::string peerid,const webrtc::IceCandidateInterface* candidate) override;
     void OnCreateSuccess(bool offer,std::string peerid,webrtc::SessionDescriptionInterface* desc) override;
     void OnCreateFailure(bool offer,std::string peerid,webrtc::RTCError error) override;




private:
    std::unique_ptr<ISignalClient> socket_signal_client_imp_;
    std::unique_ptr<PeerManager> peer_manager_;
    OnRoomStateChangeCallback * room_state_change_callback_;
    std::function<void(std::string,int,int,rtc::scoped_refptr<webrtc::VideoTrackInterface>)> local_track_callback_;
    std::string room_id_;

};

} // end namespace PCS
#endif // RTCROOMMANAGER_H
