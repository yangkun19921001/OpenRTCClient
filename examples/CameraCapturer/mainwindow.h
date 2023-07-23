#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "CameraCapturerTrackSource.h"
#include <QMutex>
#include <QWaitCondition>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow , public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // VideoSinkInterface
    void OnFrame(const webrtc::VideoFrame& frame) override;

Q_SIGNALS:
    void recvFrame();

private Q_SLOTS:
    void on_startBtn_clicked();

    void on_stopBtn_clicked();

    void OpenVideoCaptureDevice();
    void CloseVideoCaptureDevice();

    void on_updateDeviceBtn_clicked();

    void onRecvFrame();

private:
    Ui::MainWindow *ui;

    rtc::scoped_refptr<CameraCapturerTrackSource> video_capturer_;

    QMutex mutex_;
    rtc::scoped_refptr<webrtc::I420BufferInterface> i420_buffer_;
};

#endif // MAINWINDOW_H
