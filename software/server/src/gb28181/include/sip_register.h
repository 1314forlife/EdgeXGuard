#ifndef SIP_REGISTER_H
#define SIP_REGISTER_H

#include <string>
#include <eXosip2/eXosip.h>

/**
 * SIP 注册处理模块
 * 负责 REGISTER 流程（含 401 认证）
 */
class SipRegister {
public:
    SipRegister(eXosip_t* ctx);
    ~SipRegister() = default;

    /**
     * 注册到 SIP 服务器
     * @param server_ip 服务器 IP
     * @param port 服务器端口
     * @param username 用户名（国标ID）
     * @param password 密码
     * @param out_register_id 输出注册ID
     * @return 成功返回 true
     */
    bool registerToServer(const std::string& server_ip, int port,
                          const std::string& username, const std::string& password,
                          int* out_register_id);
    
    /**
     * 注销
     * @param register_id 注册ID
     * @return 成功返回 true
     */
    bool unregister(int register_id);

private:
    eXosip_t* m_ctx;
    std::string m_server_ip;
    int m_server_port;
    std::string m_username;
    std::string m_password;
    std::string m_server_id;
    
    bool waitForRegistration(int register_id, int timeout_seconds);
};

#endif