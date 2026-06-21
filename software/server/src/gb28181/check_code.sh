#!/bin/bash
echo "=========================================="
echo "  EdgeXGuard 代码检查"
echo "=========================================="

echo ""
echo "[1] 检查监听端口 (应该是 5062)"
grep -n "eXosip_listen_addr" src/sip_client.cpp | grep -v "//"

echo ""
echo "[2] 检查 eventLoop 是否有 eXosip_event_wait"
grep -n "eXosip_event_wait" src/sip_event.cpp | head -3

echo ""
echo "[3] 检查 handleInvite 是否存在"
grep -n "void SipEventHandler::handleInvite" src/sip_event.cpp

echo ""
echo "[4] 检查 handleInvite 是否有 send_answer"
grep -n "eXosip_call_send_answer" src/sip_event.cpp

echo ""
echo "[5] 检查认证是否被注释 (无认证模式)"
grep -n "eXosip_add_authentication_info" src/sip_register.cpp | head -2

echo ""
echo "[6] 检查 OnInviteCallback 定义"
grep "OnInviteCallback" include/sip_event.h

echo ""
echo "[7] 检查 CMakeLists.txt 源文件"
grep "src/" CMakeLists.txt | grep -v "#"

echo ""
echo "=========================================="
echo "  检查完成！"
echo "=========================================="