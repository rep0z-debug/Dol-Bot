#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QCheckBox>
#include <QMenuBar>
#include <QAction>
#include <QApplication>
#include <QMessageBox>
#include <QDesktopServices>
#include <memory>
#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

#include "dolbot/core/config.hpp"
#include "dolbot/core/signal.hpp"
#include "dolbot/domain/triangulator.hpp"
#include "dolbot/domain/blind_evaluator.hpp"
#include "dolbot/domain/boat_travel.hpp"
#include "dolbot/io/clipboard_watcher.hpp"
#include "dolbot/io/hotkey_service.hpp"
#include "dolbot/io/settings_store.hpp"
#include "dolbot/io/update_checker.hpp"
#include "dolbot/io/http_server.hpp"
#include "dolbot/ui/result_display.hpp"
#include "dolbot/ui/throw_list.hpp"
#include "dolbot/ui/blind_panel.hpp"
#include "dolbot/ui/settings_panel.hpp"
#include "dolbot/ui/calibration_dialog.hpp"
#include "dolbot/ui/divine_panel.hpp"
#include "dolbot/ui/overlay_window.hpp"
#include "dolbot/ui/status_bar.hpp"
#include "dolbot/ui/theme_engine.hpp"
#include "dolbot/ui/theme_editor.hpp"
#include "dolbot/io/log_watcher.hpp"
#include "dolbot/io/sound_manager.hpp"

namespace dolbot::ui {

class BoatStatusIndicator : public QFrame {
    Q_OBJECT
    
public:
    explicit BoatStatusIndicator(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setFixedHeight(24);
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(8, 4, 8, 4);
        layout->setSpacing(6);
        
        icon_label_ = new QLabel("⛵", this);
        layout->addWidget(icon_label_);
        
        status_label_ = new QLabel("OFF", this);
        layout->addWidget(status_label_);
        
        angle_label_ = new QLabel("", this);
        layout->addWidget(angle_label_);
        
        layout->addStretch();
        
        grid_label_ = new QLabel("", this);
        layout->addWidget(grid_label_);
        
        io::LogWatcher::instance().set_reset_callback([this]() {
            QMetaObject::invokeMethod(this, "on_reset", Qt::QueuedConnection);
        });
        io::LogWatcher::instance().start();
        
        theme_connection_ = ThemeEngine::instance().on_theme_changed(
            [this](const ThemeSettings&) { apply_current_styles(); }
        );
        
        update_state(domain::BoatState::None, std::nullopt);
    }
    
    void update_state(domain::BoatState state, std::optional<domain::BoatAngleInfo> info) {
        current_state_ = state;
        current_info_ = info;
        apply_current_styles();
    }
    
private:
    void apply_current_styles() {
        const auto& c = ThemeEngine::instance().colors();
        const auto& f = ThemeEngine::instance().fonts();
        int r = std::min(ThemeEngine::instance().sizes().border_radius, 4);
        
        QString bg, fg, status;
        
        switch (current_state_) {
            case domain::BoatState::None:
                bg = c.surface.name(); fg = c.text_muted.name(); status = "OFF";
                break;
            case domain::BoatState::Measuring:
                bg = c.primary.darker(150).name(); fg = c.accent.name(); status = "WAITING";
                break;
            case domain::BoatState::Valid:
                bg = c.success.darker(200).name(); fg = c.success.name(); status = "READY";
                break;
            case domain::BoatState::Error:
                bg = c.error.darker(200).name(); fg = c.error.lighter(120).name(); status = "ERROR";
                break;
        }
        
        setStyleSheet(QString("BoatStatusIndicator { background: %1; border-radius: %2px; }").arg(bg).arg(r));
        status_label_->setStyleSheet(QString("font-weight: 600; font-size: 11px; color: %1; font-family: '%2'; background: transparent;")
            .arg(fg).arg(f.primary_family));
        status_label_->setText(status);
        
        angle_label_->setStyleSheet(QString("font-family: '%1'; font-size: 11px; color: %2; background: transparent;")
            .arg(f.mono_family).arg(c.text_secondary.name()));
        grid_label_->setStyleSheet(QString("font-size: 10px; color: %1; font-family: '%2'; background: transparent;")
            .arg(c.text_muted.name()).arg(f.primary_family));
        icon_label_->setStyleSheet("background: transparent;");
        
        if (current_info_.has_value()) {
            angle_label_->setText(QString("%1°").arg(current_info_->snapped_angle, 0, 'f', 3));
            QString grid_text = current_info_->is_positive_grid ? "Grid: 1.40625°" : "Grid: 0.140625°";
            if (std::abs(current_info_->snap_error) > 0.001) {
                grid_text += QString(" (err: %1)").arg(current_info_->snap_error, 0, 'f', 4);
            }
            grid_label_->setText(grid_text);
        } else {
            angle_label_->setText("");
            grid_label_->setText("");
        }
    }
    
    QLabel* icon_label_;
    QLabel* status_label_;
    QLabel* angle_label_;
    QLabel* grid_label_;
    domain::BoatState current_state_ = domain::BoatState::None;
    std::optional<domain::BoatAngleInfo> current_info_;
    core::ConnectionHandle theme_connection_;
};



class MainPanel : public QMainWindow {
    Q_OBJECT

public:
    explicit MainPanel(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
        setWindowTitle("Dol Bot");
        setMinimumSize(400, 500);
        resize(440, 650);
        
        setup_triangulator();
        setup_io();
        setup_ui();
        setup_connections();
        load_state();
        
        apply_privacy_mode(core::Config::instance().settings().streamer_hide_mode);
        result_display_->set_fossil(core::Config::instance().settings().enable_divine);
        
        check_for_updates();
        start_http_server();
    }
    
    ~MainPanel() override {
        save_state();
    }

protected:
    void closeEvent(QCloseEvent* event) override {
        save_state();
        save_throws();
        if (http_server_) http_server_->stop();
        if (overlay_) overlay_->close();
        clipboard_watcher_->stop();
        QMainWindow::closeEvent(event);
    }

private:
    void setup_triangulator() {
        triangulator_ = std::make_unique<domain::Triangulator>();
        blind_evaluator_ = std::make_unique<domain::BlindEvaluator>();
        
        const auto& settings = core::Config::instance().settings();
        triangulator_->set_settings(
            settings.std_deviation,
            settings.std_dev_boat,
            settings.std_dev_manual,
            settings.use_advanced_stats,
            settings.mc_version,
            settings.mismeasure_threshold
        );
    }
    
    void setup_io() {
        clipboard_watcher_ = std::make_unique<io::ClipboardWatcher>(this);
        hotkey_service_ = std::make_unique<io::HotkeyService>(this);
        
        clipboard_watcher_->set_crosshair_correction(
            core::Config::instance().settings().crosshair_correction
        );
        clipboard_watcher_->start();
    }
    
    void setup_ui() {
        auto* central = new QWidget(this);
        auto* layout = new QVBoxLayout(central);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        
        
        auto* content = new QWidget(central);
        auto* content_layout = new QVBoxLayout(content);
        content_layout->setSpacing(10);
        content_layout->setContentsMargins(16, 16, 16, 12);
        
        auto* top_bar = new QHBoxLayout();
        
        boat_status_ = new BoatStatusIndicator(content);
        top_bar->addWidget(boat_status_, 1);
        

        
        content_layout->addLayout(top_bar);
        
        result_display_ = new ResultDisplay(content);
        content_layout->addWidget(result_display_);
        
        boat_mode_btn_ = new QPushButton("⛵ Boat Mode: OFF", content);
        boat_mode_btn_->setCheckable(true);
        boat_mode_btn_->setFixedHeight(36);
        boat_mode_btn_->setCursor(Qt::PointingHandCursor);
        boat_mode_btn_->setToolTip("Toggle High Precision Boat Mode (Ctrl+B)");
        boat_mode_btn_->setObjectName("boatModeBtn");
        apply_boat_button_style();
        
        content_layout->addWidget(boat_mode_btn_);
        
        blind_panel_ = new BlindPanel(content);
        blind_panel_->hide();
        content_layout->addWidget(blind_panel_);
        
        divine_panel_ = new DivinePanel(content);
        content_layout->addWidget(divine_panel_);

        throw_list_ = new ThrowList(content);
        content_layout->addWidget(throw_list_, 1);
        
        layout->addWidget(content, 1);
        
        status_bar_ = new StatusBar(central);
        status_bar_->set_version("v1.0.0");
        layout->addWidget(status_bar_);
        
        setCentralWidget(central);
        
        overlay_ = std::make_unique<OverlayWindow>();
        privacy_timer_ = new QTimer(this);
        connect(privacy_timer_, &QTimer::timeout, this, [this]() {
            auto mode = core::Config::instance().settings().streamer_hide_mode;
            apply_privacy_mode(mode);
        });
        privacy_timer_->start(500);

        theme_connection_ = ThemeEngine::instance().on_theme_changed(
            [this](const ThemeSettings& theme) { 
                apply_boat_button_style(); 
                apply_layout_order(theme.layout);
            }
        );
        
        apply_layout_order(ThemeEngine::instance().layout());

        setup_menu();
    }
    
    void apply_layout_order(const LayoutSettings& layout) {
        if (!result_display_ || !throw_list_ || !boat_status_ || !boat_mode_btn_) return;
        
        auto* parent_widget = qobject_cast<QWidget*>(result_display_->parent());
        if (!parent_widget) return;
        auto* content_layout = qobject_cast<QVBoxLayout*>(parent_widget->layout());
        if (!content_layout) return;
        
        content_layout->removeWidget(boat_status_);
        content_layout->removeWidget(result_display_);
        content_layout->removeWidget(boat_mode_btn_);
        content_layout->removeWidget(throw_list_);
        
        int insert_pos = 1;
        
        for (const auto& id : layout.main_panel_order) {
            if (id == "boat_status") {
                if (layout.show_boat_status) {
                    content_layout->insertWidget(insert_pos++, boat_status_, 0);
                    boat_status_->show();
                }
            }
            else if (id == "boat_mode_btn") {
                content_layout->insertWidget(insert_pos++, boat_mode_btn_, 0);
                boat_mode_btn_->show();
            }
            else if (id == "result_display") {
                content_layout->insertWidget(insert_pos++, result_display_, 0);
                result_display_->show();
            }
            else if (id == "throw_list") {
                content_layout->insertWidget(insert_pos++, throw_list_, 1);
                throw_list_->show();
            }
        }
        
        boat_status_->setVisible(layout.show_boat_status);
        status_bar_->setVisible(layout.show_status_bar);
    }
    
    void apply_boat_button_style() {
        const auto& c = ThemeEngine::instance().colors();
        const auto& f = ThemeEngine::instance().fonts();
        int r = std::min(ThemeEngine::instance().sizes().border_radius, 8);
        
        if (boat_mode_btn_) {
            boat_mode_btn_->setStyleSheet(QString(R"(
                QPushButton {
                    background-color: %1;
                    color: %2;
                    border: 1px solid %3;
                    border-radius: %4px;
                    font-weight: 700;
                    font-size: 14px;
                    font-family: '%5';
                }
                QPushButton:hover {
                    background-color: %6;
                }
                QPushButton:checked {
                    background-color: %7;
                    border-color: %8;
                    color: %9;
                }
            )").arg(c.surface.name())
               .arg(c.text_primary.name())
               .arg(c.border.name())
               .arg(r)
               .arg(f.primary_family)
               .arg(c.surface_hover.name())
               .arg(c.accent.name())
               .arg(c.primary.name())
               .arg(c.background.name()));
        }
    }
    
    void setup_menu() {
        auto* menu_bar = menuBar();
        
        auto* file_menu = menu_bar->addMenu("&File");
        
        auto* reset_action = file_menu->addAction("&Reset");
        reset_action->setShortcut(QKeySequence("Ctrl+R"));
        connect(reset_action, &QAction::triggered, this, &MainPanel::on_reset);
        
        auto* undo_action = file_menu->addAction("&Undo");
        undo_action->setShortcut(QKeySequence("Ctrl+Z"));
        connect(undo_action, &QAction::triggered, this, &MainPanel::on_undo);
        
        auto* redo_action = file_menu->addAction("&Redo");
        redo_action->setShortcut(QKeySequence("Ctrl+Y"));
        connect(redo_action, &QAction::triggered, this, &MainPanel::on_redo);
        
        file_menu->addSeparator();
        
        auto* lock_action = file_menu->addAction("&Lock/Unlock");
        lock_action->setShortcut(QKeySequence("Ctrl+L"));
        connect(lock_action, &QAction::triggered, this, &MainPanel::on_toggle_lock);
        
        file_menu->addSeparator();
        
        auto* quit_action = file_menu->addAction("&Quit");
        quit_action->setShortcut(QKeySequence::Quit);
        connect(quit_action, &QAction::triggered, qApp, &QApplication::quit);
        
        auto* view_menu = menu_bar->addMenu("&View");
        
        auto* overlay_action = view_menu->addAction("Show &Overlay");
        overlay_action->setCheckable(true);
        overlay_action->setChecked(core::Config::instance().settings().overlay_enabled);
        connect(overlay_action, &QAction::toggled, this, [this](bool checked) {
            core::Config::instance().modify([checked](core::AppSettings& s) {
                s.overlay_enabled = checked;
            });
        });
        
        auto* theme_editor_action = view_menu->addAction("&Theme Editor...");
        connect(theme_editor_action, &QAction::triggered, this, [this]() {
            auto* editor = new ThemeEditor(this);
            editor->exec();
            delete editor;
        });
        
        auto* settings_menu = menu_bar->addMenu("&Settings");
        
        auto* open_settings_action = settings_menu->addAction("&Preferences...");
        open_settings_action->setShortcut(QKeySequence("Ctrl+,"));
        connect(open_settings_action, &QAction::triggered, this, &MainPanel::on_open_settings);
        
        settings_menu->addSeparator();
        
        auto* calibrate_action = settings_menu->addAction("&Calibrate Std. Dev...");
        connect(calibrate_action, &QAction::triggered, this, &MainPanel::on_calibrate);
        
        auto* tools_menu = menu_bar->addMenu("&Tools");
        
        auto* boat_toggle_action = tools_menu->addAction("&Boat Mode");
        boat_toggle_action->setCheckable(true);
        boat_toggle_action->setShortcut(QKeySequence("Ctrl+B"));
        connect(boat_toggle_action, &QAction::toggled, boat_mode_btn_, &QPushButton::setChecked);
        connect(boat_mode_btn_, &QPushButton::toggled, boat_toggle_action, &QAction::setChecked);
        
        tools_menu->addSeparator();
        
        auto* privacy_toggle = tools_menu->addAction("Toggle &Privacy");
        privacy_toggle->setShortcut(QKeySequence("Ctrl+H"));
        connect(privacy_toggle, &QAction::triggered, this, &MainPanel::on_toggle_privacy);
        
        auto* help_menu = menu_bar->addMenu("&Help");
        
        auto* check_updates_action = help_menu->addAction("Check for &Updates...");
        connect(check_updates_action, &QAction::triggered, this, [this]() {
            status_bar_->set_status("Checking for updates...");
            check_for_updates(true);
        });
        
        help_menu->addSeparator();
        
        auto* about_action = help_menu->addAction("&About Dol Bot");
        connect(about_action, &QAction::triggered, this, [this]() {
            QMessageBox::about(this, "About Dol Bot",
                QString("<h3>Dol Bot v%1</h3>"
                        "<p>A fast Minecraft stronghold triangulation tool.</p>"
                        "<p>Based on the mathematics of <a href='https://github.com/Ninjabrain1/Ninjabrain-Bot'>Ninjabrain Bot</a>.</p>"
                        "<p>© 2025 Dol Bot</p>")
                    .arg(io::UpdateChecker::CURRENT_VERSION));
        });
    }
    
    void setup_connections() {
        clip_conn_ = clipboard_watcher_->on_throw_detected([this](domain::EyeThrow t) {
            const auto& settings = core::Config::instance().settings();
            
            if (boat_mode_btn_->isChecked()) {
                t.is_boat_mode = true;
                t.boat_error_limit = settings.boat_error_limit;
                double sens = (settings.mc_version == core::McVersion::V1_9_to_1_12) 
                    ? settings.boat_sensitivity_old 
                    : settings.boat_sensitivity_new;
                t.boat_sensitivity = sens;
                t.use_boat_sensitivity = settings.use_boat_sensitivity;
                
                if (settings.reduce_mod_360) {
                    double angle = t.horizontal_angle;
                    while (angle > 0) angle -= 360.0;
                    while (angle < -360.0) angle += 360.0;
                    t.horizontal_angle = angle;
                }
            }
            
            if (settings.angle_adjustment_type == core::AngleAdjustmentType::TallResolution) {
                t.set_subpixel_adjustment(settings.tall_resolution_height);
            } else if (settings.angle_adjustment_type == core::AngleAdjustmentType::Custom) {
                t.use_subpixel = true;
                t.subpixel_adjustment = settings.custom_angle_adjustment;
            }
            
            if (t.is_looking_down() && t.dimension == domain::Dimension::Nether) {
                auto result = blind_evaluator_->evaluate(t.position.x, t.position.z);
                blind_panel_->update_result(result);
                blind_panel_->show();
                status_bar_->set_status("Blind position evaluated");
                io::SoundManager::instance().play_success();
            } else if (t.is_looking_down()) {
                status_bar_->set_status("Blind travel only works in the nether");
            } else {
                if (t.is_boat_mode && stored_boat_angle_.has_value()) {
                    t.boat_sensitivity = settings.use_boat_sensitivity 
                        ? (settings.mc_version >= core::McVersion::V1_13_to_1_18 
                            ? settings.boat_sensitivity_new 
                            : settings.boat_sensitivity_old)
                        : 0.0;
                }
                
                if (t.is_boat_mode && entering_boat_) {
                    auto info = t.compute_boat_info();
                    if (info.within_tolerance(settings.boat_error_limit)) {
                        stored_boat_angle_ = info.snapped_angle;
                        entering_boat_ = false;
                        boat_status_->update_state(domain::BoatState::Valid, info);
                    } else {
                        boat_status_->update_state(domain::BoatState::Error, info);
                        status_bar_->set_status(QString("Invalid boat angle (error: %1°)").arg(info.snap_error, 0, 'f', 4));
                        io::SoundManager::instance().play_error();
                        return; 
                    }
                }
                
                if (triangulator_->add_throw(t)) {
                    blind_panel_->hide();
                    status_bar_->set_status("Eye throw added");
                    io::SoundManager::instance().play_success();
                    
                    if (t.is_boat_mode) {
                        auto info = t.compute_boat_info();
                        boat_status_->update_state(t.get_boat_state(), info);
                    }
                } else {
                    status_bar_->set_status("Duplicate throw ignored");
                }
            }
        });
        
        fossil_conn_ = clipboard_watcher_->on_fossil_detected([this](const domain::FossilLocation& fossil) {
            if (!core::Config::instance().settings().enable_divine) {
                status_bar_->set_status("Fossil divine is disabled in settings");
                return;
            }

            divine_panel_->set_fossil(fossil.segment, fossil.x, fossil.z);
            status_bar_->set_status(QString("Fossil data detected (segment %1)").arg(fossil.segment + 1));

            domain::Fossil f_ctx;
            f_ctx.x = fossil.segment;
            triangulator_->set_fossil(f_ctx);

            io::SoundManager::instance().play_success();
        });


        result_conn_ = triangulator_->on_result_changed([this](const domain::TriangulationResult& r) {
            result_display_->update_result(r);
            throw_list_->update_throws(triangulator_->throws());
            status_bar_->set_throws(r.throw_count);
            
            const auto& settings = core::Config::instance().settings();
            if (settings.mismeasure_warning_enabled && r.has_result()) {
                const auto* best = r.best();
                if (best && best->has_warning()) {
                    status_bar_->set_status(QString::fromStdString(best->warning_message));
                    io::SoundManager::instance().play_error();
                } else if (best && best->certainty >= 0.90 && r.throw_count == 2) {
                    io::SoundManager::instance().play_high_certainty();
                }
            }
            
            overlay_->set_enabled(settings.overlay_enabled);
            if (settings.overlay_enabled) {
                overlay_->update_result(r);
            }
        });
        
        connect(throw_list_, &ThrowList::undo_requested, this, &MainPanel::on_undo);
        connect(throw_list_, &ThrowList::redo_requested, this, &MainPanel::on_redo);
        connect(throw_list_, &ThrowList::reset_requested, this, &MainPanel::on_reset);
        connect(throw_list_, &ThrowList::remove_throw_requested, this, [this](int index) {
            if (triangulator_->remove_throw(index)) {
                status_bar_->set_status("Throw removed");
            }
        });
        
        config_conn_ = core::Config::instance().on_change([this](const core::AppSettings& s) {
            triangulator_->set_settings(s.std_deviation, s.std_dev_boat, s.std_dev_manual, s.use_advanced_stats, s.mc_version, s.mismeasure_threshold);
            clipboard_watcher_->set_crosshair_correction(s.crosshair_correction);
            result_display_->update_result(triangulator_->result());
            
            overlay_->set_enabled(s.overlay_enabled);
            if (s.overlay_enabled) {
                overlay_->update_result(triangulator_->result());
            }
            
            if (s.always_on_top) {
                setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
            } else {
                setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
            }
            apply_privacy_mode(s.streamer_hide_mode);
            result_display_->set_fossil(s.enable_divine);
            show();
        });
        
        hotkey_conn_ = hotkey_service_->on_hotkey([this](io::HotkeyAction action) {
            switch (action) {
                case io::HotkeyAction::Reset: on_reset(); break;
                case io::HotkeyAction::Undo: on_undo(); break;
                case io::HotkeyAction::Redo: on_redo(); break;
                case io::HotkeyAction::ToggleLock: on_toggle_lock(); break;
                case io::HotkeyAction::ToggleBoat: boat_mode_btn_->toggle(); break;
                case io::HotkeyAction::EnterBoat: on_enter_boat_hotkey(); break;
                case io::HotkeyAction::Mod360: on_mod360_hotkey(); break;
                case io::HotkeyAction::TogglePrivacy: on_toggle_privacy(); break;
                case io::HotkeyAction::AngleIncrement: triangulator_->adjust_last_angle(0.01); break;
                case io::HotkeyAction::AngleDecrement: triangulator_->adjust_last_angle(-0.01); break;
                case io::HotkeyAction::OpenSettings: on_open_settings(); break;
            }
        });

        connect(boat_mode_btn_, &QPushButton::toggled, this, [this](bool checked) {
            boat_mode_btn_->setText(checked ? "⛵ Boat Mode: ON" : "⛵ Boat Mode: OFF");
            status_bar_->set_status(checked ? "Boat Mode Enabled - press F3+C to set angle" : "Boat Mode Disabled");
            overlay_->set_boat_mode(checked);
            
            if (checked) {
                entering_boat_ = true;
                stored_boat_angle_ = std::nullopt;
                boat_status_->update_state(domain::BoatState::Measuring, std::nullopt);
            } else {
                entering_boat_ = false;
                stored_boat_angle_ = std::nullopt;
                boat_status_->update_state(domain::BoatState::None, std::nullopt);
            }
        });
    }
    
    void load_state() {
        auto& store = io::SettingsStore::instance();
        restoreGeometry(store.load_window_geometry());
        
        if (core::Config::instance().settings().always_on_top) {
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        }
        
        load_throws();
    }
    
    void save_state() {
        auto& store = io::SettingsStore::instance();
        store.save_window_geometry(saveGeometry());
        store.sync();
    }
    
    void save_throws() {
        io::SettingsStore::instance().save_throws(triangulator_->throws());
        io::SettingsStore::instance().save_redo_stack(triangulator_->redo_stack());
    }
    
    void load_throws() {
        auto throws = io::SettingsStore::instance().load_throws();
        for (const auto& t : throws) {
            triangulator_->add_throw(t);
        }
        auto redo = io::SettingsStore::instance().load_redo_stack();
        triangulator_->set_redo_stack(std::move(redo));
    }
    
    void check_for_updates(bool show_no_update_message = false) {
        update_checker_ = std::make_unique<io::UpdateChecker>(this);
        
        update_conn_available_ = update_checker_->on_update_available([this](const io::VersionInfo& info) {
            QMetaObject::invokeMethod(this, [this, info]() {
                status_bar_->set_status(QString("Update available: v%1").arg(info.version));
                auto result = QMessageBox::information(
                    this,
                    "Update Available",
                    QString("Version %1 is available.\n\nWould you like to download it?").arg(info.version),
                    QMessageBox::Yes | QMessageBox::No
                );
                if (result == QMessageBox::Yes) {
                    QDesktopServices::openUrl(QUrl(info.download_url));
                }
            }, Qt::QueuedConnection);
        });
        
        update_conn_complete_ = update_checker_->on_check_complete([this, show_no_update_message](bool success, const QString& error) {
            QMetaObject::invokeMethod(this, [this, success, error, show_no_update_message]() {
                if (success) {
                    status_bar_->set_status("You're up to date!");
                    if (show_no_update_message) {
                        QMessageBox::information(this, "No Updates", 
                            QString("You are running the latest version (%1).").arg(io::UpdateChecker::CURRENT_VERSION));
                    }
                } else {
                    status_bar_->set_status(QString("Update check failed: %1").arg(error));
                    if (show_no_update_message) {
                        QMessageBox::warning(this, "Update Check Failed", 
                            QString("Could not check for updates: %1").arg(error));
                    }
                }
            }, Qt::QueuedConnection);
        });
        
        update_checker_->check_for_updates();
    }
    
    void start_http_server() {
        http_server_ = std::make_unique<io::HttpServer>(this);
        http_server_->set_triangulator(triangulator_.get());
        http_server_->set_reset_callback([this]() {
            QMetaObject::invokeMethod(this, &MainPanel::on_reset, Qt::QueuedConnection);
        });
        http_server_->set_undo_callback([this]() {
            QMetaObject::invokeMethod(this, &MainPanel::on_undo, Qt::QueuedConnection);
        });
        if (http_server_->start()) {
            status_bar_->set_status(QString("API running on port %1").arg(http_server_->port()));
        }
    }

private slots:
    void on_reset() {
        triangulator_->reset();
        blind_panel_->hide();
        divine_panel_->clear();
        io::SettingsStore::instance().clear_throws();
        io::SettingsStore::instance().save_redo_stack(triangulator_->redo_stack());
        
        entering_boat_ = boat_mode_btn_->isChecked();
        stored_boat_angle_ = std::nullopt;
        
        boat_status_->update_state(
            boat_mode_btn_->isChecked() ? domain::BoatState::Measuring : domain::BoatState::None,
            std::nullopt
        );
        status_bar_->set_status("Reset");
    }
    
    void on_undo() {
        if (triangulator_->undo_last()) {
            status_bar_->set_status("Undid last throw");
        }
    }
    
    void on_redo() {
        if (triangulator_->redo_last()) {
            status_bar_->set_status("Redid last throw");
        }
    }
    
    void on_toggle_lock() {
        if (triangulator_->is_locked()) {
            triangulator_->unlock();
            status_bar_->set_status("Unlocked");
        } else {
            triangulator_->lock();
            status_bar_->set_status("Locked");
            io::SoundManager::instance().play_lock();
        }
    }
    
    void on_open_settings() {
        auto* dialog = new SettingsPanel(this);
        apply_privacy_mode(core::Config::instance().settings().streamer_hide_mode);
        dialog->exec();
        delete dialog;
    }
    
    void on_calibrate() {
        auto* dialog = new CalibrationDialog(this);
        apply_privacy_mode(core::Config::instance().settings().streamer_hide_mode);
        if (dialog->exec() == QDialog::Accepted && dialog->calculated_std_dev() > 0) {
            core::Config::instance().modify([dialog](core::AppSettings& s) {
                s.std_deviation = dialog->calculated_std_dev();
            });
            auto settings = core::Config::instance().settings();
            status_bar_->set_status(QString("Std. dev. set to %1").arg(settings.std_deviation, 0, 'f', 4));
        }
        delete dialog;
    }
    
    void on_toggle_privacy() {
        auto& config = core::Config::instance();
        auto settings = config.settings();
        
        config.modify([](core::AppSettings& s) {
            if (s.streamer_hide_mode == core::StreamerHideMode::Off) {
                s.streamer_hide_mode = core::StreamerHideMode::All;
            } else {
                s.streamer_hide_mode = core::StreamerHideMode::Off;
            }
        });
        
        settings = config.settings();
        status_bar_->set_status(
            settings.streamer_hide_mode == core::StreamerHideMode::Off 
                ? "Privacy: Off" 
                : "Privacy: Hidden"
        );
    }
    
    void on_enter_boat_hotkey() {
        if (!boat_mode_btn_->isChecked()) {
            boat_mode_btn_->setChecked(true);
            entering_boat_ = true;
            stored_boat_angle_ = std::nullopt;
            boat_status_->update_state(domain::BoatState::Measuring, std::nullopt);
            status_bar_->set_status("Entering boat - press F3+C to set angle");
        } else {
            boat_mode_btn_->setChecked(false);
            entering_boat_ = false;
            stored_boat_angle_ = std::nullopt;
            boat_status_->update_state(domain::BoatState::None, std::nullopt);
            status_bar_->set_status("Exited boat mode");
        }
    }
    
    void on_mod360_hotkey() {
        if (!boat_mode_btn_->isChecked()) return;
        if (!stored_boat_angle_.has_value()) {
            status_bar_->set_status("No boat angle set yet");
            return;
        }
        
        const auto& settings = core::Config::instance().settings();
        double sensitivity = settings.mc_version >= core::McVersion::V1_13_to_1_18 
            ? settings.boat_sensitivity_new 
            : settings.boat_sensitivity_old;
        
        double reduced = domain::BoatTravel::reduce_angle_mod360(*stored_boat_angle_, *stored_boat_angle_, sensitivity);
        stored_boat_angle_ = reduced;
        
        domain::BoatAngleInfo info;
        info.snapped_angle = reduced;
        info.snap_error = 0.0;
        boat_status_->update_state(domain::BoatState::Valid, info);
        status_bar_->set_status(QString("Boat angle reduced: %1°").arg(reduced, 0, 'f', 3));
    }

    void apply_privacy_mode(core::StreamerHideMode mode) {
#ifdef Q_OS_WIN
        DWORD affinity = WDA_NONE;
        if (mode != core::StreamerHideMode::Off) {
            affinity = WDA_EXCLUDEFROMCAPTURE;
        }
        
        QTimer::singleShot(100, this, [this, affinity, mode]() {
            HWND hwnd = (HWND)this->winId();
            if (hwnd) {
                BOOL result = SetWindowDisplayAffinity(hwnd, affinity);
                if (!result) {
                    DWORD error = GetLastError();
                    status_bar_->set_status(QString("Privacy mode failed (error %1)").arg(error));
                } else {
                    if (affinity != WDA_NONE) {
                        status_bar_->set_status("Privacy: Window hidden from capture");
                    }
                }
            }
            
            for (QWidget* widget : QApplication::topLevelWidgets()) {
                if (widget != this && widget->isWindow()) {
                    HWND wh = (HWND)widget->winId();
                    if (wh) {
                        SetWindowDisplayAffinity(wh, affinity);
                    }
                }
            }
            
            bool show_indicator = core::Config::instance().settings().show_privacy_indicator;
            result_display_->set_hidden(affinity != WDA_NONE && show_indicator);
        });
#else
        Q_UNUSED(mode);
        result_display_->set_hidden(false);
#endif
    }

private:
    std::unique_ptr<domain::Triangulator> triangulator_;
    std::unique_ptr<domain::BlindEvaluator> blind_evaluator_;
    std::unique_ptr<io::ClipboardWatcher> clipboard_watcher_;
    std::unique_ptr<io::HotkeyService> hotkey_service_;
    std::unique_ptr<io::UpdateChecker> update_checker_;
    std::unique_ptr<io::HttpServer> http_server_;
    std::unique_ptr<OverlayWindow> overlay_;
    
    QTimer* privacy_timer_;
    BoatStatusIndicator* boat_status_;
    ResultDisplay* result_display_;
    QPushButton* boat_mode_btn_;
    ThrowList* throw_list_;
    BlindPanel* blind_panel_;
    DivinePanel* divine_panel_;
    StatusBar* status_bar_;
    
    core::ConnectionHandle clip_conn_;
    core::ConnectionHandle fossil_conn_;
    core::ConnectionHandle result_conn_;
    core::ConnectionHandle config_conn_;
    core::ConnectionHandle hotkey_conn_;
    core::ConnectionHandle update_conn_available_;
    core::ConnectionHandle update_conn_complete_;
    
    bool entering_boat_ = false;
    std::optional<double> stored_boat_angle_;
    core::ConnectionHandle theme_connection_;
};

} // namespace dolbot::ui
