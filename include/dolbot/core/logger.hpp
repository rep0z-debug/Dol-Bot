#pragma once

#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <iostream>
#include <sstream>
#include <QStandardPaths>
#include <QString>
#include <QDir>

namespace dolbot::core {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }
    
    void set_level(LogLevel level) { min_level_ = level; }
    void set_console_output(bool enabled) { console_output_ = enabled; }
    
    void log(LogLevel level, const std::string& message) {
        if (level < min_level_) return;
        
        std::lock_guard lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        ss << " [" << level_str(level) << "] " << message << "\n";
        
        std::string formatted = ss.str();
        
        if (console_output_) {
            std::cout << formatted;
        }
        
        if (file_.is_open()) {
            file_ << formatted;
            file_.flush();
        }
    }
    
    void debug(const std::string& msg) { log(LogLevel::Debug, msg); }
    void info(const std::string& msg) { log(LogLevel::Info, msg); }
    void warning(const std::string& msg) { log(LogLevel::Warning, msg); }
    void error(const std::string& msg) { log(LogLevel::Error, msg); }
    
private:
    Logger() {
        QString log_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(log_dir);
        QString log_path = log_dir + "/dolbot.log";
        file_.open(log_path.toStdString(), std::ios::app);
    }
    
    ~Logger() {
        if (file_.is_open()) file_.close();
    }
    
    const char* level_str(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARN";
            case LogLevel::Error: return "ERROR";
        }
        return "UNKNOWN";
    }
    
    LogLevel min_level_ = LogLevel::Info;
    bool console_output_ = true;
    std::ofstream file_;
    std::mutex mutex_;
};

#define LOG_DEBUG(msg) dolbot::core::Logger::instance().debug(msg)
#define LOG_INFO(msg) dolbot::core::Logger::instance().info(msg)
#define LOG_WARN(msg) dolbot::core::Logger::instance().warning(msg)
#define LOG_ERROR(msg) dolbot::core::Logger::instance().error(msg)

} // namespace dolbot::core
