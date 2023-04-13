# WebRTC Android Studio project

A reference gradle project that let you explore WebRTC Android in Android Studio.

## Debug native code in Android Studio

配置 debug

1. 配置 build.gradle

   ``` groovy
   android {
   
     buildTypes {
           release {
               // 保留符号
               minifyEnabled false
               debuggable true
               jniDebuggable true
               proguardFiles getDefaultProguardFile('proguard-android.txt'), 'proguard-rules.pro'
           }
           debug{
               debuggable true     // 必须设置为 true
               jniDebuggable true  // 必须设置为 true
               minifyEnabled false
               proguardFiles getDefaultProguardFile('proguard-android.txt'), 'proguard-rules.pro'
   
           }
       }
   
       packagingOptions {
           // 如果不设置 doNotStrip，编译出来的安装包还是会丢失调试信息；
           // 因为我们只编译了 arm64-v8a 架构的包，所以只需要配置这一行即可
           doNotStrip "*/arm64-v8a/*.so"
       }
   }
   ```


进入 Edit Configurations -> Debugger
1. 配置描述符号-Symbol Dirrectories

   ```
   //替换成你自己项目的路径
   /Users/devyk/Data/webrtc/OpenRTCClient/examples/android_gradle/rtc/build/intermediates/cmake/debug/obj/arm64-v8a
   ```



2. 配置 LLDB Startup Commands（通过带描述符号的 so 查看）

   ```
   #通过查看 apk 中 so webrtc 源码路径，将 ../../../../webrtc  替换为真实路径
   settings set target.source-map ../../../../webrtc /Users/devyk/Data/webrtc/OpenRTCClient/webrtc
   ```

## WebRTC src extractor

`python3 webrtc_src_extractor.py <repo dir> <dst dir> <wanted src file, seperated by space>`

If you only want use a small part of WebRTC code, this script could help you find all related sources and headers, and copy them into `dst dir`. Note that it's just a best effort script, you may still need copy some files manually.


## 编译失败问题记录
### FileNotFoundError
```shell


File "/Users/devyk/Data/webrtc/OpenRTCClient/webrtc/build/android/gyp/util/server_utils.py", line 31, in MaybeRunCommand
sock.connect(SOCKET_ADDRESS)
FileNotFoundError: [Errno 2] No such file or directory

```

解决办法

```shell
cd OpenRTCClient
执行
python3 webrtc/build/android/fast_local_dev_server.py
再次执行编译函数

```




   

