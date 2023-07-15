#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <iostream>
#include <string>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    ,rtc_room_manager_(std::make_unique<PCS::RTCRoomManager>())

{
    rtc::LogMessage::SetLogToStderr(false);
    ui->setupUi(this);

//#define TEST_YUV_RENDERER
#ifdef TEST_YUV_RENDERER
    for (int var = 0; var < 4; ++var) {
        addVideoRendererWidgetToMainWindow(std::to_string(var));
    }
#endif

    this->resize(1280, 720); // 设置窗口大小
    //临时解决方案，避免窗口闪烁
    addVideoRendererWidgetToMainWindow("devyk",std::make_unique<PCS::VideoRendererWidget>());
    auto it = video_renderer_widgets_.find("devyk");
    if (it != video_renderer_widgets_.end())
    {
        it->second->hide();
    }
    connect(this, &MainWindow::requestGUIUpdate, this, &MainWindow::on_main_thread_message);
}

MainWindow::~MainWindow()
{
    rtc_room_manager_->setLocalTrackCallback(nullptr);
    rtc_room_manager_->release();
    delete ui;
}


void MainWindow::on_connect_clicked()
{
    RTC_LOG(LS_INFO) << __FUNCTION__ ;

    try {
       QLineEdit* serverInfoEdit  = this->findChild<QLineEdit*>("serverInfo");
        if (!serverInfoEdit) {
            throw std::runtime_error("serverInfo is not initialized");
        }
        QString serverInfo = serverInfoEdit->text();

        if(serverInfo.length()>0)
        {
            QByteArray bytes = serverInfo.toUtf8();
            std::string msg(bytes.data(),bytes.length());

            std::string url = msg;
            RTC_LOG(LS_INFO) << __FUNCTION__ << " url:"<<url ;
            if (!rtc_room_manager_) {
                throw std::runtime_error("rtc_room_manager_ is not initialized");
            }
            rtc_room_manager_->connect(url, this);
        }
    } catch (const std::exception& e) {
        RTC_LOG(LS_ERROR) << "Failed to click on connect: " << e.what();
    } catch (...) {
        RTC_LOG(LS_ERROR) << "Failed to click on connect: unknown error";
    }
}

void MainWindow::on_exit_clicked()
{
    // 执行你想要在点击 "exit" 按钮时进行的操作
    RTC_LOG(LS_INFO) <<"on_exit_clicked";

    if (!rtc_room_manager_) {
        RTC_LOG(LS_ERROR) <<"rtc_room_manager_ is not initialized";
       return;
    }
    QLineEdit* roomIdEdit  = this->findChild<QLineEdit*>("roomId");
    QString text = roomIdEdit->text();
    if(text.length()>0)
    {
       QByteArray bytes = text.toUtf8();
       std::string msg(bytes.data(),bytes.length());
       rtc_room_manager_->leave(msg);

    }
}


void MainWindow::notifyMainUI(PCS::Message msg)
{
    RTC_LOG(LS_INFO) <<__FUNCTION__ << " type:"<<(int)msg.what;
    Q_EMIT requestGUIUpdate(msg);
}

void MainWindow::on_main_thread_message(PCS::Message msg)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " what:"<<(int)msg.what;
    rtc_room_manager_->onUIMessage(msg);

}



//当连接中
void  MainWindow::onConnecting(){
    RTC_LOG(LS_INFO) <<__FUNCTION__;
};
//当连接上
void MainWindow::onConnected() {
    try {
        RTC_LOG(LS_INFO) <<__FUNCTION__;

        if (!rtc_room_manager_) {
            throw std::runtime_error("rtc_room_manager_ is not initialized");
        }
        QLineEdit* roomIdEdit  = this->findChild<QLineEdit*>("roomId");

        if (!roomIdEdit) {
            throw std::runtime_error("UI or roomId is not initialized");
        }

        QString text = roomIdEdit->text();
        if(text.length()>0)
        {
            QByteArray bytes = text.toUtf8();
            std::string msg(bytes.data(),bytes.length());
            rtc_room_manager_->setLocalTrackCallback(std::bind(&MainWindow::onLocalVideoTrack, this
                                                               , std::placeholders::_1
                                                               , std::placeholders::_2
                                                               , std::placeholders::_3
                                                               , std::placeholders::_4
                                                               ));
            rtc_room_manager_->join(msg);
        }
    } catch (const std::exception& e) {
        RTC_LOG(LS_ERROR) << "Failed to execute onConnected: " << e.what();
    } catch (...) {
        RTC_LOG(LS_ERROR) << "Failed to execute onConnected: unknown error";
    }
}

//当连接失败
void  MainWindow::onError(const std::string error) {
     RTC_LOG(LS_INFO) <<__FUNCTION__ << " "<< error;
};
//当自己加入
void  MainWindow::onJoined(const std::string id) {
     RTC_LOG(LS_INFO) <<__FUNCTION__ << " id:"<< id;

};
//当用户加入
void  MainWindow::onUserJoined(const std::string id) {
     RTC_LOG(LS_INFO) <<__FUNCTION__ << " id:"<< id;

};
//当离开房间
void MainWindow::onLeaved(const std::string id){
     RTC_LOG(LS_INFO) <<__FUNCTION__ << " id:"<< id;
     removeVideoRendererWidgetFromMainWindow(id);
     if(video_renderer_widgets_.empty())
     {
        exit();
     }

};
void MainWindow::onAddTrack(std::string peerId,void * track) {
     RTC_LOG(LS_INFO) <<__FUNCTION__ ;
     addVideoRendererWidgetToMainWindow(peerId,std::make_unique<PCS::VideoRendererWidget>(peerId,nullptr,static_cast<webrtc::VideoTrackInterface*>(track)));


};
     //当接收到远端的轨道
void MainWindow::onRemoveTrack(std::string peerId,void * track){
     RTC_LOG(LS_INFO) <<__FUNCTION__ ;
     removeVideoRendererWidgetFromMainWindow(peerId);
}



void MainWindow::onLocalVideoTrack(std::string localPeerId,int width,int height,rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack)
{
    removeVideoRendererWidgetFromMainWindow("devyk");
    addVideoRendererWidgetToMainWindow(localPeerId,std::make_unique<PCS::VideoRendererWidget>("",nullptr,videoTrack));
}

void MainWindow::addVideoRendererWidgetToMainWindow(std::string id, std::unique_ptr<PCS::VideoRendererWidget> renderer)
{

     const int itemsPerRow = 3;
     std::unique_ptr<PCS::VideoRendererWidget> videoRenderer;
     if(renderer == nullptr)
        videoRenderer =  std::make_unique<PCS::VideoRendererWidget>();
     else {
        videoRenderer = std::move(renderer);
     }
     videoRenderer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
     // 计算新的位置
     int count = ui->gridLayout->count();
     int row = count / itemsPerRow;
     int column = count % itemsPerRow;

     // 添加到 gridLayout 中
     ui->gridLayout->addWidget(videoRenderer.get(),row,column);
     //videoRenderer->setFixedSize(1280/3,720/3);
     // 将 widget 添加到 map 中
     video_renderer_widgets_.insert({id, std::move(videoRenderer)});


}

void MainWindow::removeVideoRendererWidgetFromMainWindow(std::string id)
    {
     // 从 layout 中移除 widget，并删除它
     // 查找 widget
     auto it = video_renderer_widgets_.find(id);
     if (it != video_renderer_widgets_.end())
     {
        // 从 layout 中移除 widget
        ui->gridLayout->removeWidget(it->second.get());

        // 从 map 中移除并删除 widget
        video_renderer_widgets_.erase(it);
     }
    }

    void MainWindow::exit()
    {
//     for (const auto& pair : video_renderer_widgets_) {
//        std::string key = pair.first;
//        removeVideoRendererWidgetFromMainWindow(key);
//     }

     PCS::Message msg;
     msg.what = PCS::RTCMainEvent::ON_RELEASE;
     notifyMainUI(msg);
    };


