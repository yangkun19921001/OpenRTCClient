package com.sample.p2p;

import org.webrtc.IceCandidate;
import org.webrtc.RTCStatsReport;
import org.webrtc.SessionDescription;

/**
   * Peer connection events.
   */
  public interface PeerConnectionEvents {
    /**
     * Callback fired once local SDP is created and set.
     */
    void OnLocalDescription(final SessionDescription sdp);

    /**
     * Callback fired once local Ice candidate is generated.
     */
    void OnIceCandidate(final IceCandidate candidate);

    /**
     * Callback fired once local ICE candidates are removed.
     */
    void OnIceCandidatesRemoved(final IceCandidate[] candidates);

    /**
     * Callback fired once connection is established (IceConnectionState is
     * CONNECTED).
     */
    void OnIceConnected();

    /**
     * Callback fired once connection is disconnected (IceConnectionState is
     * DISCONNECTED).
     */
    void OnIceDisconnected();

    /**
     * Callback fired once DTLS connection is established (PeerConnectionState
     * is CONNECTED).
     */
    void OnPeerConnectionConnected();

    /**
     * Callback fired once DTLS connection is disconnected (PeerConnectionState
     * is DISCONNECTED).
     */
    void OnPeerConnectionDisconnected();

    /**
     * Callback fired once peer connection is closed.
     */
    void OnPeerConnectionClosed();

    /**
     * Callback fired once peer connection statistics is ready.
     */
    void OnPeerConnectionStatsReady(final RTCStatsReport report);

    /**
     * Callback fired once peer connection error happened.
     */
    void OnPeerConnectionError(final String description);
  }