#ifndef SIP_CLIENT_H
#define SIP_CLIENT_H

#include <eXosip2/eXosip.h>
#include <string>
#include <thread>
#include <atomic>

class SipClient {
public:
    SipClient();
    ~SipClient();

    bool init();
    void cleanup();
    bool registerToServer(const std::string& server_ip, int port,
                          const std::string& username, const std::string& password);
    bool isInitialized() const { return m_initialized; }
    bool sendKeepAlive();
    bool unregister();
    // 设置心跳间隔（秒），默认 30 秒
    void setKeepAliveInterval(int seconds) { m_keepalive_interval = seconds; }
    
    // 启动/停止心跳线程
    void startKeepAlive();
    void stopKeepAlive();

private:
    void keepAliveLoop();  // 心跳循环线程函数
    eXosip_t* m_ctx;                // 🆕 eXosip 上下文
    bool m_initialized;
    int m_register_id;
    std::string m_username;
    std::string m_server_id;
    std::string m_server_ip;
    int m_server_port;
    int m_keepalive_interval;

    std::thread m_keepalive_thread;
    std::atomic<bool> m_running;
};

#endif