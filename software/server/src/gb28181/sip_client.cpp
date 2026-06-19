#include "sip_client.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <osip2/osip.h>
#include <gst/gst.h>
#include <glib.h>

// 🆕 SDP 解析函数声明
void parseSDP(osip_message_t* msg, char* target_ip, int* target_port);

// 🆕 RTP 推流函数声明
void startRTPStream(const char* target_ip, int target_port);

SipClient::SipClient() 
    : m_ctx(nullptr), m_initialized(false), m_register_id(-1), 
      m_keepalive_interval(30), m_running(false) {}

SipClient::~SipClient() {
    cleanup();
}

bool SipClient::init() {
    if (m_initialized) return true;

    m_ctx = eXosip_malloc();
    if (!m_ctx) {
        printf("❌ eXosip 内存分配失败\n");
        return false;
    }

    osip_t* osip = nullptr;
    int ret = osip_init(&osip);
    if (ret != 0) {
        printf("❌ osip 初始化失败: %d\n", ret);
        free(m_ctx);
        m_ctx = nullptr;
        return false;
    }

    ret = eXosip_init(m_ctx);
    if (ret != 0) {
        printf("❌ eXosip 初始化失败: %d\n", ret);
        osip_release(osip);
        free(m_ctx);
        m_ctx = nullptr;
        return false;
    }

    ret = eXosip_listen_addr(m_ctx, IPPROTO_UDP, "0.0.0.0", 5062, AF_INET, 0);
    if (ret != 0) {
        printf("❌ eXosip 监听失败: %d\n", ret);
        eXosip_quit(m_ctx);
        free(m_ctx);
        m_ctx = nullptr;
        return false;
    }

    eXosip_set_user_agent(m_ctx, "EdgeXGuard/1.0");

    m_initialized = true;
    printf("✅ SipClient 初始化成功\n");
    return true;
}

void SipClient::cleanup() {
    stopKeepAlive();
    if (m_ctx) {
        eXosip_quit(m_ctx);
        free(m_ctx);
        m_ctx = nullptr;
    }
    m_initialized = false;
}

bool SipClient::registerToServer(const std::string& server_ip, int port,
                                 const std::string& username, const std::string& password) {
    if (!m_initialized) {
        printf("❌ SipClient 未初始化\n");
        return false;
    }

    printf("📡 注册到 SIP 服务器: %s:%d (用户: %s)\n",
           server_ip.c_str(), port, username.c_str());

    std::string server_id = "3402000000"; 
    if (username.length() >= 10) {
        server_id = username.substr(0, 10);
    }

    eXosip_clear_authentication_info(m_ctx);
    eXosip_add_authentication_info(m_ctx, username.c_str(), username.c_str(), 
                                   password.c_str(), "MD5", NULL);

    char from_uri[256];
    char to_uri[256];
    char proxy_uri[256];
    snprintf(from_uri, sizeof(from_uri), "sip:%s@%s", username.c_str(), server_id.c_str());
    snprintf(to_uri, sizeof(to_uri), "sip:%s@%s", username.c_str(), server_id.c_str());
    snprintf(proxy_uri, sizeof(proxy_uri), "sip:%s:%d", server_ip.c_str(), port);

    osip_message_t* reg_msg = nullptr;
    int register_id = eXosip_register_build_initial_register(
        m_ctx, from_uri, proxy_uri, NULL, 3600, &reg_msg);

    if (register_id < 0) {
        printf("❌ 构建 REGISTER 失败: %d\n", register_id);
        return false;
    }

    eXosip_lock(m_ctx);
    int ret = eXosip_register_send_register(m_ctx, register_id, reg_msg);
    eXosip_unlock(m_ctx);

    if (ret != 0) {
        printf("❌ 发送 REGISTER 失败: %d\n", ret);
        return false;
    }

    printf("✅ 第一阶段 REGISTER 已发送，等待 401 挑战... (Register ID: %d)\n", register_id);

    eXosip_event_t* ev = nullptr;
    int count = 0;
    while (count < 100) {
    eXosip_lock(m_ctx);
    eXosip_automatic_action(m_ctx);
    eXosip_unlock(m_ctx);

    ev = eXosip_event_wait(m_ctx, 0, 100);
    if (ev) {
        // 处理 INVITE 请求
        if (ev->type == EXOSIP_CALL_INVITE) {
            printf("📞 收到 INVITE 请求，开始处理视频点播...\n");
            
            // 解析 SDP，获取对方的 IP 和端口
            osip_message_t* invite = ev->request;
            osip_message_t* answer = NULL;
            
            // 提取 SDP 中的目标 IP 和端口
            char target_ip[64] = "192.168.0.100";
            int target_port = 5000;
            
            // 解析 SDP 获取目标 IP 和端口
            // 这里简化处理，实际需要解析 SDP 文本
            parseSDP(invite, target_ip, &target_port);
            
            printf("📡 目标地址: %s:%d\n", target_ip, target_port);
            
            // 构建 200 OK 响应 (包含 SDP)
            eXosip_lock(m_ctx);
            eXosip_call_build_answer(m_ctx, ev->tid, 200, &answer);
            eXosip_unlock(m_ctx);
            
            // 添加 SDP 信息到响应中
            char sdp[1024];
            snprintf(sdp, sizeof(sdp),
                "v=0\r\n"
                "o=34020000001320000001 0 0 IN IP4 %s\r\n"
                "s=Play\r\n"
                "c=IN IP4 %s\r\n"
                "t=0 0\r\n"
                "m=video %d RTP/AVP 96\r\n"
                "a=rtpmap:96 H264/90000\r\n"
                "a=sendonly\r\n",
                "192.168.0.102",  // 本机 IP
                "192.168.0.102",  // 本机 IP
                target_port
            );
            
            osip_message_set_body(answer, sdp, strlen(sdp));
            osip_message_set_content_type(answer, "application/sdp");
            
            // 发送 200 OK
            eXosip_lock(m_ctx);
            eXosip_call_send_answer(m_ctx, ev->tid, 200, answer);
            eXosip_unlock(m_ctx);
            
            printf("✅ INVITE 处理完成，已发送 200 OK\n");
            
            // 启动视频推流（调用 GStreamer）
            startRTPStream(target_ip, target_port);
        }
        
        eXosip_event_free(ev);
    }
    count++;
}

    printf("⏰ 注册超时\n");
    return false;
}

bool SipClient::sendKeepAlive() {
    if (!m_initialized || m_register_id < 0) {
        return false;
    }

    static int sn = 1;
    time_t now = time(NULL);

    // 1. 先动态构建 XML Body，确保长度绝对精准
    char xml_body[512];
    int xml_len = snprintf(xml_body, sizeof(xml_body),
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
        "<Query>\r\n"
        "  <CmdType>Keepalive</CmdType>\n"
        "  <SN>%d</SN>\n"
        "  <DeviceID>%s</DeviceID>\n"
        "  <Status>ON</Status>\n"
        "</Query>\r\n",
        sn, m_username.c_str()
    );

    // 💡 国标规范：To 最好是服务器 ID（即 m_server_id + "2000000001"）
    // 如果你目前写 m_username 平台也能认，可以先保留，但规范推荐用服务器自身的国标ID
    std::string server_gb_id = m_server_id + "2000000001"; 

    // 2. 拼接完整的 SIP 报文，动态填入 Content-Length
    char buf[2048];
    int len = snprintf(buf, sizeof(buf),
        "MESSAGE sip:%s@%s SIP/2.0\r\n"
        "Via: SIP/2.0/UDP %s:5062;branch=z9hG4bKkeepalive_%ld_%d\r\n"
        "From: <sip:%s@%s>;tag=keepalive_%ld\r\n"
        "To: <sip:%s@%s>\r\n"
        "Call-ID: keepalive_%ld@%s\r\n"
        "CSeq: %d MESSAGE\r\n"
        "Contact: <sip:%s@%s:5062>\r\n"
        "Content-Type: application/manscdp+xml\r\n" // ⚠️ 修正全小写
        "Max-Forwards: 70\r\n"
        "User-Agent: EdgeXGuard\r\n"
        "Content-Length: %d\r\n" // ⚠️ 动态包长
        "\r\n"
        "%s",                     // ⚠️ 注入真实的 XML Body
        server_gb_id.c_str(), m_server_id.c_str(),
        m_server_ip.c_str(), now, sn,
        m_username.c_str(), m_server_id.c_str(), now,
        server_gb_id.c_str(), m_server_id.c_str(),
        now, m_server_ip.c_str(),
        sn,
        m_username.c_str(), m_server_ip.c_str(),
        xml_len,                  // 填入真实的 xml 长度
        xml_body
    );

    // 3. 原生 UDP 发送
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("❌ 创建 socket 失败\n");
        return false;
    }

    // 💡 优化：允许端口复用（如果想让这个套接字绑定本地 5062 从而能接收 200OK，可以解除注释）
    // int opt = 1;
    // setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // struct sockaddr_in local_addr;
    // ... bind(sockfd, ...)

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_server_port);
    addr.sin_addr.s_addr = inet_addr(m_server_ip.c_str());

    int ret = sendto(sockfd, buf, len, 0, (struct sockaddr*)&addr, sizeof(addr));
    close(sockfd);

    if (ret > 0) {
        printf("💓 裸发心跳成功 (SN: %d, Content-Length: %d)\n", sn, xml_len);
        sn++;
        return true;
    } else {
        printf("❌ 裸发心跳失败\n");
        return false;
    }
}

bool SipClient::unregister() {
    if (!m_initialized || m_register_id < 0) {
        return false;
    }

    osip_message_t* reg_msg = nullptr;
    int ret = eXosip_register_build_register(m_ctx, m_register_id, 0, &reg_msg);
    if (ret != 0) {
        printf("❌ 构建注销消息失败\n");
        return false;
    }

    eXosip_lock(m_ctx);
    ret = eXosip_register_send_register(m_ctx, m_register_id, reg_msg);
    eXosip_unlock(m_ctx);

    if (ret == 0) {
        printf("🔴 注销请求已发送\n");
        return true;
    }
    return false;
}

void SipClient::startKeepAlive() {
    if (!m_initialized || m_register_id < 0) {
        printf("❌ 未注册，无法启动心跳\n");
        return;
    }
    
    if (m_running) {
        printf("⚠️ 心跳已运行\n");
        return;
    }
    
    m_running = true;
    m_keepalive_thread = std::thread(&SipClient::keepAliveLoop, this);
    printf("💓 心跳线程已启动 (间隔: %d 秒)\n", m_keepalive_interval);
}

void SipClient::stopKeepAlive() {
    if (!m_running) return;
    
    m_running = false;
    if (m_keepalive_thread.joinable()) {
        m_keepalive_thread.join();
    }
    printf("💓 心跳线程已停止\n");
}

void SipClient::keepAliveLoop() {
    // 细粒度循环驱动，计算 15 秒对应多少个 100ms
    int ticks_needed = m_keepalive_interval * 10;
    int tick_count = 0;

    while (m_running) {
        // 1. 每 100ms 必须要踩一脚 eXosip 的状态机油门，否则协议栈会僵死
        eXosip_lock(m_ctx);
        eXosip_automatic_action(m_ctx);
        eXosip_unlock(m_ctx);

        // 2. 清空并处理底层事件队列，防止 socket 缓存爆掉
        eXosip_event_t* ev = eXosip_event_wait(m_ctx, 0, 100);
        if (ev) {
            if (ev->type == EXOSIP_MESSAGE_ANSWERED) {
                if (ev->response) {
                    printf("📩 收到服务器的心跳应答状态码: %d (200 OK 为正常)\n", ev->response->status_code);
                }
            }
            eXosip_event_free(ev);
        }

        tick_count++;
        if (tick_count >= ticks_needed) {
            tick_count = 0; // 重新计数
            if (!m_running) break;
            
            // 触发心跳
            if (!sendKeepAlive()) {
                printf("⚠️ 心跳发送失败\n");
            }
        }
    }
}

void parseSDP(osip_message_t* msg, char* target_ip, int* target_port) {
    // 获取 SDP body
    osip_body_t* body = NULL;
    int ret = osip_message_get_body(msg, 0, &body);
    if (ret != 0 || body == NULL) {
        printf("❌ 获取 SDP body 失败: %d\n", ret);
        return;
    }
    
    // 用 osip_body_to_str 获取 body 内容（需要 3 个参数）
    char* body_str = NULL;
    size_t length = 0;
    ret = osip_body_to_str(body, &body_str, &length);
    if (ret != 0 || body_str == NULL || length == 0) {
        printf("❌ 转换 SDP body 失败: %d\n", ret);
        return;
    }
    
    printf("📄 SDP Body:\n%s\n", body_str);
    
    // 设置默认值
    strcpy(target_ip, "192.168.0.100");
    *target_port = 5000;
    
    // 解析 c= 行获取 IP
    const char* c_line = strstr(body_str, "c=IN IP4 ");
    if (c_line) {
        c_line += 9;
        const char* end = c_line;
        while (*end && *end != '\r' && *end != '\n') end++;
        int len = end - c_line;
        if (len > 0 && len < 64) {
            strncpy(target_ip, c_line, len);
            target_ip[len] = '\0';
        }
    }
    
    // 解析 m= 行获取端口
    const char* m_line = strstr(body_str, "m=video ");
    if (m_line) {
        m_line += 9;
        if (sscanf(m_line, "%d", target_port) != 1) {
            *target_port = 5000;
        }
    }
    
    printf("📡 解析结果: IP=%s, Port=%d\n", target_ip, *target_port);
    
    // 释放 body_str
    free(body_str);
}

void startRTPStream(const char* target_ip, int target_port) {
    // 确保 GStreamer 已初始化
    static bool gst_initialized = false;
    if (!gst_initialized) {
        gst_init(NULL, NULL);
        gst_initialized = true;
    }
    
    printf("🎬 [RTP推流] 目标: %s:%d\n", target_ip, target_port);
    
    // 构建 GStreamer 命令行
    char pipeline_desc[1024];
    snprintf(pipeline_desc, sizeof(pipeline_desc),
        "rtspsrc location=rtsp://admin:z13312555@192.168.0.103:554/stream1 latency=0 ! "
        "rtph264pay ! "
        "udpsink host=%s port=%d sync=false",
        target_ip, target_port
    );
    
    printf("🎬 启动推流管道: %s\n", pipeline_desc);
    
    GError* error = NULL;
    GstElement* pipeline = gst_parse_launch(pipeline_desc, &error);
    if (error) {
        printf("❌ 构建推流管道失败: %s\n", error->message);
        g_error_free(error);
        return;
    }
    
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    printf("✅ RTP 推流已启动\n");
}