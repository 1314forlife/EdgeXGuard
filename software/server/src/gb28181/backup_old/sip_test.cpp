#include "sip_client.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("========================================\n");
    printf("🧪 GB28181 SIP 模块独立测试\n");
    printf("========================================\n\n");

    SipClient sip;
    
    printf("📌 测试 1: 初始化...\n");
    if (!sip.init()) {
        printf("❌ 初始化失败\n");
        return 1;
    }
    printf("✅ 初始化成功\n\n");

    printf("📌 测试 2: 注册到 SIP 服务器...\n");
    sip.setKeepAliveInterval(15);
    
    if (sip.registerToServer("127.0.0.1", 5060, 
                      "34020000001320000001", "123456")) {
        printf("✅ 注册成功\n");
        printf("📌 测试 3: 等待 INVITE 视频点播请求...\n");
        printf("⏳ 保持运行，按 Ctrl+C 退出\n\n");
        
        // 保持运行，等待 INVITE
        while (1) {
            sleep(10);
        }
    }

    printf("\n📌 测试完成\n");
    return 0;
}