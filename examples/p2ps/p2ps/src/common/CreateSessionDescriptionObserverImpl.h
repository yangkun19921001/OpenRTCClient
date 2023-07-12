#ifndef CREATESESSIONDESCRIPTIONOBSERVERIMPL_H
#define CREATESESSIONDESCRIPTIONOBSERVERIMPL_H
#include "../../../webrtc_headers.h"
#include "OnPeerManagerEvents.h"


namespace PCS {
class CreateSessionDescriptionObserImpl : public webrtc::CreateSessionDescriptionObserver
{
public:
    CreateSessionDescriptionObserImpl(bool offer,std::string peerid,OnPeerManagerEvents *ets) {
        this->peerid_ = peerid;
        this->ets_ = ets;
        this->offer_ = offer;

    }
     void OnSuccess(webrtc::SessionDescriptionInterface* desc) override{
        if(this->ets_)this->ets_->OnCreateSuccess(this->offer_,this->peerid_,desc);
    };
    // The OnFailure callback takes an RTCError, which consists of an
    // error code and a string.
    // RTCError is non-copyable, so it must be passed using std::move.
    // Earlier versions of the API used a string argument. This version
    // is removed; its functionality was the same as passing
    // error.message.
     void OnFailure(webrtc::RTCError error) override{
          if(this->ets_)this->ets_->OnCreateFailure(this->offer_,this->peerid_,error);
    };

     // Implement AddRef from rtc::RefCountInterface
     void AddRef() const override {
          // implementation here

     }

     // Implement Release from rtc::RefCountInterface
     rtc::RefCountReleaseStatus Release() const override {
          // implementation here
          return rtc::RefCountReleaseStatus::kDroppedLastRef;
     }

private:
    std::string peerid_;
    bool offer_;
    OnPeerManagerEvents* ets_;
};
}
#endif // CREATESESSIONDESCRIPTIONOBSERVERIMPL_H
