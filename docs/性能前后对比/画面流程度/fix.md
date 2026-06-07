性能瓶颈三：2 到 3 秒恶性时差延时的粉碎性压榨（超低延迟调优）
3.1 现象还原
在解除了闪退和死锁后，2K 视频流能够极其稳定地持续播放，但在局域网测试中，画面发生了 2 到 3 秒的严重时差滞后。对于人脸识别、白名单闸机等工业级安防场景，这属于无法落地级严重体验灾难（指标应控制在 500ms 以内）。

3.2 根本原因剖析
原 Demuxer::open 函数由于将 avformat_open_input 的参数字典传为了 nullptr。这导致 FFmpeg 启动了其默认的网络保护机制：默认开辟了庞大的探测流缓存区（分析时间默认高达 5 秒），且内部套接字开启了厚重的 Buffer 囤积，导致下游解码器吃到的永远是几秒前的“过期老包”。

3.3 零缓存、零时差重构完全体 (demuxer.cpp)
我们通过挂载挂满无延迟黑客指令的 AVDictionary 字典，强行阉割 FFmpeg 内部的所有分析时间与全局网络缓存，并强推可靠的 TCP 物理信道：
// core/demux/demuxer.cpp 极限无延迟重构版
bool Demuxer::open(const QString& url) {
    close();
    LOG_INFO("Demuxer", QString("open() - URL: %1").arg(url));

    // ==================== 【★核心降延时重构：物理切除一切缓存】 ====================
    AVDictionary* options = nullptr;

    // 1. 强行降低 FFmpeg 探测和分析流信息的时间（由 5 秒极限压缩到 50 毫秒）
    av_dict_set(&options, "probesize", "50000", 0);
    av_dict_set(&options, "analyzeduration", "50000", 0);

    // 2. 物理掐灭套接字和网络流的全局内部缓存！一有包立刻吐出，拒绝囤积
    av_dict_set(&options, "fflags", "nobuffer", 0);
    av_dict_set(&options, "tune", "zerolatency", 0); // 注入硬核零延迟内核模式

    // 3. 强制走 TCP 物理信道，防止 UDP 分包损坏造成的图像大面积绿色闪烁或花屏
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    
    // 4. 加入网络握手超时保护（3秒），防止网络断开时死锁卡主界面
    av_dict_set(&options, "stimeout", "3000000", 0);

    // 5. 点火打开 RTSP 输入流
    if (avformat_open_input(&m_formatCtx, url.toStdString().c_str(), nullptr, &options) < 0) {
        LOG_ERROR("Demuxer", "Cannot open file/stream");
        emit sigError("Cannot open file/stream");
        if (options) av_dict_free(options);
        return false;
    }

    if (options) {
        av_dict_free(&options);
    }
    // =================================================================================

    LOG_INFO("Demuxer", "avformat_open_input success");

    // 🌟 配合降延时：分析流参数传入 nullptr，防止二次解析停顿
    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        LOG_ERROR("Demuxer", "Cannot find stream info");
        emit sigError("Cannot find stream info");
        return false;
    }
    
    // ... 保持原有的视频流流索引提取与参数复制逻辑 ...
    return true;
}