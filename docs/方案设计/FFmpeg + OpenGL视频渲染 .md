FFmpeg + OpenGL 视频渲染技术详解
一、概述
在 EdgeXGuard 项目中，需要实时显示 IPC 摄像头的 RTSP 视频流。本文档详细介绍了基于 FFmpeg 解码 + OpenGL 硬件加速渲染的实现方案。

二、整体架构

┌─────────────────────────────────────────────────────────────────────────────┐
│                           FFmpeg + OpenGL 渲染架构                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐  │
│  │  RTSP 拉流  │───→│  解封装     │───→│  视频解码   │───→│  YUV Frame  │  │
│  │avformat_*   │    │avformat_*   │    │avcodec_*    │    │  AVFrame    │  │
│  └─────────────┘    └─────────────┘    └─────────────┘    └──────┬──────┘  │
│                                                                  │         │
│                                                                  ▼         │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐  │
│  │  Qt Widget  │←───│  OpenGL     │←───│  纹理上传   │←───│  格式转换   │  │
│  │  显示       │    │  渲染       │    │  glTexImage │    │  YUV→RGB    │  │
│  └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

三、多线程解码架构

┌─────────────────────────────────────────────────────────────────────────────┐
│                           多线程解码架构                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                       解封装线程 (Demuxer)                           │   │
│  │                                                                      │   │
│  │   while (running) {                                                  │   │
│  │       av_read_frame() → Packet → RingQueue.push()                    │   │
│  │   }                                                                  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                     │
│                                      │ RingQueue<Packet>                   │
│                                      ▼                                     │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                       解码线程 (Decoder)                             │   │
│  │                                                                      │   │
│  │   while (running) {                                                  │   │
│  │       RingQueue.pop() → Packet                                       │   │
│  │       avcodec_send_packet()                                          │   │
│  │       avcodec_receive_frame() → AVFrame → RingQueue.push()           │   │
│  │   }                                                                  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                      │                                     │
│                                      │ RingQueue<Frame>                    │
│                                      ▼                                     │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    主线程渲染 (OpenGL)                               │   │
│  │                                                                      │   │
│  │   QTimer::timeout() {                                                │   │
│  │       RingQueue.pop() → Frame                                        │   │
│  │       updateTextures() → update() → paintGL()                        │   │
│  │   }                                                                  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

四、核心组件实现
4.1 无锁环形队列 (RingQueue)
用于线程间传递数据，避免互斥锁开销。

template<typename T>
class RingQueue {
public:
    explicit RingQueue(size_t capacity)
        : m_capacity(capacity + 1)
        , m_buffer(capacity + 1)
        , m_readPos(0)
        , m_writePos(0) {}

    // 生产者：写入数据
    bool push(T&& item) {
        size_t write = m_writePos.load(std::memory_order_relaxed);
        size_t next = (write + 1) % m_capacity;
        
        if (next == m_readPos.load(std::memory_order_acquire)) {
            return false;  // 队列满
        }
        
        m_buffer[write] = std::move(item);
        m_writePos.store(next, std::memory_order_release);
        return true;
    }

    // 消费者：读取数据
    bool pop(T& item) {
        size_t read = m_readPos.load(std::memory_order_relaxed);
        
        if (read == m_writePos.load(std::memory_order_acquire)) {
            return false;  // 队列空
        }
        
        item = std::move(m_buffer[read]);
        m_readPos.store((read + 1) % m_capacity, std::memory_order_release);
        return true;
    }

private:
    size_t m_capacity;
    std::vector<T> m_buffer;
    std::atomic<size_t> m_readPos;
    std::atomic<size_t> m_writePos;
};
4.2 解封装线程 (Demuxer)

void Demuxer::run()
{
    AVPacket* packet = av_packet_alloc();
    
    while (m_running && !m_eof) {
        int ret = av_read_frame(m_formatCtx, packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                m_eof = true;
                break;
            }
            continue;
        }
        
        // 只处理视频流
        if (packet->stream_index == m_videoStreamIndex) {
            Packet pkt = Packet::fromAVPacket(packet);
            // 队列满则丢帧，不阻塞
            if (m_packetQueue->size() < 100) {
                m_packetQueue->push(std::move(pkt));
            }
        }
        
        av_packet_unref(packet);
    }
    
    av_packet_free(&packet);
}

4.3 解码线程 (Decoder)

void VideoDecoder::run()
{
    m_running = true;
    bool hasKeyFrame = false;
    
    while (m_running) {
        Packet packet;
        if (!m_packetQueue->popWait(packet, 10)) {
            continue;
        }
        
        AVPacket* pkt = packet.get();
        
        // 等待关键帧（I帧），避免花屏
        if (!hasKeyFrame && !(pkt->flags & AV_PKT_FLAG_KEY)) {
            continue;
        }
        if (pkt->flags & AV_PKT_FLAG_KEY) {
            hasKeyFrame = true;
        }
        
        // 发送到解码器
        if (avcodec_send_packet(m_codecCtx, pkt) < 0) continue;
        
        // 接收解码后的帧
        while (avcodec_receive_frame(m_codecCtx, m_frame) == 0) {
            int64_t pts = m_frame->pts;
            if (pts != AV_NOPTS_VALUE) {
                pts = av_rescale_q(pts, m_timeBase, {1, 1000000});
            }
            
            FrameData frame = FrameData::fromAVFrame(m_frame, pts);
            
            // 队列满则丢弃最旧的，保持实时性
            if (m_outputQueue->size() >= 10) {
                FrameData dummy;
                m_outputQueue->pop(dummy);
            }
            m_outputQueue->push(std::move(frame));
            
            m_frame = av_frame_alloc();
        }
    }
}

五、OpenGL 渲染实现
5.1 OpenGL 渲染器类结构

class OpenGLRenderer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit OpenGLRenderer(QWidget* parent = nullptr);
    
public slots:
    void updateFrame(const FrameData& frame);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    void initShaders();
    void initTextures(int width, int height);
    void updateTextures(const FrameData& frame);

    QOpenGLShaderProgram* m_program;
    QOpenGLTexture* m_textures[3];  // Y, U, V 三个纹理
    int m_videoWidth;
    int m_videoHeight;
    bool m_textureReady;
};

5.2 YUV→RGB 着色器
顶点着色器：
glsl
attribute vec4 vertexIn;
attribute vec2 textureIn;
varying vec2 textureOut;

void main(void) {
    gl_Position = vertexIn;
    textureOut = textureIn;
}

片段着色器（YUV→RGB 转换）：

glsl
uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;
varying vec2 textureOut;

void main(void) {
    float y = texture2D(tex_y, textureOut).r;
    float u = texture2D(tex_u, textureOut).r - 0.5;
    float v = texture2D(tex_v, textureOut).r - 0.5;
    
    vec3 rgb;
    rgb.r = y + 1.402 * v;
    rgb.g = y - 0.344 * u - 0.714 * v;
    rgb.b = y + 1.772 * u;
    
    gl_FragColor = vec4(rgb, 1.0);
}

5.3 纹理上传（关键优化）

void OpenGLRenderer::updateTextures(const FrameData& frame)
{
    if (!frame.isValid()) return;
    
    int width = frame.width();
    int height = frame.height();
    int uvWidth = width / 2;
    int uvHeight = height / 2;
    
    // 首次或分辨率变化时重新分配纹理
    if (m_textures[0] == nullptr || width != m_videoWidth || height != m_videoHeight) {
        initTextures(width, height);
    }
    
    // 使用 QImage 上传，自动处理 linesize 对齐
    QImage yImage(frame.getY(), width, height, frame.linesize()[0], QImage::Format_Grayscale8);
    QImage uImage(frame.getU(), uvWidth, uvHeight, frame.linesize()[1], QImage::Format_Grayscale8);
    QImage vImage(frame.getV(), uvWidth, uvHeight, frame.linesize()[2], QImage::Format_Grayscale8);
    
    m_textures[0]->setData(yImage);
    m_textures[1]->setData(uImage);
    m_textures[2]->setData(vImage);
}

5.4 渲染绘制

void OpenGLRenderer::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    if (!m_textureReady || !m_program) return;
    
    m_program->bind();
    
    m_textures[0]->bind(0);
    m_program->setUniformValue(m_program->uniformLocation("tex_y"), 0);
    
    m_textures[1]->bind(1);
    m_program->setUniformValue(m_program->uniformLocation("tex_u"), 1);
    
    m_textures[2]->bind(2);
    m_program->setUniformValue(m_program->uniformLocation("tex_v"), 2);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    m_program->release();
}

六、PTS 同步（音视频同步）

void RenderThread::run()
{
    QElapsedTimer ptsTimer;
    ptsTimer.start();
    int64_t firstFramePts = -1;
    
    while (m_running) {
        FrameData frame;
        if (!m_queue->pop(frame)) continue;
        
        int64_t framePts = frame.pts();  // 微秒
        
        if (firstFramePts == -1 && framePts != AV_NOPTS_VALUE) {
            firstFramePts = framePts;
        }
        
        // PTS 同步：计算应该等到什么时候
        if (framePts != AV_NOPTS_VALUE && firstFramePts != -1) {
            int64_t ptsOffset = framePts - firstFramePts;  // 相对 PTS
            int64_t elapsedUs = ptsTimer.nsecsElapsed() / 1000;  // 实际经过时间
            
            if (ptsOffset > elapsedUs) {
                int64_t sleepUs = ptsOffset - elapsedUs;
                if (sleepUs > 0 && sleepUs < 100000) {
                    msleep(sleepUs / 1000);
                }
            } else if (elapsedUs - ptsOffset > 50000) {
                // 落后超过 50ms，丢帧追赶
                continue;
            }
        }
        
        // 渲染
        m_renderer->updateFrame(frame);
    }
}

七、遇到的关键问题及解决方案
问题1：YUV 数据 linesize 对齐问题
现象：画面出现斜纹、颜色异常。

原因：FFmpeg 解码输出的 linesize 可能大于实际宽度（内存对齐），直接上传纹理导致错位。

解决：使用 QImage 包装 YUV 数据，传入正确的 linesize：

QImage yImage(frame.getY(), width, height, frame.linesize()[0], QImage::Format_Grayscale8);

问题2：纹理重复分配导致 OpenGL 错误
现象：Cannot change format once storage has been allocated

原因：每帧都重新分配纹理，Qt 6 不允许纹理格式改变。

解决：只在分辨率变化时重新分配纹理：
if (m_textures[0] == nullptr || width != m_videoWidth || height != m_videoHeight) {
    initTextures(width, height);
}
问题3：解码速度远快于渲染速度
现象：队列迅速积压，内存暴涨。

原因：解码线程无限制，渲染线程跟不上。

解决：

解码线程：队列满时丢弃旧帧

渲染线程：只取最新帧，跳过中间帧

// 渲染时取最新帧
FrameData frame;
while (m_queue->pop(frame)) {
    lastFrame = frame;  // 只保留最后一帧
}
问题4：关键帧丢失导致花屏
现象：画面出现绿色块、马赛克。

原因：解码器没有从 I 帧开始解码。

解决：跳过非关键帧，直到遇到关键帧：

if (!hasKeyFrame && !(pkt->flags & AV_PKT_FLAG_KEY)) {
    continue;  // 跳过非关键帧
}
if (pkt->flags & AV_PKT_FLAG_KEY) {
    hasKeyFrame = true;
}

八、性能数据
指标	数值
解码帧率	~30 fps (1080p)
渲染帧率	~30 fps
解码延迟	< 50ms
渲染延迟	< 33ms
CPU 占用	~15% (单核)
GPU 占用	~5%
内存占用	~200 MB

九、总结
优点	说明
高性能	GPU 硬件加速，零拷贝纹理上传
低延迟	多线程解码 + 无锁队列
可扩展	支持多路视频，动态分辨率切换
跨平台	Qt + OpenGL + FFmpeg 全跨平台