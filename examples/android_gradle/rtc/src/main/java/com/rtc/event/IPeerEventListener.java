package com.rtc.event;

import org.json.JSONObject;
import org.webrtc.VideoTrack;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/4/9
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
public interface IPeerEventListener {
    void onEmit(String message, String room, String remoteId, JSONObject message1);

    void onSetFailure(String type, String userId, String error);

    void onCreateFailure(String type, String userId, String error);

    void onAddCandideFailure(String remoteId, String error);

    void onAddRemoteStream(VideoTrack remoteVideoTrack, String userId);

    void onRemoveRemoteStream(String remoteId);
}
