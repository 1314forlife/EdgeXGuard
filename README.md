\# EdgeXGuard



# EdgeXGuard: 基于 RK3588 的边缘智能安防网关系统

基于 Rockchip RK3588 平台设计并实现的异构协议边缘计算安防网关。系统向下通过自主实现的轻量化 ONVIF 协议栈与各类 IP 摄像头打通，进行 H.264/H.265 视频流的高效检索与动态控制；核心层构建基于 GStreamer 的高性能视频处理管道，集成 OpenCV 机器视觉算法实现本地实时的边缘人脸识别与凭证管理；向上融合 GB/T 28181 国标协议栈与 MQTT 物联网协议，实现了视频流常态化国标推送、信令交互以及端-边-云协同的智能路由与物联控制联动。

---

## 🛠️ 技术栈与核心能力

### 1. 开发环境与硬件架构
*   **硬件平台**: Rockchip RK3588 (ARM Cortex-A76×4 + Cortex-A55×4, 6TOPS NPU) / ESP8266 MCU
*   **操作系统**: Ubuntu 22.04 LTS / Debian 11
*   **开发语言与标准**: C++17 / C (嵌入式端)
*   **构建系统**: CMake 3.10+ / PlatformIO

### 2. 媒体与视觉处理
*   **多媒体框架**: GStreamer 1.22 / FFmpeg 7.0 
*   **计算机视觉**: OpenCV 4.6 (Haar Cascade + LBPH 算法体系)
*   **编解码协议**: RTSP / RTP / RTMP / PS封装

### 3. 协议栈与物联网
*   **国标信令与媒体**: libosip2 + libeXosip2 (GB/T 28181-2016 标准)
*   **网络与控制**: 自研轻量化 SOAP (ONVIF Client) / MQTT (EMQX Broker)

---

## 🏗️ 系统架构设计

系统采用分层解耦架构设计，确保高并发网络信令与高吞吐量媒体流的隔离，保障边缘端 7×24 小时稳定运行。

```text
┌─────────────────────────────────────────────────────────────────┐
│                           应用层 (Application)                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │ 人脸识别 │  │ GB28181  │  │ MQTT 联动│  │ 设备管理 │       │
│  │ 凭证管理 │  │ 业务逻辑 │  │ 核心控制 │  │ 视音频渲染│       │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘       │
├─────────────────────────────────────────────────────────────────┤
│                           协议层 (Protocol)                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │  ONVIF   │  │  osip2   │  │   RTP    │  │   MQTT   │       │
│  │  Client  │  │ eXosip2  │  │  Stream  │  │  Client  │       │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘       │
├─────────────────────────────────────────────────────────────────┤
│                           媒体层 (Media Pipeline)                │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │GStreamer │  │ OpenCV   │  │ FFmpeg   │  │ OpenGL   │       │
│  │ Pipeline │  │  Vision  │  │ Decoder  │  │ Render   │       │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘       │
├─────────────────────────────────────────────────────────────────┤
│                           硬件层 (Hardware & Infrastructure)    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐       │
│  │ RK3588   │  │  ESP8266 │  │  ONVIF   │  │  EMQX    │       │
│  │  NPU 6T  │  │节点/舵机 │  │  Camera  │  │  Broker  │       │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘       │
└─────────────────────────────────────────────────────────────────┘

数据流拓扑 (Data Flow)
IPC (ONVIF 摄像头) 
    │
    ▼ [RTSP H.264/H.265 Stream]
GStreamer 边缘流媒体管道 
    │
    ├──▶ 视频帧解析 ──▶ OpenCV 特征提取 ──▶ Haar+LBPH 人脸识别 
    │                                              │
    │                                              ▼
    │                                     [识别结果触发]
    │                                              │
    │                                              ├──▶ MQTT 客户端 ──▶ ESP8266 节点 ──▶ 硬件拓扑联动
    │                                              └──▶ 视频流 OSD 动态图层叠加
    │
    ├──▶ GB/T 28181 模块 (SIP eXosip2 握手) ──▶ [RTP/PS 流化封装] ──▶ 接入国标视图视频平台
    └──▶ 实时 RTMP 流化推送 (带算法 OSD 渲染流)

核心技术亮点
1. 轻量化自主实现 ONVIF 协议栈
去依赖化设计: 放弃膨胀的 gSOAP 生成代码，纯手工构建符合规范的 SOAP XML 请求模板与解析器。

安全认证: 严格实现 WS-UsernameToken 摘要认证机制。

设备治理: 完美覆盖设备发现（WS-Discovery）、PTZ 绝对/相对云台控制、以及 Media 媒体流播放地址（RTSP URI）的动态获取，显著降低了嵌入式端内存占用（BSS/Data段）。

2. 标准级 GB/T 28181 信令栈与媒体流构建
基于 libosip2 与 libeXosip2 底层库，深度封装国标信令接口。

信令全生命周期管理: 稳定支持设备向上级视图平台的注册（REGISTER 携带 Digest 鉴权）、常态化心跳保活（MESSAGE Keepalive）与注销流程。

流媒体交互: 响应上级平台的点播（INVITE）信令，动态建立 RTP/PS 发送信道，实现低延迟的音视频流同步推送。

3. 多线程异构协议高并发调度

设计高内聚、低耦合的模块化系统框架。GB28181 模块、MQTT 模块、ONVIF 模块以及媒体流水线工作在各自独立的线程上下文中，通过线程安全队列进行非阻塞数据交互。

主程序通过 CMake 编译开关 (ENABLE_GB28181) 灵活控制国标组件的编入，具备优秀的工业级可扩展性。

4. 优化级边缘端流媒体管道 (Pipeline)

利用 GStreamer 框架构建定制化硬件加速（或半硬解）视频处理管道。

在保障视频流流畅解码的同时，抽取关键帧注入 OpenCV Vision 算子，降低高码率下的丢帧率。

AI 推理演进预留: 架构层面抽象出统一的 AI 推理基类，当前采用 Haar + LBPH 进行轻量化人脸识别验证，并预留了 RKNN (Rockchip NPU) 推理接口，可无缝平滑切换至高算力的深度学习检测模型（如 YOLO-Face/RetinaFace）。

📂 项目模块拓扑

EdgeXGuard/
├── software/
│   ├── client/                    # 远程跨平台监控客户端 (基于 Qt/C++)
│   │   ├── CMakeLists.txt
│   │   ├── infrastructure/        # ONVIF 通信底座与 MQTT 通信组件
│   │   ├── presentation/          # 用户视觉交互界面 (UI)
│   │   └── core/                  # 视音频软解与 OpenGL 高性能渲染引擎
│   │
│   └── server/                    # RK3588 边缘网关核心服务
│       ├── src/
│       │   ├── main.cpp           # 守护进程主入口
│       │   ├── config.h           # 静态全局系统参数配置
│       │   ├── face_database.cpp  # 本地结构化人脸特征数据库治理
│       │   ├── gst_pipeline.cpp   # GStreamer 拓扑流媒体管道控制
│       │   └── gb28181/           # GB/T 28181 标准组件
│       │       ├── sip_client.h
│       │       ├── sip_client.cpp
│       │       ├── sip_test.cpp   # 独立解耦的信令仿真测试程序
│       │       └── CMakeLists.txt
│       ├── 3rdparty/              # 静态/动态第三方依赖库管理
│       ├── models/                # 预训练级级联分类器与特征模型
│       └── CMakeLists.txt
│
├── hardware/
│   └── esp8266_node/              # 物联网分布式边缘执行节点固件
│       ├── src/
│       │   ├── main.c             # 任务调度与状态机
│       │   ├── dht11.c            # 微秒级单总线传感器驱动
│       │   ├── mqtt_mini.c        # 面向嵌入式轻量级 MQTT 协议实现
│       │   └── servo_control.c    # 硬件 PWM 舵机驱动控制
│       └── platformio.ini
│
└── docs/                          # 系统设计、协议细节与部署归档

 依赖生态与三方库配置指南 (Third-Party Integration)
本项目为了保持核心代码库的轻量化，未将第三方依赖库源码或二进制文件直接上传。在执行网关主程序编译前，请根据以下拓扑结构及指引完成依赖库的配置或编译：

1. 系统基础依赖安装
首先通过系统包管理器安装流媒体管道、编译工具链及基础网络组件：
    sudo apt-get update && sudo apt-get install -y \
    build-essential cmake git wget \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    libosip2-dev libexosip2-dev

明白你的意思了，你现在遇到的问题是：在普通的 Markdown 文本中，如果不使用表格（Table），这一大段文字就会挤成一团，缺乏视觉层级，非常不便于阅读。

如果不想用表格，而是直接用纯文字/列表的方式排版，我们应当引入清晰的标题层级、粗体强调、代码块缩进以及分割线。

以下是为你优化后的纯文字版排版方案，直接复制即可无缝嵌入你的 README.md：

2. 自建第三方库依赖拓扑与编译规范
请在项目的 third_party/（或 3rdparty/）目录下，依次克隆并完成以下核心组件的本地编译与路径链接：

🔹 opencv
技术选型与职责：负责图像基础处理、视频帧抓取及传统视觉算子。

编译与生产规范：

建议编译 Release 版本。

若需利用 RK3588 硬件加速，请确保开启 -DWITH_GSTREAMER=ON 编译开关。

🔹 ffmpeg
技术选型与职责：负责音视频流的软解、多协议解复用（Demuxing）底座。

编译与生产规范：

建议通过源码定制化配置。

深度裁剪不需要的编解码器，仅保留 H.264 / H.265 / AAC 以极致优化嵌入式端体积。

🔹 onvif-wsdl
技术选型与职责：本地自研 ONVIF 核心协议映射生成的 XML/WSDL 抽象定义文件。

编译与生产规范：

作为静态数据或动态解析源，无需编译。

请确保相对路径正确，以供本网关的轻量化 SOAP 组包逻辑进行动态调用。

🔹 hnswlib
技术选型与职责：高维向量检索库（基于 HNSW 分层有向小世界图算法），负责秒级百万级人脸特征向量的 Top-K 近邻精准匹配。

编译与生产规范：

属于 Header-only 库，无需独立编译成静态/动态库。

直接通过 CMake include_directories() 将其 hnswlib/ 核心头文件目录引入主工程即可。


编译构建与部署说明(第三方库请自行配置)

1. 边缘网关服务器端编译 (RK3588 Target)

cd software/server
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
./EdgeXGuardServer

2. GB/T 28181 模块模块化仿真测试

cd software/server/src/gb28181
mkdir build && cd build
cmake ..
make -j4
./sip_test

3. 物联网微控制端固件烧录 (ESP8266 Target)

cd hardware/esp8266_node
pio run --target upload

4. 基础拓扑中间件启动 (Docker 容器化部署)

docker run -d --name emqx -p 1883:1883 -p 18083:18083 emqx/emqx:5.3.2

系统关键配置 (config.h)

通过修改 software/server/src/config.h 实现对边缘网关各项技术参数的精确裁剪与适配：

// 边缘端接入摄像头 RTSP 流地址配置
#define RTSP_URL "rtsp://admin:password@192.168.1.100:554/stream1"

// GB/T 28181 协议栈参数配置
#define ENABLE_GB28181 1
#define GB28181_SERVER_IP "192.168.1.200"
#define GB28181_DEVICE_ID "34020000001320000001"
#define GB28181_PASSWORD "123456"

// 上级流媒体服务器 RTMP 推流定位
#define RTMP_URL "rtmp://127.0.0.1:1935/live"

📄 开源许可证
本项目依据 MIT License 规范开源。