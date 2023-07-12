#include "PeerManager.h"


namespace PCS{




PeerManager::PeerManager():peer_connection_factory_(nullptr),signaling_thread_(nullptr),main_thread_(nullptr)
{
    main_thread_ = rtc::Thread::Create();
    main_thread_->SetName("PeerManagerThread",nullptr);
    main_thread_->Start();

   //bool ret = createPeerConnectionFactory();
   //RTC_LOG(LS_INFO)<<"createPeerConnectionFactory:"<<ret;


}

PeerManager::~PeerManager()
{
   this->peers_.clear();
}



bool PeerManager::createPeerConnectionFactory(const std::string &localPeerId)
{
   RTC_LOG(LS_INFO) <<__FUNCTION__;
    // 检查是否已存在 peer_connection_factory_ 否则报错
   RTC_DCHECK(!peer_connection_factory_);
// bool ret =  main_thread_->Invoke<bool>(RTC_FROM_HERE ,[this,localPeerId](){
       this->network_thread_   = rtc::Thread::CreateWithSocketServer();
       this->signaling_thread_ = rtc::Thread::Create();
       this->worker_thread_    = rtc::Thread::Create();

       this->network_thread_->SetName("network_thread", nullptr);
       this->signaling_thread_->SetName("signaling_thread", nullptr);
       this->worker_thread_->SetName("worker_thread", nullptr);

       if (!this->network_thread_->Start() || !this->signaling_thread_->Start() || !this->worker_thread_->Start())
       {
           RTC_LOG(LS_INFO) << "thread start errored";
       }

       // 使用 signaling_thread_ 创建 PeerConnectionFactory
       // PeerConnectionFactory 是用于生成 PeerConnections, MediaStreams 和 MediaTracks 的工厂类
       peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
           this->network_thread_.get() /* network_thread */,
           this->worker_thread_.get() /* worker_thread */,
           this->signaling_thread_.get(), /* signaling_thread */
           nullptr /* default_adm */,
           webrtc::CreateBuiltinAudioEncoderFactory(),
           webrtc::CreateBuiltinAudioDecoderFactory(),
           webrtc::CreateBuiltinVideoEncoderFactory(),
           webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
           nullptr /* audio_processing */);

       // 如果 PeerConnectionFactory 初始化失败，清理资源并返回错误
       if (!peer_connection_factory_) {
           RTC_LOG(LS_INFO) <<__FUNCTION__ << " error";
           peer_connection_factory_ = nullptr;
           return false;
       }
       addTracks(localPeerId);
       return true;
//   });
  //addTracks(localPeerId);
 //return ret;
}

void PeerManager::setLocalVideoTrackCallback(std::function<void (std::string,int,int,rtc::scoped_refptr<webrtc::VideoTrackInterface>)> localVideoTrack)
{
   this->local_video_track_ = localVideoTrack;
}

bool PeerManager::createPeerConnection(const std::string &peerId, const webrtc::PeerConnectionInterface::RTCConfiguration &config
                                       ,  OnPeerManagerEvents* ets)
{
  RTC_LOG(LS_INFO) <<__FUNCTION__ <<" peerId:"<<peerId;
  RTC_DCHECK(peer_connection_factory_);
  //main_thread_->Invoke<bool>(RTC_FROM_HERE ,[this,peerId,config,ets]{
      std::unique_ptr<PeerConnectionObserverImpl> peer_conn_obimpl =
          std::make_unique<PeerConnectionObserverImpl>(peerId,ets);
      // 使用 PeerConnectionFactory 和配置创建新的 PeerConnection
      auto peerConnection = peer_connection_factory_->CreatePeerConnection(
          config,
          nullptr,
          nullptr,
          peer_conn_obimpl.get());
      if (peerConnection) {
          auto peer_ptr = std::make_unique<Peer>();
          peer_ptr->peer_conn_inter_ = peerConnection;
          peer_ptr->peer_conn_obser_impl_ =std::move(peer_conn_obimpl);
          peers_[peerId] = std::move(peer_ptr);


          for (const auto& track : this->local_stream_->GetVideoTracks()) {
              RTC_LOG(LS_INFO) <<" AddVideoTrack" <<" peerId:"<<peerId;
              peerConnection->AddTrack(track, {this->local_stream_->id()});
          }
          for (const auto& track : this->local_stream_->GetAudioTracks()) {
              RTC_LOG(LS_INFO) <<" AddAudioTrack" <<" peerId:"<<peerId;
              peerConnection->AddTrack(track, {this->local_stream_->id()});
          }
          // AddStream is not available with Unified Plan SdpSemantics. Please use AddTrack instead
          //peerConnection->AddStream(this->local_stream_);

          return true;
      }
      return false;
  //});
}

void PeerManager::createOffer(const std::string &peerId, OnPeerManagerEvents* ets)
{
  RTC_LOG(LS_INFO) <<__FUNCTION__ <<" peerId:"<<peerId;

 //signaling_thread_->Invoke<void>(RTC_FROM_HERE ,[this,peerId,ets]{
  auto it = peers_.find(peerId);
  if (it != peers_.end() && it->second->peer_conn_inter_ != nullptr) {
      auto obs = std::make_unique<CreateSessionDescriptionObserImpl>(true,peerId,ets);
      it->second->peer_conn_inter_->CreateOffer(obs.get(), webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
      it->second->create_offer_sess_des_impl_ = std::move(obs);
  }else {
      RTC_LOG(LS_ERROR) << __FUNCTION__ <<  " peers_ not found id:"<<peerId;

  }
//});
}

void PeerManager::createAnswer(const std::string &peerId, OnPeerManagerEvents* ets)
{
 RTC_LOG(LS_INFO) <<__FUNCTION__ <<" peerId:"<<peerId;

  //main_thread_->Invoke<void>(RTC_FROM_HERE ,[this,peerId,ets]{
  auto it = peers_.find(peerId);
  if (it != peers_.end() && it->second->peer_conn_inter_ != nullptr) {
      auto obs = std::make_unique<CreateSessionDescriptionObserImpl>(false,peerId,ets);
      it->second->peer_conn_inter_->CreateAnswer(obs.get(), webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
      it->second->create_answer_sess_des_impl_ = std::move(obs);
  }else {
      RTC_LOG(LS_ERROR) << __FUNCTION__ <<  " peers_ not found id:"<<peerId;

  }
//});
}

void PeerManager::removePeerConnection(const std::string &peerId)
{
  RTC_LOG(LS_INFO) <<__FUNCTION__ <<" peerId:"<<peerId;

  auto pt = peers_.find(peerId);
  if (pt != peers_.end()) {
    if(pt->second->peer_conn_inter_ != nullptr)pt->second->peer_conn_inter_->Close();

    peers_.erase(pt);
  }else {
    RTC_LOG(LS_ERROR) << __FUNCTION__ <<  " peers_ not found id:"<<peerId;

  }

}

void PeerManager::setLocalDescription(const std::string &peerId,webrtc::SessionDescriptionInterface *desc_ptr)
{
  RTC_LOG(LS_INFO) <<__FUNCTION__ <<" peerId:"<<peerId;
  //main_thread_->Invoke<void>(RTC_FROM_HERE ,[this,peerId,desc_ptr]{

  auto pt = peers_.find(peerId);
  if (pt != peers_.end()) {
      pt->second->peer_conn_inter_->SetLocalDescription(SetSessionDescriptionObserverImpl::Create(),desc_ptr);

  }else {
      RTC_LOG(LS_ERROR) << __FUNCTION__ <<  " peers_ not found id:"<<peerId;

  }

  //});
}

void PeerManager::setRemoteDescription(const std::string &peerId,webrtc::SessionDescriptionInterface *desc_ptr)
{
  RTC_LOG(LS_INFO) <<__FUNCTION__ <<" peerId:"<<peerId;
//    main_thread_->Invoke<void>(RTC_FROM_HERE ,[this,peerId,desc_ptr]{
  auto pt = peers_.find(peerId);
  if (pt != peers_.end()) {
  pt->second->peer_conn_inter_->SetRemoteDescription(SetSessionDescriptionObserverImpl::Create(),desc_ptr);
  }else {
  RTC_LOG(LS_ERROR) << __FUNCTION__ <<  " peers_ not found id:"<<peerId;

  }
//});
}

void PeerManager::addTracks(const std::string &localPeerId)
{
  RTC_LOG(LS_INFO) <<__FUNCTION__ ;

    local_stream_ =
          peer_connection_factory_->CreateLocalMediaStream(kStreamId);

  // 创建音频轨道并添加到 PeerConnection
  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
      peer_connection_factory_->CreateAudioTrack(
          kAudioLabel, peer_connection_factory_->CreateAudioSource(
              cricket::AudioOptions())));

  // 创建视频源和视频轨道并添加到 PeerConnection
  rtc::scoped_refptr<CameraCapturerTrackSource> video_device =
      CameraCapturerTrackSource::Create(1280,720,30);
  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_(
      peer_connection_factory_->CreateVideoTrack(kVideoLabel, video_device));

      if(audio_track)
          local_stream_->AddTrack(audio_track);

      if (video_track_) {
          local_stream_->AddTrack(video_track_);
          if(local_video_track_!= nullptr)
              local_video_track_(localPeerId,1280,720,video_track_);
      }
}

void PeerManager::handleOffer(std::string peerId, std::string sdp, OnPeerManagerEvents* ets)
{
      RTC_LOG(LS_INFO) << __FUNCTION__ <<" peerId:"<<peerId;
      auto pt = peers_.find(peerId);
      if (pt != peers_.end()) {
          std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
              webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdp);
          pt->second->peer_conn_inter_->SetRemoteDescription(SetSessionDescriptionObserverImpl::Create(),session_description.release());
          createAnswer(peerId,ets);
      }else {
          RTC_LOG(LS_ERROR) << __FUNCTION__ <<  " peers_ not found id:"<<peerId;

      }
}

void PeerManager::handleAnswer(std::string peerId, std::string sdp, OnPeerManagerEvents* ets)
{
      RTC_LOG(LS_INFO) << __FUNCTION__ <<" peerId:"<<peerId;
      auto pt = peers_.find(peerId);
      if (pt != peers_.end()) {
          std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
              webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp);
          pt->second->peer_conn_inter_->SetRemoteDescription(SetSessionDescriptionObserverImpl::Create(),session_description.release());
      }else {
          RTC_LOG(LS_ERROR) << __FUNCTION__ <<  " peers_ not found id:"<<peerId;

      }
}

void PeerManager::handleCandidate(std::string peerId, std::unique_ptr<webrtc::IceCandidateInterface> candidate)
{
      RTC_LOG(LS_INFO) << __FUNCTION__ <<" peerId:"<<peerId;
      auto pt = peers_.find(peerId);
      if (pt != peers_.end()) {
          pt->second->peer_conn_inter_->AddIceCandidate(
              std::move(candidate),
              [peerId](webrtc::RTCError error){
                  if (error.ok()) {
                      RTC_LOG(LS_INFO) <<" peerId:"<< peerId << " AddIceCandidate success.";
                  } else {
                      RTC_LOG(LS_INFO) <<" peerId:"<< peerId << "AddIceCandidate failed, error: " << error.message();
                  }
       });
    }else {
          RTC_LOG(LS_ERROR) << __FUNCTION__ <<  " peers_ not found id:"<<peerId;

      }
}
}
