#pragma once

#include <QObject>
#include <QApplication>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include "dolbot/core/config.hpp"

namespace dolbot::io {

class SoundManager : public QObject {
    Q_OBJECT

public:
    static SoundManager& instance() {
        static SoundManager inst;
        return inst;
    }
    
    void play_success() {
        if (!core::Config::instance().settings().enable_sounds) return;
#ifdef Q_OS_WIN
        PlaySoundA("SystemAsterisk", nullptr, SND_ALIAS | SND_ASYNC);
#else
        QApplication::beep();
#endif
    }
    
    void play_error() {
        if (!core::Config::instance().settings().enable_sounds) return;
#ifdef Q_OS_WIN
        PlaySoundA("SystemHand", nullptr, SND_ALIAS | SND_ASYNC);
#else
        QApplication::beep();
#endif
    }
    
    void play_lock() {
        if (!core::Config::instance().settings().enable_sounds) return;
#ifdef Q_OS_WIN
        PlaySoundA("SystemExclamation", nullptr, SND_ALIAS | SND_ASYNC);
#else
        QApplication::beep();
#endif
    }
    
    void play_high_certainty() {
        if (!core::Config::instance().settings().enable_sounds) return;
#ifdef Q_OS_WIN
        PlaySoundA("SystemNotification", nullptr, SND_ALIAS | SND_ASYNC);
#else
        QApplication::beep();
#endif
    }

private:
    SoundManager() = default;
    ~SoundManager() override = default;
};

} // namespace dolbot::io
