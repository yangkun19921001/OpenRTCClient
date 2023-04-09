package com.rtc.event;

import android.util.Log;

import com.rtc.Constants;

import org.webrtc.SdpObserver;
import org.webrtc.SessionDescription;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/4/2
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
public class SdpObserverImpl implements SdpObserver {
    private String userId;
    private IPeerEventListener peerEventListener;
    private String type;

    public SdpObserverImpl(String userId, String type, IPeerEventListener peerEventListener) {
        this.userId = userId;
        this.peerEventListener = peerEventListener;
        this.type = type;
    }

    public String getUserId() {
        return userId;
    }

    @Override
    public void onCreateSuccess(SessionDescription sdp) {

    }

    @Override
    public void onSetSuccess() {
        Log.e(Constants.P2PSTAG, "onSetSuccess:");


    }

    @Override
    public void onCreateFailure(String error) {
        Log.e(Constants.P2PSTAG, "onCreateFailure:" + error);
        if (this.peerEventListener != null)
            this.peerEventListener.onCreateFailure(type, getUserId(), error);
    }

    @Override
    public void onSetFailure(String error) {
        if (this.peerEventListener != null)
            this.peerEventListener.onSetFailure(type, getUserId(), error);

        Log.e(Constants.P2PSTAG, "onSetFailure:" + error);
    }
}
