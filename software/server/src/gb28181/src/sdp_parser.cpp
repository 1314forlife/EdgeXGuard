#include "sdp_parser.h"
#include <stdio.h>
#include <string.h>
#include <cstdlib>

SdpInfo SdpParser::parseFromMessage(osip_message_t* msg) {
    SdpInfo info;
    
    osip_body_t* body = NULL;
    int ret = osip_message_get_body(msg, 0, &body);
    if (ret != 0 || body == NULL) {
        printf("❌ 获取 SDP body 失败\n");
        return info;
    }
    
    char* body_str = NULL;
    size_t length = 0;
    ret = osip_body_to_str(body, &body_str, &length);
    if (ret != 0 || body_str == NULL || length == 0) {
        printf("❌ 转换 SDP body 失败\n");
        return info;
    }
    
    std::string sdp_body(body_str, length);
    free(body_str);
    
    info = parseFromString(sdp_body);
    info.is_valid = true;
    
    return info;
}

SdpInfo SdpParser::parseFromString(const std::string& sdp_body) {
    SdpInfo info;
    
    printf("📄 SDP Body:\n%s\n", sdp_body.c_str());
    
    info.target_ip = extractIP(sdp_body);
    info.target_port = extractPort(sdp_body);
    info.is_valid = true;
    
    printf("📡 解析结果: IP=%s, Port=%d\n", 
           info.target_ip.c_str(), info.target_port);
    
    return info;
}

std::string SdpParser::extractIP(const std::string& sdp) {
    const char* c_line = strstr(sdp.c_str(), "c=IN IP4 ");
    if (c_line) {
        c_line += 9;
        const char* end = c_line;
        while (*end && *end != '\r' && *end != '\n') end++;
        return std::string(c_line, end - c_line);
    }
    return "192.168.0.100";
}

int SdpParser::extractPort(const std::string& sdp) {
    const char* m_line = strstr(sdp.c_str(), "m=video ");
    if (m_line) {
        m_line += 9;
        int port = 0;
        if (sscanf(m_line, "%d", &port) == 1) {
            return port;
        }
    }
    return 5000;
}

std::string SdpParser::buildResponseSdp(const std::string& local_ip, int target_port) {
    char sdp[1024];
    snprintf(sdp, sizeof(sdp),
        "v=0\r\n"
        "o=34020000001320000001 0 0 IN IP4 %s\r\n"
        "s=Play\r\n"
        "c=IN IP4 %s\r\n"
        "t=0 0\r\n"
        "m=video %d RTP/AVP 96\r\n"
        "a=rtpmap:96 H264/90000\r\n"
        "a=sendonly\r\n",
        local_ip.c_str(),
        local_ip.c_str(),
        target_port
    );
    return std::string(sdp);
}