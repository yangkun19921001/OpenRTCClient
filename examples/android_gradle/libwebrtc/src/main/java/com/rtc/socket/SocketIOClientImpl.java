package com.rtc.socket;

import android.util.Log;

import com.rtc.Constants;
import com.rtc.event.ISignalEventListener;

import org.json.JSONArray;
import org.json.JSONObject;

import java.net.URISyntaxException;

import io.socket.client.IO;
import io.socket.client.Socket;
import io.socket.client.SocketIOException;
import io.socket.emitter.Emitter;

import static com.rtc.socket.ISignalClient.SignalEvent.JOINED;
import static com.rtc.socket.ISignalClient.SignalEvent.LEAVED;
import static com.rtc.socket.ISignalClient.SignalEvent.MESSAGE;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/4/9
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
public class SocketIOClientImpl implements ISignalClient {
    private Socket socket;
    private String mLocalSocketId;
    private ISignalEventListener events;

    @Override
    public void connect(String url, ISignalEventListener listener) {
        try {
            this.events = listener;
            IO.Options options = new IO.Options();
            SocketSSL.set(options);
            socket = IO.socket(url, options);
            setSocketListener();
        } catch (URISyntaxException e) {
            throw new RuntimeException(e);
        }
        socket.connect();
    }


    private void setSocketListener() {
        socket.on(JOINED.getEventName(), args -> {
            try {
                JSONArray otherClientIds = null;
                if (args.length == 3) {
                    otherClientIds = (JSONArray) args[2];
                }
                String room = (String) args[0];
                String id = (String) args[1];

                if (this.events != null)
                    this.events.onJoined(room, id, otherClientIds);
            } catch (Exception e) {
                e.printStackTrace();
            }
        });

        socket.on(LEAVED.getEventName(), args -> {
            String room = (String) args[0];
            String socketId = (String) args[1];
            if (events != null)
                events.onLeaved(room, socketId);
        });

        socket.on(MESSAGE.getEventName(), new Emitter.Listener() {
            @Override
            public void call(Object... args) {
                JSONObject message = (JSONObject) args[2];
                String to = (String) args[1];
                String from = (String) args[0];
                if (events != null)
                    events.onMessage(from, to, message);
            }
        });
        socket.on(Socket.EVENT_CONNECT, args -> {
            mLocalSocketId = socket.id();
            if (events != null)
                events.onConnectSuccessful();
            Log.d(Constants.P2PSTAG, "EVENT_CONNECT " + mLocalSocketId);
        });
        socket.on(Socket.EVENT_CONNECTING, args -> {
            if (events != null)
                events.onConnecting();
            Log.d(Constants.P2PSTAG, "EVENT_CONNECTING " + args);
        });
        socket.on(Socket.EVENT_CONNECT_ERROR, args -> {
            if (events != null)
                for (int i = 0; i < args.length; i++) {
                    if (args[i] instanceof SocketIOException)
                        events.onConnectError(((SocketIOException) args[i]).getMessage());
                    else {
                        events.onConnectError((String) args[i]);
                    }

                }
            Log.d(Constants.P2PSTAG, "EVENT_CONNECT_ERROR " + args);
        });
        socket.on(Socket.EVENT_CONNECT_TIMEOUT, args -> {
            if (events != null)
                events.onConnectError("timeout");
            Log.d(Constants.P2PSTAG, "EVENT_CONNECT_TIMEOUT " + args);
        });
    }

    public String getSocketId() {
        return mLocalSocketId;
    }

    @Override
    public void join(String roomId) {
        socket.emit("join", roomId);
    }

    @Override
    public void leave(String roomId) {
        socket.emit("leave", roomId);
    }

    @Override
    public void release() {
        socket.disconnect();
    }

    @Override
    public void sendMessage(String roomId, String remoteId, JSONObject message) {
        socket.emit("message", roomId, remoteId, message);
    }
}
