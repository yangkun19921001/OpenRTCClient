#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "src/common/RTCRoomManager.h"
#include "src/common/VideoRendererWidget.h"
#include "src/common/Message.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow ,public PCS::OnRoomStateChangeCallback
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //################## OnRoomStateChangeCallback ##################
    //当连接中
    virtual void onConnecting()override;
    //当连接上
    virtual void onConnected() override;
    //当连接失败
    virtual void onError(const std::string error) override;
    //当自己加入
    virtual void onJoined(const std::string id) override;
        //当用户加入
    virtual void onUserJoined(const std::string id) override;
    //当离开房间
    virtual void onLeaved(const std::string id)override;
    virtual void onAddTrack(std::string peerId,void * track) override;
        //当接收到远端的轨道
    virtual void onRemoveTrack(std::string peerId,void * track) override;
    virtual void notifyMainUI(PCS::Message msg)override;

private:
    void onLocalVideoTrack(std::string,int,int,rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack);
    void addVideoRendererWidgetToMainWindow(std::string id);
    void removeVideoRendererWidgetFromMainWindow(std::string id);

private:
    Ui::MainWindow *ui;
    std::unique_ptr<PCS::RTCRoomManager> rtc_room_manager_;
    std::map<std::string, std::unique_ptr<PCS::VideoRendererWidget>> video_renderer_widgets_;
    std::unique_ptr<rtc::Thread> testTread_;
//https://www.jianshu.com/p/67dde91e6f1f
private Q_SLOTS:
    void on_connect_clicked();
    void on_exit_clicked();
    void on_mute_clicked();
    void on_main_thread_message(PCS::Message msg);
Q_SIGNALS:
    void requestGUIUpdate(PCS::Message msg);

};
#endif // MAINWINDOW_H
