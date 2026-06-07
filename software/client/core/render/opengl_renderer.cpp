#include "opengl_renderer.h"
#include <QDebug>
#include "core/common/logger.h"
#include <QOpenGLPixelTransferOptions>
#include <QFile>
#include <QTextStream>

static const char* vertexShaderSource =
    "attribute vec4 vertexIn;\n"
    "attribute vec2 textureIn;\n"
    "varying vec2 textureOut;\n"
    "void main(void) {\n"
    "    gl_Position = vertexIn;\n"
    "    textureOut = textureIn;\n"
    "}\n";

static const char* fragmentShaderSource =
    "uniform sampler2D tex_y;\n"
    "uniform sampler2D tex_u;\n"
    "uniform sampler2D tex_v;\n"
    "varying vec2 textureOut;\n"
    "void main(void) {\n"
    "    float y = texture2D(tex_y, textureOut).r;\n"
    "    float u = texture2D(tex_u, textureOut).r - 0.5;\n"
    "    float v = texture2D(tex_v, textureOut).r - 0.5;\n"
    "    vec3 rgb;\n"
    "    rgb.r = y + 1.402 * v;\n"
    "    rgb.g = y - 0.344 * u - 0.714 * v;\n"
    "    rgb.b = y + 1.772 * u;\n"
    "    gl_FragColor = vec4(rgb, 1.0);\n"
    "}\n";

OpenGLRenderer::OpenGLRenderer(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_program(nullptr)
    , m_textureReady(false)
    , m_videoWidth(0)
    , m_videoHeight(0)
{
    for (int i = 0; i < 3; ++i) {
        m_textures[i] = nullptr;
    }
}

OpenGLRenderer::~OpenGLRenderer()
{
    makeCurrent();
    cleanup();
    doneCurrent();
}

void OpenGLRenderer::cleanup()
{
    if (m_program) {
        delete m_program;
        m_program = nullptr;
    }
    for (int i = 0; i < 3; ++i) {
        if (m_textures[i]) {
            m_textures[i]->destroy();
            delete m_textures[i];
            m_textures[i] = nullptr;
        }
    }
}

QString OpenGLRenderer::loadShaderSource(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR("OpenGLRenderer", QString("Failed to load shader: %1, error: %2")
                      .arg(path).arg(file.errorString()));
        return QString();
    }
    QTextStream stream(&file);
    QString source = stream.readAll();
    file.close();
    LOG_INFO("OpenGLRenderer", QString("Loaded shader: %1, size: %2 bytes").arg(path).arg(source.size()));
    return source;
}

void OpenGLRenderer::initializeGL()
{
    initializeOpenGLFunctions();
    initShaders();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    LOG_INFO("OpenGLRenderer", "initializeGL done");
}

void OpenGLRenderer::initShaders()
{
    QString vertexPath = ":/shaders/vertex.glsl";
    QString fragmentPath = ":/shaders/fragment_yuv.glsl";

    QString vertexSource = loadShaderSource(vertexPath);
    QString fragmentSource = loadShaderSource(fragmentPath);

    m_program = new QOpenGLShaderProgram(this);

    if (vertexSource.isEmpty() || fragmentSource.isEmpty()) {
        LOG_WARN("OpenGLRenderer", "Failed to load shader from file, using hardcoded shaders");
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    } else {
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource);
    }

    if (!m_program->link()) {
        LOG_ERROR("OpenGLRenderer", "Shader program link failed");
        return;
    }

    m_program->bind();

    GLfloat vertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f,  1.0f,
        1.0f,  1.0f
    };

    GLfloat texCoords[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(texCoords), nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(texCoords), texCoords);

    GLint vertexLoc = m_program->attributeLocation("vertexIn");
    glEnableVertexAttribArray(vertexLoc);
    glVertexAttribPointer(vertexLoc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    GLint texLoc = m_program->attributeLocation("textureIn");
    glEnableVertexAttribArray(texLoc);
    glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)sizeof(vertices));

    m_program->release();
    LOG_INFO("OpenGLRenderer", "initShaders done");
}

void OpenGLRenderer::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void OpenGLRenderer::initTextures(int width, int height)
{
    int uvWidth = width / 2;
    int uvHeight = height / 2;

    for (int i = 0; i < 3; ++i) {
        if (m_textures[i]) {
            m_textures[i]->destroy();
            delete m_textures[i];
            m_textures[i] = nullptr;
        }

        m_textures[i] = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_textures[i]->setMinificationFilter(QOpenGLTexture::Linear);
        m_textures[i]->setMagnificationFilter(QOpenGLTexture::Linear);
        m_textures[i]->setWrapMode(QOpenGLTexture::ClampToEdge);
        m_textures[i]->setFormat(QOpenGLTexture::R8_UNorm);
    }

    m_textures[0]->setSize(width, height);
    m_textures[0]->allocateStorage();

    m_textures[1]->setSize(uvWidth, uvHeight);
    m_textures[1]->allocateStorage();

    m_textures[2]->setSize(uvWidth, uvHeight);
    m_textures[2]->allocateStorage();

    m_videoWidth = width;
    m_videoHeight = height;
    LOG_INFO("OpenGLRenderer", QString("Texture initialized: %1x%2, UV: %3x%4")
                                   .arg(width).arg(height).arg(uvWidth).arg(uvHeight));
}

// 🟢 核心重构：此函数现在安全运行在具有原生上下文的 paintGL() 主线程中
void OpenGLRenderer::updateTextures(const FrameData& frame)
{
    if (!frame.isValid()) {
        return;
    }

    int width = frame.width();
    int height = frame.height();

    if (m_textures[0] == nullptr || width != m_videoWidth || height != m_videoHeight) {
        initTextures(width, height);
    }

    if (!frame.getY() || !frame.getU() || !frame.getV()) {
        return;
    }

    QOpenGLPixelTransferOptions optionsY;
    optionsY.setRowLength(frame.linesize()[0]);
    m_textures[0]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame.getY(), &optionsY);

    QOpenGLPixelTransferOptions optionsU;
    optionsU.setRowLength(frame.linesize()[1]);
    m_textures[1]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame.getU(), &optionsU);

    QOpenGLPixelTransferOptions optionsV;
    optionsV.setRowLength(frame.linesize()[2]);
    m_textures[2]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, frame.getV(), &optionsV);
}

void OpenGLRenderer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // 🟢 互斥锁护航：在主线程绘图周期内，安全把临时的最新帧数据上传到 GPU
    m_mutex.lock();
    if (m_currentFrame.isValid()) {
        updateTextures(m_currentFrame);
        m_textureReady = true;
    }
    m_mutex.unlock();

    if (!m_textureReady || !m_program || m_textures[0] == nullptr || !m_textures[0]->isCreated()) {
        return;
    }

    m_program->bind();

    for (int i = 0; i < 3; ++i) {
        m_textures[i]->bind(i);
    }
    m_program->setUniformValue(m_program->uniformLocation("tex_y"), 0);
    m_program->setUniformValue(m_program->uniformLocation("tex_u"), 1);
    m_program->setUniformValue(m_program->uniformLocation("tex_v"), 2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_program->release();
}

// 🟢 由 RenderThread 子线程高频调用
void OpenGLRenderer::updateFrame(const FrameData& frame)
{
    if (!frame.isValid()) {
        return;
    }

    // 🌟 聪明打法：子线程绝对不碰极其脆弱的 makeCurrent()！
    // 只用一个高速原子锁把数据深拷贝/暂存到临时变量里，耗时不到 1 微秒，绝不卡线程
    m_mutex.lock();
    m_currentFrame = frame;
    m_mutex.unlock();

    static int frameCount = 0;
    frameCount++;
    if (frameCount % 30 == 0) {
        LOG_INFO("OpenGLRenderer", QString("updateFrame #%1: %2x%3")
                     .arg(frameCount).arg(frame.width()).arg(frame.height()));
    }

    // 异步通知主线程：可以刷新界面了
    update();
}