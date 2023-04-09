package com.rtc.event;

import org.json.JSONArray;
import org.json.JSONObject;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/4/9
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
public interface ISignalEventListener {
    void onConnecting();

    void onConnectSuccessful();

    void onConnectError(String e);

    void onJoined(String room, String remoteId, JSONArray otherClientIds);

    void onLeaved(String room, String remoteId);

    void onMessage(String from, String to, JSONObject message);
}
