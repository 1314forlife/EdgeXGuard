#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include "infrastructure/audio/AudioInputManager.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    AudioInputManager audio;

    // 测试实时采集
    QObject::connect(&audio, &AudioInputManager::audioDataReady,
                     [](const QByteArray& data) {
                         static int count = 0;
                         count += data.size();
                         if (count > 16000 * 2) { // 约1秒数据
                             qDebug() << "[Test] 收到音频数据:" << data.size() << "bytes, 累计:" << count;
                             count = 0;
                         }
                     });

    // 测试录音
    QObject::connect(&audio, &AudioInputManager::recordingCompleted,
                     [](const QString& path) {
                         qDebug() << "[Test] 录音保存到:" << path;
                     });

    qDebug() << "启动音频采集...";
    audio.start();

    // 5秒后开始录音测试
    QTimer::singleShot(5000, [&audio]() {
        qDebug() << "开始录音(3秒)...";
        audio.startRecording("test_voiceprint.pcm");

        QTimer::singleShot(3000, [&audio]() {
            audio.stopRecording();
        });
    });

    // 10秒后退出
    QTimer::singleShot(10000, &app, &QCoreApplication::quit);

    return app.exec();
}