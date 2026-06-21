#ifndef SDP_PARSER_H
#define SDP_PARSER_H

#include <string>
#include <osip2/osip.h>

/**
 * SDP 解析结果
 */
struct SdpInfo {
    std::string target_ip;    // 目标 IP
    int target_port;          // 目标端口
    std::string session_name; // 会话名
    bool is_valid;            // 是否有效
    
    SdpInfo() : target_port(5000), is_valid(false) {}
};

/**
 * SDP 解析模块
 * 负责解析 SIP 消息中的 SDP body
 */
class SdpParser {
public:
    SdpParser() = default;
    ~SdpParser() = default;

    /**
     * 从 osip_message_t 解析 SDP
     * @param msg SIP 消息
     * @return SDP 信息
     */
    SdpInfo parseFromMessage(osip_message_t* msg);
    
    /**
     * 从字符串解析 SDP
     * @param sdp_body SDP 内容
     * @return SDP 信息
     */
    SdpInfo parseFromString(const std::string& sdp_body);
    
    /**
     * 构建 200 OK 响应的 SDP
     * @param local_ip 本机 IP
     * @param target_port 目标端口
     * @return SDP 字符串
     */
    std::string buildResponseSdp(const std::string& local_ip, int target_port);

private:
    std::string extractIP(const std::string& sdp);
    int extractPort(const std::string& sdp);
};

#endif