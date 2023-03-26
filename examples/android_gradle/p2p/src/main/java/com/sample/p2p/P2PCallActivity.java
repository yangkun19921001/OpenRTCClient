package com.sample.p2p;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;

import org.json.JSONException;
import org.json.JSONObject;
import org.webrtc.Camera2Enumerator;
import org.webrtc.CameraEnumerator;
import org.webrtc.IceCandidate;
import org.webrtc.Logging;
import org.webrtc.RTCStatsReport;
import org.webrtc.SessionDescription;
import org.webrtc.SurfaceViewRenderer;
import org.webrtc.VideoCapturer;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/3/17
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
public class P2PCallActivity extends AppCompatActivity implements ISignalEventListener {

    private P2PConnection mP2PConnection;
    private String TAG = this.getClass().getSimpleName();
    private long callStartedTimeMs;
    private SurfaceViewRenderer localRender;
    private SurfaceViewRenderer remoteRender;

    @TargetApi(19)
    private static int getSystemUiVisibility() {
        int flags = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_FULLSCREEN;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            flags |= View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
        }
        return flags;
    }

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
                | WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED | WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);
        getWindow().getDecorView().setSystemUiVisibility(getSystemUiVisibility());
        setContentView(R.layout.activity_p2p_call);

        localRender = findViewById(R.id.local);
        remoteRender = findViewById(R.id.remote);

        if (getIntent() != null) {
            String roomUrl = getIntent().getStringExtra(MainActivity.CONFIG_ROOM_URL);
            String roomId = getIntent().getStringExtra(MainActivity.CONFIG_ROOM_ID);

            mP2PConnection = new P2PConnection();
            DataChannelParameters dataChannelParameters = null;
            dataChannelParameters = new DataChannelParameters(true,
                    -1,
                    -1, "",
                    false, -1);
            PeerConnectionParameters peerConnectionParameters =
                    new PeerConnectionParameters(
                            true
                            , false
                            , false
                            , 0
                            , 0
                            , 0
                            , 0
                            , "H264 Baseline"
                            , true, false, 0
                            , "OPUS"
                            , false
                            , false
                            , false
                            , false
                            , false
                            , false
                            , false
                            , false
                            , false
                            , dataChannelParameters);
            ISignalClient iSignalClient = new SocketIOClientImpl();
            mP2PConnection.init(this, iSignalClient, null, peerConnectionParameters, localRender, remoteRender, createVideoCapture());
            callStartedTimeMs = System.currentTimeMillis();
            Log.i(Constants.P2PTAG, "connectToRoom");
            mP2PConnection.connectToRoom(new RoomConnectionParameters(roomUrl, roomId, false), this);
        }
    }

    private VideoCapturer createVideoCapture() {
        final VideoCapturer videoCapturer;
        videoCapturer = createCameraCapturer(new Camera2Enumerator(this));
        return videoCapturer;
    }

    private @Nullable
    VideoCapturer createCameraCapturer(CameraEnumerator enumerator) {
        final String[] deviceNames = enumerator.getDeviceNames();

        // First, try to find front facing camera
        Logging.d(TAG, "Looking for front facing cameras.");
        for (String deviceName : deviceNames) {
            if (enumerator.isFrontFacing(deviceName)) {
                Logging.d(TAG, "Creating front facing camera capturer.");
                VideoCapturer videoCapturer = enumerator.createCapturer(deviceName, null);

                if (videoCapturer != null) {
                    return videoCapturer;
                }
            }
        }

        // Front facing camera not found, try something else
        Logging.d(TAG, "Looking for other cameras.");
        for (String deviceName : deviceNames) {
            if (!enumerator.isFrontFacing(deviceName)) {
                Logging.d(TAG, "Creating other camera capturer.");
                VideoCapturer videoCapturer = enumerator.createCapturer(deviceName, null);

                if (videoCapturer != null) {
                    return videoCapturer;
                }
            }
        }

        return null;
    }


    @Override
    public void OnConnecting() {

    }

    @Override
    public void OnConnected() {

    }

    @Override
    public void OnDisconnected() {

    }

    @Override
    public void OnUserJoined(String roomName, String userId, boolean isInitiator) {

    }

    @Override
    public void OnUserLeaved(String roomName, String userId) {

    }

    @Override
    public void OnRemoteUserJoined(String roomName, String userId) {
    runOnUiThread(() -> remoteRender.setVisibility(View.VISIBLE));
    }

    @Override
    public void OnRemoteUserLeaved(String roomName, String userId) {
        runOnUiThread(() -> remoteRender.setVisibility(View.GONE));
    }

    @Override
    public void OnRoomFull(String roomName, String userId) {
        Toast.makeText(this, "RoomFull:"+roomName, Toast.LENGTH_SHORT).show();
    }

    @Override
    public void OnMessage(JSONObject message) {
       try {
           Log.i(Constants.P2PTAG, "onMessage: " + message.getString("type"));
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void onDestroy() {
        mP2PConnection.close();
        super.onDestroy();
    }
}
