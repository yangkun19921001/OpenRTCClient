# Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'includes': [ '../build/common.gypi', ],
  'targets': [
    {
      'target_name': 'rtc_unittest_main',
      'type': 'static_library',
      'dependencies': [
        '<(webrtc_root)/base/base_tests.gyp:rtc_base_tests_utils',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(libyuv_dir)/include',
          '<(DEPTH)/testing/gtest/include',
          '<(DEPTH)/testing/gtest',
        ],
      },
      'conditions': [
        ['build_libyuv==1', {
          'dependencies': ['<(DEPTH)/third_party/libyuv/libyuv.gyp:libyuv',],
        }],
        ['OS=="ios"', {
          # TODO(kjellander): Make the code compile without disabling these.
          # See https://bugs.chromium.org/p/webrtc/issues/detail?id=3307
          'cflags': [
            '-Wno-unused-variable',
          ],
          'xcode_settings': {
            'WARNING_CFLAGS': [
              '-Wno-unused-variable',
            ],
          },
        }],
      ],
      'include_dirs': [
         '<(DEPTH)/testing/gtest/include',
         '<(DEPTH)/testing/gtest',
       ],
      'sources': [
        'base/fakecapturemanager.h',
        'base/fakemediaengine.h',
        'base/fakenetworkinterface.h',
        'base/fakertp.h',
        'base/fakevideocapturer.h',
        'base/fakevideorenderer.h',
        'base/testutils.cc',
        'base/testutils.h',
        'devices/fakedevicemanager.h',
        'engine/fakewebrtccall.cc',
        'engine/fakewebrtccall.h',
        'engine/fakewebrtccommon.h',
        'engine/fakewebrtcdeviceinfo.h',
        'engine/fakewebrtcvcmfactory.h',
        'engine/fakewebrtcvideocapturemodule.h',
        'engine/fakewebrtcvideoengine.h',
        'engine/fakewebrtcvoiceengine.h',
      ],
      # TODO(kjellander): Make the code compile without disabling these flags.
      # See https://bugs.chromium.org/p/webrtc/issues/detail?id=3307
      'cflags_cc!': [
        '-Wnon-virtual-dtor',
      ],
    },  # target rtc_unittest_main
    {
      'target_name': 'rtc_media_unittests',
      'type': 'executable',
      'dependencies': [
        '<(webrtc_root)/base/base_tests.gyp:rtc_base_tests_utils',
        '<(webrtc_root)/media/media.gyp:rtc_media',
        'rtc_unittest_main',
      ],
      'sources': [
        'base/capturemanager_unittest.cc',
        'base/codec_unittest.cc',
        'base/rtpdataengine_unittest.cc',
        'base/rtpdump_unittest.cc',
        'base/rtputils_unittest.cc',
        'base/streamparams_unittest.cc',
        'base/turnutils_unittest.cc',
        'base/videoadapter_unittest.cc',
        'base/videocapturer_unittest.cc',
        'base/videocommon_unittest.cc',
        'base/videoengine_unittest.h',
        'base/videoframe_unittest.h',
        'devices/dummydevicemanager_unittest.cc',
        'engine/nullwebrtcvideoengine_unittest.cc',
        'engine/simulcast_unittest.cc',
        'engine/webrtcmediaengine_unittest.cc',
        'engine/webrtcvideocapturer_unittest.cc',
        'engine/webrtcvideoframe_unittest.cc',
        'engine/webrtcvideoframefactory_unittest.cc',
        # Disabled because some tests fail.
        # TODO(ronghuawu): Reenable these tests.
        # 'devices/devicemanager_unittest.cc',
        'engine/webrtcvideoengine2_unittest.cc',
        'engine/webrtcvoiceengine_unittest.cc',
        'sctp/sctpdataengine_unittest.cc',
      ],
      # TODO(kjellander): Make the code compile without disabling these flags.
      # See https://bugs.chromium.org/p/webrtc/issues/detail?id=3307
      'cflags': [
        '-Wno-sign-compare',
      ],
      'cflags_cc!': [
        '-Wnon-virtual-dtor',
        '-Woverloaded-virtual',
      ],
      'msvs_disabled_warnings': [
        4245,  # conversion from 'int' to 'uint32_t', signed/unsigned mismatch.
        4389,  # signed/unsigned mismatch.
      ],
      'conditions': [
        ['OS=="win"', {
          'conditions': [
            ['use_openssl==0', {
              'dependencies': [
                '<(DEPTH)/net/third_party/nss/ssl.gyp:libssl',
                '<(DEPTH)/third_party/nss/nss.gyp:nspr',
                '<(DEPTH)/third_party/nss/nss.gyp:nss',
              ],
            }],
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalDependencies': [
                # TODO(ronghuawu): Since we've included strmiids in
                # libjingle_media target, we shouldn't need this here.
                # Find out why it doesn't work without this.
                'strmiids.lib',
              ],
            },
          },
        }],
        ['OS=="win" and clang==1', {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'AdditionalOptions': [
                # Disable warnings failing when compiling with Clang on Windows.
                # https://bugs.chromium.org/p/webrtc/issues/detail?id=5366
                '-Wno-sign-compare',
                '-Wno-unused-function',
              ],
            },
          },
        },],
        ['clang==1', {
          # TODO(kjellander): Make the code compile without disabling these.
          # See https://bugs.chromium.org/p/webrtc/issues/detail?id=3307
          'cflags!': [
            '-Wextra',
          ],
          'xcode_settings': {
            'WARNING_CFLAGS!': ['-Wextra'],
          },
        }],
        ['OS=="ios"', {
          'sources!': [
            'sctp/sctpdataengine_unittest.cc',
          ],
          'mac_bundle_resources': [
            '<(DEPTH)/resources/media/captured-320x240-2s-48.frames',
            '<(DEPTH)/resources/media/faces.1280x720_P420.yuv',
            '<(DEPTH)/resources/media/faces_I420.jpg',
            '<(DEPTH)/resources/media/faces_I422.jpg',
            '<(DEPTH)/resources/media/faces_I444.jpg',
            '<(DEPTH)/resources/media/faces_I411.jpg',
            '<(DEPTH)/resources/media/faces_I400.jpg',
          ],
        }],
      ],
    },  # target rtc_media_unittests
  ],
}
