package com.rtc.event;

import android.util.Log;

import com.rtc.Constants;

import org.json.JSONException;
import org.json.JSONObject;
import org.webrtc.DataChannel;
import org.webrtc.IceCandidate;
import org.webrtc.Logging;
import org.webrtc.MediaStream;
import org.webrtc.PeerConnection;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/4/2
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
public class PeerConnectionObserverImpl implements PeerConnection.Observer {
    private String userId;
    private String TAG = null;

    public PeerConnectionObserverImpl(String id) {
        this.userId = id;
        TAG = "PC-" + userId;
    }

    @Override
    public void onSignalingChange(PeerConnection.SignalingState newState) {
        Logging.d(TAG, "onSignalingChange:" + newState);
    }

    @Override
    public void onIceConnectionChange(PeerConnection.IceConnectionState newState) {
        Logging.d(TAG, "onIceConnectionChange:" + newState);
        if (newState == PeerConnection.IceConnectionState.CONNECTED) {
//                    events.OnIceConnected();
        } else if (newState == PeerConnection.IceConnectionState.DISCONNECTED) {
//                    events.OnIceDisconnected();
        } else if (newState == PeerConnection.IceConnectionState.FAILED) {
            Log.e(Constants.P2PTAG, "ICE connection failed.");
        }
    }

    @Override
    public void onIceConnectionReceivingChange(boolean receiving) {
        Logging.d(TAG, "onIceConnectionReceivingChange:" + receiving);
    }

    @Override
    public void onIceGatheringChange(PeerConnection.IceGatheringState newState) {
        Logging.d(TAG, "onIceGatheringChange:" + newState);
    }

    @Override
    public void onIceCandidate(IceCandidate iceCandidate) {
        Logging.d(TAG, "onIceCandidate:" + iceCandidate);
    }

    @Override
    public void onIceCandidatesRemoved(IceCandidate[] candidates) {
        Logging.d(TAG, "onIceCandidatesRemoved:" + candidates);
    }

    @Override
    public void onAddStream(MediaStream stream) {
        Logging.d(TAG, "onAddStream:" + stream);
    }

    @Override
    public void onRemoveStream(MediaStream stream) {
        Logging.d(TAG, "onRemoveStream:" + stream);
    }

    @Override
    public void onDataChannel(DataChannel dataChannel) {
        Logging.d(TAG, "onDataChannel:" + dataChannel);

    }

    @Override
    public void onRenegotiationNeeded() {
        Logging.d(TAG, "onRenegotiationNeeded");

    }

    public String getUserId() {
        return userId;
    }
}
