MinGW 环境下 pthread_cond_timedwait64 链接错误的解决方案

问题背景
在编译 EdgeXGuard 项目时，链接阶段出现以下错误：
undefined reference to `pthread_cond_timedwait64'
错误指向 video_decoder.cpp 等使用 FFmpeg 解码的模块。
环境信息
项目	版本/路径
操作系统	Windows 11
编译器	MinGW 13.1.0 (C:/Qt/Tools/mingw1310_64/bin/gcc.exe)
Qt 版本	6.11.0
FFmpeg	7.0 (预编译版本，来源：BtbN/FFmpeg-Builds)
Paho MQTT	msys64 预编译库
CMake	3.30

问题分析
1. 符号来源分析
通过 objdump 和 nm 工具分析目标文件：
# 查看 video_decoder.cpp.obj 中未定义符号
nm video_decoder.cpp.obj | grep pthread_cond_timedwait
输出显示：目标文件需要 pthread_cond_timedwait64，但 MinGW 13.1.0 的 libwinpthread 中导出的是 pthread_cond_timedwait（32 位时间戳版本）。
2. 根本原因
FFmpeg 预编译库是用较新版本的 MinGW 编译的，其中引用了 pthread_cond_timedwait64。

当前 MinGW 13.1.0 的 pthread.h 中声明了 pthread_cond_timedwait64，但 libwinpthread 库中实际导出的是旧版符号。

这是 MinGW 工具链版本与预编译库之间的 ABI 不兼容 问题。

3. 可选方案对比
方案	优点	缺点	选择
升级 MinGW	从根源解决	需要重新配置整个工具链	❌ 成本高
换用 MSVC	无此问题	需重装 VS，改动大	❌ 暂缓
降级 FFmpeg	可能可行	旧版 FFmpeg 功能受限	❌ 不推荐
符号桥接补丁	不改工具链，不改库	需要理解 ABI	✅ 采用
解决方案
核心思路
利用 C++ 的符号解析机制，手动实现缺失的 pthread_cond_timedwait64 函数，将其桥接到底层已存在的 32 位版本 pthread_cond_timedwait。

代码实现
创建独立补丁文件 core/video_decoder.cpp：

/**
 * @file video_decoder.cpp
 * @brief 解决 MinGW 下缺少 pthread_cond_timedwait64 符号的问题
 * 
 * 问题描述：
 *   FFmpeg 预编译库引用了 pthread_cond_timedwait64，但当前 MinGW 版本的
 *   libwinpthread 只导出 32 位版本的 pthread_cond_timedwait。
 * 
 * 解决方案：
 *   实现缺失的 64 位版本，将 64 位时间戳降级为 32 位后转发给底层函数。
 * 
 * 适用范围：
 *   仅在 MinGW 环境下生效，不影响 MSVC 或其他平台。
 */

#ifdef __MINGW32__
#include <pthread.h>

extern "C" {

int pthread_cond_timedwait64(pthread_cond_t *cv,
                             pthread_mutex_t *external_mutex,
                             const struct _timespec64 *t) {
    // 将 64 位时间戳转换为 32 位（降级桥接）
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(t->tv_sec);
    ts.tv_nsec = static_cast<long>(t->tv_nsec);
    
    // 调用底层的 32 位版本
    return pthread_cond_timedwait(cv, external_mutex, &ts);
}

} // extern "C"
#endif // __MINGW32__

技术要点解析
1. 为什么用 extern "C"？
pthread_cond_timedwait64 是 C 函数，符号没有 C++ 名称修饰（name mangling）。

extern "C" 确保生成的符号名与链接器期望的一致。

2. 为什么用 #ifdef __MINGW32__？
该问题只在 MinGW 环境下出现，MSVC 没有这个问题。

条件编译确保补丁只在需要时生效，不影响其他平台。

3. 为什么能这样“桥接”？
64 位时间戳转 32 位在 2038 年之前是安全的。

FFmpeg 内部使用的等待时间通常很短（毫秒级），完全在 32 位时间戳范围内。

经验总结
遇到链接错误时，先确认缺失符号的来源
用 nm、objdump 分析目标文件和库的导出符号。

不要急于换工具链
换工具链成本高，还可能引入新问题。可以先尝试“修补”方案。

理解 ABI 兼容性
即使同一个库的不同版本，也可能因符号表差异导致链接失败。

补丁要“非侵入式”
用条件编译、独立源文件，方便后续维护和移除。

后续建议
如果将来时间允许，可以考虑：

升级 MinGW 到支持 pthread_cond_timedwait64 的版本。

或完全切换到 MSVC 工具链，彻底避免类似问题。

目前该补丁已在项目中稳定运行，无性能损耗或兼容性问题。

