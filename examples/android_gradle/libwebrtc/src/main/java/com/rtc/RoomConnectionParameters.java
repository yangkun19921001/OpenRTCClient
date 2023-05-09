package com.rtc;

/**
 * Struct holding the connection parameters of an AppRTC room.
 */
public class RoomConnectionParameters {
    public final String roomUrl;
    public final String roomId;
    public final boolean loopback;
    public final String urlParameters;

    public RoomConnectionParameters(
            String roomUrl, String roomId, boolean loopback, String urlParameters) {
        this.roomUrl = roomUrl;
        this.roomId = roomId;
        this.loopback = loopback;
        this.urlParameters = urlParameters;
    }

    public RoomConnectionParameters(String roomUrl, String roomId, boolean loopback) {
        this(roomUrl, roomId, loopback, null /* urlParameters */);
    }
}