#ifndef ISIGNALCLIENT_H
#define ISIGNALCLIENT_H

#include <string>
#include "OnSignalEventListener.h"

namespace PCS{
class ISignalClient {
public:
    enum class SignalEvent {
        JOINED = 0,
        JOIN,
        LEAVED,
        LEAVE,
        MESSAGE
    };

    static std::string SignalEventToString(SignalEvent event) {
        switch(event) {
        case SignalEvent::JOINED:  return "joined";
        case SignalEvent::LEAVED:  return "leaved";
        case SignalEvent::JOIN:    return "join";
        case SignalEvent::LEAVE:   return "leave";
        case SignalEvent::MESSAGE: return "message";
        default: return "unknown";
        }
    }

    /*连接服务端*/
    virtual bool connect(const std::string url,OnSignalEventListener* listener) = 0;
    /*加入会话*/
    virtual void join(const std::string roomId) = 0;
    /*离开会话*/
    virtual void leave(const std::string roomId) = 0;
    /*销毁 client */
    virtual void release() = 0;
    /*发送消息*/
    virtual void sendMessage(const std::string roomId, const std::string remoteId, const std::string message) = 0;

    virtual std::string getSocketId() = 0;
    virtual ~ISignalClient() = default;
};

}
#endif // ISIGNALCLIENT_H
