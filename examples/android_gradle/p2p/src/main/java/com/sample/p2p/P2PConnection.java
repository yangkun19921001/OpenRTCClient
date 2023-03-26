package com.sample.p2p;

import android.content.Context;
import android.os.Environment;
import android.support.annotation.Nullable;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;
import org.webrtc.AddIceObserver;
import org.webrtc.AudioSource;
import org.webrtc.AudioTrack;
import org.webrtc.CandidatePairChangeEvent;
import org.webrtc.DataChannel;
import org.webrtc.DefaultVideoDecoderFactory;
import org.webrtc.DefaultVideoEncoderFactory;
import org.webrtc.EglBase;
import org.webrtc.IceCandidate;
import org.webrtc.Logging;
import org.webrtc.MediaConstraints;
import org.webrtc.MediaStream;
import org.webrtc.MediaStreamTrack;
import org.webrtc.PeerConnection;
import org.webrtc.PeerConnectionFactory;
import org.webrtc.RendererCommon;
import org.webrtc.RtpReceiver;
import org.webrtc.RtpSender;
import org.webrtc.RtpTransceiver;
import org.webrtc.SdpObserver;
import org.webrtc.SessionDescription;
import org.webrtc.SoftwareVideoDecoderFactory;
import org.webrtc.SoftwareVideoEncoderFactory;
import org.webrtc.SurfaceTextureHelper;
import org.webrtc.SurfaceViewRenderer;
import org.webrtc.VideoCapturer;
import org.webrtc.VideoDecoderFactory;
import org.webrtc.VideoEncoderFactory;
import org.webrtc.VideoSource;
import org.webrtc.VideoTrack;
import org.webrtc.audio.AudioDeviceModule;
import org.webrtc.audio.JavaAudioDeviceModule;

import java.io.File;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * 基于 PeerConnectionClient 简单封装
 * <pre>
 *     author  : 马克
 *     time    : 2023/3/18
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
class P2PConnection {

    private final EglBase mRootEglBase = EglBase.create();
    private Context mContext;
    private SurfaceViewRenderer mLocalSurfaceView;
    private SurfaceViewRenderer mRemoteSurfaceView;
    private ISignalClient mISignalClient;
    private PeerConnectionParameters mPeerConnectionParameters;
    @Nullable
    private PeerConnectionFactory mPeerConnectionFactory;
    private String TAG = this.getClass().getSimpleName();

    private static final int HD_VIDEO_WIDTH = 1280;
    private static final int HD_VIDEO_HEIGHT = 720;
    public static final String VIDEO_TRACK_ID = "ARDAMSv0";
    public static final String AUDIO_TRACK_ID = "ARDAMSa0";
    public static final String VIDEO_TRACK_TYPE = "video";
    private static final String VIDEO_CODEC_VP8 = "VP8";
    private static final String VIDEO_CODEC_VP9 = "VP9";
    private static final String VIDEO_CODEC_H264 = "H264";
    private static final String VIDEO_CODEC_H264_BASELINE = "H264 Baseline";
    private static final String VIDEO_CODEC_H264_HIGH = "H264 High";
    private static final String VIDEO_CODEC_AV1 = "AV1";
    private static final String AUDIO_CODEC_OPUS = "opus";
    private static final String AUDIO_CODEC_ISAC = "ISAC";
    private final String VIDEO_FLEXFEC_FIELDTRIAL =
            "WebRTC-FlexFEC-03-Advertised/Enabled/WebRTC-FlexFEC-03/Enabled/";
    private final String VIDEO_VP8_INTEL_HW_ENCODER_FIELDTRIAL = "WebRTC-IntelVP8/Enabled/";
    private final String DISABLE_WEBRTC_AGC_FIELDTRIAL =
            "WebRTC-Audio-MinimizeResamplingOnMobile/Enabled/";
    private static final String VIDEO_CODEC_PARAM_START_BITRATE = "x-google-start-bitrate";

    private static final String AUDIO_CODEC_PARAM_BITRATE = "maxaveragebitrate";
    private static final String AUDIO_ECHO_CANCELLATION_CONSTRAINT = "googEchoCancellation";
    private static final String AUDIO_AUTO_GAIN_CONTROL_CONSTRAINT = "googAutoGainControl";
    private static final String AUDIO_HIGH_PASS_FILTER_CONSTRAINT = "googHighpassFilter";
    private static final String AUDIO_NOISE_SUPPRESSION_CONSTRAINT = "googNoiseSuppression";

    // Executor thread is started once in private ctor and is used for all
    // peer connection API calls to ensure new peer connection factory is
    // created on the same thread as previously destroyed factory.
    private static final ExecutorService executor = Executors.newSingleThreadExecutor();

    // Implements the WebRtcAudioRecordSamplesReadyCallback interface and writes
    // recorded audio samples to an output file.
    @Nullable
    private RecordedAudioToFileController mSaveRecordedAudioToFile;
    private RoomConnectionParameters mRoomConnectParameters;
    private VideoCapturer mVideoCapture;
    private MediaConstraints mAudioConstraints;
    private MediaConstraints mSdpMediaConstraints;
    private PeerConnection mPeerConnection;
    private boolean dataChannelEnabled;
    private DataChannel mDataChannel;
    private boolean isInitiator;
    long callStartedTimeMs = 0;
    @Nullable
    private SessionDescription localDescription; // either offer or answer description

    private SurfaceTextureHelper surfaceTextureHelper;
    private VideoSource videoSource;
    private int videoWidth;
    private int videoHeight;
    private int videoFps;
    private VideoTrack localVideoTrack;
    private boolean renderVideo = true;
    private boolean enableAudio = true;
    private VideoTrack remoteVideoTrack;
    private AudioSource audioSource;
    private AudioTrack localAudioTrack;
    private RtpSender localVideoSender;
    private final PCObserver pcObserver = new PCObserver();
    private final SDPObserver sdpObserver = new SDPObserver();
    private PeerConnectionEvents events;
    private ArrayList<IceCandidate> queuedRemoteCandidates;

    public void init(Context context, ISignalClient signalClient, PeerConnectionEvents events, PeerConnectionParameters peerConnectionParameters, SurfaceViewRenderer local, SurfaceViewRenderer remote, VideoCapturer videoCapture) {
        mContext = context;
        mLocalSurfaceView = local;
        mRemoteSurfaceView = remote;
        mISignalClient = signalClient;
        mVideoCapture = videoCapture;
        this.events = events;
        mPeerConnectionParameters = peerConnectionParameters;
        this.dataChannelEnabled = peerConnectionParameters.dataChannelParameters != null;

        //设置本地预览窗口
        mLocalSurfaceView.init(mRootEglBase.getEglBaseContext(), null);
        mLocalSurfaceView.setScalingType(RendererCommon.ScalingType.SCALE_ASPECT_FILL);
        mLocalSurfaceView.setMirror(true);
        mLocalSurfaceView.setEnableHardwareScaler(false /* enabled */);

        //设置远端预览窗口
        mRemoteSurfaceView.init(mRootEglBase.getEglBaseContext(), null);
        mRemoteSurfaceView.setScalingType(RendererCommon.ScalingType.SCALE_ASPECT_FILL);
        mRemoteSurfaceView.setMirror(true);
        mRemoteSurfaceView.setEnableHardwareScaler(true /* enabled */);
        mRemoteSurfaceView.setZOrderMediaOverlay(true);
        callStartedTimeMs = System.currentTimeMillis();
        //创建 factory， pc是从factory里获得的
        createPeerConnectionFactory();

        executor.execute(() ->         // NOTE: this _must_ happen while PeerConnectionFactory is alive!
                Logging.enableLogToDebugOutput(Logging.Severity.LS_VERBOSE));

    }

    public void sendMessage(JSONObject message) {
        mISignalClient.sendSignalMessage(mRoomConnectParameters.roomId, message);
    }

    private void createPeerConnectionFactory() {
        final String fieldTrials = getFieldTrials(mPeerConnectionParameters);
        executor.execute(() -> {
            Log.d(Constants.P2PTAG, "Initialize WebRTC. Field trials: " + fieldTrials);
            PeerConnectionFactory.initialize(
                    PeerConnectionFactory.InitializationOptions.builder(mContext)
                            .setFieldTrials(fieldTrials)
                            .setEnableInternalTracer(true)
                            .createInitializationOptions());
        });
        executor.execute(() -> {
            createPeerConnectionFactoryInternal();
        });
    }


    public void connectToRoom(RoomConnectionParameters parameters, ISignalEventListener signalEventListener) {
        mRoomConnectParameters = parameters;
        executor.execute(() -> {
            if (mISignalClient != null) {
                try {
                    mISignalClient.connect(parameters.roomUrl, new ISignalEventListener() {
                        @Override
                        public void OnConnecting() {
                            Log.i(Constants.P2PTAG, "OnConnecting");
                            if (signalEventListener != null) {
                                signalEventListener.OnConnecting();
                            }
                        }

                        @Override
                        public void OnConnected() {
                            Log.i(Constants.P2PTAG, "OnConnected");
                            Log.i(Constants.P2PTAG, "join:" + parameters.roomId);
                            mISignalClient.join(parameters.roomId);
                            if (signalEventListener != null) {
                                signalEventListener.OnConnected();
                            }
                        }

                        @Override
                        public void OnDisconnected() {
                            if (signalEventListener != null) {
                                signalEventListener.OnConnecting();
                            }
                        }

                        @Override
                        public void OnUserJoined(String roomName, String userId, boolean isInitiator) {
                            if (signalEventListener != null) {
                                signalEventListener.OnUserJoined(roomName, userId, isInitiator);
                            }
                            Log.i(Constants.P2PTAG, "joined:" + roomName + "-" + userId + "-" + isInitiator);
                            Log.i(Constants.P2PTAG, "createPeerConnection");
                            createPeerConnection();

//                            Log.i(TAG, "Creating OFFER...");
                            // Create offer. Offer SDP will be sent to answering client in
                            // PeerConnectionEvents.onLocalDescription event.
                            //这里如果其它人还没进来有可能导致流失数据
//                            if (isInitiator)
//                                createOffer();
                        }

                        @Override
                        public void OnUserLeaved(String roomName, String userId) {
                            if (signalEventListener != null) {
                                signalEventListener.OnUserLeaved(roomName, userId);
                            }
                        }

                        @Override
                        public void OnRemoteUserJoined(String roomName, String userId) {
                            Log.i(Constants.P2PTAG, "createOffer " + roomName + "-" + userId);
                            createOffer();
                            if (signalEventListener != null) {
                                signalEventListener.OnRemoteUserJoined(roomName, userId);
                            }
                        }

                        @Override
                        public void OnRemoteUserLeaved(String roomName, String userId) {
                            if (signalEventListener != null) {
                                signalEventListener.OnRemoteUserLeaved(roomName, userId);
                            }
                        }

                        @Override
                        public void OnRoomFull(String roomName, String userId) {
                            if (signalEventListener != null) {
                                signalEventListener.OnRoomFull(roomName, userId);
                            }
                        }

                        @Override
                        public void OnMessage(JSONObject message) {
                            HandleMessage(message);

                        }
                    });

                } catch (Exception e) {
                    Log.e(TAG, e.getMessage());
                }
            }
        });
    }

    private void HandleMessage(JSONObject message) {
        try {
            Log.i(Constants.P2PTAG, "onMessage: " + message.getString("type"));
            String type = message.getString("type");
            if (type.equals("offer") || type.equals("answer")) {
                onRemoteDescription(new SessionDescription(
                        SessionDescription.Type.fromCanonicalForm(type), message.getString("sdp")))
                ;
            } else if (type.equals("candidate")) {
                onRemoteCandidateReceived(message);
            } else {
                Log.w(TAG, "the type is invalid: " + type);
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }


    private void onRemoteCandidateReceived(JSONObject json) {
        try {
            IceCandidate iceCandidate = new IceCandidate(
                    json.getString("id"), json.getInt("label"), json.getString("candidate"));
            Log.i(Constants.P2PTAG, "onRemoteCandidateReceived:" + json.getString("candidate"));
            addRemoteIceCandidate(iceCandidate);
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    private void onRemoteDescription(SessionDescription desc) {
        final long delta = System.currentTimeMillis() - callStartedTimeMs;
        Log.i(Constants.P2PTAG, "Received remote " + desc.type + ", delay=" + delta + "ms");
        setRemoteDescription(desc);
        if (desc.type == SessionDescription.Type.OFFER) {
            Log.i(Constants.P2PTAG, "Creating ANSWER...");
            createAnswer();
        }
    }

    //当连接成功并且进入到房间中执行
    private void createPeerConnection() {
        executor.execute(() -> {
            try {
                createMediaConstraintsInternal();
                createPeerConnectionInternal();
                Log.i(Constants.P2PTAG, "createPeerConnection Succeed");
            } catch (Exception e) {
                Log.e(TAG, "Failed to create peer connection: " + e.getMessage());
                throw e;
            }
        });
    }

    public void createOffer() {
        executor.execute(() -> {
            if (mPeerConnection != null) {
                Log.d(Constants.P2PTAG, "PC Create OFFER");
                isInitiator = true;
                mPeerConnection.createOffer(sdpObserver, mSdpMediaConstraints);
            }
        });
    }

    public void createAnswer() {
        executor.execute(() -> {
            if (mPeerConnection != null) {
                Log.d(Constants.P2PTAG, "PC create ANSWER");
                isInitiator = false;
                mPeerConnection.createAnswer(sdpObserver, mSdpMediaConstraints);
            }
        });
    }

    public void addRemoteIceCandidate(final IceCandidate candidate) {
        executor.execute(() -> {
            if (mPeerConnection != null) {
                if (queuedRemoteCandidates != null) {
                    queuedRemoteCandidates.add(candidate);
                    Log.i(Constants.P2PTAG, "addRemoteIceCandidate-enqueue");
                } else {
                    mPeerConnection.addIceCandidate(candidate, new AddIceObserver() {
                        @Override
                        public void onAddSuccess() {
                            Log.d(Constants.P2PTAG, "Candidate " + candidate.serverUrl + " successfully added.");
                        }

                        @Override
                        public void onAddFailure(String error) {
                            Log.d(Constants.P2PTAG, "Candidate " + candidate.serverUrl + " addition failed: " + error);
                        }
                    });
                }
            }
        });
    }

    public void removeRemoteIceCandidates(final IceCandidate[] candidates) {
        executor.execute(() -> {
            if (mPeerConnection == null) {
                return;
            }
            // Drain the queued remote candidates if there is any so that
            // they are processed in the proper order.
            drainCandidates();
            mPeerConnection.removeIceCandidates(candidates);
        });
    }

    public void setLocalDescription(final SessionDescription desc) {
        if (localDescription != null) {
            Log.e(Constants.P2PTAG, "Multiple SDP create.");
            return;
        }
        String sdp = desc.description;
        sdp = preferCodec(sdp, AUDIO_CODEC_ISAC, true);
        if (isVideoCallEnabled()) {
            sdp = preferCodec(sdp, getSdpVideoCodecName(mPeerConnectionParameters), false);
        }
        final SessionDescription newDesc = new SessionDescription(desc.type, sdp);
        localDescription = newDesc;
        executor.execute(() -> {
            if (mPeerConnection != null) {
                Log.d(Constants.P2PTAG, "Set local SDP from " + desc.type);
                mPeerConnection.setLocalDescription(sdpObserver, newDesc);
            }
        });
    }

    public void setRemoteDescription(SessionDescription desc) {
        executor.execute(() -> {
            if (mPeerConnection == null) {
                return;
            }
            String sdp = desc.description;
            sdp = preferCodec(sdp, AUDIO_CODEC_ISAC, true);
            if (isVideoCallEnabled()) {
                sdp = preferCodec(sdp, getSdpVideoCodecName(mPeerConnectionParameters), false);
            }
            if (mPeerConnectionParameters.audioStartBitrate > 0) {
                sdp = setStartBitrate(
                        AUDIO_CODEC_OPUS, false, sdp, mPeerConnectionParameters.audioStartBitrate);
            }
            Log.d(TAG, "Set remote SDP.");
            SessionDescription sdpRemote = new SessionDescription(desc.type, sdp);
            mPeerConnection.setRemoteDescription(sdpObserver, sdpRemote);
        });
    }

    @SuppressWarnings("StringSplitter")
    private String setStartBitrate(
            String codec, boolean isVideoCodec, String sdp, int bitrateKbps) {
        String[] lines = sdp.split("\r\n");
        int rtpmapLineIndex = -1;
        boolean sdpFormatUpdated = false;
        String codecRtpMap = null;
        // Search for codec rtpmap in format
        // a=rtpmap:<payload type> <encoding name>/<clock rate> [/<encoding parameters>]
        String regex = "^a=rtpmap:(\\d+) " + codec + "(/\\d+)+[\r]?$";
        Pattern codecPattern = Pattern.compile(regex);
        for (int i = 0; i < lines.length; i++) {
            Matcher codecMatcher = codecPattern.matcher(lines[i]);
            if (codecMatcher.matches()) {
                codecRtpMap = codecMatcher.group(1);
                rtpmapLineIndex = i;
                break;
            }
        }
        if (codecRtpMap == null) {
            Log.w(TAG, "No rtpmap for " + codec + " codec");
            return sdp;
        }
        Log.d(TAG, "Found " + codec + " rtpmap " + codecRtpMap + " at " + lines[rtpmapLineIndex]);

        // Check if a=fmtp string already exist in remote SDP for this codec and
        // update it with new bitrate parameter.
        regex = "^a=fmtp:" + codecRtpMap + " \\w+=\\d+.*[\r]?$";
        codecPattern = Pattern.compile(regex);
        for (int i = 0; i < lines.length; i++) {
            Matcher codecMatcher = codecPattern.matcher(lines[i]);
            if (codecMatcher.matches()) {
                Log.d(TAG, "Found " + codec + " " + lines[i]);
                if (isVideoCodec) {
                    lines[i] += "; " + VIDEO_CODEC_PARAM_START_BITRATE + "=" + bitrateKbps;
                } else {
                    lines[i] += "; " + AUDIO_CODEC_PARAM_BITRATE + "=" + (bitrateKbps * 1000);
                }
                Log.d(TAG, "Update remote SDP line: " + lines[i]);
                sdpFormatUpdated = true;
                break;
            }
        }

        StringBuilder newSdpDescription = new StringBuilder();
        for (int i = 0; i < lines.length; i++) {
            newSdpDescription.append(lines[i]).append("\r\n");
            // Append new a=fmtp line if no such line exist for a codec.
            if (!sdpFormatUpdated && i == rtpmapLineIndex) {
                String bitrateSet;
                if (isVideoCodec) {
                    bitrateSet =
                            "a=fmtp:" + codecRtpMap + " " + VIDEO_CODEC_PARAM_START_BITRATE + "=" + bitrateKbps;
                } else {
                    bitrateSet = "a=fmtp:" + codecRtpMap + " " + AUDIO_CODEC_PARAM_BITRATE + "="
                            + (bitrateKbps * 1000);
                }
                Log.d(TAG, "Add remote SDP line: " + bitrateSet);
                newSdpDescription.append(bitrateSet).append("\r\n");
            }
        }
        return newSdpDescription.toString();
    }

    private boolean isVideoCallEnabled() {
        return mPeerConnectionParameters.videoCallEnabled && mVideoCapture != null;
    }

    private void createPeerConnectionInternal() {
        if (mPeerConnectionFactory == null) {
            Log.e(TAG, "Peerconnection factory is not created");
            return;
        }
        Log.d(TAG, "Create peer connection.");
        queuedRemoteCandidates = new ArrayList<>();
        List<PeerConnection.IceServer> iceServers = new ArrayList<>();

        iceServers.add(PeerConnection.IceServer
                .builder("turn:rtcmedia.top:3478")
                .setPassword("devyk")
                .setUsername("devyk")
                .createIceServer());
        PeerConnection.RTCConfiguration rtcConfig =
                new PeerConnection.RTCConfiguration(iceServers);
        // TCP candidates are only useful when connecting to a server that supports
        // ICE-TCP.
        rtcConfig.tcpCandidatePolicy = PeerConnection.TcpCandidatePolicy.DISABLED;
        rtcConfig.bundlePolicy = PeerConnection.BundlePolicy.MAXBUNDLE;
        rtcConfig.rtcpMuxPolicy = PeerConnection.RtcpMuxPolicy.REQUIRE;
        rtcConfig.continualGatheringPolicy = PeerConnection.ContinualGatheringPolicy.GATHER_CONTINUALLY;
        // Use ECDSA encryption.
        rtcConfig.keyType = PeerConnection.KeyType.ECDSA;
        rtcConfig.sdpSemantics = PeerConnection.SdpSemantics.UNIFIED_PLAN;
        mPeerConnection = mPeerConnectionFactory.createPeerConnection(rtcConfig, pcObserver);

        if (dataChannelEnabled) {
            DataChannel.Init init = new DataChannel.Init();
            init.ordered = mPeerConnectionParameters.dataChannelParameters.ordered;
            init.negotiated = mPeerConnectionParameters.dataChannelParameters.negotiated;
            init.maxRetransmits = mPeerConnectionParameters.dataChannelParameters.maxRetransmits;
            init.maxRetransmitTimeMs = mPeerConnectionParameters.dataChannelParameters.maxRetransmitTimeMs;
            init.id = mPeerConnectionParameters.dataChannelParameters.id;
            init.protocol = mPeerConnectionParameters.dataChannelParameters.protocol;
            mDataChannel = mPeerConnection.createDataChannel("P2P data", init);
        }
        isInitiator = false;
        // Set INFO libjingle logging.
        // NOTE: this _must_ happen while `factory` is alive!
        Logging.enableLogToDebugOutput(Logging.Severity.LS_INFO);
        List<String> mediaStreamLabels = Collections.singletonList("ARDAMS");
        if (isVideoCallEnabled()) {
            mPeerConnection.addTrack(createVideoTrack(mVideoCapture), mediaStreamLabels);
            // We can add the renderers right away because we don't need to wait for an
            // answer to get the remote track.
            remoteVideoTrack = getRemoteVideoTrack();
            remoteVideoTrack.setEnabled(renderVideo);
            //目前就一个
            remoteVideoTrack.addSink(mRemoteSurfaceView);
        }

        mPeerConnection.addTrack(createAudioTrack(), mediaStreamLabels);
        if (isVideoCallEnabled()) {
            findVideoSender();
        }
    }

    private void findVideoSender() {
        for (RtpSender sender : mPeerConnection.getSenders()) {
            if (sender.track() != null) {
                String trackType = sender.track().kind();
                if (trackType.equals(VIDEO_TRACK_TYPE)) {
                    Log.d(TAG, "Found video sender.");
                    localVideoSender = sender;
                }
            }
        }
    }

    private void createMediaConstraintsInternal() {
        // Create video constraints if video call is enabled.
        if (isVideoCallEnabled()) {
            videoWidth = mPeerConnectionParameters.videoWidth;
            videoHeight = mPeerConnectionParameters.videoHeight;
            videoFps = mPeerConnectionParameters.videoFps;

            // If video resolution is not specified, default to HD.
            if (videoWidth == 0 || videoHeight == 0) {
                videoWidth = HD_VIDEO_WIDTH;
                videoHeight = HD_VIDEO_HEIGHT;
            }

            // If fps is not specified, default to 30.
            if (videoFps == 0) {
                videoFps = 30;
            }
            Logging.d(TAG, "Capturing format: " + videoWidth + "x" + videoHeight + "@" + videoFps);
        }
        // Create audio constraints.
        mAudioConstraints = new MediaConstraints();
        // added for audio performance measurements
        if (mPeerConnectionParameters.noAudioProcessing) {
            Log.d(TAG, "Disabling audio processing");
            mAudioConstraints.mandatory.add(
                    new MediaConstraints.KeyValuePair(AUDIO_ECHO_CANCELLATION_CONSTRAINT, "false"));
            mAudioConstraints.mandatory.add(
                    new MediaConstraints.KeyValuePair(AUDIO_AUTO_GAIN_CONTROL_CONSTRAINT, "false"));
            mAudioConstraints.mandatory.add(
                    new MediaConstraints.KeyValuePair(AUDIO_HIGH_PASS_FILTER_CONSTRAINT, "false"));
            mAudioConstraints.mandatory.add(
                    new MediaConstraints.KeyValuePair(AUDIO_NOISE_SUPPRESSION_CONSTRAINT, "false"));
        }
        // Create SDP constraints.
        mSdpMediaConstraints = new MediaConstraints();
        mSdpMediaConstraints.mandatory.add(
                new MediaConstraints.KeyValuePair("OfferToReceiveAudio", "true"));
        mSdpMediaConstraints.mandatory.add(new MediaConstraints.KeyValuePair(
                "OfferToReceiveVideo", Boolean.toString(isVideoCallEnabled())));

    }

    private void createPeerConnectionFactoryInternal() {
        if (mPeerConnectionFactory != null) {
            throw new IllegalStateException("PeerConnectionFactory has already been constructed");
        }

        if (mPeerConnectionParameters.tracing) {
            PeerConnectionFactory.startInternalTracingCapture(
                    Environment.getExternalStorageDirectory().getAbsolutePath() + File.separator
                            + "webrtc-trace.txt");
        }

        final VideoEncoderFactory encoderFactory;
        final VideoDecoderFactory decoderFactory;

        if (mPeerConnectionParameters.videoCodecHwAcceleration) {
            encoderFactory = new DefaultVideoEncoderFactory(
                    mRootEglBase.getEglBaseContext()
                    , true /* enableIntelVp8Encoder */
                    , true);
            decoderFactory = new DefaultVideoDecoderFactory(mRootEglBase.getEglBaseContext());
        } else {
            encoderFactory = new SoftwareVideoEncoderFactory();
            decoderFactory = new SoftwareVideoDecoderFactory();
        }
        PeerConnectionFactory.Options options = new PeerConnectionFactory.Options();
        if (mPeerConnectionParameters.loopback) {
            options.networkIgnoreMask = 0;
            options.disableEncryption = true;
        }
        final AudioDeviceModule adm = createJavaAudioDevice();
        mPeerConnectionFactory = PeerConnectionFactory.builder()
                .setOptions(options)
                .setAudioDeviceModule(adm)
                .setVideoEncoderFactory(encoderFactory)
                .setVideoDecoderFactory(decoderFactory)
                .createPeerConnectionFactory();
        Log.d(TAG, "Peer connection factory created.");
        adm.release();
    }

    private AudioDeviceModule createJavaAudioDevice() {
        // Enable/disable OpenSL ES playback.
        if (!mPeerConnectionParameters.useOpenSLES) {
            Log.w(TAG, "External OpenSLES ADM not implemented yet.");
            // TODO(magjed): Add support for external OpenSLES ADM.
        }


        // Set audio record error callbacks.
        JavaAudioDeviceModule.AudioRecordErrorCallback audioRecordErrorCallback = new JavaAudioDeviceModule.AudioRecordErrorCallback() {
            @Override
            public void onWebRtcAudioRecordInitError(String errorMessage) {
                Log.e(TAG, "onWebRtcAudioRecordInitError: " + errorMessage);
            }

            @Override
            public void onWebRtcAudioRecordStartError(
                    JavaAudioDeviceModule.AudioRecordStartErrorCode errorCode, String errorMessage) {
                Log.e(TAG, "onWebRtcAudioRecordStartError: " + errorCode + ". " + errorMessage);
            }

            @Override
            public void onWebRtcAudioRecordError(String errorMessage) {
                Log.e(TAG, "onWebRtcAudioRecordError: " + errorMessage);
            }
        };

        JavaAudioDeviceModule.AudioTrackErrorCallback audioTrackErrorCallback = new JavaAudioDeviceModule.AudioTrackErrorCallback() {
            @Override
            public void onWebRtcAudioTrackInitError(String errorMessage) {
                Log.e(TAG, "onWebRtcAudioTrackInitError: " + errorMessage);
            }

            @Override
            public void onWebRtcAudioTrackStartError(
                    JavaAudioDeviceModule.AudioTrackStartErrorCode errorCode, String errorMessage) {
                Log.e(TAG, "onWebRtcAudioTrackStartError: " + errorCode + ". " + errorMessage);
            }

            @Override
            public void onWebRtcAudioTrackError(String errorMessage) {
                Log.e(TAG, "onWebRtcAudioTrackError: " + errorMessage);
            }
        };

        // Set audio record state callbacks.
        JavaAudioDeviceModule.AudioRecordStateCallback audioRecordStateCallback = new JavaAudioDeviceModule.AudioRecordStateCallback() {
            @Override
            public void onWebRtcAudioRecordStart() {
                Log.i(TAG, "Audio recording starts");
            }

            @Override
            public void onWebRtcAudioRecordStop() {
                Log.i(TAG, "Audio recording stops");
            }
        };

        // Set audio track state callbacks.
        JavaAudioDeviceModule.AudioTrackStateCallback audioTrackStateCallback = new JavaAudioDeviceModule.AudioTrackStateCallback() {
            @Override
            public void onWebRtcAudioTrackStart() {
                Log.i(TAG, "Audio playout starts");
            }

            @Override
            public void onWebRtcAudioTrackStop() {
                Log.i(TAG, "Audio playout stops");
            }
        };

        return JavaAudioDeviceModule.builder(mContext)
                .setSamplesReadyCallback(mSaveRecordedAudioToFile)
                .setUseHardwareAcousticEchoCanceler(!mPeerConnectionParameters.disableBuiltInAEC)
                .setUseHardwareNoiseSuppressor(!mPeerConnectionParameters.disableBuiltInNS)
                .setAudioRecordErrorCallback(audioRecordErrorCallback)
                .setAudioTrackErrorCallback(audioTrackErrorCallback)
                .setAudioRecordStateCallback(audioRecordStateCallback)
                .setAudioTrackStateCallback(audioTrackStateCallback)
                .createAudioDeviceModule();
    }

    private String getFieldTrials(PeerConnectionParameters peerConnectionParameters) {
        String fieldTrials = "";
        if (peerConnectionParameters.videoFlexfecEnabled) {
            fieldTrials += VIDEO_FLEXFEC_FIELDTRIAL;
            Log.d(TAG, "Enable FlexFEC field trial.");
        }
        fieldTrials += VIDEO_VP8_INTEL_HW_ENCODER_FIELDTRIAL;
        if (peerConnectionParameters.disableWebRtcAGCAndHPF) {
            fieldTrials += DISABLE_WEBRTC_AGC_FIELDTRIAL;
            Log.d(TAG, "Disable WebRTC AGC field trial.");
        }
        return fieldTrials;
    }


    @Nullable
    private VideoTrack createVideoTrack(VideoCapturer capturer) {
        surfaceTextureHelper = SurfaceTextureHelper.create("CaptureThread", mRootEglBase.getEglBaseContext());
        videoSource = mPeerConnectionFactory.createVideoSource(capturer.isScreencast());
        capturer.initialize(surfaceTextureHelper, mContext, videoSource.getCapturerObserver());
        capturer.startCapture(videoWidth, videoHeight, videoFps);

        localVideoTrack = mPeerConnectionFactory.createVideoTrack(VIDEO_TRACK_ID, videoSource);
        localVideoTrack.setEnabled(renderVideo);
        localVideoTrack.addSink(mLocalSurfaceView);
        return localVideoTrack;
    }

    // Returns the remote VideoTrack, assuming there is only one.
    private @Nullable
    VideoTrack getRemoteVideoTrack() {
        for (RtpTransceiver transceiver : mPeerConnection.getTransceivers()) {
            MediaStreamTrack track = transceiver.getReceiver().track();
            if (track instanceof VideoTrack) {
                return (VideoTrack) track;
            }
        }
        return null;
    }

    @Nullable
    private AudioTrack createAudioTrack() {
        audioSource = mPeerConnectionFactory.createAudioSource(mAudioConstraints);
        localAudioTrack = mPeerConnectionFactory.createAudioTrack(AUDIO_TRACK_ID, audioSource);
        localAudioTrack.setEnabled(enableAudio);
        return localAudioTrack;
    }

    // Implementation detail: observe ICE & stream changes and react accordingly.
    private class PCObserver implements PeerConnection.Observer {
        @Override
        public void onIceCandidate(final IceCandidate iceCandidate) {
            executor.execute(() -> {
                        Log.i(Constants.P2PTAG, "onIceCandidate: " + iceCandidate);
                        try {
                            JSONObject message = new JSONObject();
                            //message.put("userId", RTCSignalClient.getInstance().getUserId());
                            message.put("type", "candidate");
                            message.put("label", iceCandidate.sdpMLineIndex);
                            message.put("id", iceCandidate.sdpMid);
                            message.put("candidate", iceCandidate.sdp);
                            mISignalClient.sendSignalMessage(mRoomConnectParameters.roomId, message);
                        } catch (JSONException e) {
                            e.printStackTrace();
                        }
//                        events.OnIceCandidate(iceCandidate);
                    }
            );
        }

        @Override
        public void onIceCandidatesRemoved(final IceCandidate[] candidates) {
//            executor.execute(() -> events.OnIceCandidatesRemoved(candidates));
        }

        @Override
        public void onSignalingChange(PeerConnection.SignalingState newState) {
            Log.d(TAG, "SignalingState: " + newState);
        }

        @Override
        public void onIceConnectionChange(final PeerConnection.IceConnectionState newState) {
            executor.execute(() -> {
                Log.d(Constants.P2PTAG, "IceConnectionState: " + newState);
                if (newState == PeerConnection.IceConnectionState.CONNECTED) {
//                    events.OnIceConnected();
                } else if (newState == PeerConnection.IceConnectionState.DISCONNECTED) {
//                    events.OnIceDisconnected();
                } else if (newState == PeerConnection.IceConnectionState.FAILED) {
                    Log.e(Constants.P2PTAG, "ICE connection failed.");
                }
            });
        }

        @Override
        public void onConnectionChange(final PeerConnection.PeerConnectionState newState) {
            executor.execute(() -> {
                Log.d(Constants.P2PTAG, "PeerConnectionState: " + newState);
                if (newState == PeerConnection.PeerConnectionState.CONNECTED) {
//                    events.OnPeerConnectionConnected();
                } else if (newState == PeerConnection.PeerConnectionState.DISCONNECTED) {
//                    events.OnPeerConnectionDisconnected();
                } else if (newState == PeerConnection.PeerConnectionState.FAILED) {
                    Log.e(Constants.P2PTAG, "DTLS connection failed.");
                }
            });
        }

        @Override
        public void onIceGatheringChange(PeerConnection.IceGatheringState newState) {
            Log.d(Constants.P2PTAG, "IceGatheringState: " + newState);
        }

        @Override
        public void onIceConnectionReceivingChange(boolean receiving) {
            Log.d(Constants.P2PTAG, "IceConnectionReceiving changed to " + receiving);
        }

        @Override
        public void onSelectedCandidatePairChanged(CandidatePairChangeEvent event) {
            Log.d(Constants.P2PTAG, "Selected candidate pair changed because: " + event);
        }

        @Override
        public void onAddStream(final MediaStream stream) {
        }

        @Override
        public void onRemoveStream(final MediaStream stream) {
        }

        @Override
        public void onDataChannel(final DataChannel dc) {
            Log.d(Constants.P2PTAG, "NewDatachannel " + dc.label());

            if (!dataChannelEnabled)
                return;
            dc.registerObserver(new DataChannel.Observer() {
                @Override
                public void onBufferedAmountChange(long previousAmount) {
                    Log.d(Constants.P2PTAG, "Datachannel buffered amount changed: " + dc.label() + ": " + dc.state());
                }

                @Override
                public void onStateChange() {
                    Log.d(Constants.P2PTAG, "Datachannel state changed: " + dc.label() + ": " + dc.state());
                }

                @Override
                public void onMessage(final DataChannel.Buffer buffer) {
                    if (buffer.binary) {
                        Log.d(Constants.P2PTAG, "Datachannel Received binary msg over " + dc);
                        return;
                    }
                    ByteBuffer data = buffer.data;
                    final byte[] bytes = new byte[data.capacity()];
                    data.get(bytes);
                    String strData = new String(bytes, Charset.forName("UTF-8"));
                    Log.d(Constants.P2PTAG, "Datachannel Got msg: " + strData + " over " + dc);
                }
            });
        }

        @Override
        public void onRenegotiationNeeded() {
            // No need to do anything; AppRTC follows a pre-agreed-upon
            // signaling/negotiation protocol.
        }

        @Override
        public void onAddTrack(final RtpReceiver receiver, final MediaStream[] mediaStreams) {
        }

        @Override
        public void onRemoveTrack(final RtpReceiver receiver) {
        }
    }

    /**
     * Returns the line number containing "m=audio|video", or -1 if no such line exists.
     */
    private int findMediaDescriptionLine(boolean isAudio, String[] sdpLines) {
        final String mediaDescription = isAudio ? "m=audio " : "m=video ";
        for (int i = 0; i < sdpLines.length; ++i) {
            if (sdpLines[i].startsWith(mediaDescription)) {
                return i;
            }
        }
        return -1;
    }

    private String joinString(
            Iterable<? extends CharSequence> s, String delimiter, boolean delimiterAtEnd) {
        Iterator<? extends CharSequence> iter = s.iterator();
        if (!iter.hasNext()) {
            return "";
        }
        StringBuilder buffer = new StringBuilder(iter.next());
        while (iter.hasNext()) {
            buffer.append(delimiter).append(iter.next());
        }
        if (delimiterAtEnd) {
            buffer.append(delimiter);
        }
        return buffer.toString();
    }

    private @Nullable
    String movePayloadTypesToFront(
            List<String> preferredPayloadTypes, String mLine) {
        // The format of the media description line should be: m=<media> <port> <proto> <fmt> ...
        final List<String> origLineParts = Arrays.asList(mLine.split(" "));
        if (origLineParts.size() <= 3) {
            Log.e(TAG, "Wrong SDP media description format: " + mLine);
            return null;
        }
        final List<String> header = origLineParts.subList(0, 3);
        final List<String> unpreferredPayloadTypes =
                new ArrayList<>(origLineParts.subList(3, origLineParts.size()));
        unpreferredPayloadTypes.removeAll(preferredPayloadTypes);
        // Reconstruct the line with `preferredPayloadTypes` moved to the beginning of the payload
        // types.
        final List<String> newLineParts = new ArrayList<>();
        newLineParts.addAll(header);
        newLineParts.addAll(preferredPayloadTypes);
        newLineParts.addAll(unpreferredPayloadTypes);
        return joinString(newLineParts, " ", false /* delimiterAtEnd */);
    }

    private String preferCodec(String sdp, String codec, boolean isAudio) {
        final String[] lines = sdp.split("\r\n");
        final int mLineIndex = findMediaDescriptionLine(isAudio, lines);
        if (mLineIndex == -1) {
            Log.w(TAG, "No mediaDescription line, so can't prefer " + codec);
            return sdp;
        }
        // A list with all the payload types with name `codec`. The payload types are integers in the
        // range 96-127, but they are stored as strings here.
        final List<String> codecPayloadTypes = new ArrayList<>();
        // a=rtpmap:<payload type> <encoding name>/<clock rate> [/<encoding parameters>]
        final Pattern codecPattern = Pattern.compile("^a=rtpmap:(\\d+) " + codec + "(/\\d+)+[\r]?$");
        for (String line : lines) {
            Matcher codecMatcher = codecPattern.matcher(line);
            if (codecMatcher.matches()) {
                codecPayloadTypes.add(codecMatcher.group(1));
            }
        }
        if (codecPayloadTypes.isEmpty()) {
            Log.w(TAG, "No payload types with name " + codec);
            return sdp;
        }

        final String newMLine = movePayloadTypesToFront(codecPayloadTypes, lines[mLineIndex]);
        if (newMLine == null) {
            return sdp;
        }
        Log.d(TAG, "Change media description from: " + lines[mLineIndex] + " to " + newMLine);
        lines[mLineIndex] = newMLine;
        return joinString(Arrays.asList(lines), "\r\n", true /* delimiterAtEnd */);
    }

    private static String getSdpVideoCodecName(PeerConnectionParameters parameters) {
        switch (parameters.videoCodec) {
            case VIDEO_CODEC_VP8:
                return VIDEO_CODEC_VP8;
            case VIDEO_CODEC_VP9:
                return VIDEO_CODEC_VP9;
            case VIDEO_CODEC_AV1:
                return VIDEO_CODEC_AV1;
            case VIDEO_CODEC_H264_HIGH:
            case VIDEO_CODEC_H264_BASELINE:
                return VIDEO_CODEC_H264;
            default:
                return VIDEO_CODEC_VP8;
        }
    }

    private void drainCandidates() {
        if (queuedRemoteCandidates != null) {
            Log.d(Constants.P2PTAG, "Add " + queuedRemoteCandidates.size() + " remote candidates");
            for (IceCandidate candidate : queuedRemoteCandidates) {
                mPeerConnection.addIceCandidate(candidate, new AddIceObserver() {
                    @Override
                    public void onAddSuccess() {
                        Log.d(Constants.P2PTAG, "Candidate " + candidate.serverUrl + " successfully added.");
                    }

                    @Override
                    public void onAddFailure(String error) {
                        Log.d(Constants.P2PTAG, "Candidate " + candidate.serverUrl + " addition failed: " + error);
                    }
                });
            }
            queuedRemoteCandidates = null;
        }
    }

    // Implementation detail: handle offer creation/signaling and answer setting,
// as well as adding remote ICE candidates once the answer SDP is set.
    private class SDPObserver implements SdpObserver {
        @Override
        public void onCreateSuccess(final SessionDescription desc) {
            setLocalDescription(desc);
        }

        @Override
        public void onSetSuccess() {
            executor.execute(() -> {
                if (mPeerConnection == null) {
                    return;
                }
                //成功，代表自己加入成功
                if (isInitiator) {
                    // For offering peer connection we first create offer and set
                    // local SDP, then after receiving answer set remote SDP.
                    if (mPeerConnection.getRemoteDescription() == null) {
                        // We've just set our local SDP so time to send it.
                        Log.d(Constants.P2PTAG, "Local SDP set succesfully");
                        OnLocalDescription(localDescription);
                    } else {
                        // We've just set remote description, so drain remote
                        // and send local ICE candidates.
                        Log.d(Constants.P2PTAG, "Remote SDP set succesfully");
                        drainCandidates();
                    }
                } else {
                    // For answering peer connection we set remote SDP and then
                    // create answer and set local SDP.
                    if (mPeerConnection.getLocalDescription() != null) {
                        // We've just set our local SDP so time to send it, drain
                        // remote and send local ICE candidates.
                        Log.d(Constants.P2PTAG, "Local SDP set succesfully");
                        OnLocalDescription(localDescription);
                        drainCandidates();
                    } else {
                        // We've just set remote SDP - do nothing for now -
                        // answer will be created soon.
                        Log.d(Constants.P2PTAG, "Remote SDP set succesfully");
                    }
                }
            });
        }

        @Override
        public void onCreateFailure(final String error) {
            Log.e(TAG, "createSDP error: " + error);
        }

        @Override
        public void onSetFailure(final String error) {
            Log.e(TAG, "setSDP error: " + error);
        }
    }

    private void OnLocalDescription(SessionDescription sdp) {
        JSONObject message = new JSONObject();
        try {
            String type = "offer";
            if (sdp.type == SessionDescription.Type.ANSWER)
                type = "answer";
            message.put("type", type);
            message.put("sdp", sdp.description);
            sendMessage(message);
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    public void close() {
        if (mISignalClient != null) {
            mISignalClient.leave(mRoomConnectParameters.roomId);
            mISignalClient = null;
        }
        if (mLocalSurfaceView != null) {
            mLocalSurfaceView.release();
            mLocalSurfaceView = null;
        }
        if (mVideoCapture != null) {
            mVideoCapture.dispose();
            mVideoCapture = null;
        }
        if (mRemoteSurfaceView != null) {
            mRemoteSurfaceView.release();
            mRemoteSurfaceView = null;
        }
        if (mPeerConnection != null) {
            mPeerConnection.close();
            mPeerConnection = null;
        }
        if (surfaceTextureHelper != null) {
            surfaceTextureHelper.dispose();
            surfaceTextureHelper = null;
        }

        if (mPeerConnectionFactory != null) {
            mPeerConnectionFactory.dispose();
            mPeerConnectionFactory = null;
        }

    }
}
