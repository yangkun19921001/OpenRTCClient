#ifndef WEBRTC_HEADERS_H
#define WEBRTC_HEADERS_H

//fix sigslot.h build bug
// see https://groups.google.com/g/discuss-webrtc/c/8uF-YG_HxV0/m/W0w3F_klCgAJ
//如果emit已经被定义（可能是Qt库中定义的），则把当前的emit定义保存到QT_EMIT_STORE中，并且取消定义emit。这样可以避免与你的模板函数emit冲突。
//然后包含你需要的头文件（其中可能包含了你的模板函数emit的定义）。
//在头文件被包含之后，如果QT_EMIT_STORE已经被定义，那么重新定义emit为QT_EMIT_STORE中保存的内容，并取消定义QT_EMIT_STORE。
//这样可以把emit的定义恢复为原来的Qt版本。
#define QT_NO_KEYWORDS
#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include "rtc_base/strings/json.h"
#include "rtc_base/thread.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "api/peer_connection_interface.h"


//#ifdef emit
//// back-up then undef emit
//#define QT_EMIT_STORE emit

//#undef emit
//#endif




//#ifdef QT_EMIT_STORE
//// return emit back
//#define emit QT_EMIT_STORE
//#undef QT_EMIT_STORE
//#endif


#endif // WEBRTC_HEADERS_H
