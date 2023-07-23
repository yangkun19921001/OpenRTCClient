#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "modules/video_capture/video_capture_factory.h"
#include "api/video/i420_buffer.h"
#include "third_party/libyuv/include/libyuv/convert_argb.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stopBtn->setEnabled(false);
    on_updateDeviceBtn_clicked();

    connect(this, &MainWindow::recvFrame, this, &MainWindow::onRecvFrame, Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    CloseVideoCaptureDevice();
    delete ui;
}

void MainWindow::OnFrame(const webrtc::VideoFrame &frame)
{
    QMutexLocker locker(&mutex_);
    i420_buffer_ = frame.video_frame_buffer()->ToI420();

    //qDebug() << Q_FUNC_INFO << ">>>>>>>frame: " << frame.width() << frame.height() << frame.size();
    if (frame.rotation() != webrtc::kVideoRotation_0) {
        i420_buffer_ = webrtc::I420Buffer::Rotate(*i420_buffer_, frame.rotation());
    }

    Q_EMIT recvFrame();
}

void MainWindow::on_startBtn_clicked()
{
    OpenVideoCaptureDevice();
}

void MainWindow::on_stopBtn_clicked()
{
    CloseVideoCaptureDevice();
}

void MainWindow::OpenVideoCaptureDevice()
{
    const size_t kWidth = 1280;
    const size_t kHeight = 720;
    const size_t kFps = 30;
    const size_t kDeviceIndex = ui->deviceComBox->currentIndex();
    video_capturer_ = CameraCapturerTrackSource::Create(kWidth, kHeight, kFps, kDeviceIndex);
    if (video_capturer_) {
        video_capturer_->AddOrUpdateSink(this, rtc::VideoSinkWants());
        ui->startBtn->setEnabled(false);
        ui->stopBtn->setEnabled(true);
    } else {
        QMessageBox::warning(this,
                             tr("OpenCamera"),
                             tr("Open Video Capture Device Failed"),
                             QMessageBox::Ok);
    }
}

void MainWindow::CloseVideoCaptureDevice()
{
    if (video_capturer_) {
        video_capturer_->RemoveSink(this);
        video_capturer_.release();
        ui->startBtn->setEnabled(true);
        ui->stopBtn->setEnabled(false);
    }
}

void MainWindow::on_updateDeviceBtn_clicked()
{
    ui->deviceComBox->clear();
    // get device name
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
        webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) {
        return;
    }
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
        const uint32_t kSize = 256;
        char name[kSize] = {0};
        char id[kSize] = {0};
        if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) {
            ui->deviceComBox->addItem(name);
        }
    }
}

void MainWindow::onRecvFrame()
{
    QMutexLocker locker(&mutex_);
    webrtc::I420BufferInterface *buffer = i420_buffer_.get();
    if (buffer) {
        ui->videoWidget->setFrameSize(QSize(buffer->width(), buffer->height()));
        ui->videoWidget->updateTextures(buffer->DataY(), buffer->DataU(), buffer->DataV(),
                                        buffer->StrideY(), buffer->StrideU(), buffer->StrideV());
    }
}

