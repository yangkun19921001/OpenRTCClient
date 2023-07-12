#include "SocketIoSignalClientImpl.h"


namespace PCS{
SocketIoSignalClientImpl::SocketIoSignalClientImpl(): socket(std::make_unique<sio::client>()),events(nullptr)
{

}

bool SocketIoSignalClientImpl::connect(const std::string url,OnSignalEventListener* listener)
{
    RTC_LOG(LS_INFO) <<"socketio>>"<<__FUNCTION__;
    this->events = listener;
    setSocketListener();
    socket->connect(url);
    return true;
}

void SocketIoSignalClientImpl::join(const std::string roomId)
{    RTC_LOG(LS_INFO) <<"socketio>>"<<__FUNCTION__<<" roomId:"<<roomId;

  socket->socket()->emit(ISignalClient::SignalEventToString(ISignalClient::SignalEvent::JOIN), roomId);
}

void SocketIoSignalClientImpl::leave(const std::string roomId)
{
    RTC_LOG(LS_INFO)<<"socketio>>" <<__FUNCTION__<<" roomId:"<<roomId;
socket->socket()->emit(ISignalClient::SignalEventToString(ISignalClient::SignalEvent::LEAVE), roomId);
}

void SocketIoSignalClientImpl::release()
{
     RTC_LOG(LS_INFO) <<"socketio>>"<<__FUNCTION__;
    socket->clear_socket_listeners();
    socket->close();


}

void SocketIoSignalClientImpl::sendMessage(const std::string roomId, const std::string remoteId, const std::string jsonMessage)
{
    RTC_LOG(LS_INFO)<<"socketio>>" <<__FUNCTION__;

      sio::message::ptr roomIdMessage = sio::string_message::create(roomId);
      sio::message::ptr remoteIdMessage = sio::string_message::create(remoteId);
      sio::message::ptr jsonMessageMessage = sio::string_message::create(jsonMessage);

      sio::message::list emitMessageList;
      emitMessageList.push(roomIdMessage);
      emitMessageList.push(remoteIdMessage);
      emitMessageList.push(jsonMessageMessage);

    socket->socket()->emit(ISignalClient::SignalEventToString(ISignalClient::SignalEvent::MESSAGE), emitMessageList);
}

std::string SocketIoSignalClientImpl::getSocketId()
{
    return mLocalSocketId;
}

void SocketIoSignalClientImpl::setSocketListener()
{
    RTC_LOG(LS_INFO) <<"socketio>>"<<__FUNCTION__;
    socket->socket()->on(ISignalClient::SignalEventToString(ISignalClient::SignalEvent::JOINED), [this](sio::event& ev)
                         {
                             RTC_LOG(LS_INFO) <<"socketio<<"  <<__FUNCTION__;
                             std::string room = ev.get_messages().at(0)->get_string();
                             std::string id = ev.get_messages().at(1)->get_string();
                             std::vector<std::string> otherClientIds;

                             if(ev.get_messages().size() == 3)
                             {
                                 for (const auto& clientId : ev.get_messages().at(2)->get_vector()) {
                                     otherClientIds.push_back(clientId->get_string());
                                 }
                             }
                             this->events->onJoined(room, id, otherClientIds);
                         });

    socket->socket()->on(ISignalClient::SignalEventToString(ISignalClient::SignalEvent::LEAVED), [this](sio::event& ev)
                         {
        RTC_LOG(LS_INFO) <<"socketio<<"<<__FUNCTION__<<ev.get_ack_message().to_array_message()<<__FUNCTION__;
                             std::string room = ev.get_messages().at(0)->get_string();
                             std::string id = ev.get_messages().at(1)->get_string();
                             if(this->events)
                                 this->events->onLeaved(room, id);
                         });

    socket->socket()->on(ISignalClient::SignalEventToString(ISignalClient::SignalEvent::MESSAGE), [this](sio::event& ev)
                        {
                             RTC_LOG(LS_INFO) <<"socketio<<"<<__FUNCTION__;
                            std::string from =  ev.get_messages().at(0)->get_string();
                            std::string to =  ev.get_messages().at(1)->get_string();

                            Json::Value root;
                            for (const auto& kv : ev.get_messages().at(2)->get_map()) {
                                // 你需要确保message::ptr（这里是std::shared_ptr<Message>）可以转换为一个可以被Json::Value接受的类型
                                if (kv.second->get_flag() == sio::message::flag_integer) {
                                    root[kv.first] = kv.second->get_int();
                                } else if (kv.second->get_flag() == sio::message::flag_string) {
                                    root[kv.first] = kv.second->get_string();
                                }
                            }
                           //std::string message =  ev.get_messages().at(2)->get_string();
                            Json::StreamWriterBuilder builder;
                            std::string message = Json::writeString(builder, root);

                            RTC_LOG(LS_INFO) <<"socketio<<"<<__FUNCTION__ " from:"<<from << " to:"<<to <<" message:"<<message;
                            if(this->events)
                                this->events->onMessage(from, to, message);
                        });

    socket->set_socket_open_listener([this](std::string const& nsp){
        RTC_LOG(LS_INFO)<<"socketio<< " <<"set_socket_open_listener";

        mLocalSocketId = socket->get_sessionid();;

        if(this->events)
            this->events->onConnectSuccessful();
    });

    socket->set_close_listener([this](sio::client::close_reason const& reason){
        RTC_LOG(LS_INFO) <<"set_close_listener";
        if(this->events)
            this->events->onConnectError("close");
    });

    socket->set_fail_listener([this](){
        RTC_LOG(LS_INFO) <<"set_fail_listener";

        if(this->events)
            this->events->onConnectError("connect fail");
    });
}

}
