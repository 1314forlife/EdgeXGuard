#ifndef SIP_EVENT_H
#define SIP_EVENT_H

#include <eXosip2/eXosip.h>
#include <functional>
#include <string>  // ← 添加这行

// 回调函数类型
using OnInviteCallback = std::function<void(const std::string& target_ip, int target_port)>;
using OnByeCallback = std::function<void()>;

/**
 * SIP 事件处理模块
 * 负责处理 INVITE、BYE 等呼叫事件
 */
class SipEventHandler {
public:
    SipEventHandler(eXosip_t* ctx);
    ~SipEventHandler() = default;

    void setOnInvite(OnInviteCallback callback);
    void setOnBye(OnByeCallback callback);
    bool processOneEvent();
    void eventLoop();
    void stop();

private:
    eXosip_t* m_ctx;
    bool m_running;
    OnInviteCallback m_on_invite;
    OnByeCallback m_on_bye;
    
    void handleInvite(eXosip_event_t* ev);
    void handleBye(eXosip_event_t* ev);
    void handleMessage(eXosip_event_t* ev);
};

#endif