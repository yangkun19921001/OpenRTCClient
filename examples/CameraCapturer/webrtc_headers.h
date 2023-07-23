#ifndef WEBRTC_HEADERS_H
#define WEBRTC_HEADERS_H

//fix sigslot.h build bug
// see https://groups.google.com/g/discuss-webrtc/c/8uF-YG_HxV0/m/W0w3F_klCgAJ
//如果emit已经被定义（可能是Qt库中定义的），则把当前的emit定义保存到QT_EMIT_STORE中，并且取消定义emit。这样可以避免与你的模板函数emit冲突。
//然后包含你需要的头文件（其中可能包含了你的模板函数emit的定义）。
//在头文件被包含之后，如果QT_EMIT_STORE已经被定义，那么重新定义emit为QT_EMIT_STORE中保存的内容，并取消定义QT_EMIT_STORE。
//这样可以把emit的定义恢复为原来的Qt版本。

#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include "rtc_base/strings/json.h"
#include "rtc_base/thread.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "api/peer_connection_interface.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/audio_options.h"
#include "api/create_peerconnection_factory.h"
#include "api/media_stream_interface.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "pc/video_track_source.h"
#include "api/jsep.h"
#include "api/media_stream_interface.h"


#include "api/audio/audio_mixer.h"
#include "api/rtp_sender_interface.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"

#include "api/video/video_frame.h"
#include "api/video/video_source_interface.h"
#include "media/base/video_adapter.h"
#include "media/base/video_broadcaster.h"
#include "rtc_base/synchronization/mutex.h"

#include "api/scoped_refptr.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_rotation.h"

#endif // WEBRTC_HEADERS_H
