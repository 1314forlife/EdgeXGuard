// core/common/logger.cpp
#include "logger.h"
#include <QDateTime>

void Logger::setFile(const QString& path)
{
    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_file.close();
    }
    m_file.setFileName(path);
    if (m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_file);
    }
}

void Logger::flush()
{
    QMutexLocker locker(&m_mutex);
    m_stream.flush();
}

void Logger::log(LogLevel level, const QString& tag, const QString& msg)
{
    QMutexLocker locker(&m_mutex);

    if (level < m_level) {
        return;
    }

    const char* levelStrs[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    QString line = QString("%1 [%2] %3 - %4\n")
                       .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
                       .arg(levelStrs[level])
                       .arg(tag)
                       .arg(msg);

    // 输出到控制台
    fprintf(stdout, "%s", line.toUtf8().constData());
    fflush(stdout);

    // 输出到文件
    if (m_file.isOpen()) {
        m_stream << line;
        m_stream.flush();
    }
}