#pragma once

#include <QString>
#include <QColor>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>
#include <string>

namespace dolbot::ui {

struct ThemeColors {
    QColor background;
    QColor surface;
    QColor surface_hover;
    QColor primary;
    QColor primary_hover;
    QColor secondary;
    QColor text_primary;
    QColor text_secondary;
    QColor text_muted;
    QColor success;
    QColor warning;
    QColor error;
    QColor border;
    QColor shadow;
    QColor accent;
    QColor card;

    [[nodiscard]] QJsonObject to_json() const {
        QJsonObject obj;
        obj["background"] = background.name();
        obj["surface"] = surface.name();
        obj["surface_hover"] = surface_hover.name();
        obj["primary"] = primary.name();
        obj["primary_hover"] = primary_hover.name();
        obj["secondary"] = secondary.name();
        obj["text_primary"] = text_primary.name();
        obj["text_secondary"] = text_secondary.name();
        obj["text_muted"] = text_muted.name();
        obj["success"] = success.name();
        obj["warning"] = warning.name();
        obj["error"] = error.name();
        obj["border"] = border.name();
        obj["shadow"] = shadow.name(QColor::HexArgb);
        obj["accent"] = accent.name();
        obj["card"] = card.name();
        return obj;
    }

    static ThemeColors from_json(const QJsonObject& obj) {
        ThemeColors c;
        c.background = QColor(obj.value("background").toString("#0f0f14"));
        c.surface = QColor(obj.value("surface").toString("#1a1a24"));
        c.surface_hover = QColor(obj.value("surface_hover").toString("#252532"));
        c.primary = QColor(obj.value("primary").toString("#6366f1"));
        c.primary_hover = QColor(obj.value("primary_hover").toString("#818cf8"));
        c.secondary = QColor(obj.value("secondary").toString("#8b5cf6"));
        c.text_primary = QColor(obj.value("text_primary").toString("#f8fafc"));
        c.text_secondary = QColor(obj.value("text_secondary").toString("#cbd5e1"));
        c.text_muted = QColor(obj.value("text_muted").toString("#64748b"));
        c.success = QColor(obj.value("success").toString("#22c55e"));
        c.warning = QColor(obj.value("warning").toString("#f59e0b"));
        c.error = QColor(obj.value("error").toString("#ef4444"));
        c.border = QColor(obj.value("border").toString("#2d2d42"));
        c.shadow = QColor(obj.value("shadow").toString("#00000078"));
        c.accent = QColor(obj.value("accent").toString("#3b82f6"));
        c.card = QColor(obj.value("card").toString("#16161e"));
        return c;
    }

    static ThemeColors dark_default() {
        return {
            .background = QColor("#0f0f14"),
            .surface = QColor("#1a1a24"),
            .surface_hover = QColor("#252532"),
            .primary = QColor("#6366f1"),
            .primary_hover = QColor("#818cf8"),
            .secondary = QColor("#8b5cf6"),
            .text_primary = QColor("#f8fafc"),
            .text_secondary = QColor("#cbd5e1"),
            .text_muted = QColor("#64748b"),
            .success = QColor("#22c55e"),
            .warning = QColor("#f59e0b"),
            .error = QColor("#ef4444"),
            .border = QColor("#2d2d42"),
            .shadow = QColor(0, 0, 0, 120),
            .accent = QColor("#3b82f6"),
            .card = QColor("#16161e")
        };
    }

    static ThemeColors light_default() {
        return {
            .background = QColor("#fafafa"),
            .surface = QColor("#ffffff"),
            .surface_hover = QColor("#f5f5f5"),
            .primary = QColor("#4f46e5"),
            .primary_hover = QColor("#6366f1"),
            .secondary = QColor("#7c3aed"),
            .text_primary = QColor("#0f172a"),
            .text_secondary = QColor("#334155"),
            .text_muted = QColor("#94a3b8"),
            .success = QColor("#16a34a"),
            .warning = QColor("#d97706"),
            .error = QColor("#dc2626"),
            .border = QColor("#e2e8f0"),
            .shadow = QColor(0, 0, 0, 25),
            .accent = QColor("#2563eb"),
            .card = QColor("#ffffff")
        };
    }

    static ThemeColors midnight_default() {
        return {
            .background = QColor("#080810"),
            .surface = QColor("#0d0d16"),
            .surface_hover = QColor("#141420"),
            .primary = QColor("#38bdf8"),
            .primary_hover = QColor("#60d1ff"),
            .secondary = QColor("#a855f7"),
            .text_primary = QColor("#f1f5f9"),
            .text_secondary = QColor("#94a3b8"),
            .text_muted = QColor("#475569"),
            .success = QColor("#14b8a6"),
            .warning = QColor("#fb923c"),
            .error = QColor("#fb7185"),
            .border = QColor("#1e1e30"),
            .shadow = QColor(0, 0, 0, 150),
            .accent = QColor("#0ea5e9"),
            .card = QColor("#0a0a12")
        };
    }
};

struct FontSettings {
    QString primary_family = "Segoe UI";
    QString mono_family = "Consolas";
    int base_size = 13;
    int header_size = 16;
    int small_size = 11;

    [[nodiscard]] QJsonObject to_json() const {
        QJsonObject obj;
        obj["primary_family"] = primary_family;
        obj["mono_family"] = mono_family;
        obj["base_size"] = base_size;
        obj["header_size"] = header_size;
        obj["small_size"] = small_size;
        return obj;
    }

    static FontSettings from_json(const QJsonObject& obj) {
        FontSettings f;
        f.primary_family = obj.value("primary_family").toString("Segoe UI");
        f.mono_family = obj.value("mono_family").toString("Consolas");
        f.base_size = obj.value("base_size").toInt(13);
        f.header_size = obj.value("header_size").toInt(16);
        f.small_size = obj.value("small_size").toInt(11);
        return f;
    }
};

struct SizeSettings {
    int border_radius = 8;
    int spacing = 10;
    int padding = 16;
    int icon_size = 16;

    [[nodiscard]] QJsonObject to_json() const {
        QJsonObject obj;
        obj["border_radius"] = border_radius;
        obj["spacing"] = spacing;
        obj["padding"] = padding;
        obj["icon_size"] = icon_size;
        return obj;
    }

    static SizeSettings from_json(const QJsonObject& obj) {
        SizeSettings s;
        s.border_radius = std::clamp(obj.value("border_radius").toInt(8), 0, 24);
        s.spacing = std::clamp(obj.value("spacing").toInt(10), 0, 32);
        s.padding = std::clamp(obj.value("padding").toInt(16), 0, 48);
        s.icon_size = std::clamp(obj.value("icon_size").toInt(16), 8, 32);
        return s;
    }
};

struct LayoutSettings {
    std::vector<std::string> main_panel_order = {"boat_status", "boat_mode_btn", "result_display", "throw_list"};
    bool show_boat_status = true;
    bool show_status_bar = true;
    bool compact_mode = false;

    [[nodiscard]] QJsonObject to_json() const {
        QJsonObject obj;
        QJsonArray order;
        for (const auto& item : main_panel_order) {
            order.append(QString::fromStdString(item));
        }
        obj["main_panel_order"] = order;
        obj["show_boat_status"] = show_boat_status;
        obj["show_status_bar"] = show_status_bar;
        obj["compact_mode"] = compact_mode;
        return obj;
    }

    static LayoutSettings from_json(const QJsonObject& obj) {
        LayoutSettings l;
        if (obj.contains("main_panel_order")) {
            l.main_panel_order.clear();
            QJsonArray arr = obj.value("main_panel_order").toArray();
            for (const auto& val : arr) {
                l.main_panel_order.push_back(val.toString().toStdString());
            }
            if (l.main_panel_order.empty()) {
                l.main_panel_order = {"boat_status", "boat_mode_btn", "result_display", "throw_list"};
            }
        }
        l.show_boat_status = obj.value("show_boat_status").toBool(true);
        l.show_status_bar = obj.value("show_status_bar").toBool(true);
        l.compact_mode = obj.value("compact_mode").toBool(false);
        return l;
    }

    [[nodiscard]] bool is_valid() const {
        if (main_panel_order.size() != 4) return false;
        bool has_boat = false, has_result = false, has_throw = false, has_btn = false;
        for (const auto& item : main_panel_order) {
            if (item == "boat_status") has_boat = true;
            else if (item == "result_display") has_result = true;
            else if (item == "throw_list") has_throw = true;
            else if (item == "boat_mode_btn") has_btn = true;
        }
        return has_boat && has_result && has_throw && has_btn;
    }

    void reset_to_default() {
        main_panel_order = {"boat_status", "boat_mode_btn", "result_display", "throw_list"};
    }
};

struct OverlayOverride {
    bool use_custom = false;
    ThemeColors colors = ThemeColors::dark_default();
    int opacity = 96;
    int border_radius = 12;

    [[nodiscard]] QJsonObject to_json() const {
        QJsonObject obj;
        obj["use_custom"] = use_custom;
        obj["colors"] = colors.to_json();
        obj["opacity"] = opacity;
        obj["border_radius"] = border_radius;
        return obj;
    }

    static OverlayOverride from_json(const QJsonObject& obj) {
        OverlayOverride o;
        o.use_custom = obj.value("use_custom").toBool(false);
        if (obj.contains("colors")) {
            o.colors = ThemeColors::from_json(obj.value("colors").toObject());
        }
        o.opacity = std::clamp(obj.value("opacity").toInt(96), 0, 100);
        o.border_radius = std::clamp(obj.value("border_radius").toInt(12), 0, 24);
        return o;
    }
};

struct ThemeSettings {
    QString name = "Custom";
    bool is_builtin = false;
    ThemeColors colors = ThemeColors::dark_default();
    FontSettings fonts;
    SizeSettings sizes;
    LayoutSettings layout;
    OverlayOverride overlay;

    [[nodiscard]] QJsonObject to_json() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["is_builtin"] = is_builtin;
        obj["colors"] = colors.to_json();
        obj["fonts"] = fonts.to_json();
        obj["sizes"] = sizes.to_json();
        obj["layout"] = layout.to_json();
        obj["overlay"] = overlay.to_json();
        return obj;
    }

    static ThemeSettings from_json(const QJsonObject& obj) {
        ThemeSettings t;
        t.name = obj.value("name").toString("Custom");
        t.is_builtin = obj.value("is_builtin").toBool(false);
        if (obj.contains("colors")) {
            t.colors = ThemeColors::from_json(obj.value("colors").toObject());
        }
        if (obj.contains("fonts")) {
            t.fonts = FontSettings::from_json(obj.value("fonts").toObject());
        }
        if (obj.contains("sizes")) {
            t.sizes = SizeSettings::from_json(obj.value("sizes").toObject());
        }
        if (obj.contains("layout")) {
            t.layout = LayoutSettings::from_json(obj.value("layout").toObject());
        }
        if (obj.contains("overlay")) {
            t.overlay = OverlayOverride::from_json(obj.value("overlay").toObject());
        }
        if (!t.layout.is_valid()) {
            t.layout.reset_to_default();
        }
        return t;
    }

    static ThemeSettings dark_preset() {
        ThemeSettings t;
        t.name = "Dark";
        t.is_builtin = true;
        t.colors = ThemeColors::dark_default();
        return t;
    }

    static ThemeSettings light_preset() {
        ThemeSettings t;
        t.name = "Light";
        t.is_builtin = true;
        t.colors = ThemeColors::light_default();
        return t;
    }

    static ThemeSettings midnight_preset() {
        ThemeSettings t;
        t.name = "Midnight";
        t.is_builtin = true;
        t.colors = ThemeColors::midnight_default();
        return t;
    }

    // Ocean
    static ThemeSettings ocean_preset() {
        ThemeSettings t;
        t.name = "Ocean";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#0a1628"),
            .surface = QColor("#0f2340"),
            .surface_hover = QColor("#163357"),
            .primary = QColor("#0ea5e9"),
            .primary_hover = QColor("#38bdf8"),
            .secondary = QColor("#06b6d4"),
            .text_primary = QColor("#f0f9ff"),
            .text_secondary = QColor("#bae6fd"),
            .text_muted = QColor("#7dd3fc"),
            .success = QColor("#10b981"),
            .warning = QColor("#fbbf24"),
            .error = QColor("#f43f5e"),
            .border = QColor("#1e3a5f"),
            .shadow = QColor(0, 0, 0, 140),
            .accent = QColor("#0284c7"),
            .card = QColor("#0c1e36")
        };
        return t;
    }

    // Forest
    static ThemeSettings forest_preset() {
        ThemeSettings t;
        t.name = "Forest";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#0f1a14"),
            .surface = QColor("#162b1f"),
            .surface_hover = QColor("#1f3d2a"),
            .primary = QColor("#22c55e"),
            .primary_hover = QColor("#4ade80"),
            .secondary = QColor("#10b981"),
            .text_primary = QColor("#f0fdf4"),
            .text_secondary = QColor("#bbf7d0"),
            .text_muted = QColor("#86efac"),
            .success = QColor("#34d399"),
            .warning = QColor("#fcd34d"),
            .error = QColor("#fb7185"),
            .border = QColor("#1b4028"),
            .shadow = QColor(0, 0, 0, 130),
            .accent = QColor("#16a34a"),
            .card = QColor("#122318")
        };
        return t;
    }

    // Sunset
    static ThemeSettings sunset_preset() {
        ThemeSettings t;
        t.name = "Sunset";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#1a0f16"),
            .surface = QColor("#2a1420"),
            .surface_hover = QColor("#3d1a2e"),
            .primary = QColor("#f97316"),
            .primary_hover = QColor("#fb923c"),
            .secondary = QColor("#ec4899"),
            .text_primary = QColor("#fff7ed"),
            .text_secondary = QColor("#fed7aa"),
            .text_muted = QColor("#fdba74"),
            .success = QColor("#84cc16"),
            .warning = QColor("#facc15"),
            .error = QColor("#f43f5e"),
            .border = QColor("#3d1c2b"),
            .shadow = QColor(0, 0, 0, 135),
            .accent = QColor("#ea580c"),
            .card = QColor("#201018")
        };
        return t;
    }

    // Rose
    static ThemeSettings rose_preset() {
        ThemeSettings t;
        t.name = "Rose";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#1a1015"),
            .surface = QColor("#2a1a22"),
            .surface_hover = QColor("#3a2530"),
            .primary = QColor("#f43f5e"),
            .primary_hover = QColor("#fb7185"),
            .secondary = QColor("#ec4899"),
            .text_primary = QColor("#fff1f2"),
            .text_secondary = QColor("#fecdd3"),
            .text_muted = QColor("#fda4af"),
            .success = QColor("#4ade80"),
            .warning = QColor("#fbbf24"),
            .error = QColor("#ef4444"),
            .border = QColor("#3f1d28"),
            .shadow = QColor(0, 0, 0, 130),
            .accent = QColor("#e11d48"),
            .card = QColor("#22121a")
        };
        return t;
    }

    // Cyber
    static ThemeSettings cyber_preset() {
        ThemeSettings t;
        t.name = "Cyber";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#0a0a0f"),
            .surface = QColor("#121218"),
            .surface_hover = QColor("#1a1a22"),
            .primary = QColor("#00ff9f"),
            .primary_hover = QColor("#5dffb8"),
            .secondary = QColor("#ff00ff"),
            .text_primary = QColor("#e0ffe0"),
            .text_secondary = QColor("#a0ffa0"),
            .text_muted = QColor("#60c060"),
            .success = QColor("#00ff9f"),
            .warning = QColor("#ffff00"),
            .error = QColor("#ff0055"),
            .border = QColor("#1a1a24"),
            .shadow = QColor(0, 0, 0, 150),
            .accent = QColor("#00ccff"),
            .card = QColor("#0e0e14")
        };
        return t;
    }

    // Nord
    static ThemeSettings nord_preset() {
        ThemeSettings t;
        t.name = "Nord";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#2e3440"),
            .surface = QColor("#3b4252"),
            .surface_hover = QColor("#434c5e"),
            .primary = QColor("#88c0d0"),
            .primary_hover = QColor("#8fbcbb"),
            .secondary = QColor("#81a1c1"),
            .text_primary = QColor("#eceff4"),
            .text_secondary = QColor("#d8dee9"),
            .text_muted = QColor("#a3be8c"),
            .success = QColor("#a3be8c"),
            .warning = QColor("#ebcb8b"),
            .error = QColor("#bf616a"),
            .border = QColor("#4c566a"),
            .shadow = QColor(0, 0, 0, 100),
            .accent = QColor("#5e81ac"),
            .card = QColor("#353c4a")
        };
        return t;
    }

    // Dracula
    static ThemeSettings dracula_preset() {
        ThemeSettings t;
        t.name = "Dracula";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#282a36"),
            .surface = QColor("#343746"),
            .surface_hover = QColor("#44475a"),
            .primary = QColor("#bd93f9"),
            .primary_hover = QColor("#caa8fb"),
            .secondary = QColor("#ff79c6"),
            .text_primary = QColor("#f8f8f2"),
            .text_secondary = QColor("#d6d6d0"),
            .text_muted = QColor("#6272a4"),
            .success = QColor("#50fa7b"),
            .warning = QColor("#ffb86c"),
            .error = QColor("#ff5555"),
            .border = QColor("#44475a"),
            .shadow = QColor(0, 0, 0, 120),
            .accent = QColor("#8be9fd"),
            .card = QColor("#2e3140")
        };
        return t;
    }

    // Monokai 
    static ThemeSettings monokai_preset() {
        ThemeSettings t;
        t.name = "Monokai";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#272822"),
            .surface = QColor("#2d2e27"),
            .surface_hover = QColor("#3e3d32"),
            .primary = QColor("#f92672"),
            .primary_hover = QColor("#ff4d8a"),
            .secondary = QColor("#ae81ff"),
            .text_primary = QColor("#f8f8f2"),
            .text_secondary = QColor("#cfcfc2"),
            .text_muted = QColor("#75715e"),
            .success = QColor("#a6e22e"),
            .warning = QColor("#e6db74"),
            .error = QColor("#f92672"),
            .border = QColor("#3e3d32"),
            .shadow = QColor(0, 0, 0, 130),
            .accent = QColor("#66d9ef"),
            .card = QColor("#2a2b24")
        };
        return t;
    }

    // Solarized
    static ThemeSettings solarized_preset() {
        ThemeSettings t;
        t.name = "Solarized";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#002b36"),
            .surface = QColor("#073642"),
            .surface_hover = QColor("#094654"),
            .primary = QColor("#268bd2"),
            .primary_hover = QColor("#4aa3e0"),
            .secondary = QColor("#2aa198"),
            .text_primary = QColor("#fdf6e3"),
            .text_secondary = QColor("#eee8d5"),
            .text_muted = QColor("#839496"),
            .success = QColor("#859900"),
            .warning = QColor("#b58900"),
            .error = QColor("#dc322f"),
            .border = QColor("#094552"),
            .shadow = QColor(0, 0, 0, 120),
            .accent = QColor("#6c71c4"),
            .card = QColor("#003847")
        };
        return t;
    }

    // Tokyo Night 
    static ThemeSettings tokyo_night_preset() {
        ThemeSettings t;
        t.name = "Tokyo Night";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#1a1b26"),
            .surface = QColor("#24283b"),
            .surface_hover = QColor("#2f3549"),
            .primary = QColor("#7aa2f7"),
            .primary_hover = QColor("#89b4fa"),
            .secondary = QColor("#bb9af7"),
            .text_primary = QColor("#c0caf5"),
            .text_secondary = QColor("#a9b1d6"),
            .text_muted = QColor("#565f89"),
            .success = QColor("#9ece6a"),
            .warning = QColor("#e0af68"),
            .error = QColor("#f7768e"),
            .border = QColor("#3b4261"),
            .shadow = QColor(0, 0, 0, 140),
            .accent = QColor("#7dcfff"),
            .card = QColor("#1f2335")
        };
        return t;
    }

    // Catppuccin
    static ThemeSettings catppuccin_preset() {
        ThemeSettings t;
        t.name = "Catppuccin";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#1e1e2e"),
            .surface = QColor("#302d41"),
            .surface_hover = QColor("#3e3a50"),
            .primary = QColor("#cba6f7"),
            .primary_hover = QColor("#dbbeff"),
            .secondary = QColor("#f5c2e7"),
            .text_primary = QColor("#cdd6f4"),
            .text_secondary = QColor("#bac2de"),
            .text_muted = QColor("#6c7086"),
            .success = QColor("#a6e3a1"),
            .warning = QColor("#fab387"),
            .error = QColor("#f38ba8"),
            .border = QColor("#45475a"),
            .shadow = QColor(0, 0, 0, 125),
            .accent = QColor("#89dceb"),
            .card = QColor("#26233a")
        };
        return t;
    }

    // One Dark
    static ThemeSettings one_dark_preset() {
        ThemeSettings t;
        t.name = "One Dark";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#282c34"),
            .surface = QColor("#2c323c"),
            .surface_hover = QColor("#3a3f4b"),
            .primary = QColor("#61afef"),
            .primary_hover = QColor("#74c0ff"),
            .secondary = QColor("#c678dd"),
            .text_primary = QColor("#abb2bf"),
            .text_secondary = QColor("#9da5b4"),
            .text_muted = QColor("#5c6370"),
            .success = QColor("#98c379"),
            .warning = QColor("#e5c07b"),
            .error = QColor("#e06c75"),
            .border = QColor("#3e4451"),
            .shadow = QColor(0, 0, 0, 115),
            .accent = QColor("#56b6c2"),
            .card = QColor("#2d333e")
        };
        return t;
    }

    // Amethyst
    static ThemeSettings amethyst_preset() {
        ThemeSettings t;
        t.name = "Amethyst";
        t.is_builtin = true;
        t.colors = {
            .background = QColor("#120f1a"),
            .surface = QColor("#1e1830"),
            .surface_hover = QColor("#2a2244"),
            .primary = QColor("#a855f7"),
            .primary_hover = QColor("#c084fc"),
            .secondary = QColor("#8b5cf6"),
            .text_primary = QColor("#faf5ff"),
            .text_secondary = QColor("#e9d5ff"),
            .text_muted = QColor("#c4b5fd"),
            .success = QColor("#4ade80"),
            .warning = QColor("#fbbf24"),
            .error = QColor("#f87171"),
            .border = QColor("#2e2548"),
            .shadow = QColor(0, 0, 0, 135),
            .accent = QColor("#7c3aed"),
            .card = QColor("#170f24")
        };
        return t;
    }

    static std::vector<ThemeSettings> all_builtin_presets() {
        return {
            dark_preset(),
            light_preset(),
            midnight_preset(),
            ocean_preset(),
            forest_preset(),
            sunset_preset(),
            rose_preset(),
            cyber_preset(),
            nord_preset(),
            dracula_preset(),
            monokai_preset(),
            solarized_preset(),
            tokyo_night_preset(),
            catppuccin_preset(),
            one_dark_preset(),
            amethyst_preset()
        };
    }
};

} // namespace dolbot::ui
