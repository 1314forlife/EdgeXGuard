#include "sip_client.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>

SipClient* g_client = nullptr;

void signal_handler(int sig) {
    (void)sig;  // 消除未使用参数警告
    printf("\n⚠️ 收到信号，正在退出...\n");
    if (g_client) {
        g_client->stopEventLoop();
        g_client->stopKeepAlive();
        g_client->unregister();
        g_client->cleanup();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    (void)argc;   // 消除未使用参数警告
    (void)argv;   // 消除未使用参数警告
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << "========================================\n";
    std::cout << "  EdgeXGuard GB28181 测试程序\n";
    std::cout << "========================================\n\n";
    
    SipClient client;
    g_client = &client;
    
    std::cout << "[1] 初始化 SIP 客户端...\n";
    if (!client.init()) {
        std::cout << "❌ 初始化失败\n";
        return -1;
    }
    
    std::cout << "\n[2] 注册到 SIP 服务器...\n";
    if (!client.registerToServer("127.0.0.1", 5060, 
                                 "34020000002000000001", "12345678")) {
        std::cout << "❌ 注册失败\n";
        return -1;
    }
    
    std::cout << "\n[3] 启动心跳...\n";
    client.startKeepAlive();
    
    std::cout << "\n[4] 启动事件循环 (等待 INVITE)...\n";
    std::cout << "    按 Ctrl+C 退出\n\n";
    client.startEventLoop();
    
    while (true) {
        sleep(1);
    }
    
    return 0;
}