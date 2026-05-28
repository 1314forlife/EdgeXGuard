#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QDebug>
#include <QFile>
#include <QMutex>
#include <QDateTime>
#include <QTextStream>
#include <cstdio>

enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3
};

class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    void setLevel(LogLevel level) { m_level = level; }
    void setFile(const QString& path);
    void flush();

    void debug(const QString& tag, const QString& msg) {
        if (m_level <= LOG_DEBUG) log(LOG_DEBUG, tag, msg);
    }
    void info(const QString& tag, const QString& msg) {
        if (m_level <= LOG_INFO) log(LOG_INFO, tag, msg);
    }
    void warn(const QString& tag, const QString& msg) {
        if (m_level <= LOG_WARN) log(LOG_WARN, tag, msg);
    }
    void error(const QString& tag, const QString& msg) {
        if (m_level <= LOG_ERROR) log(LOG_ERROR, tag, msg);
    }

private:
    Logger() : m_level(LOG_DEBUG), m_stream(&m_file) {}
    void log(LogLevel level, const QString& tag, const QString& msg);

    LogLevel m_level;
    QFile m_file;
    QTextStream m_stream;
    QMutex m_mutex;
};

// 便捷宏
#define LOG_DEBUG(tag, msg) Logger::instance().debug(tag, msg)
#define LOG_INFO(tag, msg)  Logger::instance().info(tag, msg)
#define LOG_WARN(tag, msg)  Logger::instance().warn(tag, msg)
#define LOG_ERROR(tag, msg) Logger::instance().error(tag, msg)

#endif