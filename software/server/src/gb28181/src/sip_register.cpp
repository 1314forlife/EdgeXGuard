#include "sip_register.h"
#include <stdio.h>
#include <string.h>

SipRegister::SipRegister(eXosip_t* ctx) : m_ctx(ctx) {}

bool SipRegister::registerToServer(const std::string& server_ip, int port,
                                   const std::string& username, const std::string& password,
                                   int* out_register_id) {
    m_server_ip = server_ip;
    m_server_port = port;
    m_username = username;
    m_password = password;
    m_server_id = username.substr(0, 10);
    
    printf("========================================\n");
    printf("📡 注册到 SIP 服务器: %s:%d\n", server_ip.c_str(), port);
    printf("   用户名: %s\n", username.c_str());
    printf("========================================\n");

    // ★★★ 注释掉认证（测试无认证模式）★★★
    // eXosip_clear_authentication_info(m_ctx);
    // eXosip_add_authentication_info(m_ctx, 
    //                                username.c_str(), 
    //                                username.c_str(), 
    //                                password.c_str(), 
    //                                "MD5", 
    //                                "test_realm");

    char from_uri[256];
    char proxy_uri[256];
    char reg_uri[256];
    
    snprintf(from_uri, sizeof(from_uri), "sip:%s@%s", username.c_str(), m_server_id.c_str());
    snprintf(proxy_uri, sizeof(proxy_uri), "sip:%s:%d", server_ip.c_str(), port);
    snprintf(reg_uri, sizeof(reg_uri), "sip:%s", m_server_id.c_str());

    osip_message_t* reg_msg = nullptr;
    int register_id = eXosip_register_build_initial_register(
        m_ctx, from_uri, proxy_uri, reg_uri, 3600, &reg_msg);

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

    printf("✅ REGISTER 已发送\n");
    printf("🔄 触发发送...\n");
    
    eXosip_lock(m_ctx);
    eXosip_automatic_action(m_ctx);
    eXosip_unlock(m_ctx);
    
    if (waitForRegistration(register_id, 20)) {
        if (out_register_id) *out_register_id = register_id;
        return true;
    }
    
    return false;
}

bool SipRegister::waitForRegistration(int register_id, int timeout_seconds) {
    int wait_count = 0;
    int max_wait = timeout_seconds * 10;
    
    (void)register_id;
    
    printf("⏳ 等待注册响应 (超时: %d 秒)...\n", timeout_seconds);
    
    while (wait_count < max_wait) {
        eXosip_lock(m_ctx);
        eXosip_automatic_action(m_ctx);
        eXosip_unlock(m_ctx);
        
        eXosip_event_t* ev = eXosip_event_wait(m_ctx, 0, 100);
        
        if (ev) {
            // ★★★ 打印所有事件 ★★★
            printf("📩 事件: type=%d", ev->type);
            if (ev->response) {
                printf(", status=%d", ev->response->status_code);
            }
            if (ev->request && ev->request->sip_method) {
                printf(", method=%s", ev->request->sip_method);
            }
            printf("\n");
            
            switch (ev->type) {
                case EXOSIP_REGISTRATION_SUCCESS:
                    printf("✅ 注册成功！\n");
                    eXosip_event_free(ev);
                    return true;
                    
                case EXOSIP_REGISTRATION_FAILURE: {
                    int status = ev->response ? ev->response->status_code : 0;
                    printf("❌ 注册失败！状态码: %d\n", status);
                    eXosip_event_free(ev);
                    return false;
                }
                    
                default:
                    // 如果收到 200 OK，也认为成功
                    if (ev->response && ev->response->status_code == 200) {
                        if (ev->request && ev->request->sip_method && 
                            strcmp(ev->request->sip_method, "REGISTER") == 0) {
                            printf("✅ 注册成功！(收到 200 OK)\n");
                            eXosip_event_free(ev);
                            return true;
                        }
                    }
                    eXosip_event_free(ev);
                    break;
            }
        }
        
        wait_count++;
        if (wait_count % 20 == 0) {
            printf("⏳ 等待中... (%d/%d 秒)\n", wait_count/10, timeout_seconds);
        }
    }
    
    printf("⏰ 注册超时\n");
    return false;
}

bool SipRegister::unregister(int register_id) {
    osip_message_t* reg_msg = nullptr;
    int ret = eXosip_register_build_register(m_ctx, register_id, 0, &reg_msg);
    if (ret != 0) {
        printf("❌ 构建注销消息失败\n");
        return false;
    }

    eXosip_lock(m_ctx);
    ret = eXosip_register_send_register(m_ctx, register_id, reg_msg);
    eXosip_unlock(m_ctx);

    if (ret == 0) {
        printf("🔴 注销请求已发送\n");
        return true;
    }
    return false;
}