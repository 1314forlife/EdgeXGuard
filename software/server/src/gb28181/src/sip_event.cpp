#include "sip_event.h"
#include "sdp_parser.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>   // ← 添加这行
#include <unistd.h>    // ← 添加这行（usleep）

SipEventHandler::SipEventHandler(eXosip_t* ctx) 
    : m_ctx(ctx), m_running(false) {}

void SipEventHandler::setOnInvite(OnInviteCallback callback) {
    m_on_invite = callback;
}

void SipEventHandler::setOnBye(OnByeCallback callback) {
    m_on_bye = callback;
}

void SipEventHandler::eventLoop() {
    m_running = true;
    printf("🔄 事件循环已启动\n");
    
    while (m_running) {
        eXosip_lock(m_ctx);
        eXosip_automatic_action(m_ctx);
        eXosip_unlock(m_ctx);
        
        eXosip_event_t* ev = eXosip_event_wait(m_ctx, 0, 1000);
        
        if (ev) {
            // 打印所有事件
            printf("📩 事件: type=%d", ev->type);
            if (ev->response) {
                printf(", status=%d", ev->response->status_code);
            }
            if (ev->request && ev->request->sip_method) {
                printf(", method=%s", ev->request->sip_method);
            }
            printf("\n");
            
            // 如果是 INVITE，处理它
            if (ev->request && ev->request->sip_method && 
                strcmp(ev->request->sip_method, "INVITE") == 0) {
                printf("📞 收到 INVITE！\n");
                handleInvite(ev);
                eXosip_event_free(ev);
                continue;
            }
            
            switch (ev->type) {
                case EXOSIP_CALL_INVITE:
                    printf("📞 收到 INVITE (type)\n");
                    handleInvite(ev);
                    break;
                case EXOSIP_CALL_CLOSED:
                case EXOSIP_CALL_RELEASED:
                    printf("📞 呼叫关闭\n");
                    handleBye(ev);
                    break;
                default:
                    break;
            }
            
            eXosip_event_free(ev);
        }
    }
}

void SipEventHandler::stop() {
    m_running = false;
}

bool SipEventHandler::processOneEvent() {
    eXosip_event_t* ev = eXosip_event_wait(m_ctx, 0, 0);
    if (!ev) return false;
    eXosip_event_free(ev);
    return true;
}

void SipEventHandler::handleInvite(eXosip_event_t* ev) {
    printf("📞 收到 INVITE 请求\n");
    
    SdpParser parser;
    SdpInfo sdp_info;
    
    if (ev->request) {
        sdp_info = parser.parseFromMessage(ev->request);
    }
    
    if (!sdp_info.is_valid) {
        printf("⚠️ SDP 解析失败，使用默认值\n");
        sdp_info.target_ip = "192.168.0.100";
        sdp_info.target_port = 5000;
    }
    
    printf("📡 目标地址: %s:%d\n", 
           sdp_info.target_ip.c_str(), sdp_info.target_port);
    
    osip_message_t* answer = nullptr;
    eXosip_lock(m_ctx);
    int ret = eXosip_call_build_answer(m_ctx, ev->tid, 200, &answer);
    eXosip_unlock(m_ctx);
    
    if (ret == 0 && answer) {
        std::string sdp = parser.buildResponseSdp("192.168.0.102", sdp_info.target_port);
        
        osip_message_set_body(answer, sdp.c_str(), sdp.length());
        osip_message_set_content_type(answer, "application/sdp");
        
        eXosip_lock(m_ctx);
        eXosip_call_send_answer(m_ctx, ev->tid, 200, answer);
        eXosip_unlock(m_ctx);
        
        printf("✅ 已发送 200 OK\n");
        
        // 修复：使用 std::string 作为参数
        if (m_on_invite) {
            m_on_invite(sdp_info.target_ip, sdp_info.target_port);
        }
    } else {
        printf("❌ 构建 200 OK 失败\n");
    }
}

void SipEventHandler::handleBye(eXosip_event_t* ev) {
    (void)ev;  // 消除未使用参数警告
    printf("📞 呼叫已关闭\n");
    if (m_on_bye) {
        m_on_bye();
    }
}

void SipEventHandler::handleMessage(eXosip_event_t* ev) {
    if (ev->type == EXOSIP_MESSAGE_ANSWERED && ev->response) {
        printf("📩 收到消息应答: %d\n", ev->response->status_code);
    }
}