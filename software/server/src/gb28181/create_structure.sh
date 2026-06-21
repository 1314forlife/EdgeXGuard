# 把上面的脚本内容粘贴进来
#!/bin/bash

# EdgeXGuard GB28181 模块 - 创建目录和空文件结构
# 使用方法: chmod +x create_structure.sh && ./create_structure.sh

set -e

echo "=========================================="
echo "  EdgeXGuard GB28181 模块"
echo "  创建目录和空文件"
echo "=========================================="

BASE_DIR="$(pwd)"
echo "📁 基础目录: $BASE_DIR"

# 1. 创建目录
echo ""
echo "[1] 创建目录..."
mkdir -p include
mkdir -p src
mkdir -p backup_old

# 2. 备份旧文件
echo "[2] 备份旧文件..."
[ -f "sip_client.cpp" ] && mv sip_client.cpp backup_old/
[ -f "sip_client.h" ] && mv sip_client.h backup_old/
[ -f "sip_test.cpp" ] && mv sip_test.cpp backup_old/

# 3. 创建头文件（空）
echo "[3] 创建头文件..."
touch include/sip_client.h
touch include/sip_register.h
touch include/sip_event.h
touch include/sip_keepalive.h
touch include/sdp_parser.h
touch include/rtp_streamer.h
echo "    ✅ 已创建 6 个头文件"

# 4. 创建源文件（空）
echo "[4] 创建源文件..."
touch src/sip_client.cpp
touch src/sip_register.cpp
touch src/sip_event.cpp
touch src/sip_keepalive.cpp
touch src/sdp_parser.cpp
touch src/rtp_streamer.cpp
echo "    ✅ 已创建 6 个源文件"

# 5. 创建测试文件
echo "[5] 创建测试文件..."
touch sip_test.cpp
touch sip_invite_test.cpp

# 6. 更新 CMakeLists.txt（空模板）
echo "[6] 更新 CMakeLists.txt..."
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(gb28181)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(PkgConfig REQUIRED)
pkg_check_modules(EXOSIP REQUIRED libeXosip2)
pkg_check_modules(GST REQUIRED gstreamer-1.0)

set(SOURCES
    src/sip_client.cpp
    src/sip_register.cpp
    src/sip_event.cpp
    src/sip_keepalive.cpp
    src/sdp_parser.cpp
    src/rtp_streamer.cpp
)

add_executable(sip_test sip_test.cpp ${SOURCES})

target_include_directories(sip_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${EXOSIP_INCLUDE_DIRS}
    ${GST_INCLUDE_DIRS}
)

target_link_libraries(sip_test
    ${EXOSIP_LIBRARIES}
    ${GST_LIBRARIES}
    pthread
)

target_compile_options(sip_test PRIVATE -Wall -Wextra)
EOF
echo "    ✅ CMakeLists.txt 已更新"

# 7. 显示目录结构
echo ""
echo "[7] 目录结构："
echo ""
tree -L 2 2>/dev/null || find . -type f -name "*.h" -o -name "*.cpp" -o -name "CMakeLists.txt" | grep -v build | sort

echo ""
echo "=========================================="
echo "✅ 完成！创建了以下文件："
echo "   include/  (6个头文件)"
echo "   src/      (6个源文件)"
echo "   sip_test.cpp"
echo "   sip_invite_test.cpp"
echo "   CMakeLists.txt"
echo "=========================================="
echo ""
echo "📝 现在你可以填充各文件的内容了"