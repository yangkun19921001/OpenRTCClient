cmake_minimum_required(VERSION 3.5)

project(p2ps VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# Project configuration.
set(LIBWEBRTC_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../webrtc)
set(LIBWEBRTC_BINARY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../build/win/x64/debug/obj)

if(NOT LIBWEBRTC_INCLUDE_PATH)
	message(FATAL_ERROR "LIBWEBRTC_INCLUDE_PATH not provided")
endif()

if(NOT LIBWEBRTC_BINARY_PATH)
	message(FATAL_ERROR "LIBWEBRTC_BINARY_PATH not provided")
endif()

message(STATUS "LIBWEBRTC_INCLUDE_PATH      : " ${LIBWEBRTC_INCLUDE_PATH})
message(STATUS "LIBWEBRTC_BINARY_PATH       : " ${LIBWEBRTC_BINARY_PATH})

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools OpenGL)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools OpenGL)


set(TS_FILES p2ps_zh_CN.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        webrtc_headers.h

        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(p2ps
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        src/common/PeerManager.cpp
        src/common/PeerManager.h
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(p2ps SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(p2ps
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

#deps see:https://zhuanlan.zhihu.com/p/604361972
target_include_directories(${PROJECT_NAME} PUBLIC
        "${LIBWEBRTC_INCLUDE_PATH}"
        "${LIBWEBRTC_INCLUDE_PATH}/third_party/abseil-cpp"
        "${LIBWEBRTC_INCLUDE_PATH}/third_party/jsoncpp/source/include"
        "${LIBWEBRTC_INCLUDE_PATH}/third_party/jsoncpp/generated"
        "${LIBWEBRTC_INCLUDE_PATH}/third_party/libyuv/include"
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
        -DWINAPI_FAMILY=WINAPI_FAMILY_PC_APP
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

    if(MSVC)
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
      set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
    endif()

#find_package(jsoncpp CONFIG REQUIRED)
#find_package(sioclient CONFIG REQUIRED)
target_link_libraries(p2ps PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGL 
#JsonCpp::JsonCpp
#sioclient::sioclient sioclient::sioclient_tls
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
        msvcprtd.lib
)

set_target_properties(p2ps PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS p2ps
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(p2ps)
endif()
