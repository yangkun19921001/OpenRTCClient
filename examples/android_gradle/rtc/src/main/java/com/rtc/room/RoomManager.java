package com.rtc.room;

import android.content.Context;
import android.support.annotation.Nullable;
import android.util.Log;

import com.rtc.Constants;
import com.rtc.PeerConnectionParameters;
import com.rtc.RoomConnectionParameters;
import com.rtc.event.IPeerEventListener;
import com.rtc.event.ISignalEventListener;
import com.rtc.socket.SocketIOClientImpl;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.webrtc.EglBase;
import org.webrtc.IceCandidate;
import org.webrtc.VideoCapturer;
import org.webrtc.VideoSink;
import org.webrtc.VideoTrack;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/4/9
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
public class RoomManager implements ISignalEventListener, IPeerEventListener {
    private String TAG = this.getClass().getSimpleName();

    @Nullable
    private SocketIOClientImpl socket;
    @Nullable
    private PeerManager pm;
    @Nullable
    private IRoomEventListener roomEventListener;
    @Nullable
    private PeerConnectionParameters peerParameters;
    @Nullable
    private String roomName;
    @Nullable
    private RoomConnectionParameters roomParameters;
    private final EglBase mRootEglBase = EglBase.create();



    public RoomManager() {
        socket = new SocketIOClientImpl();
    }

    public void setRoomEventListener(@Nullable IRoomEventListener roomEventListener) {
        this.roomEventListener = roomEventListener;
    }

    public void joinRoom(@Nullable Context context, @Nullable VideoSink localSink, @Nullable VideoCapturer videoCapturer, @Nullable PeerConnectionParameters parameters, @Nullable RoomConnectionParameters roomConnectionParameters) {
        this.peerParameters = parameters;
        this.roomName = roomConnectionParameters.roomId;
        roomParameters = roomConnectionParameters;
        socket.connect(roomConnectionParameters.roomUrl, this);
        pm = new PeerManager(context, mRootEglBase, localSink, videoCapturer, peerParameters, peerParameters.dataChannelParameters, this);
    }

    public void leaveRoom() {
        socket.leave(roomName);
    }

    public void release() {
        leaveRoom();
        socket.release();
        pm.release();
        socket = null;
        pm = null;
    }

    @Override
    public void onConnecting() {
        if (roomEventListener != null) {
            roomEventListener.OnConnecting();
        }
    }

    @Override
    public void onConnectSuccessful() {
        socket.join(roomName);
        if (roomEventListener != null) {
            roomEventListener.OnConnected();
        }
    }

    @Override
    public void onConnectError(String e) {
        if (roomEventListener != null) {
            roomEventListener.OnConnectError(e);
        }
    }

    @Override
    public void onJoined(String room, String remoteId, JSONArray otherClientIds) {
        if (remoteId.equals(socket.getSocketId())) {
            Log.d(Constants.P2PSTAG, "Joined room " + room + " with ID " + remoteId);
            if (otherClientIds == null || otherClientIds.length() == 0) {
                Log.d(Constants.P2PSTAG, "Joined room " + room + " with ID " + remoteId + " joom other size:" + 0);
                return;
            }
            // 遍历 otherClientIds 并为每个客户端创建一个新的 PeerConnection
            for (int i = 0; i < otherClientIds.length(); i++) {
                try {
                    String otherClientId = otherClientIds.getString(i);
                    pm.createPeerConnection(otherClientId, false);
                    Log.d(Constants.P2PSTAG, "createPeerConnection Remote joined room " + room + " with ID " + otherClientId);
                } catch (JSONException e) {
                    e.printStackTrace();
                }
            }
        } else {
            Log.d(Constants.P2PSTAG, "Remote client joined room " + room + " with ID " + remoteId);
            pm.createPeerConnection(remoteId, true);
        }
    }

    @Override
    public void onLeaved(String room, String remoteId) {
        pm.removePeerConnection(room, remoteId);

    }

    @Override
    public void onMessage(String from, String to, JSONObject message) {
        if (message.has("type")) {
            try {
                String type = message.getString("type");
                if (type.equals("offer")) {
                    pm.handleOffer(from, message.getString("sdp"));
                } else if (type.equals("answer")) {
                    pm.handleAnswer(from, message.getString("sdp"));
                } else if (type.equals("candidate")) {
                    IceCandidate candidate = new IceCandidate(message.getString("id"), message.getInt("label"), message.getString("candidate"));
                    pm.handleCandidate(from, candidate);
                }
            } catch (JSONException e) {
                throw new RuntimeException(e);
            }
        }
    }

    @Override
    public void onEmit(String message, String room, String remoteId, JSONObject message1) {
        if (socket != null) {
            socket.sendMessage(roomName, remoteId, message1);
        }
    }

    @Override
    public void onSetFailure(String type, String remoteId, String error) {
        if (roomEventListener != null) {
            roomEventListener.OnFailed("remoteId:" + remoteId + " onSetFailure:" + error);
        }
    }

    @Override
    public void onCreateFailure(String type, String remoteId, String error) {
        if (roomEventListener != null) {
            roomEventListener.OnFailed("remoteId:" + remoteId + " onCreateFailure:" + error);
        }
    }

    @Override
    public void onAddCandideFailure(String remoteId, String error) {
        if (roomEventListener != null) {
            roomEventListener.OnFailed("remoteId:" + remoteId + " AddCandideFailure:" + error);
        }
    }

    @Override
    public void onAddRemoteStream(VideoTrack remoteVideoTrack, String remoteId) {
        if (roomEventListener != null) {
            roomEventListener.addRemoteStream(remoteVideoTrack, remoteId);
        }
    }

    @Override
    public void onRemoveRemoteStream(String remoteId) {
        if (roomEventListener != null) {
            roomEventListener.removeRemoteStream(remoteId);
        }
    }

    public EglBase.Context getEglBaseContext() {
        assert pm != null;
        return mRootEglBase.getEglBaseContext();
    }

    public void resume() {
        pm.startVideoSource();
    }

    public void pause() {
        pm.stopVideoSource();
    }
}
