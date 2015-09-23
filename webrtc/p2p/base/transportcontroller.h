/*
 *  Copyright 2015 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_P2P_BASE_TRANSPORTCONTROLLER_H_
#define WEBRTC_P2P_BASE_TRANSPORTCONTROLLER_H_

#include <map>
#include <string>
#include <vector>

#include "webrtc/base/sigslot.h"
#include "webrtc/base/sslstreamadapter.h"
#include "webrtc/p2p/base/candidate.h"
#include "webrtc/p2p/base/transport.h"

namespace rtc {
class Thread;
}

namespace cricket {

class TransportController : public sigslot::has_slots<>,
                            public rtc::MessageHandler {
 public:
  TransportController(rtc::Thread* signaling_thread,
                      rtc::Thread* worker_thread,
                      PortAllocator* port_allocator);

  virtual ~TransportController();

  rtc::Thread* signaling_thread() const { return signaling_thread_; }
  rtc::Thread* worker_thread() const { return worker_thread_; }

  PortAllocator* port_allocator() const { return port_allocator_; }

  // Can only be set before transports are created.
  // TODO(deadbeef): Make this an argument to the constructor once BaseSession
  // and WebRtcSession are combined
  bool SetSslMaxProtocolVersion(rtc::SSLProtocolVersion version);

  void SetIceConnectionReceivingTimeout(int timeout_ms);
  void SetIceRole(IceRole ice_role);

  // TODO(deadbeef) - Return role of each transport, as role may differ from
  // one another.
  // In current implementaion we just return the role of the first transport
  // alphabetically.
  bool GetSslRole(rtc::SSLRole* role);

  // Specifies the identity to use in this session.
  // Can only be called once.
  bool SetLocalCertificate(
      const rtc::scoped_refptr<rtc::RTCCertificate>& certificate);
  bool GetLocalCertificate(
      const std::string& transport_name,
      rtc::scoped_refptr<rtc::RTCCertificate>* certificate);
  // Caller owns returned certificate
  bool GetRemoteSSLCertificate(const std::string& transport_name,
                               rtc::SSLCertificate** cert);
  bool SetLocalTransportDescription(const std::string& transport_name,
                                    const TransportDescription& tdesc,
                                    ContentAction action,
                                    std::string* err);
  bool SetRemoteTransportDescription(const std::string& transport_name,
                                     const TransportDescription& tdesc,
                                     ContentAction action,
                                     std::string* err);
  // Start gathering candidates for any new transports, or transports doing an
  // ICE restart.
  void MaybeStartGathering();
  bool AddRemoteCandidates(const std::string& transport_name,
                           const Candidates& candidates,
                           std::string* err);
  bool ReadyForRemoteCandidates(const std::string& transport_name);
  bool GetStats(const std::string& transport_name, TransportStats* stats);

  virtual TransportChannel* CreateTransportChannel_w(
      const std::string& transport_name,
      int component);
  virtual void DestroyTransportChannel_w(const std::string& transport_name,
                                         int component);

  // All of these signals are fired on the signalling thread.

  // If any transport failed => failed,
  // Else if all completed => completed,
  // Else if all connected => connected,
  // Else => connecting
  sigslot::signal1<IceConnectionState> SignalConnectionState;

  // Receiving if any transport is receiving
  sigslot::signal1<bool> SignalReceiving;

  // If all transports done gathering => complete,
  // Else if any are gathering => gathering,
  // Else => new
  sigslot::signal1<IceGatheringState> SignalGatheringState;

  // (transport_name, candidates)
  sigslot::signal2<const std::string&, const Candidates&>
      SignalCandidatesGathered;

  // for unit test
  const rtc::scoped_refptr<rtc::RTCCertificate>& certificate_for_testing();

 protected:
  // Protected and virtual so we can override it in unit tests.
  virtual Transport* CreateTransport_w(const std::string& transport_name);

  // For unit tests
  const std::map<std::string, Transport*>& transports() { return transports_; }
  Transport* GetTransport_w(const std::string& transport_name);

 private:
  void OnMessage(rtc::Message* pmsg) override;

  Transport* GetOrCreateTransport_w(const std::string& transport_name);
  void DestroyTransport_w(const std::string& transport_name);
  void DestroyAllTransports_w();

  bool SetSslMaxProtocolVersion_w(rtc::SSLProtocolVersion version);
  void SetIceConnectionReceivingTimeout_w(int timeout_ms);
  void SetIceRole_w(IceRole ice_role);
  bool GetSslRole_w(rtc::SSLRole* role);
  bool SetLocalCertificate_w(
      const rtc::scoped_refptr<rtc::RTCCertificate>& certificate);
  bool GetLocalCertificate_w(
      const std::string& transport_name,
      rtc::scoped_refptr<rtc::RTCCertificate>* certificate);
  bool GetRemoteSSLCertificate_w(const std::string& transport_name,
                                 rtc::SSLCertificate** cert);
  bool SetLocalTransportDescription_w(const std::string& transport_name,
                                      const TransportDescription& tdesc,
                                      ContentAction action,
                                      std::string* err);
  bool SetRemoteTransportDescription_w(const std::string& transport_name,
                                       const TransportDescription& tdesc,
                                       ContentAction action,
                                       std::string* err);
  void MaybeStartGathering_w();
  bool AddRemoteCandidates_w(const std::string& transport_name,
                             const Candidates& candidates,
                             std::string* err);
  bool ReadyForRemoteCandidates_w(const std::string& transport_name);
  bool GetStats_w(const std::string& transport_name, TransportStats* stats);

  // Handlers for signals from Transport.
  void OnTransportConnecting_w(Transport* transport);
  void OnTransportWritableState_w(Transport* transport);
  void OnTransportReceivingState_w(Transport* transport);
  void OnTransportCompleted_w(Transport* transport);
  void OnTransportFailed_w(Transport* transport);
  void OnTransportGatheringState_w(Transport* transport);
  void OnTransportCandidatesGathered_w(
      Transport* transport,
      const std::vector<Candidate>& candidates);
  void OnTransportRoleConflict_w();

  void UpdateAggregateStates_w();
  bool HasChannels_w();

  rtc::Thread* const signaling_thread_ = nullptr;
  rtc::Thread* const worker_thread_ = nullptr;
  typedef std::map<std::string, Transport*> TransportMap;
  TransportMap transports_;

  PortAllocator* const port_allocator_ = nullptr;
  rtc::SSLProtocolVersion ssl_max_version_ = rtc::SSL_PROTOCOL_DTLS_10;

  // Aggregate state for Transports
  IceConnectionState connection_state_ = kIceConnectionConnecting;
  bool receiving_ = false;
  IceGatheringState gathering_state_ = kIceGatheringNew;

  // TODO(deadbeef): Move the fields below down to the transports themselves

  // Timeout value in milliseconds for which no ICE connection receives
  // any packets
  int ice_receiving_timeout_ms_ = -1;
  IceRole ice_role_ = ICEROLE_CONTROLLING;
  // Flag which will be set to true after the first role switch
  bool ice_role_switch_ = false;
  uint64 ice_tiebreaker_ = rtc::CreateRandomId64();
  rtc::scoped_refptr<rtc::RTCCertificate> certificate_;
};

}  // namespace cricket

#endif  // WEBRTC_P2P_BASE_TRANSPORTCONTROLLER_H_
