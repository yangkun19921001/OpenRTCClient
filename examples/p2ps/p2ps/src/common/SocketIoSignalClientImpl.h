#ifndef SOCKETIOSIGNALCLIENTIMPL_H
#define SOCKETIOSIGNALCLIENTIMPL_H

#include "ISignalClient.h"
#include "sio_client.h"
#include "rtc_base/strings/json.h"
#include <memory>
#include "rtc_base/logging.h"

namespace PCS{
class SocketIoSignalClientImpl : public ISignalClient
{
public:
    SocketIoSignalClientImpl();

    /*连接服务端*/
    virtual bool connect(const std::string url,OnSignalEventListener* listener)override;
    /*加入会话*/
    virtual void join(const std::string roomId)override;
    /*离开会话*/
    virtual void leave(const std::string roomId)override;
    /*销毁 client */
    virtual void release()override;
    /*发送消息*/
    virtual void sendMessage(const std::string roomId, const std::string remoteId, const std::string message)override;
    virtual std::string getSocketId()override;


private:
    void setSocketListener();

private:
    std::unique_ptr<sio::client> socket;
    std::string mLocalSocketId;
    OnSignalEventListener * events;
};
}
#endif // SOCKETIOSIGNALCLIENTIMPL_H
