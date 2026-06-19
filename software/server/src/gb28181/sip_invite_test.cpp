#include "sip_client.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("========================================\n");
    printf("🧪 GB28181 INVITE 视频点播测试\n");
    printf("========================================\n\n");

    SipClient sip;
    
    printf("📌 步骤 1: 初始化...\n");
    if (!sip.init()) {
        printf("❌ 初始化失败\n");
        return 1;
    }
    printf("✅ 初始化成功\n\n");

    printf("📌 步骤 2: 注册到 SIP 服务器...\n");
    if (sip.registerToServer("192.168.0.102", 5060, 
                              "34020000001320000001", "123456")) {
        printf("✅ 注册成功，等待 INVITE 请求...\n");
        printf("📌 步骤 3: 事件循环运行中（等待平台发起 INVITE）...\n");
        
        // 保持运行，等待 INVITE
        while (1) {
            sleep(10);
            printf("⏳ 等待 INVITE...\n");
        }
    }

    return 0;
}