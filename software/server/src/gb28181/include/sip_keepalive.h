#ifndef SIP_KEEPALIVE_H
#define SIP_KEEPALIVE_H

#include <string>
#include <atomic>
#include <thread>

/**
 * SIP 心跳模块
 * 负责定期发送 Keepalive 消息
 */
class SipKeepAlive {
public:
    SipKeepAlive();
    ~SipKeepAlive();

    /**
     * 初始化心跳
     */
    void init(const std::string& server_ip, int server_port,
              const std::string& username, const std::string& server_id);
    
    /**
     * 启动心跳（独立线程）
     * @param interval_seconds 心跳间隔（秒）
     */
    void start(int interval_seconds = 30);
    
    /**
     * 停止心跳
     */
    void stop();
    
    /**
     * 手动发送一次心跳
     */
    bool sendKeepAlive();

private:
    std::string m_server_ip;
    int m_server_port;
    std::string m_username;
    std::string m_server_id;
    std::atomic<bool> m_running;
    std::thread m_thread;
    int m_interval_seconds;
    
    void keepAliveLoop();
    std::string buildKeepAliveMessage(int sn);
};

#endif