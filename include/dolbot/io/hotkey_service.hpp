#pragma once

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QCoreApplication>
#include <QMap>
#include <QKeySequence>
#include <functional>
#include <string>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "dolbot/core/signal.hpp"
#include "dolbot/core/config.hpp"

namespace dolbot::io {

enum class HotkeyAction {
    Reset,
    Undo,
    Redo,
    ToggleLock,
    ToggleBoat,
    EnterBoat,
    Mod360,
    TogglePrivacy,
    AngleIncrement,
    AngleDecrement,
    OpenSettings
};

struct HotkeyBinding {
    int key = 0;
    int modifiers = 0;
    HotkeyAction action;
    bool enabled = true;
    std::string display_name;
};

class HotkeyService : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    explicit HotkeyService(QObject* parent = nullptr)
        : QObject(parent)
    {
        QCoreApplication::instance()->installNativeEventFilter(this);
        load_from_config();
        
        config_conn_ = core::Config::instance().on_change([this](const auto&) {
            load_from_config();
        });
    }
    
    ~HotkeyService() override {
        QCoreApplication::instance()->removeNativeEventFilter(this);
        unregister_all();
    }
    
    void load_from_config() {
        unregister_all();
        const auto& settings = core::Config::instance().settings();
        
        if (!settings.hotkeys_enabled) return;
        
        const auto& hk = settings.hotkeys;
        
        parse_and_register(HotkeyAction::Reset, hk.reset, "Reset");
        parse_and_register(HotkeyAction::Undo, hk.undo, "Undo");
        parse_and_register(HotkeyAction::Redo, hk.redo, "Redo");
        parse_and_register(HotkeyAction::ToggleLock, hk.toggle_lock, "Toggle Lock");
        parse_and_register(HotkeyAction::ToggleBoat, hk.toggle_boat, "Toggle Boat Mode");
        parse_and_register(HotkeyAction::EnterBoat, hk.enter_boat, "Enter Boat");
        parse_and_register(HotkeyAction::Mod360, hk.mod_360, "Reduce Mod 360");
        parse_and_register(HotkeyAction::TogglePrivacy, hk.toggle_privacy, "Toggle Privacy");
        parse_and_register(HotkeyAction::AngleIncrement, hk.angle_up, "Angle +");
        parse_and_register(HotkeyAction::AngleDecrement, hk.angle_down, "Angle -");
        parse_and_register(HotkeyAction::OpenSettings, hk.open_settings, "Open Settings");
    }
    
    bool register_hotkey(HotkeyAction action, int key, int modifiers = 0) {
#ifdef Q_OS_WIN
        int id = static_cast<int>(action) + 1000;
        
        UINT win_modifiers = 0;
        if (modifiers & Qt::ControlModifier) win_modifiers |= MOD_CONTROL;
        if (modifiers & Qt::AltModifier) win_modifiers |= MOD_ALT;
        if (modifiers & Qt::ShiftModifier) win_modifiers |= MOD_SHIFT;
        
        if (RegisterHotKey(nullptr, id, win_modifiers | MOD_NOREPEAT, key)) {
            bindings_[action].key = key;
            bindings_[action].modifiers = modifiers;
            bindings_[action].enabled = true;
            return true;
        }
#else
        Q_UNUSED(action);
        Q_UNUSED(key);
        Q_UNUSED(modifiers);
#endif
        return false;
    }
    
    void unregister_hotkey(HotkeyAction action) {
#ifdef Q_OS_WIN
        UnregisterHotKey(nullptr, static_cast<int>(action) + 1000);
#endif
        bindings_.remove(action);
    }
    
    void unregister_all() {
#ifdef Q_OS_WIN
        for (auto it = bindings_.begin(); it != bindings_.end(); ++it) {
            UnregisterHotKey(nullptr, static_cast<int>(it.key()) + 1000);
        }
#endif
        bindings_.clear();
    }
    
    core::ConnectionHandle on_hotkey(std::function<void(HotkeyAction)> callback) {
        return hotkey_pressed_.connect(std::move(callback));
    }
    
    void trigger(HotkeyAction action) {
        if (bindings_.contains(action) && bindings_[action].enabled) {
            hotkey_pressed_.fire(action);
        }
    }
    
    void set_enabled(HotkeyAction action, bool enabled) {
        if (bindings_.contains(action)) {
            bindings_[action].enabled = enabled;
        }
    }
    
    const QMap<HotkeyAction, HotkeyBinding>& bindings() const {
        return bindings_;
    }
    
    static QString action_name(HotkeyAction action) {
        switch (action) {
            case HotkeyAction::Reset: return "Reset";
            case HotkeyAction::Undo: return "Undo";
            case HotkeyAction::Redo: return "Redo";
            case HotkeyAction::ToggleLock: return "Toggle Lock";
            case HotkeyAction::ToggleBoat: return "Toggle Boat Mode";
            case HotkeyAction::EnterBoat: return "Enter Boat";
            case HotkeyAction::Mod360: return "Reduce Mod 360";
            case HotkeyAction::TogglePrivacy: return "Toggle Privacy";
            case HotkeyAction::AngleIncrement: return "Angle +";
            case HotkeyAction::AngleDecrement: return "Angle -";
            case HotkeyAction::OpenSettings: return "Open Settings";
        }
        return "Unknown";
    }
    
#ifdef Q_OS_WIN
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override {
        Q_UNUSED(result);
        if (eventType == "windows_generic_MSG") {
            MSG* msg = static_cast<MSG*>(message);
            if (msg->message == WM_HOTKEY) {
                int id = static_cast<int>(msg->wParam) - 1000;
                auto action = static_cast<HotkeyAction>(id);
                trigger(action);
                return true;
            }
        }
        return false;
    }
#endif

private:
    void parse_and_register(HotkeyAction action, const std::string& shortcut, const std::string& name) {
        bindings_[action] = {0, 0, action, true, name};
        
        QString qs = QString::fromStdString(shortcut);
        QKeySequence seq(qs);
        if (seq.isEmpty()) return;
        
        int key = seq[0].key();
        Qt::KeyboardModifiers mods = seq[0].keyboardModifiers();
        
        int vk = qt_key_to_vk(key);
        if (vk != 0) {
            register_hotkey(action, vk, static_cast<int>(mods));
        }
    }
    
    int qt_key_to_vk(int qtKey) {
#ifdef Q_OS_WIN
        if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) {
            return 'A' + (qtKey - Qt::Key_A);
        }
        if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9) {
            return '0' + (qtKey - Qt::Key_0);
        }
        switch (qtKey) {
            case Qt::Key_BracketLeft: return VK_OEM_4;
            case Qt::Key_BracketRight: return VK_OEM_6;
            case Qt::Key_Plus: return VK_OEM_PLUS;
            case Qt::Key_Minus: return VK_OEM_MINUS;
            case Qt::Key_Equal: return VK_OEM_PLUS;
            case Qt::Key_Escape: return VK_ESCAPE;
            case Qt::Key_Space: return VK_SPACE;
            case Qt::Key_Return: return VK_RETURN;
            case Qt::Key_Enter: return VK_RETURN;
            case Qt::Key_Tab: return VK_TAB;
            case Qt::Key_Backspace: return VK_BACK;
            case Qt::Key_Delete: return VK_DELETE;
            case Qt::Key_Insert: return VK_INSERT;
            case Qt::Key_Home: return VK_HOME;
            case Qt::Key_End: return VK_END;
            case Qt::Key_PageUp: return VK_PRIOR;
            case Qt::Key_PageDown: return VK_NEXT;
            case Qt::Key_Left: return VK_LEFT;
            case Qt::Key_Right: return VK_RIGHT;
            case Qt::Key_Up: return VK_UP;
            case Qt::Key_Down: return VK_DOWN;
            case Qt::Key_Comma: return VK_OEM_COMMA;
            case Qt::Key_Period: return VK_OEM_PERIOD;
            case Qt::Key_Slash: return VK_OEM_2;
            case Qt::Key_Backslash: return VK_OEM_5;
            case Qt::Key_Semicolon: return VK_OEM_1;
            case Qt::Key_Apostrophe: return VK_OEM_7;
            case Qt::Key_QuoteLeft: return VK_OEM_3;
            case Qt::Key_CapsLock: return VK_CAPITAL;
            case Qt::Key_NumLock: return VK_NUMLOCK;
            case Qt::Key_ScrollLock: return VK_SCROLL;
            case Qt::Key_Pause: return VK_PAUSE;
            case Qt::Key_Print: return VK_SNAPSHOT;
            case Qt::Key_F1: return VK_F1;
            case Qt::Key_F2: return VK_F2;
            case Qt::Key_F3: return VK_F3;
            case Qt::Key_F4: return VK_F4;
            case Qt::Key_F5: return VK_F5;
            case Qt::Key_F6: return VK_F6;
            case Qt::Key_F7: return VK_F7;
            case Qt::Key_F8: return VK_F8;
            case Qt::Key_F9: return VK_F9;
            case Qt::Key_F10: return VK_F10;
            case Qt::Key_F11: return VK_F11;
            case Qt::Key_F12: return VK_F12;
            default: return 0;
        }
#else
        Q_UNUSED(qtKey);
        return 0;
#endif
    }
    
    core::ConnectionHandle config_conn_;
    QMap<HotkeyAction, HotkeyBinding> bindings_;
    core::Signal<HotkeyAction> hotkey_pressed_;
};

} // namespace dolbot::io
