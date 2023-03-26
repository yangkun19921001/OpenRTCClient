package com.sample.p2p;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/3/23
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
interface IPeerConnectionListener {

    /**
     * 连接中
     */
    void OnConnecting();

    /**
     * 连接成功
     */
    void OnConnected();


    /**
     * 当失败时
     *
     * @param error
     */
    void OnFailed(String error);


}
