#ifndef ONSIGNALEVENTLISTENER_H
#define ONSIGNALEVENTLISTENER_H
#include <string>
#include <vector>

namespace PCS {

class OnSignalEventListener {
public:
    virtual void onJoined(const std::string& room, const std::string& id, const std::vector<std::string>& otherClientIds) = 0;
    virtual void onLeaved(const std::string& room, const std::string& id) = 0;
    virtual void onMessage(const std::string& from, const std::string& to, const std::string& message) = 0;
    virtual void onConnectSuccessful() = 0;
    virtual void onConnecting() = 0;
    virtual void onConnectError(const std::string& error) = 0;
    // 可以根据需要添加其他的事件处理函数
    virtual ~OnSignalEventListener() = default; // 确保有虚析构函数
};
}

#endif // ONSIGNALEVENTLISTENER_H
