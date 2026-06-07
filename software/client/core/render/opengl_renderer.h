#ifndef OPENGL_RENDERER_H
#define OPENGL_RENDERER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include "core/common/frame.h"
#include <QMutex>

class OpenGLRenderer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit OpenGLRenderer(QWidget* parent = nullptr);
    ~OpenGLRenderer();

public slots:
    void updateFrame(const FrameData& frame);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    QMutex m_mutex;            // #include <QMutex> 别忘了加在头文件顶部
    FrameData m_currentFrame;  // 用来暂存子线程丢过来的最新一帧数据
    void initShaders();
    void initTextures(int width, int height);  // 改为带参数
    void updateTextures(const FrameData& frame);
    void cleanup();
    QString loadShaderSource(const QString& path);

    QOpenGLShaderProgram* m_program;
    QOpenGLTexture* m_textures[3];
    int m_videoWidth;
    int m_videoHeight;
    bool m_textureReady;
};

#endif