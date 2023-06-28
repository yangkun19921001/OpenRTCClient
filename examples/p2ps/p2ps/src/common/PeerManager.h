#ifndef PEERMANAGER_H
#define PEERMANAGER_H
#include "../../webrtc_headers.h"



#include <map>



namespace PCS {

/**
 *
 * PeerClient 管理
 * @brief The PeerManager class
 */
class PeerManager {
public:
    PeerManager();
    ~PeerManager();

private:
    std::map<std::string, rtc::scoped_refptr<webrtc::PeerConnectionInterface>> peer_connections_;
};

}

#endif // PEERMANAGER_H
