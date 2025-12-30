#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QKeySequenceEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QRegularExpression>
#include <QCloseEvent>
#include "dolbot/core/config.hpp"
#include "dolbot/ui/calibration_dialog.hpp"
#include "dolbot/ui/theme_engine.hpp"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace dolbot::ui {

class HotkeyEdit : public QWidget {
    Q_OBJECT

public:
    explicit HotkeyEdit(const QString& label, const QString& default_key, QWidget* parent = nullptr)
        : QWidget(parent)
        , default_key_(default_key)
    {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 4, 0, 4);
        layout->setSpacing(8);
        
        auto* lbl = new QLabel(label, this);
        lbl->setMinimumWidth(130);
        layout->addWidget(lbl);
        
        edit_ = new QKeySequenceEdit(QKeySequence(default_key), this);
        edit_->setToolTip("Click and press the new key combination");
        layout->addWidget(edit_, 1);
        
        auto* reset = new QPushButton("Reset", this);
        reset->setProperty("flat", true);
        reset->setFixedWidth(80);
        connect(reset, &QPushButton::clicked, [this]() {
            edit_->setKeySequence(QKeySequence(default_key_));
        });
        layout->addWidget(reset);

        auto* clear_btn = new QPushButton("✕", this);
        clear_btn->setFixedWidth(24);
        clear_btn->setToolTip("Clear hotkey");
        clear_btn->setProperty("flat", true);
        connect(clear_btn, &QPushButton::clicked, [this]() {
            edit_->setKeySequence(QKeySequence());
        });
        layout->addWidget(clear_btn);
    }
    
    QString key_string() const {
        return edit_->keySequence().toString();
    }
    
    void set_key(const QString& k) {
        edit_->setKeySequence(QKeySequence(k));
    }
    
private:
    QKeySequenceEdit* edit_;
    QString default_key_;
};

class SettingsPanel : public QDialog {
    Q_OBJECT

public:
    explicit SettingsPanel(QWidget* parent = nullptr)
        : QDialog(parent)
        , dirty_(false)
    {
        setWindowTitle("Dol Bot Settings");
        setMinimumSize(520, 600);
        setup_ui();
        load_settings();
        connect_dirty_tracking();
        
        theme_connection_ = ThemeEngine::instance().on_theme_changed(
            [this](const ThemeSettings&) {
                style()->unpolish(this);
                style()->polish(this);
                update();
            }
        );
    }
    
protected:
    void closeEvent(QCloseEvent* event) override {
        if (dirty_) {
            auto result = QMessageBox::warning(this, "Unsaved Changes",
                "You have unsaved settings changes.\n\n"
                "Do you want to apply them before closing?",
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            
            if (result == QMessageBox::Save) {
                apply_settings();
                event->accept();
            } else if (result == QMessageBox::Discard) {
                event->accept();
            } else {
                event->ignore();
            }
        } else {
            event->accept();
        }
    }

signals:
    void settings_changed();

private slots:
    void reset_to_defaults() {
        auto result = QMessageBox::warning(this, "Reset Settings", 
            "Are you sure you want to reset all settings to their default values?\nThis cannot be undone.",
            QMessageBox::Yes | QMessageBox::No);
            
        if (result == QMessageBox::Yes) {
            core::Config::instance().modify([](core::AppSettings& s) {
                s = core::AppSettings();
            });
            load_settings();
            QMessageBox::information(this, "Reset Settings", "Settings have been reset.");
        }
    }

    void apply_settings() {
        core::Config::instance().modify([this](core::AppSettings& settings) {
            settings.std_deviation = std_dev_spin_->value();
            settings.std_dev_boat = boat_std_spin_->value();
            settings.std_dev_manual = manual_std_spin_->value();
            settings.crosshair_correction = crosshair_spin_->value();
            settings.use_advanced_stats = true; 
            settings.show_angle_errors = errors_check_->isChecked();
            settings.show_direction = direction_check_->isChecked();
            settings.mc_version = static_cast<core::McVersion>(version_combo_->currentIndex());
            settings.display_mode = static_cast<core::DisplayMode>(display_combo_->currentIndex());
            settings.always_on_top = top_check_->isChecked();
            settings.overlay_enabled = overlay_check_->isChecked();
            settings.show_nether_coords = nether_check_->isChecked();
            settings.show_angle_updates = updates_check_->isChecked();
            
            settings.boat_error_limit = boat_error_spin_->value();
            settings.boat_sensitivity_old = boat_sens_old_spin_->value();
            settings.boat_sensitivity_new = boat_sens_new_spin_->value();
            settings.use_boat_sensitivity = (boat_type_combo_->currentIndex() == 2);
            settings.reduce_mod_360 = mod360_check_->isChecked();
            settings.default_boat_type = boat_type_combo_->currentIndex();
            settings.show_boat_angle_reset_indicator = show_angle_reset_check_->isChecked();
            settings.show_mod360_indicator = show_mod360_indicator_check_->isChecked();
            settings.angle_adjustment_type = static_cast<core::AngleAdjustmentType>(angle_adj_combo_->currentIndex());
            settings.tall_resolution_height = tall_res_spin_->value();
            settings.custom_angle_adjustment = custom_adj_spin_->value();
            settings.streamer_hide_mode = static_cast<core::StreamerHideMode>(streamer_combo_->currentIndex());
            settings.show_privacy_indicator = privacy_indicator_check_->isChecked();
            settings.enable_divine = divine_check_->isChecked();
            settings.enable_sounds = sounds_check_->isChecked();
            
            settings.prediction_count = prediction_count_spin_->value();
            settings.fake_coords_enabled = fake_coords_check_->isChecked();
            settings.mismeasure_warning_enabled = mismeasure_warning_check_->isChecked();
            settings.portal_linking_warning = portal_linking_check_->isChecked();
            settings.show_throw_suggestions = suggestions_check_->isChecked();
            settings.mismeasure_threshold = mismeasure_threshold_spin_->value();
            
            settings.hotkeys_enabled = enable_hotkeys_check_->isChecked();
            settings.hotkeys.reset = hk_reset_->key_string().toStdString();
            settings.hotkeys.undo = hk_undo_->key_string().toStdString();
            settings.hotkeys.redo = hk_redo_->key_string().toStdString();
            settings.hotkeys.toggle_lock = hk_lock_->key_string().toStdString();
            settings.hotkeys.toggle_boat = hk_boat_->key_string().toStdString();
            settings.hotkeys.enter_boat = hk_enter_boat_->key_string().toStdString();
            settings.hotkeys.mod_360 = hk_mod360_->key_string().toStdString();
            settings.hotkeys.toggle_privacy = hk_privacy_->key_string().toStdString();
            settings.hotkeys.angle_up = hk_angle_up_->key_string().toStdString();
            settings.hotkeys.angle_down = hk_angle_down_->key_string().toStdString();
            settings.hotkeys.open_settings = hk_open_settings_->key_string().toStdString();
            
            settings.obs_hide = (settings.streamer_hide_mode != core::StreamerHideMode::Off);
        });
        
        dirty_ = false;
        emit settings_changed();
    }
    
    void export_settings() {
        QString path = QFileDialog::getSaveFileName(
            this, "Export Settings", "dolbot_settings.json", "JSON Files (*.json)"
        );
        if (path.isEmpty()) return;
        
        if (core::Config::instance().export_settings(path)) {
            QMessageBox::information(this, "Export", "Settings exported successfully.");
        } else {
            QMessageBox::warning(this, "Export Failed", "Could not export settings.");
        }
    }
    
    void import_settings() {
        QString path = QFileDialog::getOpenFileName(
            this, "Import Settings", "", "JSON Files (*.json)"
        );
        if (path.isEmpty()) return;
        
        auto [success, message] = core::Config::instance().import_settings(path);
        if (success) {
            load_settings();
            if (message.isEmpty()) {
                QMessageBox::information(this, "Import", "Settings imported successfully.");
            } else {
                QMessageBox::information(this, "Import", message);
            }
            emit settings_changed();
        } else {
            QMessageBox msgBox(this);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("Import Blocked");
            msgBox.setText("The settings file was rejected for security reasons.");
            msgBox.setDetailedText(message);
            msgBox.exec();
        }
    }
    
    void regenerate_fake_offset() {
        core::FakeCoordGenerator::instance().regenerate();
        QMessageBox::information(this, "Fake Coordinates", 
            QString("New offset: (%1, %2)")
                .arg(core::FakeCoordGenerator::instance().offset_x())
                .arg(core::FakeCoordGenerator::instance().offset_z()));
    }

    void on_reset_page() {
        int index = tabs_->currentIndex();
        core::AppSettings d;
        
        switch (index) {
            case 0: // general settings
                version_combo_->setCurrentIndex(static_cast<int>(d.mc_version));
                top_check_->setChecked(d.always_on_top);
                overlay_check_->setChecked(d.overlay_enabled);
                divine_check_->setChecked(d.enable_divine);
                sounds_check_->setChecked(d.enable_sounds);
                break;
            case 1: // accuracy settings
                std_dev_spin_->setValue(d.std_deviation);
                boat_std_spin_->setValue(d.std_dev_boat);
                manual_std_spin_->setValue(d.std_dev_manual);
                crosshair_spin_->setValue(d.crosshair_correction);
                angle_adj_combo_->setCurrentIndex(static_cast<int>(d.angle_adjustment_type));
                tall_res_spin_->setValue(d.tall_resolution_height);
                custom_adj_spin_->setValue(d.custom_angle_adjustment);
                mismeasure_warning_check_->setChecked(d.mismeasure_warning_enabled);
                mismeasure_threshold_spin_->setValue(d.mismeasure_threshold);
                mismeasure_threshold_spin_->setEnabled(d.mismeasure_warning_enabled);
                break;
            case 2: // boat settings
                boat_type_combo_->setCurrentIndex(d.default_boat_type);
                boat_sens_old_spin_->setValue(d.boat_sensitivity_old);
                boat_sens_new_spin_->setValue(d.boat_sensitivity_new);
                show_angle_reset_check_->setChecked(d.show_boat_angle_reset_indicator);
                show_mod360_indicator_check_->setChecked(d.show_mod360_indicator);
                mod360_check_->setChecked(d.reduce_mod_360);
                boat_error_spin_->setValue(d.boat_error_limit);
                boat_std_spin_->setValue(d.std_dev_boat);
                break;
            case 3: // privacy settings
                streamer_combo_->setCurrentIndex(static_cast<int>(d.streamer_hide_mode));
                privacy_indicator_check_->setChecked(d.show_privacy_indicator);
                fake_coords_check_->setChecked(d.fake_coords_enabled);
                break;
            case 4: // display settings
                prediction_count_spin_->setValue(d.prediction_count);
                display_combo_->setCurrentIndex(static_cast<int>(d.display_mode));
                errors_check_->setChecked(d.show_angle_errors);
                updates_check_->setChecked(d.show_angle_updates);
                nether_check_->setChecked(d.show_nether_coords);
                direction_check_->setChecked(d.show_direction);
                suggestions_check_->setChecked(d.show_throw_suggestions);
                break;
            case 5: // hotkeys settings
                enable_hotkeys_check_->setChecked(d.hotkeys_enabled);
                hk_reset_->set_key(QString::fromStdString(d.hotkeys.reset));
                hk_undo_->set_key(QString::fromStdString(d.hotkeys.undo));
                hk_redo_->set_key(QString::fromStdString(d.hotkeys.redo));
                hk_lock_->set_key(QString::fromStdString(d.hotkeys.toggle_lock));
                hk_boat_->set_key(QString::fromStdString(d.hotkeys.toggle_boat));
                hk_privacy_->set_key(QString::fromStdString(d.hotkeys.toggle_privacy));
                hk_angle_up_->set_key(QString::fromStdString(d.hotkeys.angle_up));
                hk_angle_down_->set_key(QString::fromStdString(d.hotkeys.angle_down));
                break;
        }
    }

private:
    void setup_ui() {
        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);
        
        tabs_ = new QTabWidget(this);
        
        tabs_->addTab(create_general_tab(), "General");
        tabs_->addTab(create_accuracy_tab(), "Accuracy");
        tabs_->addTab(create_boat_tab(), "Boat");
        tabs_->addTab(create_privacy_tab(), "Privacy");
        tabs_->addTab(create_display_tab(), "Display");
        tabs_->addTab(create_hotkeys_tab(), "Hotkeys");
        tabs_->addTab(create_advanced_tab(), "Advanced");
        tabs_->addTab(create_about_tab(), "About");
        
        layout->addWidget(tabs_, 1);
        
        auto* buttons = new QHBoxLayout();
        buttons->setSpacing(10);
        
        auto* reset_btn = new QPushButton("Reset All", this);
        reset_btn->setProperty("flat", true);
        reset_btn->setStyleSheet("color: #ef4444; font-weight: bold;");
        connect(reset_btn, &QPushButton::clicked, this, &SettingsPanel::reset_to_defaults);
        buttons->addWidget(reset_btn);
        
        auto* reset_page_btn = new QPushButton("Reset Page", this);
        reset_page_btn->setProperty("flat", true);
        reset_page_btn->setStyleSheet("color: #f59e0b; font-weight: bold;");
        connect(reset_page_btn, &QPushButton::clicked, this, &SettingsPanel::on_reset_page);
        buttons->addWidget(reset_page_btn);
        
        buttons->addStretch();
        
        auto* apply_btn = new QPushButton("Apply", this);
        apply_btn->setDefault(true);
        apply_btn->setMinimumWidth(100);
        connect(apply_btn, &QPushButton::clicked, this, &SettingsPanel::apply_settings);
        buttons->addWidget(apply_btn);
        
        auto* close_btn = new QPushButton("Close", this);
        close_btn->setProperty("flat", true);
        close_btn->setMinimumWidth(80);
        connect(close_btn, &QPushButton::clicked, this, &SettingsPanel::close);
        buttons->addWidget(close_btn);
        
        layout->addLayout(buttons);
    }
    
    QWidget* create_general_tab() {
        auto* widget = new QWidget();
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(16);
        
        auto* version_group = new QGroupBox("Minecraft Version", widget);
        auto* version_layout = new QFormLayout(version_group);
        version_layout->setSpacing(10);
        
        version_combo_ = new QComboBox(widget);
        version_combo_->addItems({"Pre-1.9", "1.9 - 1.12", "1.13 - 1.18", "1.19+"});
        version_combo_->setToolTip(
            "Select your Minecraft version:\n"
            "• Pre-1.9: Old random stronghold generation\n"
            "• 1.9-1.12: Ring-based generation (3 strongholds)\n"
            "• 1.13-1.18: Modern ring system (Offset 8)\n"
            "• 1.19+: Deep Dark update (Offset 0)"
        );
        version_layout->addRow("Version:", version_combo_);
        
        layout->addWidget(version_group);
        
        auto* window_group = new QGroupBox("Window", widget);
        auto* window_layout = new QVBoxLayout(window_group);
        window_layout->setSpacing(8);
        
        top_check_ = new QCheckBox("Always on top", widget);
        top_check_->setToolTip(
            "Keep Dol Bot visible above other windows.\n"
            "Useful while playing Minecraft in windowed mode."
        );
        window_layout->addWidget(top_check_);
        
        overlay_check_ = new QCheckBox("Enable overlay mode", widget);
        overlay_check_->setToolTip("Show compact overlay");
        window_layout->addWidget(overlay_check_);
        
        layout->addWidget(window_group);
        
        auto* features_group = new QGroupBox("Features", widget);
        auto* features_layout = new QVBoxLayout(features_group);
        features_layout->setSpacing(8);
        
        divine_check_ = new QCheckBox("Enable Divine (Fossil)", widget);
        divine_check_->setToolTip(
            "Parse fossil chunk data to narrow down stronghold rings.\n"
            "When you find a fossil, press F3+I to copy chunk data,\n"
            "then Dol Bot can determine which stronghold ring you're near."
        );
        features_layout->addWidget(divine_check_);
        
        sounds_check_ = new QCheckBox("Enable sounds", widget);
        sounds_check_->setToolTip("Play audio cues");
        features_layout->addWidget(sounds_check_);
        
        layout->addWidget(features_group);
        layout->addStretch();
        
        return widget;
    }
    
    QWidget* create_accuracy_tab() {
        auto* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        
        auto* widget = new QWidget();
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(16);
        
        auto* std_group = new QGroupBox("Standard Deviation", widget);
        auto* std_layout = new QFormLayout(std_group);
        std_layout->setSpacing(10);
        
        std_dev_spin_ = new QDoubleSpinBox(widget);
        std_dev_spin_->setRange(0.001, 1.0);
        std_dev_spin_->setDecimals(4);
        std_dev_spin_->setSingleStep(0.005);
        std_dev_spin_->setToolTip(
            "Standard deviation for normal eye throws.\n"
            "Default: 0.03 (typical human aiming error)\n"
            "Lower = more precision assumed, results converge faster\n"
            "Higher = more tolerance for error, needs more throws"
        );
        std_layout->addRow("Normal:", std_dev_spin_);
        
        manual_std_spin_ = new QDoubleSpinBox(widget);
        manual_std_spin_->setRange(0.001, 1.0);
        manual_std_spin_->setDecimals(4);
        manual_std_spin_->setToolTip("For manual entry");
        std_layout->addRow("Manual:", manual_std_spin_);
        
        layout->addWidget(std_group);
        
        auto* cross_group = new QGroupBox("Crosshair", widget);
        auto* cross_layout = new QFormLayout(cross_group);
        cross_layout->setSpacing(10);
        
        crosshair_spin_ = new QDoubleSpinBox(widget);
        crosshair_spin_->setRange(-0.5, 0.5);
        crosshair_spin_->setDecimals(4);
        crosshair_spin_->setSingleStep(0.001);
        crosshair_spin_->setToolTip(
            "Fixed offset applied to all F3+C angles.\n"
            "Use this if your crosshair is slightly off-center.\n"
            "Leave at 0 unless you've calibrated with a known stronghold."
        );
        cross_layout->addRow("Correction:", crosshair_spin_);
        
        layout->addWidget(cross_group);
        
        auto* adj_group = new QGroupBox("Angle Adjustment", widget);
        auto* adj_layout = new QFormLayout(adj_group);
        adj_layout->setSpacing(10);
        
        angle_adj_combo_ = new QComboBox(widget);
        angle_adj_combo_->addItems({"Subpixel", "Tall Resolution", "Custom"});
        angle_adj_combo_->setToolTip(
            "How to handle Minecraft's angle precision:\n"
            "• Subpixel: Best for normal gameplay (no adjustment)\n"
            "• Tall Resolution: For very tall resolution screenshots\n"
            "• Custom: Manually specify adjustment value"
        );
        adj_layout->addRow("Type:", angle_adj_combo_);
        
        tall_res_spin_ = new QDoubleSpinBox(widget);
        tall_res_spin_->setRange(1000, 100000);
        tall_res_spin_->setDecimals(0);
        adj_layout->addRow("Tall Height:", tall_res_spin_);
        
        custom_adj_spin_ = new QDoubleSpinBox(widget);
        custom_adj_spin_->setRange(-1.0, 1.0);
        custom_adj_spin_->setDecimals(6);
        adj_layout->addRow("Custom Value:", custom_adj_spin_);
        
        layout->addWidget(adj_group);
        
        auto* outlier_group = new QGroupBox("Mismeasure Detection", widget);
        auto* outlier_layout = new QFormLayout(outlier_group);
        outlier_layout->setSpacing(10);
        
        mismeasure_warning_check_ = new QCheckBox("Enable mismeasure warnings", widget);
        mismeasure_warning_check_->setToolTip(
            "Warn when angle errors are unusually large\n"
            "or when throws are nearly parallel (poor triangulation).\n"
            "This helps identify when coordinates might be wrong."
        );
        outlier_layout->addRow(mismeasure_warning_check_);
        
        mismeasure_threshold_spin_ = new QDoubleSpinBox(widget);
        mismeasure_threshold_spin_->setRange(0.5, 10.0);
        mismeasure_threshold_spin_->setDecimals(1);
        mismeasure_threshold_spin_->setSingleStep(0.5);
        mismeasure_threshold_spin_->setFixedWidth(80);
        mismeasure_threshold_spin_->setToolTip(
            "Threshold (in standard deviations) for flagging throws as mismeasured.\n"
            "Default: 3.0\n"
            "Higher = fewer warnings, lower = more sensitive detection."
        );
        mismeasure_threshold_spin_->setEnabled(false);
        outlier_layout->addRow("Threshold (σ):", mismeasure_threshold_spin_);
        
        connect(mismeasure_warning_check_, &QCheckBox::toggled, this, [this](bool checked) {
            mismeasure_threshold_spin_->setEnabled(checked);
        });
        
        layout->addWidget(outlier_group);
        
        auto* calibrate_btn = new QPushButton("Calibrate Standard Deviation...", widget);
        calibrate_btn->setToolTip(
            "Open the calibration dialog to statistically determine\n"
            "your optimal standard deviation using known stronghold locations."
        );
        connect(calibrate_btn, &QPushButton::clicked, this, [this]() {
            CalibrationDialog dialog(this);
            if (dialog.exec() == QDialog::Accepted) {
                double calculated = dialog.calculated_std_dev();
                if (calculated > 0) {
                    std_dev_spin_->setValue(calculated);
                    QMessageBox::information(this, "Calibration Complete",
                        QString("Standard deviation set to %1").arg(calculated, 0, 'f', 4));
                }
            }
        });
        layout->addWidget(calibrate_btn);
        
        layout->addStretch();
        
        scroll->setWidget(widget);
        return scroll;
    }
    
    QWidget* create_boat_tab() {
        auto* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        
        auto* widget = new QWidget();
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(16);
        
        auto* mode_group = new QGroupBox("Boat Mode", widget);
        auto* mode_layout = new QVBoxLayout(mode_group);
        mode_layout->setSpacing(10);
        
        auto* mode_desc = new QLabel(
            "Select the default boat type based on your measurement style.\n"
            "Gray/Blue boats use the standard angle grid.\n"
            "Green boat (God Sens) requires sensitivity configuration.",
            mode_group
        );
        mode_desc->setWordWrap(true);
        mode_desc->setStyleSheet("font-size: 11px; color: #94a3b8;");
        mode_layout->addWidget(mode_desc);
        
        boat_type_combo_ = new QComboBox(widget);
        boat_type_combo_->addItems({"Gray boat", "Blue boat", "Green boat (with a boat angle of 0)"});
        boat_type_combo_->setToolTip(
            "Gray boat: Standard negative angle grid\n"
            "Blue boat: Standard positive angle grid\n"
            "Green boat: Uses sensitivity-based validation (God Sens)"
        );
        mode_layout->addWidget(boat_type_combo_);
        
        layout->addWidget(mode_group);
        
        auto* sens_group = new QGroupBox("Sensitivity (from options.txt)", widget);
        auto* sens_layout = new QFormLayout(sens_group);
        sens_layout->setSpacing(10);
        
        auto* sens_info = new QLabel(
            "Enter your Minecraft mouseSensitivity value from options.txt.\n"
            "Only used when Green boat mode is selected.",
            sens_group
        );
        sens_info->setWordWrap(true);
        sens_info->setStyleSheet("font-size: 11px; color: #94a3b8;");
        sens_layout->addRow(sens_info);
        
        boat_sens_old_spin_ = new QDoubleSpinBox(widget);
        boat_sens_old_spin_->setRange(0.0, 2.0);
        boat_sens_old_spin_->setDecimals(10);
        boat_sens_old_spin_->setSingleStep(0.0001);
        boat_sens_old_spin_->setToolTip("Sensitivity for Minecraft 1.9-1.12");
        sens_layout->addRow("Sensitivity 1.9-1.12:", boat_sens_old_spin_);
        
        boat_sens_new_spin_ = new QDoubleSpinBox(widget);
        boat_sens_new_spin_->setRange(0.0, 2.0);
        boat_sens_new_spin_->setDecimals(10);
        boat_sens_new_spin_->setSingleStep(0.0001);
        boat_sens_new_spin_->setToolTip("Sensitivity for Minecraft 1.13+");
        sens_layout->addRow("Sensitivity 1.13+:", boat_sens_new_spin_);
        
        layout->addWidget(sens_group);
        
        auto* autodetect_btn = new QPushButton("Auto-detect Sensitivity", widget);
        autodetect_btn->setToolTip(
            "Attempt to read sensitivity from Minecraft's options.txt file.\n"
            "Looks in %APPDATA%\\.minecraft\\options.txt"
        );
        connect(autodetect_btn, &QPushButton::clicked, this, [this]() {
            auto confirm = QMessageBox::question(this, "Auto-detect Sensitivity",
                "This will overwrite your current Boat Sensitivity settings.\n\n"
                "Only the two sensitivity fields on this tab will be changed.\n"
                "No other settings will be affected.\n\n"
                "Continue?",
                QMessageBox::Yes | QMessageBox::No);
            
            if (confirm != QMessageBox::Yes) {
                return;
            }
            
            QString mc_path = QDir::homePath() + "/AppData/Roaming/.minecraft/options.txt";
            QFile file(mc_path);
            
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox::warning(this, "Auto-detect Failed",
                    "Could not read Minecraft options.txt file.\n\n"
                    "Expected location: " + mc_path + "\n\n"
                    "Make sure Minecraft has been run at least once.");
                return;
            }
            
            QString content = file.readAll();
            file.close();
            
            QRegularExpression re("mouseSensitivity:([0-9.]+)");
            QRegularExpressionMatch match = re.match(content);
            
            if (!match.hasMatch()) {
                QMessageBox::warning(this, "Auto-detect Failed",
                    "Could not find mouseSensitivity in options.txt.\n\n"
                    "The file format may have changed or the setting is missing.");
                return;
            }
            
            bool ok;
            double sensitivity = match.captured(1).toDouble(&ok);
            
            if (!ok || sensitivity < 0 || sensitivity > 2) {
                QMessageBox::warning(this, "Auto-detect Failed",
                    QString("Invalid sensitivity value: %1").arg(match.captured(1)));
                return;
            }
            
            boat_sens_old_spin_->setValue(sensitivity);
            boat_sens_new_spin_->setValue(sensitivity);
            
            QMessageBox::information(this, "Auto-detect Successful",
                QString("Sensitivity detected: %1\n\n"
                        "Applied to both boat sensitivity fields.\n"
                        "Click Apply to save.").arg(sensitivity, 0, 'f', 10));
        });
        layout->addWidget(autodetect_btn);
        
        auto* indicator_group = new QGroupBox("Indicators", widget);
        auto* indicator_layout = new QVBoxLayout(indicator_group);
        indicator_layout->setSpacing(8);
        
        show_angle_reset_check_ = new QCheckBox("Indicate boat angle reset on next F3+C", widget);
        show_angle_reset_check_->setToolTip(
            "Shows an indicator when the boat angle has been reset.\n"
            "Useful for tracking angle adjustments."
        );
        indicator_layout->addWidget(show_angle_reset_check_);
        
        show_mod360_indicator_check_ = new QCheckBox("Indicate angle reduction mod 360 on next F3+C", widget);
        show_mod360_indicator_check_->setToolTip(
            "Shows an indicator when mod-360 reduction is applied.\n"
            "Helps track when the angle wraps around."
        );
        indicator_layout->addWidget(show_mod360_indicator_check_);
        
        mod360_check_ = new QCheckBox("Enable Mod-360 Reduction", widget);
        mod360_check_->setToolTip(
            "Keep boat angles in the negative quadrant.\n"
            "Enable this if you use the negative angle technique\n"
            "for more consistent boat measurements."
        );
        indicator_layout->addWidget(mod360_check_);
        
        layout->addWidget(indicator_group);
        
        auto* accuracy_group = new QGroupBox("Accuracy Settings", widget);
        auto* accuracy_layout = new QFormLayout(accuracy_group);
        accuracy_layout->setSpacing(10);
        
        boat_error_spin_ = new QDoubleSpinBox(widget);
        boat_error_spin_->setRange(0.001, 0.5);
        boat_error_spin_->setDecimals(3);
        boat_error_spin_->setSingleStep(0.01);
        boat_error_spin_->setToolTip(
            "Maximum allowed deviation from the boat angle grid.\n"
            "Default: 0.03\n"
            "Throws outside this limit will be flagged as errors.\n"
            "Increase if you're getting too many false error flags."
        );
        accuracy_layout->addRow("Allowable boat angle error:", boat_error_spin_);
        
        boat_std_spin_ = new QDoubleSpinBox(widget);
        boat_std_spin_->setRange(0.0001, 0.1);
        boat_std_spin_->setDecimals(5);
        boat_std_spin_->setSingleStep(0.0005);
        boat_std_spin_->setToolTip(
            "Standard deviation for boat throws (~10x more precise).\n"
            "Default: 0.001\n"
            "Boat angles snap to a grid, providing much higher accuracy."
        );
        accuracy_layout->addRow("Standard deviation for boat throws:", boat_std_spin_);
        
        layout->addWidget(accuracy_group);
        
        layout->addStretch();
        scroll->setWidget(widget);
        return scroll;
    }
    
    QWidget* create_privacy_tab() {
        auto* widget = new QWidget();
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(16);
        
        auto* mode_group = new QGroupBox("Capture Protection", widget);
        auto* mode_layout = new QFormLayout(mode_group);
        mode_layout->setSpacing(10);
        
        streamer_combo_ = new QComboBox(widget);
        streamer_combo_->addItems({"Off", "Hide from OBS", "Hide from Discord", "Hide from All"});
        streamer_combo_->setToolTip(
            "Hide Dol Bot from screen capture software.\n"
            "Uses Windows Display Affinity (requires Windows 10 2004+).\n"
            "Works best with Window Capture in OBS, not Game Capture.\n\n"
            "To test: Start OBS preview, you should see a black window."
        );
        mode_layout->addRow("Hide Mode:", streamer_combo_);
        
        privacy_indicator_check_ = new QCheckBox("Show privacy indicator", widget);
        privacy_indicator_check_->setToolTip("Show visual feedback when privacy is active");
        mode_layout->addRow("", privacy_indicator_check_);
        
        layout->addWidget(mode_group);
        
        auto* fake_group = new QGroupBox("Fake Coordinates", widget);
        auto* fake_layout = new QVBoxLayout(fake_group);
        fake_layout->setSpacing(10);
        
        fake_coords_check_ = new QCheckBox("Enable fake coordinates", widget);
        fake_coords_check_->setToolTip(
            "Display randomized fake coordinates instead of real ones.\n"
            "A random offset is added to all displayed coordinates.\n"
        );
        fake_layout->addWidget(fake_coords_check_);
        
        auto* regen_btn = new QPushButton("Regenerate Offset", widget);
        regen_btn->setProperty("flat", true);
        regen_btn->setToolTip("Generate new random coordinate offset");
        connect(regen_btn, &QPushButton::clicked, this, &SettingsPanel::regenerate_fake_offset);
        fake_layout->addWidget(regen_btn);
        
        layout->addWidget(fake_group);
        layout->addStretch();
        
        return widget;
    }
    
    QWidget* create_display_tab() {
        auto* widget = new QWidget();
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(16);
        
        auto* display_group = new QGroupBox("Predictions", widget);
        auto* display_layout = new QFormLayout(display_group);
        display_layout->setSpacing(10);
        
        prediction_count_spin_ = new QSpinBox(widget);
        prediction_count_spin_->setRange(1, 3);
        prediction_count_spin_->setToolTip("Number of predictions to display (1-3)");
        display_layout->addRow("Show predictions:", prediction_count_spin_);
        
        display_combo_ = new QComboBox(widget);
        display_combo_->addItems({"(4, 4) - Staircase", "(8, 8) - Center", "Chunk Coords"});
        display_combo_->setToolTip("Coordinate display format");
        display_layout->addRow("Format:", display_combo_);
        
        layout->addWidget(display_group);
        
        auto* info_group = new QGroupBox("Information Display", widget);
        auto* info_layout = new QVBoxLayout(info_group);
        info_layout->setSpacing(8);
        
        errors_check_ = new QCheckBox("Show angle errors", widget);
        info_layout->addWidget(errors_check_);
        
        updates_check_ = new QCheckBox("Show angle updates", widget);
        info_layout->addWidget(updates_check_);
        
        nether_check_ = new QCheckBox("Show Nether coordinates", widget);
        info_layout->addWidget(nether_check_);
        
        direction_check_ = new QCheckBox("Show direction indicator", widget);
        info_layout->addWidget(direction_check_);
        
        suggestions_check_ = new QCheckBox("Show throw suggestions", widget);
        suggestions_check_->setToolTip("Show advice on where to throw next");
        info_layout->addWidget(suggestions_check_);
        

        
        portal_linking_check_ = new QCheckBox("Show portal linking warnings", widget);
        portal_linking_check_->setToolTip(
            "Warn when the stronghold might not link to a portal\n"
            "from your current Nether position (22+ blocks away)."
        );
        info_layout->addWidget(portal_linking_check_);
        
        layout->addWidget(info_group);
        layout->addStretch();
        
        return widget;
    }
    
    QWidget* create_hotkeys_tab() {
        auto* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        
        auto* widget = new QWidget();
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(4);
        
        auto* info = new QLabel("Click a field and press your desired key combination:", widget);
        info->setWordWrap(true);
        layout->addWidget(info);
        
        enable_hotkeys_check_ = new QCheckBox("Enable global hotkeys", widget);
        enable_hotkeys_check_->setToolTip("Enable or disable all system-wide hotkeys.");
        layout->addWidget(enable_hotkeys_check_);
        
        layout->addSpacing(8);
        
        hk_reset_ = new HotkeyEdit("Reset:", "Ctrl+R", widget);
        layout->addWidget(hk_reset_);
        
        hk_undo_ = new HotkeyEdit("Undo:", "Ctrl+Z", widget);
        layout->addWidget(hk_undo_);
        
        hk_redo_ = new HotkeyEdit("Redo:", "Ctrl+Y", widget);
        layout->addWidget(hk_redo_);
        
        hk_lock_ = new HotkeyEdit("Toggle Lock:", "Ctrl+L", widget);
        layout->addWidget(hk_lock_);
        
        hk_boat_ = new HotkeyEdit("Toggle Boat:", "Ctrl+B", widget);
        layout->addWidget(hk_boat_);
        
        hk_privacy_ = new HotkeyEdit("Toggle Privacy:", "Ctrl+H", widget);
        layout->addWidget(hk_privacy_);
        
        hk_angle_up_ = new HotkeyEdit("Angle +:", "Ctrl+]", widget);
        layout->addWidget(hk_angle_up_);
        
        hk_angle_down_ = new HotkeyEdit("Angle -:", "Ctrl+[", widget);
        layout->addWidget(hk_angle_down_);
        
        hk_enter_boat_ = new HotkeyEdit("Enter Boat:", "", widget);
        layout->addWidget(hk_enter_boat_);
        
        hk_mod360_ = new HotkeyEdit("Mod 360:", "", widget);
        layout->addWidget(hk_mod360_);
        
        hk_open_settings_ = new HotkeyEdit("Open Settings:", "Ctrl+,", widget);
        layout->addWidget(hk_open_settings_);
        
        layout->addStretch();
        scroll->setWidget(widget);
        return scroll;
    }
    
    QWidget* create_advanced_tab() {
        auto* widget = new QWidget();
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(16);
        
        auto* data_group = new QGroupBox("Settings Backup", widget);
        auto* data_layout = new QVBoxLayout(data_group);
        data_layout->setSpacing(10);
        
        auto* export_btn = new QPushButton("Export Settings...", widget);
        export_btn->setToolTip("Save all settings to a JSON file");
        connect(export_btn, &QPushButton::clicked, this, &SettingsPanel::export_settings);
        data_layout->addWidget(export_btn);
        
        auto* import_btn = new QPushButton("Import Settings...", widget);
        import_btn->setToolTip("Load settings from a JSON file");
        connect(import_btn, &QPushButton::clicked, this, &SettingsPanel::import_settings);
        data_layout->addWidget(import_btn);
        
        layout->addWidget(data_group);
        layout->addStretch();
        
        return widget;
    }
    
    QWidget* create_about_tab() {
        auto* widget = new QWidget();
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(12);
        
        auto* title = new QLabel("Dol Bot", widget);
        title->setStyleSheet("font-size: 24px; font-weight: bold; color: #6366f1;");
        title->setAlignment(Qt::AlignCenter);
        layout->addWidget(title);
        
        auto* version = new QLabel("Version 1.0.0", widget);
        version->setAlignment(Qt::AlignCenter);
        version->setStyleSheet("color: #94a3b8; font-size: 14px;");
        layout->addWidget(version);
        
        auto* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        
        auto* content = new QWidget();
        auto* content_layout = new QVBoxLayout(content);
        content_layout->setSpacing(20);
        content_layout->setContentsMargins(4, 4, 4, 4);
        
        auto add_section = [&](const QString& title, const QString& text) {
            auto* grp = new QGroupBox(title, content);
            auto* l = new QVBoxLayout(grp);
            auto* lbl = new QLabel(text, grp);
            lbl->setWordWrap(true);
            lbl->setOpenExternalLinks(true);
            l->addWidget(lbl);
            content_layout->addWidget(grp);
        };
        
        add_section("Main Developer", "<b>rep0z-debug</b>");
        
        add_section("Inspiration & Mathematics", 
            "The probability mathematics and stronghold generation logic are based on "
            "<a href='https://github.com/Ninjabrain1/Ninjabrain-Bot'>Ninjabrain Bot</a> and minecraft's logic<br>"
            "This project operates on the same core principles and research."
        );
        
        add_section("Assets", 
            "The SVG icons used in this application are <b>AI-generated</b>."
        );
        
        add_section("Technology", 
            "• Qt 6 (UI & Networking)<br>"
            "• C++20<br>"
            "• CMake"
        );
        
        content_layout->addStretch();
        scroll->setWidget(content);
        layout->addWidget(scroll);
        
        return widget;
    }

    void load_settings() {
        const auto& s = core::Config::instance().settings();
        std_dev_spin_->setValue(s.std_deviation);
        boat_std_spin_->setValue(s.std_dev_boat);
        manual_std_spin_->setValue(s.std_dev_manual);
        crosshair_spin_->setValue(s.crosshair_correction);
        errors_check_->setChecked(s.show_angle_errors);
        direction_check_->setChecked(s.show_direction);
        version_combo_->setCurrentIndex(static_cast<int>(s.mc_version));
        display_combo_->setCurrentIndex(static_cast<int>(s.display_mode));
        top_check_->setChecked(s.always_on_top);
        overlay_check_->setChecked(s.overlay_enabled);
        nether_check_->setChecked(s.show_nether_coords);
        updates_check_->setChecked(s.show_angle_updates);
        
        boat_type_combo_->setCurrentIndex(s.default_boat_type);
        boat_sens_old_spin_->setValue(s.boat_sensitivity_old);
        boat_sens_new_spin_->setValue(s.boat_sensitivity_new);
        show_angle_reset_check_->setChecked(s.show_boat_angle_reset_indicator);
        show_mod360_indicator_check_->setChecked(s.show_mod360_indicator);
        mod360_check_->setChecked(s.reduce_mod_360);
        boat_error_spin_->setValue(s.boat_error_limit);
        angle_adj_combo_->setCurrentIndex(static_cast<int>(s.angle_adjustment_type));
        tall_res_spin_->setValue(s.tall_resolution_height);
        custom_adj_spin_->setValue(s.custom_angle_adjustment);
        streamer_combo_->setCurrentIndex(static_cast<int>(s.streamer_hide_mode));
        privacy_indicator_check_->setChecked(s.show_privacy_indicator);
        divine_check_->setChecked(s.enable_divine);
        sounds_check_->setChecked(s.enable_sounds);
        
        prediction_count_spin_->setValue(s.prediction_count);
        fake_coords_check_->setChecked(s.fake_coords_enabled);
        mismeasure_warning_check_->setChecked(s.mismeasure_warning_enabled);
        mismeasure_threshold_spin_->setValue(s.mismeasure_threshold);
        mismeasure_threshold_spin_->setEnabled(s.mismeasure_warning_enabled);
        portal_linking_check_->setChecked(s.portal_linking_warning);
        suggestions_check_->setChecked(s.show_throw_suggestions);
        
        enable_hotkeys_check_->setChecked(s.hotkeys_enabled);
        hk_reset_->set_key(QString::fromStdString(s.hotkeys.reset));
        hk_undo_->set_key(QString::fromStdString(s.hotkeys.undo));
        hk_redo_->set_key(QString::fromStdString(s.hotkeys.redo));
        hk_lock_->set_key(QString::fromStdString(s.hotkeys.toggle_lock));
        hk_boat_->set_key(QString::fromStdString(s.hotkeys.toggle_boat));
        hk_privacy_->set_key(QString::fromStdString(s.hotkeys.toggle_privacy));
        hk_angle_up_->set_key(QString::fromStdString(s.hotkeys.angle_up));
        hk_angle_down_->set_key(QString::fromStdString(s.hotkeys.angle_down));
        hk_enter_boat_->set_key(QString::fromStdString(s.hotkeys.enter_boat));
        hk_mod360_->set_key(QString::fromStdString(s.hotkeys.mod_360));
        hk_open_settings_->set_key(QString::fromStdString(s.hotkeys.open_settings));
        
        dirty_ = false;
    }
    
    void connect_dirty_tracking() {
        auto mark_dirty = [this]() { dirty_ = true; };
        
        connect(std_dev_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, mark_dirty);
        connect(boat_std_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, mark_dirty);
        connect(manual_std_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, mark_dirty);
        connect(crosshair_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, mark_dirty);
        connect(boat_error_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, mark_dirty);
        connect(boat_sens_old_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, mark_dirty);
        connect(boat_sens_new_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, mark_dirty);
        connect(tall_res_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, mark_dirty);
        connect(custom_adj_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, mark_dirty);
        connect(mismeasure_threshold_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, mark_dirty);
        connect(prediction_count_spin_, QOverload<int>::of(&QSpinBox::valueChanged), this, mark_dirty);
        
        connect(version_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, mark_dirty);
        connect(display_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, mark_dirty);
        connect(boat_type_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, mark_dirty);
        connect(angle_adj_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, mark_dirty);
        connect(streamer_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, mark_dirty);
        
        connect(errors_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(direction_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(suggestions_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(top_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(overlay_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(nether_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(updates_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(show_angle_reset_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(show_mod360_indicator_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(mod360_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(privacy_indicator_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(divine_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(sounds_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(fake_coords_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(mismeasure_warning_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(portal_linking_check_, &QCheckBox::toggled, this, mark_dirty);
        connect(enable_hotkeys_check_, &QCheckBox::toggled, this, mark_dirty);
    }
    
    QTabWidget* tabs_;
    QDoubleSpinBox* std_dev_spin_;
    QDoubleSpinBox* boat_std_spin_;
    QDoubleSpinBox* manual_std_spin_;
    QDoubleSpinBox* crosshair_spin_;
    QCheckBox* errors_check_;
    QCheckBox* direction_check_;
    QCheckBox* suggestions_check_;
    QComboBox* version_combo_;
    QComboBox* display_combo_;
    QCheckBox* top_check_;
    QCheckBox* overlay_check_;
    QCheckBox* nether_check_;
    QCheckBox* updates_check_;
    
    QDoubleSpinBox* boat_error_spin_;
    QComboBox* boat_type_combo_;
    QDoubleSpinBox* boat_sens_old_spin_;
    QDoubleSpinBox* boat_sens_new_spin_;
    QCheckBox* show_angle_reset_check_;
    QCheckBox* show_mod360_indicator_check_;
    QCheckBox* mod360_check_;
    QComboBox* angle_adj_combo_;
    QDoubleSpinBox* tall_res_spin_;
    QDoubleSpinBox* custom_adj_spin_;
    QComboBox* streamer_combo_;
    QCheckBox* privacy_indicator_check_;
    QCheckBox* divine_check_;
    QCheckBox* sounds_check_;
    
    QSpinBox* prediction_count_spin_;
    QCheckBox* fake_coords_check_;
    QCheckBox* mismeasure_warning_check_;
    QDoubleSpinBox* mismeasure_threshold_spin_;
    QCheckBox* portal_linking_check_;
    
    HotkeyEdit* hk_reset_;
    HotkeyEdit* hk_undo_;
    HotkeyEdit* hk_redo_;
    HotkeyEdit* hk_lock_;
    HotkeyEdit* hk_boat_;
    HotkeyEdit* hk_privacy_;
    HotkeyEdit* hk_angle_up_;
    HotkeyEdit* hk_angle_down_;
    HotkeyEdit* hk_enter_boat_;
    HotkeyEdit* hk_mod360_;
    HotkeyEdit* hk_open_settings_;
    
    QCheckBox* enable_hotkeys_check_;
    
    bool dirty_;
    core::ConnectionHandle theme_connection_;
};

} // namespace dolbot::ui
