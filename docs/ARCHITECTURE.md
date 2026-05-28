\# EdgeXGuard - 系统架构



\## 项目定位

基于RK3588的边缘智能安防网关



\## 硬件组成

\- 主控：RK3588 (Rock 5T)

\- 摄像头：IPC (RTSP/ONVIF)

\- 传感器：AHT20 (温湿度) + HC-SR501 (PIR)

\- 执行器：ESP8266 + 风扇 + LED灯



\## 软件分层

1\. 硬件抽象层 - MPP/RGA/RKNN

2\. 核心处理层 - RTSP拉流/硬解/YOLO推理

3\. 业务逻辑层 - 规则引擎/告警管理

4\. 服务层 - HTTP/WebSocket

5\. 客户端层 - Qt/手机浏览器



\## 模块清单

| 模块 | 功能 | 状态 |

|------|------|------|

| i2c\_sensor | 温湿度读取 | 待实现 |

| pir\_sensor | 人体检测 | 待实现 |

| rtsp\_streamer | RTSP拉流 | 待实现 |

| mpp\_decoder | 视频硬解 | 待实现 |

| yolo\_detector | NPU推理 | 待实现 |

| rule\_engine | 规则引擎 | 待实现 |

| http\_server | API服务 | 待实现 |

| websocket\_server | WebSocket | 待实现 |

