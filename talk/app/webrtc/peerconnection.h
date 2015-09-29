/*
 * libjingle
 * Copyright 2012 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TALK_APP_WEBRTC_PEERCONNECTION_H_
#define TALK_APP_WEBRTC_PEERCONNECTION_H_

#include <string>

#include "talk/app/webrtc/dtlsidentitystore.h"
#include "talk/app/webrtc/mediastreamsignaling.h"
#include "talk/app/webrtc/peerconnectionfactory.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "talk/app/webrtc/rtpreceiverinterface.h"
#include "talk/app/webrtc/rtpsenderinterface.h"
#include "talk/app/webrtc/statscollector.h"
#include "talk/app/webrtc/streamcollection.h"
#include "talk/app/webrtc/webrtcsession.h"
#include "webrtc/base/scoped_ptr.h"

namespace webrtc {

typedef std::vector<PortAllocatorFactoryInterface::StunConfiguration>
    StunConfigurations;
typedef std::vector<PortAllocatorFactoryInterface::TurnConfiguration>
    TurnConfigurations;

// PeerConnection implements the PeerConnectionInterface interface.
// It uses MediaStreamSignaling and WebRtcSession to implement
// the PeerConnection functionality.
class PeerConnection : public PeerConnectionInterface,
                       public MediaStreamSignalingObserver,
                       public IceObserver,
                       public rtc::MessageHandler,
                       public sigslot::has_slots<> {
 public:
  explicit PeerConnection(PeerConnectionFactory* factory);

  bool Initialize(
      const PeerConnectionInterface::RTCConfiguration& configuration,
      const MediaConstraintsInterface* constraints,
      PortAllocatorFactoryInterface* allocator_factory,
      rtc::scoped_ptr<DtlsIdentityStoreInterface> dtls_identity_store,
      PeerConnectionObserver* observer);
  rtc::scoped_refptr<StreamCollectionInterface> local_streams() override;
  rtc::scoped_refptr<StreamCollectionInterface> remote_streams() override;
  bool AddStream(MediaStreamInterface* local_stream) override;
  void RemoveStream(MediaStreamInterface* local_stream) override;

  rtc::scoped_refptr<DtmfSenderInterface> CreateDtmfSender(
      AudioTrackInterface* track) override;

  std::vector<rtc::scoped_refptr<RtpSenderInterface>> GetSenders()
      const override;
  std::vector<rtc::scoped_refptr<RtpReceiverInterface>> GetReceivers()
      const override;

  rtc::scoped_refptr<DataChannelInterface> CreateDataChannel(
      const std::string& label,
      const DataChannelInit* config) override;
  bool GetStats(StatsObserver* observer,
                webrtc::MediaStreamTrackInterface* track,
                StatsOutputLevel level) override;

  SignalingState signaling_state() override;

  // TODO(bemasc): Remove ice_state() when callers are removed.
  IceState ice_state() override;
  IceConnectionState ice_connection_state() override;
  IceGatheringState ice_gathering_state() override;

  const SessionDescriptionInterface* local_description() const override;
  const SessionDescriptionInterface* remote_description() const override;

  // JSEP01
  void CreateOffer(CreateSessionDescriptionObserver* observer,
                   const MediaConstraintsInterface* constraints) override;
  void CreateOffer(CreateSessionDescriptionObserver* observer,
                   const RTCOfferAnswerOptions& options) override;
  void CreateAnswer(CreateSessionDescriptionObserver* observer,
                    const MediaConstraintsInterface* constraints) override;
  void SetLocalDescription(SetSessionDescriptionObserver* observer,
                           SessionDescriptionInterface* desc) override;
  void SetRemoteDescription(SetSessionDescriptionObserver* observer,
                            SessionDescriptionInterface* desc) override;
  bool SetConfiguration(
      const PeerConnectionInterface::RTCConfiguration& config) override;
  bool AddIceCandidate(const IceCandidateInterface* candidate) override;

  void RegisterUMAObserver(UMAObserver* observer) override;

  void Close() override;

 protected:
  ~PeerConnection() override;

 private:
  // Implements MessageHandler.
  void OnMessage(rtc::Message* msg) override;

  // Implements MediaStreamSignalingObserver.
  void OnAddRemoteStream(MediaStreamInterface* stream) override;
  void OnRemoveRemoteStream(MediaStreamInterface* stream) override;
  void OnAddDataChannel(DataChannelInterface* data_channel) override;
  void OnAddRemoteAudioTrack(MediaStreamInterface* stream,
                             AudioTrackInterface* audio_track,
                             uint32 ssrc) override;
  void OnAddRemoteVideoTrack(MediaStreamInterface* stream,
                             VideoTrackInterface* video_track,
                             uint32 ssrc) override;
  void OnRemoveRemoteAudioTrack(MediaStreamInterface* stream,
                                AudioTrackInterface* audio_track) override;
  void OnRemoveRemoteVideoTrack(MediaStreamInterface* stream,
                                VideoTrackInterface* video_track) override;
  void OnAddLocalAudioTrack(MediaStreamInterface* stream,
                            AudioTrackInterface* audio_track,
                            uint32 ssrc) override;
  void OnAddLocalVideoTrack(MediaStreamInterface* stream,
                            VideoTrackInterface* video_track,
                            uint32 ssrc) override;
  void OnRemoveLocalAudioTrack(MediaStreamInterface* stream,
                               AudioTrackInterface* audio_track,
                               uint32 ssrc) override;
  void OnRemoveLocalVideoTrack(MediaStreamInterface* stream,
                               VideoTrackInterface* video_track) override;
  void OnRemoveLocalStream(MediaStreamInterface* stream) override;

  // Implements IceObserver
  void OnIceConnectionChange(IceConnectionState new_state) override;
  void OnIceGatheringChange(IceGatheringState new_state) override;
  void OnIceCandidate(const IceCandidateInterface* candidate) override;
  void OnIceComplete() override;
  void OnIceConnectionReceivingChange(bool receiving) override;

  // Signals from WebRtcSession.
  void OnSessionStateChange(cricket::BaseSession* session,
                            cricket::BaseSession::State state);
  void ChangeSignalingState(SignalingState signaling_state);

  rtc::Thread* signaling_thread() const {
    return factory_->signaling_thread();
  }

  void PostSetSessionDescriptionFailure(SetSessionDescriptionObserver* observer,
                                        const std::string& error);

  bool IsClosed() const {
    return signaling_state_ == PeerConnectionInterface::kClosed;
  }

  std::vector<rtc::scoped_refptr<RtpSenderInterface>>::iterator
  FindSenderForTrack(MediaStreamTrackInterface* track);
  std::vector<rtc::scoped_refptr<RtpReceiverInterface>>::iterator
  FindReceiverForTrack(MediaStreamTrackInterface* track);

  // Storing the factory as a scoped reference pointer ensures that the memory
  // in the PeerConnectionFactoryImpl remains available as long as the
  // PeerConnection is running. It is passed to PeerConnection as a raw pointer.
  // However, since the reference counting is done in the
  // PeerConnectionFactoryInteface all instances created using the raw pointer
  // will refer to the same reference count.
  rtc::scoped_refptr<PeerConnectionFactory> factory_;
  PeerConnectionObserver* observer_;
  UMAObserver* uma_observer_;
  SignalingState signaling_state_;
  // TODO(bemasc): Remove ice_state_.
  IceState ice_state_;
  IceConnectionState ice_connection_state_;
  IceGatheringState ice_gathering_state_;

  rtc::scoped_ptr<cricket::PortAllocator> port_allocator_;
  rtc::scoped_ptr<WebRtcSession> session_;
  rtc::scoped_ptr<MediaStreamSignaling> mediastream_signaling_;
  rtc::scoped_ptr<StatsCollector> stats_;

  std::vector<rtc::scoped_refptr<RtpSenderInterface>> senders_;
  std::vector<rtc::scoped_refptr<RtpReceiverInterface>> receivers_;
};

}  // namespace webrtc

#endif  // TALK_APP_WEBRTC_PEERCONNECTION_H_
