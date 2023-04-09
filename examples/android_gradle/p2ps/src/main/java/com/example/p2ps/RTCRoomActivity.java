package com.example.p2ps;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.GridLayout;
import android.widget.Toast;

import com.rtc.DataChannelParameters;
import com.rtc.PeerConnectionParameters;
import com.rtc.RoomConnectionParameters;
import com.rtc.room.IRoomEventListener;
import com.rtc.room.RoomManager;

import org.webrtc.Camera2Enumerator;
import org.webrtc.CameraEnumerator;
import org.webrtc.Logging;
import org.webrtc.RendererCommon;
import org.webrtc.SurfaceViewRenderer;
import org.webrtc.VideoCapturer;
import org.webrtc.VideoSink;
import org.webrtc.VideoTrack;

import java.util.HashMap;
import java.util.Map;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/4/9
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
public class RTCRoomActivity extends AppCompatActivity implements IRoomEventListener {
    private String TAG = this.getClass().getSimpleName();
    private GridLayout mGridLayout;
    private int mScreenWidth, mScreenHeight;
    private RoomManager roomManager;
    private Map<String, SurfaceViewRenderer> videoRenderers = new HashMap<>();
    private SurfaceViewRenderer localVideoSink;

    private boolean enableVideoCall = true,enableAudioCall = true;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 隐藏状态栏和导航栏
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_p2ps);
        mGridLayout = findViewById(R.id.gridLayout);
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        mScreenWidth = displayMetrics.widthPixels;
        mScreenHeight = displayMetrics.heightPixels;

        String signalingServerUrl = getIntent().getStringExtra(MainActivity.CONFIG_ROOM_URL);
        String roomName = getIntent().getStringExtra(MainActivity.CONFIG_ROOM_ID);
        if (!TextUtils.isEmpty(signalingServerUrl) && !TextUtils.isEmpty(roomName)) {
            roomManager = new RoomManager();
            roomManager.setRoomEventListener(this);
            PeerConnectionParameters peerConnectionParameters = getPeerConnectionParameters();
            if (enableVideoCall){
                localVideoSink = createLocalVideoSink();
                addSurfaceView(localVideoSink);
            }

            Log.i("joinRoom", "PeerConnectionParameters:" + peerConnectionParameters.toString());
            roomManager.joinRoom(this, localVideoSink, createVideoCapture(), peerConnectionParameters, new RoomConnectionParameters(signalingServerUrl, roomName, false));
        } else {
            Toast.makeText(this, "请检查传递的参数是否正确.", Toast.LENGTH_SHORT).show();
        }
    }


    private void addSurfaceView(SurfaceViewRenderer localVideoRenderer) {
        mGridLayout.addView(localVideoRenderer);
        localVideoRenderer.getLayoutParams().width = mScreenWidth / 3;
        localVideoRenderer.getLayoutParams().height = 600;
        GridLayout.LayoutParams layoutParams = (GridLayout.LayoutParams) localVideoRenderer.getLayoutParams();
//        layoutParams.rightMargin = 30;
//        layoutParams.topMargin = 10;
    }

    private SurfaceViewRenderer createVideoRenderer() {
        SurfaceViewRenderer renderer = new SurfaceViewRenderer(this);
        //设置本地预览窗口
        renderer.init(roomManager.getEglBaseContext(), null);
        renderer.setScalingType(RendererCommon.ScalingType.SCALE_ASPECT_FILL);
        renderer.setMirror(true);
        renderer.setEnableHardwareScaler(false /* enabled */);
        return renderer;
    }


    private SurfaceViewRenderer createLocalVideoSink() {
        return createVideoRenderer();
    }

    public PeerConnectionParameters getPeerConnectionParameters() {
        DataChannelParameters dataChannelParameters = new DataChannelParameters(true,
                -1,
                -1, "",
                false, -1);
        PeerConnectionParameters peerConnectionParameters =
                new PeerConnectionParameters(
                        enableVideoCall, enableAudioCall
                        , false
                        , false
                        , mScreenWidth
                        , mScreenHeight
                        , 30
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
        return peerConnectionParameters;
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
    public void OnFailed(String error) {

    }

    @Override
    public void removeRemoteStream(String remoteId) {
        runOnUiThread(() -> {
            SurfaceViewRenderer renderer = videoRenderers.get(remoteId);
            if (renderer != null) {
                mGridLayout.removeView(renderer);
                renderer.release();
                renderer = null;
            }
        });
    }

    @Override
    public void addRemoteStream(VideoTrack track, String remoteId) {
        runOnUiThread(() -> {
            SurfaceViewRenderer videoRenderer = createVideoRenderer();
            addSurfaceView(videoRenderer);
            track.addSink(videoRenderer);
            videoRenderers.put(remoteId, videoRenderer);
        });
    }

    @Override
    public void OnConnectError(String error) {

    }

    @Override
    protected void onStart() {
        super.onStart();
        roomManager.resume();
    }

    @Override
    protected void onStop() {
        super.onStop();
        roomManager.pause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        roomManager.release();
        for (String key : videoRenderers.keySet()) {
            removeRemoteStream(key);
        }
        videoRenderers.clear();
        if (localVideoSink != null) {
            localVideoSink.release();
        }
    }
}
