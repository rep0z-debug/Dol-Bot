#pragma once

#include <QObject>
#include <QClipboard>
#include <QApplication>
#include <QTimer>
#include <QElapsedTimer>
#include <string>
#include <functional>
#include <mutex>
#include "dolbot/core/signal.hpp"
#include "dolbot/io/f3c_parser.hpp"
#include "dolbot/domain/fossil_divine.hpp"

namespace dolbot::io {

class ClipboardWatcher : public QObject {
    Q_OBJECT

public:
    explicit ClipboardWatcher(QObject* parent = nullptr)
        : QObject(parent)
        , clipboard_(QApplication::clipboard())
    {
        connect(clipboard_, &QClipboard::dataChanged, this, &ClipboardWatcher::on_clipboard_changed);
        
        poll_timer_ = new QTimer(this);
        connect(poll_timer_, &QTimer::timeout, this, &ClipboardWatcher::check_clipboard);
    }
    
    void start(int poll_interval_ms = 100) {
        running_ = true;
        poll_timer_->start(poll_interval_ms);
    }
    
    void stop() {
        running_ = false;
        poll_timer_->stop();
    }
    
    [[nodiscard]] bool is_running() const { return running_; }
    
    void set_crosshair_correction(double correction) {
        crosshair_correction_ = correction;
    }
    
    core::ConnectionHandle on_throw_detected(std::function<void(const domain::EyeThrow&)> callback) {
        return throw_detected_.connect(std::move(callback));
    }
    
    core::ConnectionHandle on_raw_data(std::function<void(const ParsedF3C&)> callback) {
        return raw_data_.connect(std::move(callback));
    }

    core::ConnectionHandle on_fossil_detected(std::function<void(const domain::FossilLocation&)> callback) {
        return fossil_detected_.connect(std::move(callback));
    }

private slots:
    void on_clipboard_changed() {
        if (!running_) return;
        
        QString current = clipboard_->text();
        if (current.isEmpty() || current == last_content_) return;
        
        if (current.toStdString() == last_processed_) return;
        
        last_content_ = current;
        process_clipboard(current);
    }
    
    void check_clipboard() {
        if (!running_) return;
        
        QString current = clipboard_->text();
        if (current.isEmpty() || current == last_content_) return;
        
        if (current.toStdString() == last_processed_) return;
        
        last_content_ = current;
        process_clipboard(current);
    }

private:
    void process_clipboard(const QString& text) {
        std::string content = text.toStdString();
        
        auto parsed = F3CParser::parse(content);
        if (parsed) {
            {
                std::lock_guard lock(mutex_);
                if (content == last_processed_ && last_process_time_.elapsed() < 1000) return;
                last_processed_ = content;
                last_process_time_.start();
            }

            raw_data_.fire(*parsed);

            auto throw_data = F3CParser::to_throw(*parsed, crosshair_correction_);
            if (throw_data) {
                throw_detected_.fire(*throw_data);
            }
            return;
        }

        // Try parsing fossil
        auto fossil = domain::FossilDivine::parse_f3i_bone(content);
        if (fossil) {
            {
                std::lock_guard lock(mutex_);
                if (content == last_processed_ && last_process_time_.elapsed() < 1000) return;
                last_processed_ = content;
                last_process_time_.start();
            }
            fossil_detected_.fire(*fossil);
        }
    }
    
    QClipboard* clipboard_;
    QTimer* poll_timer_;
    bool running_ = false;
    QString last_content_;
    std::string last_processed_;
    QElapsedTimer last_process_time_;
    double crosshair_correction_ = 0.0;
    std::mutex mutex_;
    
    core::Signal<const domain::EyeThrow&> throw_detected_;
    core::Signal<const ParsedF3C&> raw_data_;
    core::Signal<const domain::FossilLocation&> fossil_detected_;
};

} // namespace dolbot::io
