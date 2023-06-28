echo off

::该脚本必须放在webrtc源码src目录的上一级目录

:: 创建目标目录
mkdir include

::定义源目录
set srcpath=.\webrtc
::定义目标路径
set dstpath=.\include

:: /s 复制目录和子目录，除了空的。
:: /e 复制目录和子目录，包括空的。 
:: /y 禁止提示以确认改写一个现存目标文件。
:: /c 即使有错误也继续执行
:: /h 也复杂隐藏文件和系统文件
:: /r 覆盖只读文件
xcopy %srcpath%\*.h %dstpath%\ /s /e /c /y /h /r

pause