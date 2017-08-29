# This file contains dependencies for WebRTC.

vars = {
  'chromium_git': 'https://chromium.googlesource.com',
  'chromium_revision': '84d100a1fbf2bbc65207ae73d7622f2b50610a6f',
  'boringssl_git': 'https://boringssl.googlesource.com',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling swarming_client
  # and whatever else without interference from each other.
  'swarming_revision': '42721e128da760b345ab60d7cf34e300269112d7',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling openmax_dl
  # and whatever else without interference from each other.
  'openmax_dl_revision': '7acede9c039ea5d14cf326f44aad1245b9e674a7',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling BoringSSL
  # and whatever else without interference from each other.
  'boringssl_revision': 'ae9f0616c58bddcbe7a6d80d29d796bee9aaff2e',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling lss
  # and whatever else without interference from each other.
  'lss_revision': '63f24c8221a229f677d26ebe8f3d1528a9d787ac',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling catapult
  # and whatever else without interference from each other.
  'catapult_revision': 'eebaedf9bcacd089d8561842ce75b64ebf71a26b',
  # Three lines of non-changing comments so that
  # the commit queue can handle CLs rolling libFuzzer
  # and whatever else without interference from each other.
  'libfuzzer_revision': '6d39c6ba7f52b61664c982d6ac89d7491b6f8730',
}
deps = {
  # TODO(kjellander): Move this to be Android-only once the libevent dependency
  # in base/third_party/libevent is solved.
  'src/base':
    Var('chromium_git') + '/chromium/src/base' + '@' + '0563e6ee794c62c6bc78107ddd426de9a4a4157f',
  'src/build':
    Var('chromium_git') + '/chromium/src/build' + '@' + 'ba6bd88e275ff783692ff833e30ed7c9a0a52ed1',
  'src/buildtools':
    Var('chromium_git') + '/chromium/buildtools.git' + '@' + '5af0a3a8b89827a8634132080a39ab4b63dee489',
  'src/testing':
    Var('chromium_git') + '/chromium/src/testing' + '@' + '316d4522a84ea061f59906f6df68c8a82daa5bac',
  'src/third_party':
    Var('chromium_git') + '/chromium/src/third_party' + '@' + 'd4584cc32c571a3a0278934939e5fdee3db2de02',
  'src/third_party/boringssl/src':
   Var('boringssl_git') + '/boringssl.git' + '@' +  Var('boringssl_revision'),
  'src/third_party/catapult':
   Var('chromium_git') + '/external/github.com/catapult-project/catapult.git' + '@' + Var('catapult_revision'),
  'src/third_party/colorama/src':
    Var('chromium_git') + '/external/colorama.git' + '@' + '799604a1041e9b3bc5d2789ecbd7e8db2e18e6b8',
  'src/third_party/depot_tools':
    Var('chromium_git') + '/chromium/tools/depot_tools.git' + '@' + 'e081cbe5aae6b131955b6c3bd7308d95d650948f',
  'src/third_party/ffmpeg':
    Var('chromium_git') + '/chromium/third_party/ffmpeg.git' + '@' + '1e201feaa3260336aa63545c9471b76e5aef2e0a',
  'src/third_party/googletest/src':
    Var('chromium_git') + '/external/github.com/google/googletest.git' + '@' + '42bc671f47b122fad36db5eccbc06868afdf7862',
  'src/third_party/jsoncpp/source':
    Var('chromium_git') + '/external/github.com/open-source-parsers/jsoncpp.git' + '@' + 'f572e8e42e22cfcf5ab0aea26574f408943edfa4', # from svn 248
  # Used for building libFuzzers (only supports Linux).
  'src/third_party/libFuzzer/src':
    Var('chromium_git') + '/chromium/llvm-project/llvm/lib/Fuzzer.git' + '@' +  Var('libfuzzer_revision'),
  'src/third_party/libjpeg_turbo':
    Var('chromium_git') + '/chromium/deps/libjpeg_turbo.git' + '@' + 'a1750dbc79a8792dde3d3f7d7d8ac28ba01ac9dd',
  'src/third_party/libsrtp':
   Var('chromium_git') + '/chromium/deps/libsrtp.git' + '@' + '1d45b8e599dc2db6ea3ae22dbc94a8c504652423',
  'src/third_party/libvpx/source/libvpx':
    Var('chromium_git') + '/webm/libvpx.git' + '@' +  '30c261b1ebe8f06d687cac5b3b442d51a7839d00',
  'src/third_party/libyuv':
    Var('chromium_git') + '/libyuv/libyuv.git' + '@' + '27036e33e86c9ce3b5087d55c18bf04964343c60',
  'src/third_party/openh264/src':
    Var('chromium_git') + '/external/github.com/cisco/openh264' + '@' + '0fd88df93c5dcaf858c57eb7892bd27763f0f0ac',
  'src/third_party/openmax_dl':
    Var('chromium_git') + '/external/webrtc/deps/third_party/openmax.git' + '@' +  Var('openmax_dl_revision'),
  'src/third_party/usrsctp/usrsctplib':
    Var('chromium_git') + '/external/github.com/sctplab/usrsctp' + '@' + 'f4819e1b177f7bfdd761c147f5a649b9f1a78c06',
  'src/third_party/yasm/source/patched-yasm':
    Var('chromium_git') + '/chromium/deps/yasm/patched-yasm.git' + '@' + 'b98114e18d8b9b84586b10d24353ab8616d4c5fc',
  'src/tools':
    Var('chromium_git') + '/chromium/src/tools' + '@' + 'e0d76b62239b1849bc99c97f7f4ea3e7dde5f7e1',
  'src/tools/gyp':
    Var('chromium_git') + '/external/gyp.git' + '@' + 'd61a9397e668fa9843c4aa7da9e79460fe590bfb',
   'src/tools/swarming_client':
     Var('chromium_git') + '/external/swarming.client.git' + '@' +  Var('swarming_revision'),
  # WebRTC-only dependencies (not present in Chromium).
  'src/third_party/gflags':
    Var('chromium_git') + '/external/webrtc/deps/third_party/gflags' + '@' + '892576179b45861b53e04a112996a738309cf364',
  'src/third_party/gflags/src':
    Var('chromium_git') + '/external/github.com/gflags/gflags' + '@' + '03bebcb065c83beff83d50ae025a55a4bf94dfca',
  'src/third_party/gtest-parallel':
    Var('chromium_git') + '/external/github.com/google/gtest-parallel' + '@' + 'd80ab5d82cc76fb51c4deea2fa34113be69d9af2',
}
deps_os = {
  'android': {
    'src/third_party/android_tools':
      Var('chromium_git') + '/android_tools.git' + '@' + 'e9d4018e149d50172ed462a7c21137aa915940ec',
    'src/third_party/ced/src':
      Var('chromium_git') + '/external/github.com/google/compact_enc_det.git' + '@' + '910cca22d881b02cbc8950fa02ccbcdcfb782456',
    'src/third_party/icu':
      Var('chromium_git') + '/chromium/deps/icu.git' + '@' + '08cb956852a5ccdba7f9c941728bb833529ba3c6',
    'src/third_party/jsr-305/src':
      Var('chromium_git') + '/external/jsr-305.git' + '@' + '642c508235471f7220af6d5df2d3210e3bfc0919',
    'src/third_party/junit/src':
      Var('chromium_git') + '/external/junit.git' + '@' + '64155f8a9babcfcf4263cf4d08253a1556e75481',
    'src/third_party/lss':
      Var('chromium_git') + '/linux-syscall-support.git' + '@' + Var('lss_revision'),
    'src/third_party/mockito/src':
      Var('chromium_git') + '/external/mockito/mockito.git' + '@' + 'de83ad4598ad4cf5ea53c69a8a8053780b04b850',
    'src/third_party/requests/src':
      Var('chromium_git') + '/external/github.com/kennethreitz/requests.git' + '@' + 'f172b30356d821d180fa4ecfa3e71c7274a32de4',
    'src/third_party/robolectric/robolectric':
      Var('chromium_git') + '/external/robolectric.git' + '@' + '0ccaf33cf6a6d98cbef7d0f3604fda45b13cce54',
    'src/third_party/ub-uiautomator/lib':
      Var('chromium_git') + '/chromium/third_party/ub-uiautomator.git' + '@' + '00270549ce3161ae72ceb24712618ea28b4f9434',
    # Gradle 3.5.0. Used for testing Android Studio project generation for WebRTC.
    'src/webrtc/examples/androidtests/third_party/gradle':
      Var('chromium_git') + '/external/github.com/gradle/gradle.git' + '@' +
      '941559e020f6c357ebb08d5c67acdb858a3defc2',
  },
  'ios': {
    'src/ios':
      Var('chromium_git') + '/chromium/src/ios' + '@' + 'c59f325bf1959923b19d09c43585a9db807e3f1e',
  },
  'unix': {
    'src/third_party/lss':
      Var('chromium_git') + '/linux-syscall-support.git' + '@' + Var('lss_revision'),
  },
  'win': {
    # Dependencies used by libjpeg-turbo
    'src/third_party/yasm/binaries':
      Var('chromium_git') + '/chromium/deps/yasm/binaries.git' + '@' + '52f9b3f4b0aa06da24ef8b123058bb61ee468881',
    # WebRTC-only dependency (not present in Chromium).
    'src/third_party/winsdk_samples':
      Var('chromium_git') + '/external/webrtc/deps/third_party/winsdk_samples_v71' + '@' + '2d31a1cbecc86359e6ec041fb9ff6c082babd073',
  },
}

hooks = [
  {
    # This clobbers when necessary (based on get_landmines.py). It should be
    # an early hook but it will need to be run after syncing Chromium and
    # setting up the links, so the script actually exists.
    'name': 'landmines',
    'pattern': '.',
    'action': [
        'python',
        'src/build/landmines.py',
        '--landmine-scripts',
        'src/tools_webrtc/get_landmines.py',
        '--src-dir',
        'src',
    ],
  },
  {
    # Ensure that the DEPS'd "depot_tools" has its self-update capability
    # disabled.
    'name': 'disable_depot_tools_selfupdate',
    'pattern': '.',
    'action': [
        'python',
        'src/third_party/depot_tools/update_depot_tools_toggle.py',
        '--disable',
    ],
  },
  {
    # Downloads the current stable linux sysroot to build/linux/ if needed.
    # This sysroot updates at about the same rate that the chrome build deps
    # change. This script is a no-op except for linux users who are doing
    # official chrome builds or cross compiling.
    'name': 'sysroot',
    'pattern': '.',
    'action': ['python', 'src/build/linux/sysroot_scripts/install-sysroot.py',
               '--running-as-hook'],
  },
  {
    # Update the Windows toolchain if necessary.
    'name': 'win_toolchain',
    'pattern': '.',
    'action': ['python', 'src/build/vs_toolchain.py', 'update'],
  },
  # Pull binutils for linux, enabled debug fission for faster linking /
  # debugging when used with clang on Ubuntu Precise.
  # https://code.google.com/p/chromium/issues/detail?id=352046
  {
    'name': 'binutils',
    'pattern': 'src/third_party/binutils',
    'action': [
        'python',
        'src/third_party/binutils/download.py',
    ],
  },
  {
    # Pull clang if needed or requested via GYP_DEFINES.
    # Note: On Win, this should run after win_toolchain, as it may use it.
    'name': 'clang',
    'pattern': '.',
    'action': ['python', 'src/tools/clang/scripts/update.py', '--if-needed'],
  },
  {
    # Update LASTCHANGE.
    'name': 'lastchange',
    'pattern': '.',
    'action': ['python', 'src/build/util/lastchange.py',
               '-o', 'src/build/util/LASTCHANGE'],
  },
  # Pull GN binaries.
  {
    'name': 'gn_win',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=win32',
                '--no_auth',
                '--bucket', 'chromium-gn',
                '-s', 'src/buildtools/win/gn.exe.sha1',
    ],
  },
  {
    'name': 'gn_mac',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'chromium-gn',
                '-s', 'src/buildtools/mac/gn.sha1',
    ],
  },
  {
    'name': 'gn_linux64',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'chromium-gn',
                '-s', 'src/buildtools/linux64/gn.sha1',
    ],
  },
  # Pull clang-format binaries using checked-in hashes.
  {
    'name': 'clang_format_win',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=win32',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'src/buildtools/win/clang-format.exe.sha1',
    ],
  },
  {
    'name': 'clang_format_mac',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'src/buildtools/mac/clang-format.sha1',
    ],
  },
  {
    'name': 'clang_format_linux',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'src/buildtools/linux64/clang-format.sha1',
    ],
  },
  # Pull luci-go binaries (isolate, swarming) using checked-in hashes.
  {
    'name': 'luci-go_win',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=win32',
                '--no_auth',
                '--bucket', 'chromium-luci',
                '-d', 'src/tools/luci-go/win64',
    ],
  },
  {
    'name': 'luci-go_mac',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'chromium-luci',
                '-d', 'src/tools/luci-go/mac64',
    ],
  },
  {
    'name': 'luci-go_linux',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'chromium-luci',
                '-d', 'src/tools/luci-go/linux64',
    ],
  },
  # Pull the Syzygy binaries, used for optimization and instrumentation.
  {
    'name': 'syzygy-binaries',
    'pattern': '.',
    'action': ['python',
               'src/build/get_syzygy_binaries.py',
               '--output-dir=src/third_party/syzygy/binaries',
               '--revision=a8456d9248a126881dcfb8707ca7dcdae56e1ac7',
               '--overwrite',
    ],
  },
  {
    # Pull sanitizer-instrumented third-party libraries if requested via
    # GYP_DEFINES.
    # See src/third_party/instrumented_libraries/scripts/download_binaries.py.
    # TODO(kjellander): Update comment when GYP is completely cleaned up.
    'name': 'instrumented_libraries',
    'pattern': '\\.sha1',
    'action': ['python', 'src/third_party/instrumented_libraries/scripts/download_binaries.py'],
  },
  {
    # Download test resources, i.e. video and audio files from Google Storage.
    'pattern': '.',
    'action': ['download_from_google_storage',
               '--directory',
               '--recursive',
               '--num_threads=10',
               '--no_auth',
               '--quiet',
               '--bucket', 'chromium-webrtc-resources',
               'src/resources'],
  },
]

# Note: These are keyed off target os, not host os. So don't move things here
# that depend on the target os.
hooks_os = {
  'android': [
    # Android dependencies. Many are downloaded using Google Storage these days.
    # They're copied from https://cs.chromium.org/chromium/src/DEPS for all
    # such dependencies we share with Chromium.
    {
      # This downloads SDK extras and puts them in the
      # third_party/android_tools/sdk/extras directory.
      'name': 'sdkextras',
      'pattern': '.',
      # When adding a new sdk extras package to download, add the package
      # directory and zip file to .gitignore in third_party/android_tools.
      'action': ['python',
                 'src/build/android/play_services/update.py',
                 'download'
      ],
    },
    {
      'name': 'intellij',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-intellij',
                 '-l', 'third_party/intellij'
      ],
    },
    {
      'name': 'javax_inject',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-javax-inject',
                 '-l', 'third_party/javax_inject'
      ],
    },
    {
      'name': 'hamcrest',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-hamcrest',
                 '-l', 'third_party/hamcrest'
      ],
    },
    {
      'name': 'guava',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-guava',
                 '-l', 'third_party/guava'
      ],
    },
    {
      'name': 'android_support_test_runner',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-android-support-test-runner',
                 '-l', 'third_party/android_support_test_runner'
      ],
    },
    {
      'name': 'byte_buddy',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-byte-buddy',
                 '-l', 'third_party/byte_buddy'
      ],
    },
    {
      'name': 'espresso',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-espresso',
                 '-l', 'third_party/espresso'
      ],
    },
    {
      'name': 'robolectric_libs',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-robolectric',
                 '-l', 'third_party/robolectric'
      ],
    },
    {
      'name': 'apache_velocity',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-apache-velocity',
                 '-l', 'third_party/apache_velocity'
      ],
    },
    {
      'name': 'ow2_asm',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-ow2-asm',
                 '-l', 'third_party/ow2_asm'
      ],
    },
    {
      'name': 'desugar',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-android-tools/bazel/desugar',
                 '-l', 'third_party/bazel/desugar'
      ],
    },
    {
      'name': 'icu4j',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-icu4j',
                 '-l', 'third_party/icu4j'
      ],
    },
    {
      'name': 'accessibility_test_framework',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-accessibility-test-framework',
                 '-l', 'third_party/accessibility_test_framework'
      ],
    },
    {
      'name': 'bouncycastle',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-bouncycastle',
                 '-l', 'third_party/bouncycastle'
      ],
    },
    {
      'name': 'sqlite4java',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-sqlite4java',
                 '-l', 'third_party/sqlite4java'
      ],
    },
    {
      'name': 'xstream',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-robolectric',
                 '-l', 'third_party/xstream'
      ],
    },
    {
      'name': 'objenesis',
      'pattern': '.',
      'action': ['python',
                 'src/build/android/update_deps/update_third_party_deps.py',
                 'download',
                 '-b', 'chromium-objenesis',
                 '-l', 'third_party/objenesis'
      ],
    },
  ],
}
recursedeps = [
  # buildtools provides clang_format, libc++, and libc++abi.
  'src/buildtools',
  # android_tools manages the NDK.
  'src/third_party/android_tools',
]
