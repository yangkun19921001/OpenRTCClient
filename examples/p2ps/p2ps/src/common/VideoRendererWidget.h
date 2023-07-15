#ifndef VIDEORENDERERWIDGET_H
#define VIDEORENDERERWIDGET_H

//#include <GL/gl.h>

#include <QOpenGLFunctions>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLShaderProgram>

#include <QOpenGLTexture>

#include "../../../webrtc_headers.h"
#include <mutex>


#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4
namespace PCS {
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
    /**
     * 纹理是一个2D图片，它可以用来添加物体的细节（贴图），纹理可以各种变形后
     * 贴到不同形状的区域内。这里直接用纹理显示视频帧
     */
    GLuint textureUniformY; // y纹理数据位置
    GLuint textureUniformU; // u纹理数据位置
    GLuint textureUniformV; // v纹理数据位置
    GLuint id_y; // y纹理对象ID
    GLuint id_u; // u纹理对象ID
    GLuint id_v; // v纹理对象ID
    QOpenGLTexture *m_pTextureY;  // y纹理对象
    QOpenGLTexture *m_pTextureU;  // u纹理对象
    QOpenGLTexture *m_pTextureV;  // v纹理对象
    /* 着色器：控制GPU进行绘制 */
    QOpenGLShader *m_pVSHader;  // 顶点着色器程序对象
    QOpenGLShader *m_pFSHader;  // 片段着色器对象
    QOpenGLShaderProgram *m_pShaderProgram; // 着色器程序容器
    int m_nVideoW; // 视频分辨率宽
    int m_nVideoH; // 视频分辨率高
//    unsigned char *m_pBufYuv420p;
    std::unique_ptr<uint8_t[]> video_data_;
    std::mutex renderer_mutex_;
    std::string peer_id_;

public:
   webrtc::VideoTrackInterface*  video_track_;
};

}

#endif // VIDEORENDERERWIDGET_H
