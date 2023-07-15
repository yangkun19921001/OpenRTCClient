#include "RTCRoomManager.h"
#include "third_party/jsoncpp/source/include/json/json.h"
namespace PCS {

webrtc::PeerConnectionInterface::RTCConfiguration getDefaultRTCConfig(){
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    webrtc::PeerConnectionInterface::IceServer server;
    server.uri = "stun:stun.l.google.com:19302";

    webrtc::PeerConnectionInterface::IceServer server_devyk;
    server_devyk.uri = "turn:rtcmedia.top:3478";
    server_devyk.username = "devyk";
    server_devyk.password = "devyk";
    //config.servers.push_back(server);
    config.servers.push_back(server_devyk);
    return config;
}


RTCRoomManager::RTCRoomManager():
    socket_signal_client_imp_(nullptr)
    ,peer_manager_(nullptr)
    ,room_state_change_callback_(nullptr)
{
    socket_signal_client_imp_ = std::make_unique<SocketIoSignalClientImpl>();
    peer_manager_ = std::make_unique<PeerManager>();
}

RTCRoomManager::~RTCRoomManager()
{

}

//############################### 对外 api ###############################
void RTCRoomManager::connect(const std::string url, OnRoomStateChangeCallback *callback)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ <<" url:"<<url;
    this->room_state_change_callback_ = callback;
    if(this->room_state_change_callback_)
    {
        PCS::Message msg;
        msg.what = PCS::RTCMainEvent::CONNECT;
        msg.data = std::make_shared<std::string>(url);
        this->room_state_change_callback_->notifyMainUI(msg);
    }


}

void RTCRoomManager::setLocalTrackCallback(std::function<void (std::string,int,int,rtc::scoped_refptr<webrtc::VideoTrackInterface>)> localVideoTrack)
{
    this->local_track_callback_ = localVideoTrack;
}

void RTCRoomManager::join(const std::string roomId)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ <<" roomid:"<<roomId;
    this->room_id_ = roomId;
    if(this->room_state_change_callback_)
    {
        PCS::Message msg;
        msg.what = PCS::RTCMainEvent::JOIN;
        msg.data = std::make_shared<std::string>(roomId);
        this->room_state_change_callback_->notifyMainUI(msg);
    }


}

void RTCRoomManager::leave(const std::string roomId)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ <<" roomId:"<<roomId;
     if(this->room_state_change_callback_)
     {
      PCS::Message msg;
      msg.what = PCS::RTCMainEvent::LEAVE;
      msg.data = std::make_shared<std::string>(roomId);
      this->room_state_change_callback_->notifyMainUI(msg);
     }


}

void RTCRoomManager::release()
{
    RTC_LOG(LS_INFO) << __FUNCTION__ ;
  this->socket_signal_client_imp_->release();
  this->peer_manager_->release();
}

void RTCRoomManager::onUIMessage(Message msg)
{
  RTC_LOG(LS_INFO) << __FUNCTION__ << " what:"<<(int)msg.what;
  switch(msg.what)
  {
  case PCS::RTCMainEvent::JOIN:{
       auto roomId = std::static_pointer_cast<std::string>(msg.data);
      if(socket_signal_client_imp_)
          this->socket_signal_client_imp_->join(*roomId);
      break;
  }
  case PCS::RTCMainEvent::CONNECT:
  {
      auto url = std::static_pointer_cast<std::string>(msg.data);
      if(socket_signal_client_imp_)
          socket_signal_client_imp_->connect(*url,this);
      break;
  }
  case PCS::RTCMainEvent::LEAVE:
  {
      auto roomId = std::static_pointer_cast<std::string>(msg.data);
      if(socket_signal_client_imp_)
          socket_signal_client_imp_->leave(*roomId);
      break;
  }
  case PCS::RTCMainEvent::ON_RELEASE:{
      release();
      break;
  }
  case PCS::RTCMainEvent::ON_LEAVED:{
      auto roomId = std::static_pointer_cast<std::string>(msg.data);
      auto peerId = std::static_pointer_cast<std::string>(msg.data_1);
      if(this->room_state_change_callback_)
          this->room_state_change_callback_->onLeaved(*peerId);
      if(this->peer_manager_)
          this->peer_manager_->removePeerConnection(*peerId);
      break;
  }
  case PCS::RTCMainEvent::ON_JOINED:
  {
      auto roomId = std::static_pointer_cast<std::string>(msg.data);
      auto peerId = std::static_pointer_cast<std::string>(msg.data_1);
      auto otherClientIds = std::static_pointer_cast<std::vector<std::string>>(msg.data_2);
      if(*peerId == this->socket_signal_client_imp_->getSocketId())//代表自己加入，并且处理已在房间中的 peer
      {
          this->peer_manager_->setLocalVideoTrackCallback(this->local_track_callback_);
          //init peerconnectionfactory & addTracks

          bool ret = this->peer_manager_->createPeerConnectionFactory(*peerId);
          if(!ret)
          {
              RTC_LOG(LS_ERROR) << "createPeerConnectionFactory error:"<<ret;
              return;
          }

          if((*otherClientIds).empty()){
              RTC_LOG(LS_ERROR) << " otherClientIds size empty";
              return;
          }

          for(const auto& clientId : *otherClientIds) {
               RTC_LOG(LS_ERROR) << " createPeerConnection clientId:"<<clientId;
              this->peer_manager_->createPeerConnection(clientId,getDefaultRTCConfig(),this);
          }

      }else{
          bool ret = this->peer_manager_->createPeerConnection(*peerId,getDefaultRTCConfig(),this);
          RTC_LOG(LS_INFO) << " createPeerConnection " << " peerId:"<<*peerId <<" return:"<<ret ;

          if(ret){

              this->peer_manager_->createOffer(*peerId,this);
          }
      }
      break;
  }
  case PCS::RTCMainEvent::ON_OFFER:
  {
      auto peerId = std::static_pointer_cast<std::string>(msg.data);
      auto sdp = std::static_pointer_cast<std::string>(msg.data_1);
      this->peer_manager_->handleOffer(*peerId,*sdp,this);
      break;
  }
  case PCS::RTCMainEvent::ON_ANSWER:
  {
      auto peerId = std::static_pointer_cast<std::string>(msg.data);
      auto sdp = std::static_pointer_cast<std::string>(msg.data_1);
      this->peer_manager_->handleAnswer(*peerId,*sdp,this);
      break;
  }
  case PCS::RTCMainEvent::ON_CANDIDATE:
  {
      auto peerId = std::static_pointer_cast<std::string>(msg.data);
      auto id = std::static_pointer_cast<std::string>(msg.data_1);
      auto label = std::static_pointer_cast<int>(msg.data_2);
      auto candidate = std::static_pointer_cast<std::string>(msg.data_3);

      webrtc::SdpParseError error;
      std::unique_ptr<webrtc::IceCandidateInterface> ice_candidate(
          webrtc::CreateIceCandidate(*id,*label, *candidate, &error));
      if (!ice_candidate.get()) {
          RTC_LOG(LS_WARNING) << "Can't parse received candidate message. "
                                 "SdpParseError was: "
                              << error.description;
          return;
      }
      this->peer_manager_->handleCandidate(*peerId,std::move(ice_candidate));
      break;
    }

  case PCS::RTCMainEvent::ON_ADD_REMOTE_TRACK :{
       auto peerId = std::static_pointer_cast<std::string>(msg.data);
       auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(msg.nsptr_data);
     if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
          auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
          if(this->room_state_change_callback_)
          {
              this->room_state_change_callback_->onAddTrack(*peerId,video_track);
          }
      }
      track->Release();
      break;
    }
  default:
    break;
  }
}


//############################### 信令回调 ###############################
void RTCRoomManager::onConnectSuccessful()
{
    RTC_LOG(LS_INFO) << __FUNCTION__ ;
  if(this->room_state_change_callback_)
      this->room_state_change_callback_->onConnected();
}

void RTCRoomManager::onConnecting()
{
      RTC_LOG(LS_INFO) << __FUNCTION__ ;
  if(this->room_state_change_callback_)
      this->room_state_change_callback_->onConnecting();
}

void RTCRoomManager::onConnectError(const std::string &error)
{
  RTC_LOG(LS_ERROR) << __FUNCTION__ <<" error:"<<error;
  if(this->room_state_change_callback_)
      this->room_state_change_callback_->onError(error);
}



void RTCRoomManager::onJoined(const std::string &room, const std::string &id, const std::vector<std::string> &otherClientIds)
{
  RTC_LOG(LS_INFO) << __FUNCTION__ <<" room:"<<room <<" id:"<<id <<" otherClientIds:"<<otherClientIds.data();
  if(id.empty())  return;

  Message msg;
  msg.what = RTCMainEvent::ON_JOINED;
  msg.data = std::make_shared<std::string>(room);
  msg.data_1 = std::make_shared<std::string>(id);
  msg.data_2 = std::make_shared<std::vector<std::string>>(otherClientIds);
  this->room_state_change_callback_->notifyMainUI(msg);
}

void RTCRoomManager::onLeaved(const std::string &room, const std::string &id)
{
  RTC_LOG(LS_INFO) << __FUNCTION__ <<" room:"<<room <<" id:"<<id;
  Message msg;
  msg.what = RTCMainEvent::ON_LEAVED;
  msg.data = std::make_shared<std::string>(room);
  msg.data_1 = std::make_shared<std::string>(id);
  this->room_state_change_callback_->notifyMainUI(msg);
}

void RTCRoomManager::onMessage(const std::string &from, const std::string &to, const std::string &message)
{
  RTC_LOG(LS_INFO) << __FUNCTION__ <<" from:"<<from <<" to:"<<to << " message:"<<message;
  // Parse JSON message
  Json::Value root;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(message, root);

  if (!parsingSuccessful) {
      // Report to the user the failure and their locations in the document.
      RTC_LOG(LS_ERROR) << "Failed to parse the message: " << reader.getFormattedErrorMessages();
      return;
  }


  if (root.isMember("type")) {
      Message msg;
      std::string type = root["type"].asString();
      if (type == "offer") {
         msg.what = RTCMainEvent::ON_OFFER;
         msg.data = std::make_shared<std::string>(from);
         msg.data_1 = std::make_shared<std::string>(root["sdp"].asString());
      }
      else if (type == "answer") {
        msg.what = RTCMainEvent::ON_ANSWER;
        msg.data = std::make_shared<std::string>(from);
        msg.data_1 = std::make_shared<std::string>(root["sdp"].asString());
      }
      else if (type == "candidate") {
        msg.what = RTCMainEvent::ON_CANDIDATE;
          msg.data = std::make_shared<std::string>(from);
          msg.data_1 = std::make_shared<std::string>((root["id"].asString()));
          msg.data_2 = std::make_shared<int>(root["label"].asInt());
          msg.data_3 = std::make_shared<std::string>(root["candidate"].asString());
      }

    this->room_state_change_callback_->notifyMainUI(msg);
  }
}

//############################### PeerConnection callback ###############################
void RTCRoomManager::OnAddTrack(std::string peerid, rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver, const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface> > &streams)
{
  RTC_LOG(LS_INFO) << __FUNCTION__ <<" peerid:"<<peerid ;
  Message message;
  message.what = RTCMainEvent::ON_ADD_REMOTE_TRACK;
  message.data = std::make_shared<std::string>(peerid);
  message.nsptr_data = reinterpret_cast<int64_t>(receiver->track().release());
  this->room_state_change_callback_->notifyMainUI(message);
}


void RTCRoomManager::OnRemoveTrack(std::string peerid, rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
  RTC_LOG(LS_INFO) << __FUNCTION__ <<" peerid:"<<peerid ;
  if(this->room_state_change_callback_)
    this->room_state_change_callback_->onRemoveTrack(peerid,receiver);
}

void RTCRoomManager::OnDataChannel(std::string peerid, rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
  RTC_LOG(LS_INFO) << __FUNCTION__ <<" peerid:"<<peerid ;

}

void RTCRoomManager::OnIceCandidate(std::string peerid, const webrtc::IceCandidateInterface *candidate)
{
  RTC_LOG(LS_INFO) << __FUNCTION__ <<" peerid:"<<peerid ;
  Json::Value message;
  message["type"] = "cadidate";
  message["label"] = candidate->sdp_mline_index();
  message["id"] = candidate->sdp_mid();
  std::string sdp;
  if (!candidate->ToString(&sdp)) {
      RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
      return;
  }
  message["candidate"] = sdp;
  Json::StreamWriterBuilder writerBuilder;
  std::string messageStr = Json::writeString(writerBuilder, message);
  this->socket_signal_client_imp_->sendMessage(this->room_id_,peerid,messageStr);
}

void RTCRoomManager::OnCreateSuccess(bool offer, std::string peerid, webrtc::SessionDescriptionInterface *desc)
{
  RTC_LOG(LS_INFO) << __FUNCTION__ <<" offer"<<offer<<" peerid:"<<peerid ;
  this->peer_manager_->setLocalDescription(peerid,desc);
  Json::Value message;
  std::string sdp;
  desc->ToString(&sdp);
  if(offer)
  {
    message["type"] = "offer";
  }else {
    message["type"] = "answer";
  }
  message["sdp"] = sdp;


  Json::StreamWriterBuilder writerBuilder;
  std::string messageStr = Json::writeString(writerBuilder, message);
  this->socket_signal_client_imp_->sendMessage(this->room_id_,peerid,messageStr);

}

void RTCRoomManager::OnCreateFailure(bool offer, std::string peerid, webrtc::RTCError error)
{
  RTC_LOG(LS_INFO) << __FUNCTION__ <<" offer"<<offer<<" peerid:"<<peerid ;

}

}

