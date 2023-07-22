#ifndef VIDEORENDERERWIDGET_H
#define VIDEORENDERERWIDGET_H

//#include <GL/gl.h>

#include <QOpenGLFunctions>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLShaderProgram>

#include <QOpenGLTexture>

#include "../../../webrtc_headers.h"
#include <mutex>


#define A_VER 0
#define T_VER 1




namespace PCS {
//传递顶点和材质坐标
//顶点
static const GLfloat VER[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    -1.0f, 1.0f,
    1.0f, 1.0f
};

//材质
static const GLfloat TEX[] = {
    0.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f
};
class VideoRendererWidget : public QOpenGLWidget, protected QOpenGLFunctions,
                            public rtc::VideoSinkInterface<webrtc::VideoFrame>
{
    Q_OBJECT

public:
    VideoRendererWidget(std::string peerId ="",QWidget* parent = nullptr,webrtc::VideoTrackInterface *track =nullptr);
    ~VideoRendererWidget();

public Q_SLOTS:
    void PlayOneFrame();

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;



public:
    // VideoSinkInterface implementation
    void OnFrame(const webrtc::VideoFrame& frame) override;


private:
    //openg的 texture地址
    GLuint texs[3] = {0};
    QOpenGLShader*          mVShader;
    QOpenGLShader*          mFShader;
    QOpenGLShaderProgram*   mShaderProgram;

    GLuint                  id_y, id_u, id_v;
    int                     textureUniformY, textureUniformU, textureUniformV;
    int m_nVideoW; // 视频分辨率宽
    int m_nVideoH; // 视频分辨率高
    std::unique_ptr<uint8_t[]> video_data_;
    std::mutex renderer_mutex_;
    std::string peer_id_;

public:
   webrtc::VideoTrackInterface*  video_track_;
};

}

#endif // VIDEORENDERERWIDGET_H
