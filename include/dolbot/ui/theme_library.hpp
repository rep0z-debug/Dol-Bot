#pragma once

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <optional>
#include <vector>
#include <set>
#include "dolbot/ui/theme_data.hpp"

namespace dolbot::ui {

class ThemeLibrary {
public:
    static ThemeLibrary& instance() {
        static ThemeLibrary inst;
        return inst;
    }

    [[nodiscard]] std::vector<ThemeSettings> list_all_themes() const {
        std::vector<ThemeSettings> themes = ThemeSettings::all_builtin_presets();

        QDir dir(get_themes_directory());
        if (!dir.exists()) return themes;

        QStringList filters;
        filters << "*.json";
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
        for (const auto& file : files) {
            auto theme = load_theme_file(file.absoluteFilePath());
            if (theme.has_value()) {
                themes.push_back(theme.value());
            }
        }
        return themes;
    }

    [[nodiscard]] std::vector<ThemeSettings> list_custom_themes() const {
        std::vector<ThemeSettings> themes;
        QDir dir(get_themes_directory());
        if (!dir.exists()) return themes;

        QStringList filters;
        filters << "*.json";
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
        for (const auto& file : files) {
            auto theme = load_theme_file(file.absoluteFilePath());
            if (theme.has_value()) {
                themes.push_back(theme.value());
            }
        }
        return themes;
    }

    bool save_theme(const ThemeSettings& theme) {
        if (theme.is_builtin) return false;

        QString dir_path = get_themes_directory();
        QDir dir(dir_path);
        if (!dir.exists()) {
            dir.mkpath(dir_path);
        }

        QString filename = sanitize_filename(theme.name) + ".json";
        QString filepath = dir_path + "/" + filename;

        QFile file(filepath);
        if (!file.open(QIODevice::WriteOnly)) return false;

        QJsonDocument doc(theme.to_json());
        file.write(doc.toJson(QJsonDocument::Indented));
        return true;
    }

    bool delete_theme(const QString& name) {
        QString filepath = get_theme_filepath(name);
        if (filepath.isEmpty()) return false;
        return QFile::remove(filepath);
    }

    bool rename_theme(const QString& old_name, const QString& new_name) {
        auto theme = load_theme(old_name);
        if (!theme.has_value() || theme->is_builtin) return false;

        theme->name = new_name;
        if (!save_theme(theme.value())) return false;

        QString old_filepath = get_theme_filepath(old_name);
        if (!old_filepath.isEmpty() && old_filepath != get_theme_filepath(new_name)) {
            QFile::remove(old_filepath);
        }
        return true;
    }

    [[nodiscard]] std::optional<ThemeSettings> load_theme(const QString& name) const {
        if (name == "Dark") return ThemeSettings::dark_preset();
        if (name == "Light") return ThemeSettings::light_preset();
        if (name == "Midnight") return ThemeSettings::midnight_preset();
        if (name == "Ocean") return ThemeSettings::ocean_preset();
        if (name == "Forest") return ThemeSettings::forest_preset();
        if (name == "Sunset") return ThemeSettings::sunset_preset();
        if (name == "Rose") return ThemeSettings::rose_preset();
        if (name == "Cyber") return ThemeSettings::cyber_preset();
        if (name == "Nord") return ThemeSettings::nord_preset();
        if (name == "Dracula") return ThemeSettings::dracula_preset();
        if (name == "Monokai") return ThemeSettings::monokai_preset();
        if (name == "Solarized") return ThemeSettings::solarized_preset();
        if (name == "Tokyo Night") return ThemeSettings::tokyo_night_preset();
        if (name == "Catppuccin") return ThemeSettings::catppuccin_preset();
        if (name == "One Dark") return ThemeSettings::one_dark_preset();
        if (name == "Amethyst") return ThemeSettings::amethyst_preset();

        QString filepath = get_theme_filepath(name);
        if (filepath.isEmpty()) return std::nullopt;
        return load_theme_file(filepath);
    }

    bool import_from_file(const QString& path) {
        auto theme = load_theme_file(path);
        if (!theme.has_value()) return false;

        theme->is_builtin = false;
        return save_theme(theme.value());
    }

    bool export_to_file(const QString& name, const QString& path) const {
        auto theme = load_theme(name);
        if (!theme.has_value()) return false;

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;

        QJsonDocument doc(theme->to_json());
        file.write(doc.toJson(QJsonDocument::Indented));
        return true;
    }

    [[nodiscard]] bool theme_exists(const QString& name) const {
        if (name == "Dark" || name == "Light" || name == "Midnight" || 
            name == "Ocean" || name == "Forest" || name == "Sunset" ||
            name == "Rose" || name == "Cyber" || name == "Nord" || name == "Dracula" ||
            name == "Monokai" || name == "Solarized" || name == "Tokyo Night" ||
            name == "Catppuccin" || name == "One Dark" || name == "Amethyst") return true;
        return !get_theme_filepath(name).isEmpty();
    }

    void hide_theme(const QString& name) {
        hidden_themes_.insert(name);
        save_hidden_themes();
    }

    void unhide_theme(const QString& name) {
        hidden_themes_.erase(name);
        save_hidden_themes();
    }

    [[nodiscard]] bool is_hidden(const QString& name) const {
        return hidden_themes_.count(name) > 0;
    }

    [[nodiscard]] std::vector<QString> get_hidden_theme_names() const {
        return std::vector<QString>(hidden_themes_.begin(), hidden_themes_.end());
    }

private:
    ThemeLibrary() {
        load_hidden_themes();
    }

    std::set<QString> hidden_themes_;

    [[nodiscard]] QString get_themes_directory() const {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/themes";
    }

    [[nodiscard]] QString get_theme_filepath(const QString& name) const {
        QString dir_path = get_themes_directory();
        QString filename = sanitize_filename(name) + ".json";
        QString filepath = dir_path + "/" + filename;
        if (QFile::exists(filepath)) return filepath;
        return QString();
    }

    [[nodiscard]] QString sanitize_filename(const QString& name) const {
        QString result = name;
        result.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
        result = result.trimmed();
        if (result.isEmpty()) result = "untitled";
        return result;
    }

    [[nodiscard]] std::optional<ThemeSettings> load_theme_file(const QString& path) const {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return std::nullopt;

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isObject()) return std::nullopt;

        return ThemeSettings::from_json(doc.object());
    }

    void save_hidden_themes() const {
        QString filepath = get_themes_directory() + "/.hidden";
        QDir().mkpath(get_themes_directory());
        QFile file(filepath);
        if (!file.open(QIODevice::WriteOnly)) return;
        QJsonArray arr;
        for (const auto& name : hidden_themes_) {
            arr.append(name);
        }
        file.write(QJsonDocument(arr).toJson());
    }

    void load_hidden_themes() {
        QString filepath = get_themes_directory() + "/.hidden";
        QFile file(filepath);
        if (!file.open(QIODevice::ReadOnly)) return;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isArray()) return;
        for (const auto& val : doc.array()) {
            if (val.isString()) {
                hidden_themes_.insert(val.toString());
            }
        }
    }
};

} // namespace dolbot::ui
