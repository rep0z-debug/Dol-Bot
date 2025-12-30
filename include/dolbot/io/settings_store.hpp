#pragma once

#include <QSettings>
#include <QString>
#include <QVariant>
#include <QPoint>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <vector>
#include "dolbot/core/config.hpp"
#include "dolbot/domain/eye_throw.hpp"

namespace dolbot::io {

class SettingsStore {
public:
    static SettingsStore& instance() {
        static SettingsStore inst;
        return inst;
    }
    
    void save_window_geometry(const QByteArray& geometry) {
        settings_.setValue("window/geometry", geometry);
    }
    
    QByteArray load_window_geometry() {
        return settings_.value("window/geometry").toByteArray();
    }
    
    void save_window_state(const QByteArray& state) {
        settings_.setValue("window/state", state);
    }
    
    QByteArray load_window_state() {
        return settings_.value("window/state").toByteArray();
    }
    
    void save_last_theme(const QString& theme) {
        settings_.setValue("ui/theme", theme);
    }
    
    QString load_last_theme() {
        return settings_.value("ui/theme", "dark").toString();
    }
    
    void save_overlay_position(int x, int y) {
        settings_.setValue("overlay/x", x);
        settings_.setValue("overlay/y", y);
    }
    
    QPoint load_overlay_position() {
        return {
            settings_.value("overlay/x", 100).toInt(),
            settings_.value("overlay/y", 100).toInt()
        };
    }
    
    void save_throws(const std::vector<domain::EyeThrow>& throws) {
        QJsonArray arr;
        for (const auto& t : throws) {
            QJsonObject obj;
            obj["x"] = t.position.x;
            obj["z"] = t.position.z;
            obj["h_angle"] = t.horizontal_angle;
            obj["v_angle"] = t.vertical_angle;
            obj["correction"] = t.correction;
            obj["type"] = static_cast<int>(t.type);
            obj["dimension"] = static_cast<int>(t.dimension);
            obj["boat_mode"] = t.is_boat_mode;
            obj["boat_error_limit"] = t.boat_error_limit;
            obj["boat_sensitivity"] = t.boat_sensitivity;
            obj["use_boat_sensitivity"] = t.use_boat_sensitivity;
            arr.append(obj);
        }
        
        QString path = get_throws_path();
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(arr).toJson());
        }
    }
    
    std::vector<domain::EyeThrow> load_throws() {
        std::vector<domain::EyeThrow> result;
        
        QFile file(get_throws_path());
        if (!file.open(QIODevice::ReadOnly)) {
            return result;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isArray()) {
            return result;
        }
        
        for (const auto& item : doc.array()) {
            QJsonObject obj = item.toObject();
            domain::EyeThrow t;
            t.position.x = obj["x"].toDouble();
            t.position.z = obj["z"].toDouble();
            t.horizontal_angle = obj["h_angle"].toDouble();
            t.vertical_angle = obj["v_angle"].toDouble();
            t.correction = obj["correction"].toDouble();
            t.type = static_cast<domain::ThrowType>(obj["type"].toInt());
            t.dimension = static_cast<domain::Dimension>(obj["dimension"].toInt());
            t.is_boat_mode = obj["boat_mode"].toBool();
            t.boat_error_limit = obj["boat_error_limit"].toDouble(0.03);
            t.boat_sensitivity = obj["boat_sensitivity"].toDouble(0.0);
            t.use_boat_sensitivity = obj["use_boat_sensitivity"].toBool(false);
            result.push_back(t);
        }
        
        return result;
    }
    
    void clear_throws() {
        QFile::remove(get_throws_path());
    }
    
    void save_redo_stack(const std::vector<domain::EyeThrow>& redo_stack) {
        QJsonArray arr;
        for (const auto& t : redo_stack) {
            QJsonObject obj;
            obj["x"] = t.position.x;
            obj["z"] = t.position.z;
            obj["h_angle"] = t.horizontal_angle;
            obj["v_angle"] = t.vertical_angle;
            obj["correction"] = t.correction;
            obj["type"] = static_cast<int>(t.type);
            obj["dimension"] = static_cast<int>(t.dimension);
            obj["boat_mode"] = t.is_boat_mode;
            obj["boat_error_limit"] = t.boat_error_limit;
            obj["boat_sensitivity"] = t.boat_sensitivity;
            obj["use_boat_sensitivity"] = t.use_boat_sensitivity;
            arr.append(obj);
        }
        
        QString path = get_redo_path();
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(arr).toJson());
        }
    }
    
    std::vector<domain::EyeThrow> load_redo_stack() {
        std::vector<domain::EyeThrow> result;
        
        QFile file(get_redo_path());
        if (!file.open(QIODevice::ReadOnly)) {
            return result;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isArray()) {
            return result;
        }
        
        for (const auto& item : doc.array()) {
            QJsonObject obj = item.toObject();
            domain::EyeThrow t;
            t.position.x = obj["x"].toDouble();
            t.position.z = obj["z"].toDouble();
            t.horizontal_angle = obj["h_angle"].toDouble();
            t.vertical_angle = obj["v_angle"].toDouble();
            t.correction = obj["correction"].toDouble();
            t.type = static_cast<domain::ThrowType>(obj["type"].toInt());
            t.dimension = static_cast<domain::Dimension>(obj["dimension"].toInt());
            t.is_boat_mode = obj["boat_mode"].toBool();
            t.boat_error_limit = obj["boat_error_limit"].toDouble(0.03);
            t.boat_sensitivity = obj["boat_sensitivity"].toDouble(0.0);
            t.use_boat_sensitivity = obj["use_boat_sensitivity"].toBool(false);
            result.push_back(t);
        }
        
        return result;
    }
    
    void clear_redo_stack() {
        QFile::remove(get_redo_path());
    }
    
    template<typename T>
    void save(const QString& key, const T& value) {
        settings_.setValue(key, QVariant::fromValue(value));
    }
    
    template<typename T>
    T load(const QString& key, const T& default_value = T{}) {
        return settings_.value(key, QVariant::fromValue(default_value)).template value<T>();
    }
    
    void sync() {
        settings_.sync();
    }

private:
    SettingsStore() : settings_("DolBot", "DolBot") {}
    
    QString get_throws_path() {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
               + "/dolbot_session.json";
    }
    
    QString get_redo_path() {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
               + "/dolbot_redo.json";
    }
    
    QSettings settings_;
};

} // namespace dolbot::io
