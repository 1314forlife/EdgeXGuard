#ifndef SIP_CLIENT_H
#define SIP_CLIENT_H

#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <eXosip2/eXosip.h>
#include <gst/gst.h>

// 前置声明
class SipRegister;
class SipEventHandler;
class SipKeepAlive;
class SdpParser;
class RtpStreamer;

class SipClient {
public:
    SipClient();
    ~SipClient();

    bool init();
    void cleanup();
    bool registerToServer(const std::string& server_ip, int port,
                         const std::string& username, const std::string& password);
    bool unregister();
    void startKeepAlive();
    void stopKeepAlive();
    void startEventLoop();
    void stopEventLoop();
    
    bool isInitialized() const { return m_initialized; }
    bool isRegistered() const { return m_register_id >= 0; }
    bool isRunning() const { return m_running; }
    
    eXosip_t* getContext() { return m_ctx; }
    int getRegisterId() const { return m_register_id; }
    std::string getUsername() const { return m_username; }
    std::string getServerIp() const { return m_server_ip; }
    int getServerPort() const { return m_server_port; }
    std::string getServerId() const { return m_server_id; }

private:
    eXosip_t* m_ctx;
    bool m_initialized;
    std::atomic<bool> m_running;
    
    int m_register_id;
    std::string m_username;
    std::string m_password;
    std::string m_server_ip;
    int m_server_port;
    std::string m_server_id;
    
    std::unique_ptr<SipRegister> m_register;
    std::unique_ptr<SipEventHandler> m_event_handler;
    std::unique_ptr<SipKeepAlive> m_keepalive;
    std::unique_ptr<SdpParser> m_sdp_parser;
    std::unique_ptr<RtpStreamer> m_rtp_streamer;
    
    std::thread m_event_thread;
};

#endif