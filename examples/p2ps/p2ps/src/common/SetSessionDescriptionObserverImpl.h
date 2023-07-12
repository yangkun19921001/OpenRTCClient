#ifndef SETSESSIONDESCRIPTIONOBSERVERIMPL_H
#define SETSESSIONDESCRIPTIONOBSERVERIMPL_H

#include "../../webrtc_headers.h"

namespace PCS {
class SetSessionDescriptionObserverImpl
    : public webrtc::SetSessionDescriptionObserver {
public:

    static SetSessionDescriptionObserverImpl* Create() {
        return new rtc::RefCountedObject<SetSessionDescriptionObserverImpl>();
    }
    virtual void OnSuccess() { RTC_LOG(LS_INFO) << __FUNCTION__; }
    virtual void OnFailure(webrtc::RTCError error) {
        RTC_LOG(LS_INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                         << error.message();
    }
};
}


#endif // SETSESSIONDESCRIPTIONOBSERVERIMPL_H
