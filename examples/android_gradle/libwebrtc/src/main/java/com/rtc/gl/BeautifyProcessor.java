package com.rtc.gl;

import android.support.annotation.Nullable;

import org.webrtc.VideoFrame;
import org.webrtc.VideoProcessor;
import org.webrtc.VideoSink;

/**
 * <pre>
 *     author  : 马克
 *     time    : 2023/4/17
 *     mailbox : make@pplabs.org
 *     desc    :
 * </pre>
 */
public class BeautifyProcessor implements VideoProcessor {
    private VideoSink sink;

    @Override
    public void onCapturerStarted(boolean success) {

    }

    @Override
    public void onCapturerStopped() {

    }

    @Override
    public void onFrameCaptured(VideoFrame frame) {
        //1. 拿到对应的相机数据
        if (frame.getBuffer() instanceof VideoFrame.TextureBuffer) {
            //这里可以对视频进行预处理
            //VideoFrame.I420Buffer i420Buffer = frame.getBuffer().toI420();
        }
        //2. 把处理好的 frame 返回
        sink.onFrame(frame);
    }

    @Override
    public void setSink(@Nullable VideoSink sink) {
        this.sink = sink;
    }
}
