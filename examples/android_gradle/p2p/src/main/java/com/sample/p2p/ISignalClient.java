package com.sample.p2p;

import org.json.JSONObject;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/3/18
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
interface ISignalClient {

    enum SignalEvent {
        JOINED("joined"),
        LEAVED("leaved"),
        OTHERJOIN("otherjoin"),
        BYE("bye"),
        FULL("full"),
        MESSAGE("message");

        private String eventName;

        SignalEvent(String eventName) {
            this.eventName = eventName;
        }

        public String getEventName() {
            return eventName;
        }
    }

    /**
     * 连接服务
     *
     * @param url
     */
    void connect(String url, ISignalEventListener events);

    /**
     * 加入房间
     *
     * @param roomId
     */
    void join(String roomId);

    /**
     * 离开房间
     *
     * @param roomId
     */
    void leave(String roomId);

    /**
     * 发送信令消息
     *
     * @param message
     */
    void sendSignalMessage(String roomId, JSONObject message);
}
