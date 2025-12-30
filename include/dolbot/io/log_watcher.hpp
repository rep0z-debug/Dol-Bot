#pragma once

#include <QObject>
#include <QFileSystemWatcher>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QTimer>
#include <deque>
#include <string>
#include <functional>
#include "dolbot/core/logger.hpp"
#include "dolbot/core/config.hpp"

namespace dolbot::io {

class LogWatcher : public QObject {
    Q_OBJECT

public:
    static LogWatcher& instance() {
        static LogWatcher inst;
        return inst;
    }

    void start() {
        if (config_connected_) return;
        
        const auto& settings = core::Config::instance().settings();
        if (settings.auto_reset_enabled && !settings.log_file_path.empty()) {
            watch_file(QString::fromStdString(settings.log_file_path));
        }

        config_handle_ = core::Config::instance().on_change([this](const core::AppSettings& s) {
            if (s.log_file_path != current_file_.toStdString() || s.auto_reset_enabled != enabled_) {
                if (s.auto_reset_enabled && !s.log_file_path.empty()) {
                    watch_file(QString::fromStdString(s.log_file_path));
                } else {
                    stop_watching();
                }
            }
        });
        config_connected_ = true;
    }

    void set_reset_callback(std::function<void()> callback) {
        reset_callback_ = std::move(callback);
    }

private:
    LogWatcher() : watcher_(this) {
        connect(&watcher_, &QFileSystemWatcher::fileChanged, this, &LogWatcher::on_file_changed);
        timer_.setSingleShot(true);
        connect(&timer_, &QTimer::timeout, this, &LogWatcher::on_file_changed_delayed);
    }

    void watch_file(const QString& path) {
        if (path == current_file_ && enabled_) return;
        
        if (!current_file_.isEmpty()) {
            watcher_.removePath(current_file_);
        }

        if (QFile::exists(path)) {
            watcher_.addPath(path);
            current_file_ = path;
            enabled_ = true;
            file_pos_ = QFileInfo(path).size();
            LOG_INFO("Watching log file: " + path.toStdString());
        } else {
            LOG_ERROR("Log file not found: " + path.toStdString());
            enabled_ = false;
        }
    }

    void stop_watching() {
        if (!current_file_.isEmpty()) {
            watcher_.removePath(current_file_);
        }
        current_file_.clear();
        enabled_ = false;
    }

    void on_file_changed(const QString& path) {
        timer_.start(100); 
    }

    void on_file_changed_delayed() {
        if (current_file_.isEmpty()) return;

        QFile file(current_file_);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        if (file.size() < file_pos_) {
            file_pos_ = 0; 
        }

        if (!file.seek(file_pos_)) return;

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            process_line(line);
        }

        file_pos_ = file.pos();
    }

    void process_line(const QString& line) {
        if (line.contains("Stopping!") || line.contains("Saving chunks for level 'ServerLevel'")) {
             if (reset_callback_) reset_callback_();
        } else if (line.contains("LoggedInWith") || line.contains("Joined Game")) {
             if (reset_callback_) reset_callback_();
        } else if (line.contains("Scanning for legacy world structure data")) {
             if (reset_callback_) reset_callback_();
        }
    }

    QFileSystemWatcher watcher_;
    QString current_file_;
    qint64 file_pos_ = 0;
    bool enabled_ = false;
    bool config_connected_ = false;
    std::function<void()> reset_callback_;
    QTimer timer_;
    core::ConnectionHandle config_handle_;
};

} // namespace dolbot::io
