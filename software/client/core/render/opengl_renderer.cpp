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
{
}

OpenGLRenderer::~OpenGLRenderer()
{
    makeCurrent();
    cleanup();
}

void OpenGLRenderer::cleanup()
{
    if (m_program) {
        delete m_program;
        m_program = nullptr;
    }
    for (int i = 0; i < 3; ++i) {
        m_textures[i] = nullptr;
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
    // 尝试从文件加载着色器
    QString vertexPath = ":/shaders/vertex.glsl";
    QString fragmentPath = ":/shaders/fragment_yuv.glsl";

    QString vertexSource = loadShaderSource(vertexPath);
    QString fragmentSource = loadShaderSource(fragmentPath);

    m_program = new QOpenGLShaderProgram(this);

    if (vertexSource.isEmpty() || fragmentSource.isEmpty()) {
        LOG_WARN("OpenGLRenderer", "Failed to load shader from file, using hardcoded shaders");

        // 硬编码的顶点着色器
        const char* hardcodedVertex =
            "attribute vec4 vertexIn;\n"
            "attribute vec2 textureIn;\n"
            "varying vec2 textureOut;\n"
            "void main(void) {\n"
            "    gl_Position = vertexIn;\n"
            "    textureOut = textureIn;\n"
            "}\n";

        // 硬编码的片段着色器
        const char* hardcodedFragment =
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

        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, hardcodedVertex);
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, hardcodedFragment);
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
        // 【修复】如果分辨率动态改变，先销毁旧纹理，防止内存泄漏
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

void OpenGLRenderer::updateTextures(const FrameData& frame)
{
    if (!frame.isValid()) {
        LOG_ERROR("OpenGLRenderer", "Invalid frame");
        return;
    }

    int width = frame.width();
    int height = frame.height();

    // 如果是第一帧或者分辨率发生了改变，初始化纹理
    if (m_textures[0] == nullptr || width != m_videoWidth || height != m_videoHeight) {
        initTextures(width, height);
    }

    if (!frame.getY() || !frame.getU() || !frame.getV()) {
        LOG_ERROR("OpenGLRenderer", "Frame data pointer is null");
        return;
    }

    // 【核心修复】不使用 QImage，直接上传底层数据
    // 使用 QOpenGLPixelTransferOptions 来处理 FFmpeg 的 linesize（行对齐）

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

    if (!m_textureReady || !m_program || m_textures[0] == nullptr) {
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

void OpenGLRenderer::updateFrame(const FrameData& frame)
{
    if (!frame.isValid()) {
        LOG_WARN("OpenGLRenderer", "updateFrame: invalid frame");
        return;
    }

    static int frameCount = 0;
    frameCount++;
    if (frameCount % 30 == 0) {
        LOG_INFO("OpenGLRenderer", QString("updateFrame #%1: %2x%3, linesize: [%4, %5, %6]")
                     .arg(frameCount)
                     .arg(frame.width()).arg(frame.height())
                     .arg(frame.linesize()[0])
                     .arg(frame.linesize()[1])
                     .arg(frame.linesize()[2]));
    }

    updateTextures(frame);
    m_textureReady = true;
    update();
}