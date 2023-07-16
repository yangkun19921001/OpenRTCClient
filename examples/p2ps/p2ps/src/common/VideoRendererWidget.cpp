#include "VideoRendererWidget.h"

#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QMouseEvent>
#include <QTimer>
#include "third_party/libyuv/include/libyuv/convert_argb.h"

#define GET_STR(x) #x
namespace PCS {


//顶点shader
const char *vString = GET_STR(
    attribute vec4 vertexIn;
    attribute vec2 textureIn;
    varying vec2 textureOut;
    void main(void) {
        gl_Position = vertexIn;
        textureOut = textureIn;
    }
    );


//片元shader
const char *fString = GET_STR(
    varying vec2 textureOut;
    uniform sampler2D tex_y;
    uniform sampler2D tex_u;
    uniform sampler2D tex_v;
    void main(void) {
        vec3 yuv;
        vec3 rgb;
        yuv.x = texture2D(tex_y, textureOut).r;
        yuv.y = texture2D(tex_u, textureOut).r - 0.5;
        yuv.z = texture2D(tex_v, textureOut).r - 0.5;
        rgb = mat3(1.0, 1.0, 1.0,
                   0.0, -0.39465, 2.03211,
                   1.13983, -0.58060, 0.0) * yuv;
        gl_FragColor = vec4(rgb, 1.0);
    }

    );

VideoRendererWidget::VideoRendererWidget(std::string peerId,QWidget *parent,webrtc::VideoTrackInterface* track)
    : QOpenGLWidget(parent)
    ,mVShader(nullptr)
    ,mFShader(nullptr)
    ,mShaderProgram(nullptr)
    ,video_track_(track)
    ,peer_id_(peerId)
    ,m_nVideoW(0)
    ,m_nVideoH(0)
    ,video_data_(nullptr),
    id_y(0), id_u(0), id_v(0),

    textureUniformY(0), textureUniformU(0), textureUniformV(0)
{

    if(video_track_ != nullptr)
    {
        video_track_->AddOrUpdateSink(this,rtc::VideoSinkWants());
    }

}



VideoRendererWidget::~VideoRendererWidget()
{
    if(video_track_)
        video_track_->RemoveSink(this);
    video_track_ = nullptr;

}

void VideoRendererWidget::PlayOneFrame()
{

    // 刷新界面,触发paintGL接口
    update();

    return;
}

void VideoRendererWidget::initializeGL()
{
    qDebug() << "initializeGL";
    //初始化 opengl
    initializeOpenGLFunctions();

    //启用深度测试
    //根据坐标的远近自动隐藏被遮住的图形（材料）
    glEnable(GL_DEPTH_TEST);

    //初始化顶点 shader obj
    mVShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    //编译顶点 shader program
    if (!mVShader->compileSourceCode(vString)) {
        RTC_LOG(LS_ERROR) << " compileSourceCode Vertex error";
    }

    //初始化片元 shader obj
    mFShader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    //编译片元 shader program
    if (!mFShader->compileSourceCode(fString)) {
        RTC_LOG(LS_ERROR) << " compileSourceCode Fragment error";
    }

    //创建 shader program 容器
    mShaderProgram = new QOpenGLShaderProgram(this);
    //将顶点 片元 shader 添加到程序容器中
    mShaderProgram->addShader(mFShader);
    mShaderProgram->addShader(mVShader);

    //设置顶点坐标的变量
    mShaderProgram->bindAttributeLocation("vertexIn", A_VER);

    //设置材质坐标
    mShaderProgram->bindAttributeLocation("textureIn", T_VER);

    //编译shader
    RTC_LOG(LS_INFO) << "program.link() = " << mShaderProgram->link();

    RTC_LOG(LS_INFO)  << "program.bind() = " << mShaderProgram->bind();

    //从shader获取材质
    textureUniformY = mShaderProgram->uniformLocation("tex_y");
    textureUniformU = mShaderProgram->uniformLocation("tex_u");
    textureUniformV = mShaderProgram->uniformLocation("tex_v");

    //顶点
    glVertexAttribPointer(A_VER, 2, GL_FLOAT, 0, 0, VER);
    glEnableVertexAttribArray(A_VER);

    //材质
    glVertexAttribPointer(T_VER, 2, GL_FLOAT, 0, 0, TEX);
    glEnableVertexAttribArray(T_VER);


    //创建 y,u,v 纹理 id
    glGenTextures(3, texs);
    id_y = texs[0];
    id_u = texs[1];
    id_v = texs[2];
    glClearColor(0.0, 0.0, 0.0, 0.0);


}
void VideoRendererWidget::resizeGL(int w, int h)
{
    if (h == 0) // 防止被零除
    {
        h = 1;  // 将高设为1
    }

    // 设置视口
    glViewport(0, 0, w, h);
}

void VideoRendererWidget::paintGL()
{
    std::lock_guard<std::mutex> guard(renderer_mutex_);
    glActiveTexture(GL_TEXTURE0);//激活纹理单元GL_TEXTURE0,系统里面的
    glBindTexture(GL_TEXTURE_2D, id_y); //绑定y分量纹理对象id到激活的纹理单元

    // 使用内存中video_data_数据创建真正的y数据纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_nVideoW, m_nVideoH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, video_data_.get());
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 加载u数据纹理
    glActiveTexture(GL_TEXTURE1); // 激活纹理单元GL_TEXTURE1
    glBindTexture(GL_TEXTURE_2D, id_u);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_nVideoW / 2, m_nVideoH / 2, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, (char *)video_data_.get() + m_nVideoW * m_nVideoH);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 加载v数据纹理
    glActiveTexture(GL_TEXTURE2); // 激活纹理单元GL_TEXTURE2
    glBindTexture(GL_TEXTURE_2D, id_v);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_nVideoW / 2, m_nVideoH / 2, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, (char *)video_data_.get() + m_nVideoW * m_nVideoH * 5 / 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //与shader uni遍历关联
    //指定y纹理要使用新值,只能用0,1,2等表示纹理单元的索引
    glUniform1i(textureUniformY, 0);
    glUniform1i(textureUniformU, 1);
    glUniform1i(textureUniformV, 2);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // 使用顶点数组方式绘制图形
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    return;
}

void VideoRendererWidget::OnFrame(const webrtc::VideoFrame &video_frame)
{
    std::lock_guard<std::mutex> guard(renderer_mutex_);
    rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
        video_frame.video_frame_buffer()->ToI420());
    if (video_frame.rotation() != webrtc::kVideoRotation_0) {
        buffer = webrtc::I420Buffer::Rotate(*buffer, video_frame.rotation());
    }

    int width = buffer->width();
    int height = buffer->height();

    int ySize = width * height;
    int uvSize = ySize / 4;  // For each U and V component

    if(m_nVideoW == 0 && m_nVideoH ==0)
    {
       if(!peer_id_.empty())  RTC_LOG(LS_INFO) << __FUNCTION__ << " init w:"<<width<<" height:"<<height << " peerId:" <<peer_id_;
    }

    if(width != m_nVideoW && height != m_nVideoH && video_data_ != nullptr)
    {
         video_data_ .reset();

        if(!peer_id_.empty()) RTC_LOG(LS_INFO) << __FUNCTION__ << " change w:"<<width<<" height:"<<height << " peerId:" <<peer_id_;

    }

    if(video_data_ == nullptr){
      video_data_ = std::make_unique<uint8_t[]>(width * height * 1.5); // Use make_unique to allocate array
      if(!peer_id_.empty())
      RTC_LOG(LS_INFO) << __FUNCTION__ << " malloc Id:"<<peer_id_<<" width:" << width <<" height:"<<height;
    }


    int strideY = buffer->StrideY();
    int strideU = buffer->StrideU();
    int strideV = buffer->StrideV();
  if(!peer_id_.empty()) RTC_LOG(LS_INFO) << __FUNCTION__ << " Id"<<peer_id_<< " width:" << width <<" height:"<<height
                       << " strideY:"<<strideY
          << " strideU:"<<strideU
          << " strideV:"<<strideV
          ;

    memcpy(video_data_.get(), buffer->DataY(), ySize);
    memcpy(video_data_.get() + ySize, buffer->DataU(), uvSize);
    memcpy(video_data_.get() + ySize + uvSize, buffer->DataV(), uvSize);
    m_nVideoW = width;
    m_nVideoH = height;


    // 刷新界面,触发paintGL接口
    Q_EMIT PlayOneFrame();

}
}
