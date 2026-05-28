@echo off
echo ========================================
echo 重构 core 目录结构
echo ========================================
echo.

cd /d %~dp0core

echo 1. 创建新目录...
mkdir common 2>nul
mkdir decode 2>nul
mkdir render 2>nul
mkdir player 2>nul
echo   - common (通用工具)
echo   - decode (解码模块)
echo   - render (渲染模块)
echo   - player (播放器模块)
echo.

echo 2. 移动原有文件到 common...
if exist config\* (
    move config\* common\ 2>nul
    echo   - config\* -> common\
    rmdir config 2>nul
)
if exist logger\* (
    move logger\* common\ 2>nul
    echo   - logger\* -> common\
    rmdir logger 2>nul
)
if exist utils\* (
    move utils\* common\ 2>nul
    echo   - utils\* -> common\
    rmdir utils 2>nul
)
echo.

echo 3. thread 目录保留
if not exist thread mkdir thread
echo   - thread (线程工具)
echo.

echo 4. 创建占位文件（可选）
type nul > common\logger.h 2>nul
type nul > common\config.h 2>nul
type nul > common\utils.h 2>nul
type nul > decode\demuxer.h 2>nul
type nul > decode\demuxer.cpp 2>nul
type nul > decode\video_decoder.h 2>nul
type nul > decode\video_decoder.cpp 2>nul
type nul > render\opengl_renderer.h 2>nul
type nul > render\opengl_renderer.cpp 2>nul
type nul > player\rtmp_player.h 2>nul
type nul > player\rtmp_player.cpp 2>nul
echo   - 占位文件已创建
echo.

echo ========================================
echo 重构完成！
echo ========================================
echo.
echo 目录结构：
echo core/
echo ├── common/
echo ├── thread/
echo ├── decode/
echo ├── render/
echo └── player/
echo.
pause