#ifndef ONROOMSTATECHANGECALLBACK_H
#define ONROOMSTATECHANGECALLBACK_H

#include <string>
#include "Message.h"
namespace PCS {

class OnRoomStateChangeCallback {
public:
    //当连接中
    virtual void onConnecting() = 0;
    //当连接上
    virtual void onConnected() = 0;
    //当连接失败
    virtual void onError(std::string error) = 0;
   //当自己加入
   virtual void onJoined(const std::string id) = 0;
    //当用户加入
   virtual void onUserJoined(const std::string id) = 0;
   //当离开房间
   virtual void onLeaved(const std::string id) = 0;
   //当接收到远端的轨道
   virtual void onAddTrack(std::string peerId,void* track) = 0;
   //当接收到远端的轨道
   virtual void onRemoveTrack(std::string peerId,void * track) = 0;
   //当需要更新 UI 时
   virtual void notifyMainUI(Message msg) = 0;

   virtual ~OnRoomStateChangeCallback() = default;
};


}
#endif // ONROOMSTATECHANGECALLBACK_H
