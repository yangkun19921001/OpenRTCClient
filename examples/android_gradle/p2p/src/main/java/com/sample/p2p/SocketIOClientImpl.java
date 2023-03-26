package com.sample.p2p;

import android.support.annotation.Nullable;
import android.util.Log;

import org.json.JSONObject;

import java.net.URISyntaxException;

import io.socket.client.IO;
import io.socket.client.Socket;
import io.socket.emitter.Emitter;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/3/18
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
class SocketIOClientImpl implements ISignalClient {

    @Nullable
    private ISignalEventListener mISignalEventListener;

    @Nullable
    private Socket mSocket;

    private String TAG = this.getClass().getSimpleName(), mRoomId;


    private void OnSocketEvent() {
        mSocket.on(Socket.EVENT_CONNECT_ERROR, new Emitter.Listener() {
            @Override
            public void call(Object... args) {
                Log.e(TAG, "onConnectError: " + args[0]);
            }
        });

        mSocket.on(Socket.EVENT_CONNECT_ERROR, new Emitter.Listener() {
            @Override
            public void call(Object... args) {
                for (Object arg : args) {
                    Log.e(TAG, "onError: " + arg);
                }
            }
        });

        mSocket.on(Socket.EVENT_CONNECT, new Emitter.Listener() {
            @Override
            public void call(Object... args) {
                String sessionId = mSocket.id();
                Log.i(TAG, "onConnected");
                if (mISignalEventListener != null) {
                    mISignalEventListener.OnConnected();
                }
            }
        });

        mSocket.on(Socket.EVENT_CONNECTING, new Emitter.Listener() {
            @Override
            public void call(Object... args) {
                Log.i(TAG, "onConnecting");
                if (mISignalEventListener != null) {
                    mISignalEventListener.OnConnecting();
                }
            }
        });

        mSocket.on(Socket.EVENT_DISCONNECT, new Emitter.Listener() {
            @Override
            public void call(Object... args) {
                Log.i(Constants.P2PTAG, "onDisconnected");
                if (mISignalEventListener != null) {
                    mISignalEventListener.OnDisconnected();
                }
            }
        });

        //当加入
        mSocket.on(SignalEvent.JOINED.getEventName(), args -> {
            String roomName = (String) args[0];
            String userId = (String) args[1];
            if (mISignalEventListener != null) {
                mISignalEventListener.OnUserJoined(roomName, userId, false);
            }
            Log.i(TAG, "onUserJoined, room:" + roomName + "uid:" + userId);
        });

        //当离开
        mSocket.on(SignalEvent.LEAVED.getEventName(), args -> {
            String roomName = (String) args[0];
            String userId = (String) args[1];
            if (/*!mUserId.equals(userId) &&*/ mISignalEventListener != null) {
                //mOnSignalEventListener.onRemoteUserLeft(userId);
                mISignalEventListener.OnUserLeaved(roomName, userId);
            }
            Log.i(Constants.P2PTAG, "onUserLeaved, room:" + roomName + "uid:" + userId);

        });

        //当其它用户加入
        mSocket.on(SignalEvent.OTHERJOIN.getEventName(), args -> {
            String roomName = (String) args[0];
            String userId = (String) args[1];
            if (mISignalEventListener != null) {
                mISignalEventListener.OnRemoteUserJoined(roomName, userId);
            }
            Log.i(Constants.P2PTAG, "onRemoteUserJoined, room:" + roomName + "uid:" + userId);

        });

        //当对方发送离开消息
        mSocket.on(SignalEvent.BYE.getEventName(), args -> {
            String roomName = (String) args[0];
            String userId = (String) args[1];
            if (mISignalEventListener != null) {
                mISignalEventListener.OnRemoteUserLeaved(roomName, userId);
            }
            Log.i(Constants.P2PTAG, "onRemoteUserLeaved, room:" + roomName + "uid:" + userId);
        });

        //当房间满了
        mSocket.on(SignalEvent.FULL.getEventName(), args -> {
            //释放资源
            mSocket.disconnect();
            mSocket.close();
            mSocket = null;

            String roomName = (String) args[0];
            String userId = (String) args[1];

            if (mISignalEventListener != null) {
                mISignalEventListener.OnRoomFull(roomName, userId);
            }

            Log.i(Constants.P2PTAG, "onRoomFull, room:" + roomName + "uid:" + userId);

        });

        //信令消息
        mSocket.on(SignalEvent.MESSAGE.getEventName(), args -> {
            String roomName = (String) args[0];
            JSONObject msg = (JSONObject) args[1];

            if (mISignalEventListener != null) {
                mISignalEventListener.OnMessage(msg);
            }
            Log.i(Constants.P2PTAG, "onMessage, room:" + roomName + "data:");
        });
    }

    @Override
    public void connect(String url, ISignalEventListener events) {
        Log.i(Constants.P2PTAG, "connect: " + url);
        this.mISignalEventListener = events;
        IO.Options options = new IO.Options();
        SocketSSL.set(options);
        try {
            mSocket = IO.socket(url, options);
            OnSocketEvent();
            mSocket.connect();
        } catch (URISyntaxException e) {
            e.printStackTrace();
            Log.e(Constants.P2PTAG, e.getMessage());
        }
    }

    @Override
    public void join(String roomId) {
        Log.i(Constants.P2PTAG, "join: " + roomId);
        mSocket.emit("join", roomId);
    }

    @Override
    public void leave(String roomId) {
        Log.i(Constants.P2PTAG, "leave: " + roomId);
        mSocket.emit("leave", roomId);
        mSocket.close();
    }

    @Override
    public void sendSignalMessage(String roomId, JSONObject message) {
        Log.i(Constants.P2PTAG, "sendSignalMessage: " + roomId);
        mSocket.emit("message", roomId, message);
    }
}
