package com.rtc.room;

import android.content.Context;
import android.support.annotation.Nullable;
import android.util.Log;

import com.rtc.Constants;
import com.rtc.DataChannelParameters;
import com.rtc.PeerConnectionParameters;
import com.rtc.event.PeerConnectionObserverImpl;
import com.rtc.event.IPeerEventListener;
import com.rtc.event.SdpObserverImpl;
import com.rtc.gl.BeautifyProcessor;

import org.json.JSONException;
import org.json.JSONObject;
import org.webrtc.AddIceObserver;
import org.webrtc.AudioSource;
import org.webrtc.AudioTrack;
import org.webrtc.DefaultVideoDecoderFactory;
import org.webrtc.DefaultVideoEncoderFactory;
import org.webrtc.EglBase;
import org.webrtc.IceCandidate;
import org.webrtc.MediaConstraints;
import org.webrtc.MediaStream;
import org.webrtc.PeerConnection;
import org.webrtc.PeerConnectionFactory;
import org.webrtc.SessionDescription;
import org.webrtc.SurfaceTextureHelper;
import org.webrtc.VideoCapturer;
import org.webrtc.VideoSink;
import org.webrtc.VideoSource;
import org.webrtc.VideoTrack;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/4/9
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
class PeerManager {
    private static String TAG = "PeerManager";
    @Nullable
    private final Context applicationContext;

    private static final ExecutorService executor = Executors.newSingleThreadExecutor();

    @Nullable
    private boolean videoCapturerStopped = true;

    @Nullable
    private PeerConnectionFactory peerConnectionFactory;
    @Nullable
    private Map<String, PeerConnection> peerConnections;
    @Nullable
    private MediaStream localStream;
    @Nullable
    private VideoSink localVideoRenderer;
    @Nullable
    private AudioSource audioSource;
    @Nullable
    private AudioTrack localAudioTrack;
    @Nullable
    private VideoCapturer localVideoCapturer;
    @Nullable
    private PeerConnectionParameters peerConnectionParameters;
    @Nullable
    private DataChannelParameters dataChannelParameters;
    @Nullable
    private EglBase rootEglBase;
    @Nullable
    private IPeerEventListener peerEventListener;

    public PeerManager(@Nullable Context context, @Nullable EglBase rootEglBase, @Nullable VideoSink localVideoSink, @Nullable VideoCapturer videoCapturer, @Nullable PeerConnectionParameters parameters, @Nullable DataChannelParameters dataParameters, @Nullable IPeerEventListener peerEventListener) {
        applicationContext = context.getApplicationContext();
        this.rootEglBase = rootEglBase;
        this.peerEventListener = peerEventListener;
        peerConnections = new HashMap<>();
        localVideoRenderer = localVideoSink;
        localVideoCapturer = videoCapturer;
        peerConnectionParameters = parameters;
        dataChannelParameters = dataParameters;
        executor.execute(() -> {
            peerConnectionFactory = createPeerConnectionFactory();
            localStream = peerConnectionFactory.createLocalMediaStream("ARDAMS");
            if (peerConnectionParameters.videoCallEnabled)
                createLocalAudioTrack();
            if (peerConnectionParameters.audioCallEnabled)
                createLocalVideoTrack();
        });
    }

    private void createLocalVideoTrack() {
        // 创建 VideoSource
        VideoSource videoSource = peerConnectionFactory.createVideoSource(localVideoCapturer.isScreencast());
        videoSource.setVideoProcessor(new BeautifyProcessor());
        SurfaceTextureHelper surfaceTextureHelper = SurfaceTextureHelper.create("CaptureThread", rootEglBase.getEglBaseContext());
        localVideoCapturer.initialize(surfaceTextureHelper, applicationContext, videoSource.getCapturerObserver());

        // 创建 VideoTrack
        VideoTrack localVideoTrack = peerConnectionFactory.createVideoTrack("100", videoSource);

        // 添加 VideoTrack 到 SurfaceViewRenderer
        localVideoTrack.addSink(localVideoRenderer);


        // 启动 VideoCapturer
        localVideoCapturer.startCapture(peerConnectionParameters.videoWidth, peerConnectionParameters.videoHeight, peerConnectionParameters.videoFps);
        videoCapturerStopped = false;
        // 将 VideoTrack 添加到 localStream
        localStream.addTrack(localVideoTrack);
    }

    private void createLocalAudioTrack() {
        audioSource = peerConnectionFactory.createAudioSource(new MediaConstraints());
        localAudioTrack = peerConnectionFactory.createAudioTrack("101", audioSource);
        localStream.addTrack(localAudioTrack);
    }

    private PeerConnectionFactory createPeerConnectionFactory() {
        PeerConnectionFactory.initialize(
                PeerConnectionFactory.InitializationOptions.builder(applicationContext)
                        .setEnableInternalTracer(true)
                        .createInitializationOptions());

        PeerConnectionFactory.Options options = new PeerConnectionFactory.Options();
        DefaultVideoEncoderFactory defaultVideoEncoderFactory =
                new DefaultVideoEncoderFactory(
                        rootEglBase.getEglBaseContext(), true /* enableIntelVp8Encoder */, true);
        DefaultVideoDecoderFactory defaultVideoDecoderFactory =
                new DefaultVideoDecoderFactory(rootEglBase.getEglBaseContext());

        return PeerConnectionFactory.builder()
                .setOptions(options)
                .setVideoEncoderFactory(defaultVideoEncoderFactory)
                .setVideoDecoderFactory(defaultVideoDecoderFactory)
                .createPeerConnectionFactory();
    }

    public void createPeerConnection(String remoteId, boolean createOffer) {
        executor.execute(() -> {
            createPeerConnectionInternal(remoteId, createOffer);
        });
    }

    public void handleOffer(String remoteId, String sdp) {
        executor.execute(() -> {
            handleOfferInternal(remoteId, sdp);
        });
    }

    private void handleOfferInternal(String remoteId, String sdp) {
        PeerConnection peerConnection = peerConnections.get(remoteId);
        SessionDescription sessionDescription = new SessionDescription(SessionDescription.Type.OFFER, sdp);
        peerConnection.setRemoteDescription(new SdpObserverImpl("setRemoteDescription", remoteId, peerEventListener), sessionDescription);
        peerConnection.createAnswer(new SdpObserverImpl("createAnswer", remoteId, peerEventListener) {
            @Override
            public void onCreateSuccess(SessionDescription sessionDescription) {
                String newsdp = sessionDescription.description;
                if (peerConnectionParameters.videoCallEnabled) {
                    newsdp = preferCodec(newsdp, "H264", false);
                }
                peerConnection.setLocalDescription(new SdpObserverImpl("setLocalDescription", remoteId, peerEventListener), new SessionDescription(sessionDescription.type, newsdp));
                JSONObject message = new JSONObject();
                try {
                    message.put("type", "answer");
                    message.put("sdp", newsdp);
                } catch (JSONException e) {
                    e.printStackTrace();
                }
                if (peerEventListener != null) {
                    peerEventListener.onEmit("message", null, remoteId, message);
                }
            }
        }, new MediaConstraints());

    }

    public void handleAnswer(String remoteId, String sdp) {
        executor.execute(() -> {
            handleAnswerInternal(remoteId, sdp);
        });
    }

    private void handleAnswerInternal(String remoteId, String sdp) {
        PeerConnection peerConnection = peerConnections.get(remoteId);
        SessionDescription sessionDescription = new SessionDescription(SessionDescription.Type.ANSWER, sdp);
        Log.d(Constants.P2PSTAG, " setRemoteDescription  UserId:" + remoteId);
        peerConnection.setRemoteDescription(new SdpObserverImpl("setRemoteDescription", remoteId, peerEventListener), sessionDescription);
    }

    public void handleCandidate(String remoteId, IceCandidate candidate) {
        executor.execute(() -> {
            handleCandidateInternal(remoteId, candidate);
        });
    }

    private void handleCandidateInternal(String remoteId, IceCandidate candidate) {
        PeerConnection peerConnection = peerConnections.get(remoteId);
        peerConnection.addIceCandidate(candidate, new AddIceObserver() {
            @Override
            public void onAddSuccess() {

            }

            @Override
            public void onAddFailure(String error) {
                if (peerEventListener != null) {
                    peerEventListener.onAddCandideFailure(remoteId, error);
                }
            }
        });
    }


    /**
     * Returns the line number containing "m=audio|video", or -1 if no such line exists.
     */
    private static int findMediaDescriptionLine(boolean isAudio, String[] sdpLines) {
        final String mediaDescription = isAudio ? "m=audio " : "m=video ";
        for (int i = 0; i < sdpLines.length; ++i) {
            if (sdpLines[i].startsWith(mediaDescription)) {
                return i;
            }
        }
        return -1;
    }

    private static String joinString(
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

    private static @Nullable String movePayloadTypesToFront(
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
        // Reconstruct the line with |preferredPayloadTypes| moved to the beginning of the payload
        // types.
        final List<String> newLineParts = new ArrayList<>();
        newLineParts.addAll(header);
        newLineParts.addAll(preferredPayloadTypes);
        newLineParts.addAll(unpreferredPayloadTypes);
        return joinString(newLineParts, " ", false /* delimiterAtEnd */);
    }

    private static String preferCodec(String sdpDescription, String codec, boolean isAudio) {
        final String[] lines = sdpDescription.split("\r\n");
        final int mLineIndex = findMediaDescriptionLine(isAudio, lines);
        if (mLineIndex == -1) {
            Log.w(TAG, "No mediaDescription line, so can't prefer " + codec);
            return sdpDescription;
        }
        // A list with all the payload types with name |codec|. The payload types are integers in the
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
            return sdpDescription;
        }

        final String newMLine = movePayloadTypesToFront(codecPayloadTypes, lines[mLineIndex]);
        if (newMLine == null) {
            return sdpDescription;
        }
        Log.d(TAG, "Change media description from: " + lines[mLineIndex] + " to " + newMLine);
        lines[mLineIndex] = newMLine;
        return joinString(Arrays.asList(lines), "\r\n", true /* delimiterAtEnd */);
    }

    private void createPeerConnectionInternal(String remoteId, boolean createOffer) {
        List<PeerConnection.IceServer> iceServers = getIceServers();
        PeerConnection.RTCConfiguration rtcConfig = new PeerConnection.RTCConfiguration(iceServers);
        PeerConnection.Observer observer = new PeerConnectionObserverImpl(remoteId) {
            @Override
            public void onIceCandidate(IceCandidate iceCandidate) {
                super.onIceCandidate(iceCandidate);
                JSONObject message = new JSONObject();
                try {
                    message.put("type", "candidate");
                    message.put("label", iceCandidate.sdpMLineIndex);
                    message.put("id", iceCandidate.sdpMid);
                    message.put("candidate", iceCandidate.sdp);
                    if (peerEventListener != null) {
                        peerEventListener.onEmit("message", null, getUserId(), message);
                    }
                } catch (JSONException e) {
                    e.printStackTrace();
                }

            }

            @Override
            public void onAddStream(MediaStream mediaStream) {
                super.onAddStream(mediaStream);
                Log.d(Constants.P2PSTAG, "onAddStream UserId:" + getUserId());
                if (mediaStream.videoTracks.size() > 0) {
                    VideoTrack remoteVideoTrack = mediaStream.videoTracks.get(0);
                    if (peerEventListener != null) {
                        peerEventListener.onAddRemoteStream(remoteVideoTrack, getUserId());
                    }
                }
            }
        };
        PeerConnection peerConnection = peerConnectionFactory.createPeerConnection(rtcConfig, observer);

        //todo 添加远端视频
        peerConnections.put(remoteId, peerConnection);
        peerConnection.addStream(localStream);
        MediaConstraints mediaConstraints = new MediaConstraints();
        if (!peerConnectionParameters.audioCallEnabled)
            mediaConstraints.mandatory.add(new MediaConstraints.KeyValuePair("OfferToReceiveAudio", "false"));
        if (!peerConnectionParameters.videoCallEnabled)
            mediaConstraints.mandatory.add(new MediaConstraints.KeyValuePair("OfferToReceiveVideo", "false"));

        // Send SDP Offer
        if (createOffer)
            peerConnection.createOffer(new SdpObserverImpl("createOffer", remoteId, peerEventListener) {
                                           @Override
                                           public void onCreateSuccess(SessionDescription sessionDescription) {
                                               Log.d(Constants.P2PSTAG, "createOffer onCreateSuccess, UserId:" + remoteId);

                                               String newsdp = sessionDescription.description;
                                               if (peerConnectionParameters.videoCallEnabled) {
                                                   newsdp = preferCodec(newsdp, "H264", false);
                                               }
                                               peerConnection.setLocalDescription(new SdpObserverImpl("setLocalDescription", remoteId, peerEventListener), new SessionDescription(sessionDescription.type, newsdp));
                                               JSONObject message = new JSONObject();
                                               try {
                                                   message.put("type", "offer");
                                                   message.put("sdp", newsdp);
                                               } catch (JSONException e) {
                                                   e.printStackTrace();
                                               }
                                               Log.d(Constants.P2PSTAG, "send local sdp ,to  UserId:" + remoteId);
                                               if (peerEventListener != null) {
                                                   peerEventListener.onEmit("message", null, remoteId, message);
                                               }
                                           }
                                       },
                    mediaConstraints
            );

    }

    private List<PeerConnection.IceServer> getIceServers() {
        List<PeerConnection.IceServer> iceServers = new ArrayList<>();
        iceServers.add(PeerConnection.IceServer
                .builder("turn:rtcmedia.top:3478")
                .setPassword("devyk")
                .setUsername("devyk")
                .createIceServer());
        iceServers.add(PeerConnection.IceServer
                .builder("stun:stun.l.google.com:19302")
                .createIceServer());
        return iceServers;
    }

    public void removePeerConnection(String room, String remoteId) {
        executor.execute(() -> {
            // 移除 PeerConnection
            PeerConnection peerConnection = peerConnections.remove(remoteId);
            if (peerConnection != null) {
                peerConnection.close();
            }

            if (peerEventListener != null) {
                peerEventListener.onRemoveRemoteStream(remoteId);
            }
        });
    }

    public void stopVideoSource() {
        if (!peerConnectionParameters.videoCallEnabled) return;
        executor.execute(() -> {
            if (localVideoCapturer != null && !videoCapturerStopped) {
                Log.d(TAG, "Stop video source.");
                try {
                    localVideoCapturer.stopCapture();
                } catch (InterruptedException e) {
                }
                videoCapturerStopped = true;
            }
        });
    }

    public void startVideoSource() {
        if (!peerConnectionParameters.videoCallEnabled) return;
        executor.execute(() -> {
            if (localVideoCapturer != null && videoCapturerStopped) {
                Log.d(TAG, "Restart video source.");
                localVideoCapturer.startCapture(peerConnectionParameters.videoWidth, peerConnectionParameters.videoHeight, peerConnectionParameters.videoFps);
                videoCapturerStopped = false;
            }
        });
    }


    public void release() {
        // 关闭并移除所有 PeerConnections
        for (String socketId : peerConnections.keySet()) {
            PeerConnection peerConnection = peerConnections.get(socketId);
            if (peerConnection != null) {
                peerConnection.close();
            }
        }
        peerConnections.clear();

        // 释放 PeerConnectionFactory 和 VideoCapturer
        if (peerConnectionFactory != null) {
            peerConnectionFactory.dispose();
            peerConnectionFactory = null;
        }

        if (localVideoCapturer != null) {
            localVideoCapturer.dispose();
            localVideoCapturer = null;
        }
    }

}
