cmake_minimum_required(VERSION 3.5)




project(CameraCapturer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_TYPE debug)
if(MSVC)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    elseif(CMAKE_BUILD_TYPE MATCHES Release)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
        #set(BUILD_TYPE release)
    endif()
endif()


# Project configuration.
set(WEBRTC_THIRD_PARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../webrtc/third_party)
set(LIBWEBRTC_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../webrtc)
set(LIBWEBRTC_BINARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../build_system/build/win/x64/${BUILD_TYPE}/obj)




if(NOT LIBWEBRTC_INCLUDE_PATH)
        message(FATAL_ERROR "LIBWEBRTC_INCLUDE_PATH not provided")
endif()

if(NOT LIBWEBRTC_BINARY_PATH)
        message(FATAL_ERROR "LIBWEBRTC_BINARY_PATH not provided")
endif()

message(STATUS "LIBWEBRTC_INCLUDE_PATH      : " ${LIBWEBRTC_INCLUDE_PATH})
message(STATUS "LIBWEBRTC_BINARY_PATH       : " ${LIBWEBRTC_BINARY_PATH})
message(STATUS "QT_VERSION_MAJOR            : " ${QT_VERSION_MAJOR})



#find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS  Widgets LinguistTools OpenGL OpenGLWidgets   REQUIRED)
#find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS  Widgets LinguistTools OpenGL OpenGLWidgets   REQUIRED)

set(QT_VERSION_MAJOR 6)
find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL LinguistTools OpenGLWidgets REQUIRED)



set(JSONCPP_SOURCE
    "${WEBRTC_THIRD_PARTY_PATH}/jsoncpp/source/src/lib_json/json_reader.cpp"
    "${WEBRTC_THIRD_PARTY_PATH}/jsoncpp/source/src/lib_json/json_tool.h"
    "${WEBRTC_THIRD_PARTY_PATH}/jsoncpp/source/src/lib_json/json_value.cpp"
    "${WEBRTC_THIRD_PARTY_PATH}/jsoncpp/source/src/lib_json/json_writer.cpp"
)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui

        ${JSONCPP_SOURCE}
        webrtc_headers.h
        VideoRendererWidget.cpp
        VideoRendererWidget.h
        CameraCapturer.h
        CameraCapturer.cpp
        CameraCapturerTrackSource.h
        VideoCapturer.h
        VideoCapturer.cpp

)



if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} )
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(${PROJECT_NAME}
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} )
endif()

#deps see:https://zhuanlan.zhihu.com/p/604361972
target_include_directories(${PROJECT_NAME} PUBLIC
        "${LIBWEBRTC_INCLUDE_PATH}"
        "${LIBWEBRTC_INCLUDE_PATH}/third_party/abseil-cpp"
        "${LIBWEBRTC_INCLUDE_PATH}/third_party/jsoncpp/source/include"
        "${LIBWEBRTC_INCLUDE_PATH}/third_party/jsoncpp/generated"
        "${LIBWEBRTC_INCLUDE_PATH}/third_party/libyuv/include"
        "${SOCKET_IO_INCLUDE_PATH}"
)

add_definitions(
        #webrtc & qt 冲突
        -DQT_DEPRECATED_WARNINGS
        -DQT_NO_KEYWORDS

        -DUSE_AURA=1
        -D_HAS_EXCEPTIONS=0
        -D__STD_C
        -D_CRT_RAND_S
        -D_CRT_SECURE_NO_DEPRECATE
        -D_SCL_SECURE_NO_DEPRECATE
        -D_ATL_NO_OPENGL
        -D_WINDOWS
        -DCERT_CHAIN_PARA_HAS_EXTRA_FIELDS
        -DPSAPI_VERSION=2
        -DWIN32
        -D_SECURE_ATL
        -DWINUWP
        -D__WRL_NO_DEFAULT_LIB__
 #       -DWINAPI_FAMILY=WINAPI_FAMILY_PC_APP
        -DWIN10=_WIN32_WINNT_WIN10
        -DWIN32_LEAN_AND_MEAN
        -DNOMINMAX
        -D_UNICODE
        -DUNICODE
        -DNTDDI_VERSION=NTDDI_WIN10_RS2
        -D_WIN32_WINNT=0x0A00
        -DWINVER=0x0A00
        -DDEBUG
        -DNVALGRIND
        -DDYNAMIC_ANNOTATIONS_ENABLED=0
        -DWEBRTC_ENABLE_PROTOBUF=0
        -DWEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE
        -DRTC_ENABLE_VP9
        -DHAVE_SCTP
        -DWEBRTC_LIBRARY_IMPL
        -DWEBRTC_NON_STATIC_TRACE_EVENT_HANDLERS=0
        -DWEBRTC_WIN
        -DABSL_ALLOCATOR_NOTHROW=1
        -DHAVE_SCTP
        -DWEBRTC_VIDEO_CAPTURE_WINRT)





target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets

        ${LIBWEBRTC_BINARY_PATH}/webrtc.lib
        winmm.lib
        iphlpapi.lib
        wbemuuid.lib
        secur32.lib
        advapi32.lib
        Mmdevapi.lib
        Mfuuid.lib
        msdmo.lib
        dmoguids.lib
        wmcodecdspuuid.lib
        comdlg32.lib
        dbghelp.lib
        dnsapi.lib
        gdi32.lib
        msimg32.lib
        odbc32.lib
        odbccp32.lib
        oleaut32.lib
        shell32.lib
        shlwapi.lib
        user32.lib
        usp10.lib
        uuid.lib
        version.lib
        wininet.lib
        winmm.lib
        winspool.lib
        ws2_32.lib
        delayimp.lib
        kernel32.lib
        ole32.lib
        crypt32.lib
        amstrmid.lib
        strmiids.lib

        #LIBCMTD.lib(initializers.obj):-1: warning: LNK4098: 默认库“msvcrtd.lib”与其他库的使用冲突；请使用 /NODEFAULTLIB:library
        #msvcprtd.lib(nothrow.obj):-1: error: LNK2038: 检测到“RuntimeLibrary”的不匹配项: 值“MDd_DynamicDebug”不匹配值“MTd_StaticDebug”(mocs_compilation.cpp.obj 中)
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
