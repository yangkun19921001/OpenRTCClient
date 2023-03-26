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
interface ISignalEventListener {

    /**
     * 连接中
     */
    void OnConnecting();

    /**
     * 连接成功
     */
    void OnConnected();

    /**
     * 关闭连接
     */
    void OnDisconnected();

    /**
     * 当前用户加入
     *
     * @param roomName
     * @param userId
     */
    void OnUserJoined(String roomName, String userId,boolean isInitiator);

    /**
     * 当前用户离开
     */
    void OnUserLeaved(String roomName, String userId);

    /**
     * 远端用户加入
     *
     * @param roomName
     * @param data
     */
    void OnRemoteUserJoined(String roomName, String userId);

    /**
     * 当远端用户离开
     *
     * @param roomName
     * @param userId
     */
    void OnRemoteUserLeaved(String roomName, String userId);

    /**
     * 房间满了
     *
     * @param roomName
     * @param userId
     */
    void OnRoomFull(String roomName, String userId);

    /**
     * 信令消息
     *
     * @param message
     */
    void OnMessage(JSONObject message);


}
