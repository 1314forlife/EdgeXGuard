#include "sip_client.h"
#include "sip_register.h"
#include "sip_event.h"
#include "sip_keepalive.h"
#include "sdp_parser.h"
#include "rtp_streamer.h"
#include <stdio.h>
#include <netinet/in.h>   // ← 添加 AF_INET
#include <sys/socket.h>   // ← 添加 IPPROTO_UDP

SipClient::SipClient() 
    : m_ctx(nullptr), m_initialized(false), m_running(false), 
      m_register_id(-1), m_server_port(0) {
    static bool gst_init_done = false;
    if (!gst_init_done) {
        gst_init(NULL, NULL);
        gst_init_done = true;
    }
}

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
    
    m_register = std::make_unique<SipRegister>(m_ctx);
    m_event_handler = std::make_unique<SipEventHandler>(m_ctx);
    m_keepalive = std::make_unique<SipKeepAlive>();
    m_sdp_parser = std::make_unique<SdpParser>();
    m_rtp_streamer = std::make_unique<RtpStreamer>();

    m_initialized = true;
    printf("✅ SipClient 初始化成功\n");
    return true;
}

void SipClient::cleanup() {
    stopEventLoop();
    stopKeepAlive();
    unregister();
    
    if (m_ctx) {
        eXosip_quit(m_ctx);
        free(m_ctx);
        m_ctx = nullptr;
    }
    
    m_register.reset();
    m_event_handler.reset();
    m_keepalive.reset();
    m_sdp_parser.reset();
    m_rtp_streamer.reset();
    
    m_initialized = false;
    printf("✅ SipClient 已清理\n");
}

bool SipClient::registerToServer(const std::string& server_ip, int port,
                                 const std::string& username, const std::string& password) {
    if (!m_initialized) {
        printf("❌ SipClient 未初始化\n");
        return false;
    }
    
    m_server_ip = server_ip;
    m_server_port = port;
    m_username = username;
    m_password = password;
    m_server_id = username.substr(0, 10);
    
    bool success = m_register->registerToServer(
        server_ip, port, username, password, &m_register_id);
    
    if (success) {
        m_keepalive->init(server_ip, port, username, m_server_id);
    }
    
    return success;
}

bool SipClient::unregister() {
    if (m_register_id < 0) return false;
    return m_register->unregister(m_register_id);
}

void SipClient::startKeepAlive() {
    if (!m_initialized || m_register_id < 0) {
        printf("❌ 未注册，无法启动心跳\n");
        return;
    }
    m_keepalive->start(30);
}

void SipClient::stopKeepAlive() {
    m_keepalive->stop();
}

void SipClient::startEventLoop() {
    if (!m_initialized || m_register_id < 0) {
        printf("❌ 未注册，无法启动事件循环\n");
        return;
    }
    
    m_event_handler->setOnInvite([this](const std::string& target_ip, int target_port) {
        printf("📞 INVITE 回调: %s:%d\n", target_ip.c_str(), target_port);
        
        RtpStreamConfig config;
        config.rtsp_url = "rtsp://admin:z13312555@192.168.0.103:554/stream1";
        config.target_ip = target_ip;
        config.target_port = target_port;
        m_rtp_streamer->startStream(config);
    });
    
    m_event_handler->setOnBye([this]() {
        printf("📞 BYE 回调: 停止推流\n");
        m_rtp_streamer->stopStream();
    });
    
    m_running = true;
    m_event_thread = std::thread([this]() {
        m_event_handler->eventLoop();
    });
    
    printf("✅ 事件循环已启动\n");
}

void SipClient::stopEventLoop() {
    if (!m_running) return;
    m_running = false;
    m_event_handler->stop();
    if (m_event_thread.joinable()) {
        m_event_thread.join();
    }
    printf("✅ 事件循环已停止\n");
}