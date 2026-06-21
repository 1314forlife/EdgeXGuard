#include "sip_keepalive.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

SipKeepAlive::SipKeepAlive() 
    : m_server_port(0), m_running(false), m_interval_seconds(30) {}

SipKeepAlive::~SipKeepAlive() {
    stop();
}

void SipKeepAlive::init(const std::string& server_ip, int server_port,
                        const std::string& username, const std::string& server_id) {
    m_server_ip = server_ip;
    m_server_port = server_port;
    m_username = username;
    m_server_id = server_id;
}

void SipKeepAlive::start(int interval_seconds) {
    if (m_running) return;
    
    m_interval_seconds = interval_seconds;
    m_running = true;
    m_thread = std::thread(&SipKeepAlive::keepAliveLoop, this);
    printf("💓 心跳线程已启动 (间隔: %d 秒)\n", interval_seconds);
}

void SipKeepAlive::stop() {
    if (!m_running) return;
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
    printf("💓 心跳线程已停止\n");
}

bool SipKeepAlive::sendKeepAlive() {
    if (m_server_ip.empty() || m_server_port == 0) {
        return false;
    }

    static int sn = 1;
    time_t now = time(NULL);
    
    // 构建 XML Body
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

    std::string server_gb_id = m_server_id + "2000000001";

    // 构建 SIP MESSAGE
    char buf[2048];
    int len = snprintf(buf, sizeof(buf),
        "MESSAGE sip:%s@%s SIP/2.0\r\n"
        "Via: SIP/2.0/UDP %s:5062;branch=z9hG4bKkeepalive_%ld_%d\r\n"
        "From: <sip:%s@%s>;tag=keepalive_%ld\r\n"
        "To: <sip:%s@%s>\r\n"
        "Call-ID: keepalive_%ld@%s\r\n"
        "CSeq: %d MESSAGE\r\n"
        "Contact: <sip:%s@%s:5062>\r\n"
        "Content-Type: application/manscdp+xml\r\n"
        "Max-Forwards: 70\r\n"
        "User-Agent: EdgeXGuard\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        server_gb_id.c_str(), m_server_id.c_str(),
        m_server_ip.c_str(), now, sn,
        m_username.c_str(), m_server_id.c_str(), now,
        server_gb_id.c_str(), m_server_id.c_str(),
        now, m_server_ip.c_str(),
        sn,
        m_username.c_str(), m_server_ip.c_str(),
        xml_len,
        xml_body
    );

    // 发送 UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("❌ 创建 socket 失败\n");
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_server_port);
    addr.sin_addr.s_addr = inet_addr(m_server_ip.c_str());

    int ret = sendto(sockfd, buf, len, 0, (struct sockaddr*)&addr, sizeof(addr));
    close(sockfd);

    if (ret > 0) {
        printf("💓 心跳发送成功 (SN: %d)\n", sn);
        sn++;
        return true;
    } else {
        printf("❌ 心跳发送失败\n");
        return false;
    }
}

void SipKeepAlive::keepAliveLoop() {
    int ticks_needed = m_interval_seconds * 10;
    int tick_count = 0;

    while (m_running) {
        tick_count++;
        if (tick_count >= ticks_needed) {
            tick_count = 0;
            if (!m_running) break;
            sendKeepAlive();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}