#pragma once

#include <string>
#include <variant>
#include <optional>
#include <fstream>
#include <map>
#include <random>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QString>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

#include "dolbot/core/signal.hpp"

namespace dolbot::core {

enum class McVersion {
    Pre_1_9,
    V1_9_to_1_12,
    V1_13_to_1_18,
    V1_19_plus
};

enum class DisplayMode {
    FourFour,
    EightEight,
    ChunkCoords
};

enum class AngleAdjustmentType {
    Subpixel,
    TallResolution,
    Custom
};

enum class StreamerHideMode {
    Off,
    OBS,
    Discord,
    All
};

struct HotkeyConfig {
    std::string reset = "Ctrl+R";
    std::string undo = "Ctrl+Z";
    std::string redo = "Ctrl+Y";
    std::string toggle_lock = "Ctrl+L";
    std::string toggle_boat = "Ctrl+B";
    std::string enter_boat = "";
    std::string mod_360 = "";
    std::string toggle_privacy = "Ctrl+H";
    std::string angle_up = "Ctrl+]";
    std::string angle_down = "Ctrl+[";
    std::string open_settings = "Ctrl+,";
    
    bool operator==(const HotkeyConfig&) const = default;
};

struct AppSettings {
    double std_deviation = 0.1;
    double std_dev_boat = 0.001;
    double std_dev_manual = 0.03;
    double crosshair_correction = 0.0;
    bool use_advanced_stats = true;
    bool show_angle_errors = false;
    bool show_direction = true;
    McVersion mc_version = McVersion::V1_19_plus;
    DisplayMode display_mode = DisplayMode::FourFour;
    int window_opacity = 100;
    bool always_on_top = true;
    bool overlay_enabled = false;
    bool show_nether_coords = true;
    bool show_angle_updates = false;
    bool obs_hide = false;
    std::string language = "en";
    
    double boat_error_limit = 0.03;
    double boat_sensitivity_old = 0.0;
    double boat_sensitivity_new = 0.0;
    bool use_boat_sensitivity = false;
    bool reduce_mod_360 = false;
    int default_boat_type = 2;
    bool show_boat_angle_reset_indicator = false;
    bool show_mod360_indicator = false;
    
    int overlay_x = -1;
    int overlay_y = -1;
    
    AngleAdjustmentType angle_adjustment_type = AngleAdjustmentType::Subpixel;
    double tall_resolution_height = 16384.0;
    double custom_angle_adjustment = 0.0;
    
    StreamerHideMode streamer_hide_mode = StreamerHideMode::Off;
    bool show_privacy_indicator = true;
    
    bool enable_divine = false;
    
    std::string theme = "dark";
    bool enable_sounds = false;
    
    int prediction_count = 1;
    bool fake_coords_enabled = false;
    int fake_coords_offset_x = 0;
    int fake_coords_offset_z = 0;
    
    bool portal_linking_warning = false;
    
    bool mismeasure_warning_enabled = true;
    bool show_throw_suggestions = true;
    double mismeasure_threshold = 3.0;
    
    bool auto_reset_enabled = false;
    std::string log_file_path = "";
    
    bool hotkeys_enabled = true;
    HotkeyConfig hotkeys;
    
    bool operator==(const AppSettings&) const = default;
    
    [[nodiscard]] double effective_angle_adjustment() const {
        switch (angle_adjustment_type) {
            case AngleAdjustmentType::TallResolution:
                return 360.0 / tall_resolution_height;
            case AngleAdjustmentType::Custom:
                return custom_angle_adjustment;
            default:
                return 0.0;
        }
    }
};

class FakeCoordGenerator {
public:
    static FakeCoordGenerator& instance() {
        static FakeCoordGenerator inst;
        return inst;
    }
    
    void regenerate() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(-500, 500);
        offset_x_ = dist(gen);
        offset_z_ = dist(gen);
    }
    
    [[nodiscard]] std::pair<int, int> apply(int real_x, int real_z) const {
        return {real_x + offset_x_, real_z + offset_z_};
    }
    
    [[nodiscard]] int offset_x() const { return offset_x_; }
    [[nodiscard]] int offset_z() const { return offset_z_; }
    
private:
    FakeCoordGenerator() { regenerate(); }
    int offset_x_ = 0;
    int offset_z_ = 0;
};

class Config {
public:
    static Config& instance() {
        static Config inst;
        return inst;
    }
    
    void load() {
        QString path = get_config_path();
        load_from_file(path);
    }
    
    void load_from_file(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            save();
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isObject()) return;
        
        load_from_json(doc.object());
        changed_.fire(settings_);
    }
    
    void load_from_json(const QJsonObject& obj) {
        settings_.std_deviation = obj.value("std_deviation").toDouble(0.1);
        settings_.std_dev_boat = obj.value("std_dev_boat").toDouble(0.001);
        settings_.std_dev_manual = obj.value("std_dev_manual").toDouble(0.03);
        settings_.crosshair_correction = obj.value("crosshair_correction").toDouble(0.0);
        settings_.use_advanced_stats = obj.value("use_advanced_stats").toBool(true);
        settings_.show_angle_errors = obj.value("show_angle_errors").toBool(false);
        settings_.show_direction = obj.value("show_direction").toBool(true);
        settings_.mc_version = static_cast<McVersion>(obj.value("mc_version").toInt(3));
        settings_.display_mode = static_cast<DisplayMode>(obj.value("display_mode").toInt(0));
        settings_.window_opacity = obj.value("window_opacity").toInt(100);
        settings_.always_on_top = obj.value("always_on_top").toBool(true);
        settings_.overlay_enabled = obj.value("overlay_enabled").toBool(false);
        settings_.show_nether_coords = obj.value("show_nether_coords").toBool(true);
        settings_.show_angle_updates = obj.value("show_angle_updates").toBool(false);
        settings_.obs_hide = obj.value("obs_hide").toBool(false);
        settings_.language = obj.value("language").toString("en").toStdString();
        
        settings_.boat_error_limit = obj.value("boat_error_limit").toDouble(0.03);
        settings_.boat_sensitivity_old = obj.value("boat_sensitivity_old").toDouble(obj.value("boat_sensitivity").toDouble(0.0));
        settings_.boat_sensitivity_new = obj.value("boat_sensitivity_new").toDouble(obj.value("boat_sensitivity").toDouble(0.0));
        settings_.use_boat_sensitivity = obj.value("use_boat_sensitivity").toBool(false);
        settings_.reduce_mod_360 = obj.value("reduce_mod_360").toBool(false);
        settings_.default_boat_type = obj.value("default_boat_type").toInt(2);
        settings_.show_boat_angle_reset_indicator = obj.value("show_boat_angle_reset_indicator").toBool(false);
        settings_.show_mod360_indicator = obj.value("show_mod360_indicator").toBool(false);
        settings_.overlay_x = obj.value("overlay_x").toInt(-1);
        settings_.overlay_y = obj.value("overlay_y").toInt(-1);
        settings_.angle_adjustment_type = static_cast<AngleAdjustmentType>(obj.value("angle_adjustment_type").toInt(0));
        settings_.tall_resolution_height = obj.value("tall_resolution_height").toDouble(16384.0);
        settings_.custom_angle_adjustment = obj.value("custom_angle_adjustment").toDouble(0.0);
        settings_.streamer_hide_mode = static_cast<StreamerHideMode>(obj.value("streamer_hide_mode").toInt(0));
        settings_.show_privacy_indicator = obj.value("show_privacy_indicator").toBool(true);
        settings_.enable_divine = obj.value("enable_divine").toBool(false);
        settings_.theme = obj.value("theme").toString("dark").toStdString();
        settings_.enable_sounds = obj.value("enable_sounds").toBool(false);
        
        settings_.prediction_count = std::clamp(obj.value("prediction_count").toInt(1), 1, 3);
        settings_.fake_coords_enabled = obj.value("fake_coords_enabled").toBool(false);
        settings_.fake_coords_offset_x = obj.value("fake_coords_offset_x").toInt(0);
        settings_.fake_coords_offset_z = obj.value("fake_coords_offset_z").toInt(0);
        
        settings_.fake_coords_offset_x = obj.value("fake_coords_offset_x").toInt(0);
        settings_.fake_coords_offset_z = obj.value("fake_coords_offset_z").toInt(0);
        
        settings_.portal_linking_warning = obj.value("portal_linking_warning").toBool(false);
        settings_.mismeasure_warning_enabled = obj.value("mismeasure_warning_enabled").toBool(true);
        settings_.show_throw_suggestions = obj.value("show_throw_suggestions").toBool(true);
        settings_.mismeasure_threshold = obj.value("mismeasure_threshold").toDouble(3.0);
        settings_.auto_reset_enabled = obj.value("auto_reset_enabled").toBool(false);
        settings_.log_file_path = obj.value("log_file_path").toString("").toStdString();
        settings_.hotkeys_enabled = obj.value("hotkeys_enabled").toBool(true);
        
        if (obj.contains("hotkeys")) {
            QJsonObject hk = obj.value("hotkeys").toObject();
            settings_.hotkeys.reset = hk.value("reset").toString("Ctrl+R").toStdString();
            settings_.hotkeys.undo = hk.value("undo").toString("Ctrl+Z").toStdString();
            settings_.hotkeys.redo = hk.value("redo").toString("Ctrl+Y").toStdString();
            settings_.hotkeys.toggle_lock = hk.value("toggle_lock").toString("Ctrl+L").toStdString();
            settings_.hotkeys.toggle_boat = hk.value("toggle_boat").toString("Ctrl+B").toStdString();
            settings_.hotkeys.enter_boat = hk.value("enter_boat").toString("").toStdString();
            settings_.hotkeys.mod_360 = hk.value("mod_360").toString("").toStdString();
            settings_.hotkeys.toggle_privacy = hk.value("toggle_privacy").toString("Ctrl+H").toStdString();
            settings_.hotkeys.angle_up = hk.value("angle_up").toString("Ctrl+]").toStdString();
            settings_.hotkeys.angle_down = hk.value("angle_down").toString("Ctrl+[").toStdString();
            settings_.hotkeys.open_settings = hk.value("open_settings").toString("Ctrl+,").toStdString();
        }
    }
    
    void save() {
        QString path = get_config_path();
        save_to_file(path);
    }
    
    void save_to_file(const QString& path) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QJsonObject obj = to_json();
        
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        }
    }
    
    QJsonObject to_json() const {
        QJsonObject obj;
        obj["std_deviation"] = settings_.std_deviation;
        obj["std_dev_boat"] = settings_.std_dev_boat;
        obj["std_dev_manual"] = settings_.std_dev_manual;
        obj["crosshair_correction"] = settings_.crosshair_correction;
        obj["use_advanced_stats"] = settings_.use_advanced_stats;
        obj["show_angle_errors"] = settings_.show_angle_errors;
        obj["show_direction"] = settings_.show_direction;
        obj["mc_version"] = static_cast<int>(settings_.mc_version);
        obj["display_mode"] = static_cast<int>(settings_.display_mode);
        obj["window_opacity"] = settings_.window_opacity;
        obj["always_on_top"] = settings_.always_on_top;
        obj["overlay_enabled"] = settings_.overlay_enabled;
        obj["show_nether_coords"] = settings_.show_nether_coords;
        obj["show_angle_updates"] = settings_.show_angle_updates;
        obj["obs_hide"] = settings_.obs_hide;
        obj["language"] = QString::fromStdString(settings_.language);
        
        obj["boat_error_limit"] = settings_.boat_error_limit;
        obj["boat_sensitivity_old"] = settings_.boat_sensitivity_old;
        obj["boat_sensitivity_new"] = settings_.boat_sensitivity_new;
        obj["use_boat_sensitivity"] = settings_.use_boat_sensitivity;
        obj["reduce_mod_360"] = settings_.reduce_mod_360;
        obj["default_boat_type"] = settings_.default_boat_type;
        obj["show_boat_angle_reset_indicator"] = settings_.show_boat_angle_reset_indicator;
        obj["show_mod360_indicator"] = settings_.show_mod360_indicator;
        obj["overlay_x"] = settings_.overlay_x;
        obj["overlay_y"] = settings_.overlay_y;
        obj["angle_adjustment_type"] = static_cast<int>(settings_.angle_adjustment_type);
        obj["tall_resolution_height"] = settings_.tall_resolution_height;
        obj["custom_angle_adjustment"] = settings_.custom_angle_adjustment;
        obj["streamer_hide_mode"] = static_cast<int>(settings_.streamer_hide_mode);
        obj["show_privacy_indicator"] = settings_.show_privacy_indicator;
        obj["enable_divine"] = settings_.enable_divine;
        obj["theme"] = QString::fromStdString(settings_.theme);
        obj["enable_sounds"] = settings_.enable_sounds;
        
        obj["prediction_count"] = settings_.prediction_count;
        obj["fake_coords_enabled"] = settings_.fake_coords_enabled;
        obj["fake_coords_offset_x"] = settings_.fake_coords_offset_x;
        obj["fake_coords_offset_z"] = settings_.fake_coords_offset_z;
        obj["fake_coords_offset_x"] = settings_.fake_coords_offset_x;
        obj["fake_coords_offset_z"] = settings_.fake_coords_offset_z;
        
        obj["portal_linking_warning"] = settings_.portal_linking_warning;
        obj["mismeasure_warning_enabled"] = settings_.mismeasure_warning_enabled;
        obj["show_throw_suggestions"] = settings_.show_throw_suggestions;
        obj["mismeasure_threshold"] = settings_.mismeasure_threshold;
        obj["auto_reset_enabled"] = settings_.auto_reset_enabled;
        obj["log_file_path"] = QString::fromStdString(settings_.log_file_path);
        obj["hotkeys_enabled"] = settings_.hotkeys_enabled;
        
        QJsonObject hk;
        hk["reset"] = QString::fromStdString(settings_.hotkeys.reset);
        hk["undo"] = QString::fromStdString(settings_.hotkeys.undo);
        hk["redo"] = QString::fromStdString(settings_.hotkeys.redo);
        hk["toggle_lock"] = QString::fromStdString(settings_.hotkeys.toggle_lock);
        hk["toggle_boat"] = QString::fromStdString(settings_.hotkeys.toggle_boat);
        hk["enter_boat"] = QString::fromStdString(settings_.hotkeys.enter_boat);
        hk["mod_360"] = QString::fromStdString(settings_.hotkeys.mod_360);
        hk["toggle_privacy"] = QString::fromStdString(settings_.hotkeys.toggle_privacy);
        hk["angle_up"] = QString::fromStdString(settings_.hotkeys.angle_up);
        hk["angle_down"] = QString::fromStdString(settings_.hotkeys.angle_down);
        hk["open_settings"] = QString::fromStdString(settings_.hotkeys.open_settings);
        obj["hotkeys"] = hk;
        
        return obj;
    }
    
    bool export_settings(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;
        file.write(QJsonDocument(to_json()).toJson(QJsonDocument::Indented));
        return true;
    }
    
    struct ValidationResult {
        bool valid = true;
        QString error_message;
        QStringList warnings;
    };
    
    ValidationResult validate_settings_json(const QJsonObject& obj) const {
        ValidationResult result;
        QStringList issues;
        
        if (obj.contains("log_file_path")) {
            QString path = obj.value("log_file_path").toString();
            if (path.contains("..") || path.contains("../") || path.contains("..\\")) {
                issues.append("Suspicious path traversal detected in log_file_path");
            }
            QString pathLower = path.toLower();
            if (pathLower.contains("system32") || pathLower.contains("windows") ||
                pathLower.contains("program files") || pathLower.contains("programdata")) {
                issues.append("log_file_path points to protected system directory");
            }
        }
        
        auto checkNumericRange = [&](const QString& key, double min, double max, const QString& name) {
            if (obj.contains(key)) {
                double val = obj.value(key).toDouble();
                if (val < min || val > max) {
                    issues.append(QString("%1 value %2 is outside valid range [%3, %4]")
                        .arg(name).arg(val).arg(min).arg(max));
                }
            }
        };
        
        checkNumericRange("std_deviation", 0.001, 10.0, "Standard Deviation");
        checkNumericRange("std_dev_boat", 0.0001, 1.0, "Boat Standard Deviation");
        checkNumericRange("std_dev_manual", 0.001, 1.0, "Manual Standard Deviation");
        checkNumericRange("crosshair_correction", -10.0, 10.0, "Crosshair Correction");
        checkNumericRange("boat_error_limit", 0.001, 1.0, "Boat Error Limit");
        checkNumericRange("mismeasure_threshold", 0.1, 50.0, "Mismeasure Threshold");
        checkNumericRange("window_opacity", 10, 100, "Window Opacity");
        
        auto checkStringLength = [&](const QString& key, int maxLen, const QString& name) {
            if (obj.contains(key)) {
                QString val = obj.value(key).toString();
                if (val.length() > maxLen) {
                    issues.append(QString("%1 exceeds maximum length of %2 characters")
                        .arg(name).arg(maxLen));
                }
            }
        };
        
        checkStringLength("language", 10, "Language");
        checkStringLength("theme", 100, "Theme");
        checkStringLength("log_file_path", 500, "Log File Path");
        
        if (obj.contains("hotkeys")) {
            QJsonObject hk = obj.value("hotkeys").toObject();
            for (const QString& key : hk.keys()) {
                QString val = hk.value(key).toString();
                if (val.length() > 50) {
                    issues.append(QString("Hotkey '%1' value is suspiciously long").arg(key));
                }
                if (val.contains("<") || val.contains(">") || val.contains("script")) {
                    issues.append(QString("Hotkey '%1' contains suspicious characters").arg(key));
                }
            }
        }
        
        QStringList validKeys = {
            "std_deviation", "std_dev_boat", "std_dev_manual", "crosshair_correction",
            "use_advanced_stats", "show_angle_errors", "show_direction", "mc_version",
            "display_mode", "window_opacity", "always_on_top", "overlay_enabled",
            "show_nether_coords", "show_angle_updates", "obs_hide", "language",
            "boat_error_limit", "boat_sensitivity_old", "boat_sensitivity_new", "boat_sensitivity",
            "use_boat_sensitivity", "reduce_mod_360", "default_boat_type",
            "show_boat_angle_reset_indicator", "show_mod360_indicator", "overlay_x", "overlay_y",
            "angle_adjustment_type", "tall_resolution_height", "custom_angle_adjustment",
            "streamer_hide_mode", "show_privacy_indicator", "enable_divine", "theme",
            "enable_sounds", "prediction_count", "fake_coords_enabled",
            "fake_coords_offset_x", "fake_coords_offset_z", "portal_linking_warning",
            "mismeasure_warning_enabled", "show_throw_suggestions", "mismeasure_threshold",
            "auto_reset_enabled", "log_file_path", "hotkeys_enabled", "hotkeys"
        };
        
        for (const QString& key : obj.keys()) {
            if (!validKeys.contains(key)) {
                result.warnings.append(QString("Unknown setting key '%1' will be ignored").arg(key));
            }
        }
        
        if (!issues.isEmpty()) {
            result.valid = false;
            result.error_message = "Security validation failed:\n• " + issues.join("\n• ");
        }
        
        return result;
    }
    
    std::pair<bool, QString> import_settings(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return {false, "Could not open file for reading"};
        }
        
        QByteArray data = file.readAll();
        
        if (data.size() > 1024 * 1024) {
            return {false, "Settings file is suspiciously large (>1MB)"};
        }
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (doc.isNull()) {
            return {false, QString("Invalid JSON: %1").arg(parseError.errorString())};
        }
        if (!doc.isObject()) {
            return {false, "Settings file must contain a JSON object"};
        }
        
        ValidationResult validation = validate_settings_json(doc.object());
        if (!validation.valid) {
            return {false, validation.error_message};
        }
        
        load_from_json(doc.object());
        save();
        changed_.fire(settings_);
        
        if (!validation.warnings.isEmpty()) {
            return {true, "Settings imported with warnings:\n• " + validation.warnings.join("\n• ")};
        }
        return {true, QString()};
    }
    
    const AppSettings& settings() const { return settings_; }
    
    void update(const AppSettings& new_settings) {
        if (settings_ == new_settings) return;
        settings_ = new_settings;
        save();
        changed_.fire(settings_);
    }
    
    template<typename T>
    void set(T AppSettings::* member, T value) {
        if (settings_.*member == value) return;
        settings_.*member = value;
        save();
        changed_.fire(settings_);
    }

    void modify(std::function<void(AppSettings&)> func) {
        AppSettings current = settings_;
        func(current);
        if (current == settings_) return;
        settings_ = current;
        save();
        changed_.fire(settings_);
    }
    
    ConnectionHandle on_change(std::function<void(const AppSettings&)> callback) {
        return changed_.connect(std::move(callback));
    }
    
private:
    Config() { load(); }
    
    QString get_config_path() {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
               + "/dolbot_settings.json";
    }
    
    AppSettings settings_;
    Signal<const AppSettings&> changed_;
};

} // namespace dolbot::core
