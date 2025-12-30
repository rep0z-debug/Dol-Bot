#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QColorDialog>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QScrollArea>
#include <QFormLayout>
#include <QFrame>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QFontComboBox>
#include <QCheckBox>
#include <QCloseEvent>
#include <QApplication>
#include <QGridLayout>
#include <QPainter>
#include <QPainterPath>
#include "dolbot/ui/theme_data.hpp"
#include "dolbot/ui/theme_library.hpp"
#include "dolbot/ui/theme_engine.hpp"
#include "dolbot/core/config.hpp"

namespace dolbot::ui {

class ColorButton : public QPushButton {
    Q_OBJECT
public:
    explicit ColorButton(const QString& label, QWidget* parent = nullptr)
        : QPushButton(parent), label_(label) {
        setFixedSize(110, 34);
        setCursor(Qt::PointingHandCursor);
        connect(this, &QPushButton::clicked, this, &ColorButton::pick_color);
    }

    void set_color(const QColor& c) {
        color_ = c;
        update_style();
    }

    [[nodiscard]] QColor color() const { return color_; }

signals:
    void color_changed(const QColor& c);

private slots:
    void pick_color() {
        QColor c = QColorDialog::getColor(color_, this, "Select " + label_, 
            QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
        if (c.isValid()) {
            color_ = c;
            update_style();
            emit color_changed(c);
        }
    }

private:
    void update_style() {
        bool is_light = (color_.lightness() > 140);
        setStyleSheet(QString(
            "QPushButton { background-color: %1; color: %2; border: 2px solid %3; "
            "border-radius: 6px; font-weight: 600; font-size: 11px; }"
            "QPushButton:hover { border-color: #6366f1; }"
        ).arg(color_.name())
         .arg(is_light ? "#1a1a24" : "#f8fafc")
         .arg(color_.darker(130).name()));
        setText(color_.name().toUpper());
    }

    QColor color_ = QColor("#333");
    QString label_;
};

class DragListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit DragListWidget(QWidget* parent = nullptr) : QListWidget(parent) {
        setDragDropMode(QAbstractItemView::InternalMove);
        setDefaultDropAction(Qt::MoveAction);
        setMinimumHeight(130);
    }

signals:
    void order_changed();

protected:
    void dropEvent(QDropEvent* event) override {
        QListWidget::dropEvent(event);
        emit order_changed();
    }
};

class MainPreview : public QFrame {
    Q_OBJECT
public:
    explicit MainPreview(QWidget* parent = nullptr) : QFrame(parent) {
        setMinimumHeight(280);
        setup_ui();
    }

    void update_preview(const ThemeSettings& theme) {
        const auto& c = theme.colors;
        const auto& f = theme.fonts;
        int r = std::min(theme.sizes.border_radius, 12);
        QString fontFamily = f.primary_family;
        QString monoFamily = f.mono_family;

        setStyleSheet(QString("MainPreview { background: %1; border: 1px solid %2; border-radius: %3px; }")
            .arg(c.background.name()).arg(c.border.name()).arg(r));

        header_->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600; font-family: '%2'; background: transparent;")
            .arg(c.text_muted.name()).arg(fontFamily));

        card_->setStyleSheet(QString("QFrame { background: %1; border: 1px solid %2; border-radius: %3px; }")
            .arg(c.card.name()).arg(c.border.name()).arg(r));

        title_->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: 700; font-family: '%3'; background: transparent;")
            .arg(c.text_primary.name()).arg(f.header_size).arg(fontFamily));

        coords_->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: 700; font-family: '%3'; background: transparent;")
            .arg(c.accent.name()).arg(f.header_size + 4).arg(monoFamily));

        certainty_->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: 600; font-family: '%3'; background: transparent;")
            .arg(c.success.name()).arg(f.base_size).arg(fontFamily));

        muted_->setStyleSheet(QString("color: %1; font-size: %2px; font-family: '%3'; background: transparent;")
            .arg(c.text_muted.name()).arg(f.small_size).arg(fontFamily));

        warning_->setStyleSheet(QString("color: %1; font-size: %2px; font-family: '%3'; background: transparent;")
            .arg(c.warning.name()).arg(f.small_size).arg(fontFamily));

        error_->setStyleSheet(QString("color: %1; font-size: %2px; font-family: '%3'; background: transparent;")
            .arg(c.error.name()).arg(f.small_size).arg(fontFamily));

        button_->setStyleSheet(QString(
            "QPushButton { background: %1; color: white; border-radius: %2px; font-weight: 600; padding: 8px 16px; border: none; font-family: '%4'; font-size: %5px; }"
            "QPushButton:hover { background: %3; }"
        ).arg(c.primary.name()).arg(r).arg(c.primary_hover.name()).arg(fontFamily).arg(f.base_size));

        secondary_btn_->setStyleSheet(QString(
            "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 8px 16px; font-family: '%5'; font-size: %6px; }"
        ).arg(c.secondary.name()).arg(c.text_primary.name()).arg(c.border.name()).arg(r).arg(fontFamily).arg(f.base_size));

        surface_box_->setStyleSheet(QString("QFrame { background: %1; border-radius: %2px; }")
            .arg(c.surface.name()).arg(r));
        
        surface_label_->setStyleSheet(QString("color: %1; font-size: %2px; font-family: '%3'; background: transparent;")
            .arg(c.text_secondary.name()).arg(f.small_size).arg(fontFamily));
    }

private:
    void setup_ui() {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(10);

        header_ = new QLabel("Live Preview", this);
        layout->addWidget(header_);

        card_ = new QFrame(this);
        auto* card_layout = new QVBoxLayout(card_);
        card_layout->setContentsMargins(14, 12, 14, 12);
        card_layout->setSpacing(6);

        title_ = new QLabel("Stronghold #1", card_);
        card_layout->addWidget(title_);

        coords_ = new QLabel("(-1248, 672)", card_);
        card_layout->addWidget(coords_);

        auto* row = new QHBoxLayout();
        row->setSpacing(12);
        certainty_ = new QLabel("87.3%", card_);
        row->addWidget(certainty_);
        muted_ = new QLabel("Ring 2 • 1842 blocks", card_);
        row->addWidget(muted_);
        row->addStretch();
        card_layout->addLayout(row);

        layout->addWidget(card_);

        surface_box_ = new QFrame(this);
        auto* sb_layout = new QHBoxLayout(surface_box_);
        sb_layout->setContentsMargins(10, 8, 10, 8);
        surface_label_ = new QLabel("Surface color preview", surface_box_);
        sb_layout->addWidget(surface_label_);
        layout->addWidget(surface_box_);

        auto* status_row = new QHBoxLayout();
        status_row->setSpacing(12);
        warning_ = new QLabel("⚠ Warning", this);
        status_row->addWidget(warning_);
        error_ = new QLabel("✕ Error", this);
        status_row->addWidget(error_);
        status_row->addStretch();
        layout->addLayout(status_row);

        auto* btn_row = new QHBoxLayout();
        btn_row->setSpacing(10);
        button_ = new QPushButton("Primary", this);
        btn_row->addWidget(button_);
        secondary_btn_ = new QPushButton("Secondary", this);
        btn_row->addWidget(secondary_btn_);
        btn_row->addStretch();
        layout->addLayout(btn_row);

        layout->addStretch();
    }

    QLabel* header_;
    QFrame* card_;
    QLabel* title_;
    QLabel* coords_;
    QLabel* certainty_;
    QLabel* muted_;
    QLabel* warning_;
    QLabel* error_;
    QPushButton* button_;
    QPushButton* secondary_btn_;
    QFrame* surface_box_;
    QLabel* surface_label_;
};

class OverlayPreview : public QWidget {
    Q_OBJECT
public:
    explicit OverlayPreview(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(130);
        setup_ui();
    }

    void update_preview(const ThemeSettings& theme) {
        current_theme_ = theme;
        const auto& c = theme.overlay.use_custom ? theme.overlay.colors : theme.colors;
        const auto& f = theme.fonts;
        int opacity = theme.overlay.use_custom ? theme.overlay.opacity : 96;

        header_->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600; font-family: '%2'; background: transparent;")
            .arg(c.primary.name()).arg(f.primary_family));
        header_->setText("Dol Bot");

        coords_->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: 700; font-family: '%2'; background: transparent;")
            .arg(c.text_primary.name()).arg(f.mono_family));

        nether_->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 600; font-family: '%2'; background: transparent;")
            .arg(c.warning.name()).arg(f.mono_family));

        certainty_->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: 600; font-family: '%2'; background: transparent;")
            .arg(c.success.name()).arg(f.primary_family));

        distance_->setStyleSheet(QString("color: %1; font-size: 12px; font-family: '%2'; background: transparent;")
            .arg(c.text_secondary.name()).arg(f.primary_family));

        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        
        const auto& c = current_theme_.overlay.use_custom ? current_theme_.overlay.colors : current_theme_.colors;
        int opacity = current_theme_.overlay.use_custom ? current_theme_.overlay.opacity : 96;
        int r = current_theme_.overlay.use_custom ? current_theme_.overlay.border_radius : 
                std::min(current_theme_.sizes.border_radius, 12);
        
        QPainterPath path;
        path.addRoundedRect(rect().adjusted(1, 1, -1, -1), r, r);
        painter.setClipPath(path);
        
        QColor bgColor = c.background;
        bgColor.setAlpha(opacity * 255 / 100);
        
        painter.fillPath(path, bgColor);
        painter.setPen(QPen(c.border, 1));
        painter.drawPath(path);
    }

private:
    void setup_ui() {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 14, 16, 14);
        layout->setSpacing(8);

        header_ = new QLabel("Dol Bot", this);
        layout->addWidget(header_);

        coords_ = new QLabel("(-1248, 672)", this);
        layout->addWidget(coords_);

        nether_ = new QLabel("N: -156, 84", this);
        layout->addWidget(nether_);

        auto* row = new QHBoxLayout();
        row->setSpacing(12);
        certainty_ = new QLabel("87.3%", this);
        row->addWidget(certainty_);
        distance_ = new QLabel("1842 blocks", this);
        row->addWidget(distance_);
        row->addStretch();
        layout->addLayout(row);

        layout->addStretch();
    }

    QLabel* header_;
    QLabel* coords_;
    QLabel* nether_;
    QLabel* certainty_;
    QLabel* distance_;
    ThemeSettings current_theme_;
};

class ThemeEditor : public QDialog {
    Q_OBJECT
public:
    explicit ThemeEditor(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("Theme Editor");
        setMinimumSize(780, 620);
        resize(820, 680);
        
        original_theme_ = ThemeEngine::instance().current_theme();
        current_theme_ = original_theme_;
        has_changes_ = false;
        loading_ = true;
        
        setup_ui();
        load_theme_to_ui();
        update_theme_list();
        update_previews();
        
        loading_ = false;
    }

protected:
    void closeEvent(QCloseEvent* event) override {
        if (has_changes_) {
            auto result = QMessageBox::question(this, "Unsaved Changes",
                "You have unsaved changes. Do you want to save them before closing?",
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                QMessageBox::Save);
            if (result == QMessageBox::Save) {
                on_save_and_close();
                return;
            } else if (result == QMessageBox::Cancel) {
                event->ignore();
                return;
            }
        }
        ThemeEngine::instance().apply_theme(original_theme_);
        event->accept();
    }

private slots:
    void on_theme_list_clicked(QListWidgetItem* item) {
        if (!item) return;
        if (item->flags() == Qt::NoItemFlags) return;
        
        QString name = item->text();
        if (name.startsWith("───")) return;
        
        if (item->data(Qt::UserRole).toString() == "__hidden_header__") {
            hidden_expanded_ = !hidden_expanded_;
            update_theme_list();
            return;
        }
        
        if (item->data(Qt::UserRole).toString() == "__hidden__") {
            return;
        }
        
        if (has_changes_) {
            auto result = QMessageBox::question(this, "Unsaved Changes",
                "You have unsaved changes. Do you want to discard them?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (result == QMessageBox::No) {
                update_theme_list();
                return;
            }
        }
        
        auto theme = ThemeLibrary::instance().load_theme(name);
        if (!theme.has_value()) return;
        
        current_theme_ = theme.value();
        original_theme_ = current_theme_;
        loading_ = true;
        load_theme_to_ui();
        update_previews();
        loading_ = false;
        has_changes_ = false;
    }

    void on_save() {
        save_ui_to_theme();
        
        if (current_theme_.is_builtin) {
            if (!has_changes_) {
                ThemeEngine::instance().apply_theme(current_theme_);
                return;
            }
            auto result = QMessageBox::question(this, "Save Built-in Theme",
                QString("'%1' is a built-in theme and cannot be overwritten.\n\n"
                        "Would you like to save it as a new custom theme?").arg(current_theme_.name),
                QMessageBox::Save | QMessageBox::Cancel,
                QMessageBox::Save);
            
            if (result == QMessageBox::Save) {
                on_save_as();
                return;
            } else {
                ThemeEngine::instance().apply_theme(current_theme_);
                return;
            }
        }
        
        auto confirm = QMessageBox::question(this, "Confirm Override",
            QString("Are you sure you want to overwrite the custom theme '%1'?").arg(current_theme_.name),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        
        if (confirm == QMessageBox::Yes) {
            ThemeLibrary::instance().save_theme(current_theme_);
            
            core::Config::instance().modify([this](core::AppSettings& s) {
                s.theme = current_theme_.name.toStdString();
            });
            
            has_changes_ = false;
            original_theme_ = current_theme_;
            update_theme_list();
            
            QMessageBox::information(this, "Theme Saved", 
                QString("'%1' has been saved and set as default theme.").arg(current_theme_.name));
        }
    }

    void on_save_as() {
        QString name = QInputDialog::getText(this, "Save Theme As", 
            "Enter a name for your custom theme:", QLineEdit::Normal, 
            current_theme_.is_builtin ? "My " + current_theme_.name : current_theme_.name);
        if (name.isEmpty()) return;
        if (name == "Dark" || name == "Light" || name == "Midnight" || 
            name == "Ocean" || name == "Forest" || name == "Sunset" ||
            name == "Rose" || name == "Cyber" || name == "Nord" || name == "Dracula") {
            QMessageBox::warning(this, "Error", "Cannot use built-in theme names.");
            return;
        }
        save_ui_to_theme();
        current_theme_.name = name;
        current_theme_.is_builtin = false;
        ThemeLibrary::instance().save_theme(current_theme_);
        
        core::Config::instance().modify([this](core::AppSettings& s) {
            s.theme = current_theme_.name.toStdString();
        });
        
        has_changes_ = false;
        original_theme_ = current_theme_;
        update_theme_list();
        
        QMessageBox::information(this, "Saved", 
            QString("Theme '%1' saved and set as default.").arg(current_theme_.name));
    }

    void on_delete() {
        if (current_theme_.is_builtin) {
            QMessageBox::warning(this, "Error", "Cannot delete built-in themes.");
            return;
        }
        if (QMessageBox::question(this, "Delete Theme", 
            QString("Delete theme '%1'?").arg(current_theme_.name)) == QMessageBox::Yes) {
            ThemeLibrary::instance().delete_theme(current_theme_.name);
            current_theme_ = ThemeSettings::dark_preset();
            has_changes_ = false;
            load_theme_to_ui();
            update_theme_list();
            update_previews();
        }
    }

    void on_import() {
        QString path = QFileDialog::getOpenFileName(this, "Import Theme", QString(), "JSON Files (*.json)");
        if (path.isEmpty()) return;
        if (ThemeLibrary::instance().import_from_file(path)) {
            update_theme_list();
            QMessageBox::information(this, "Success", "Theme imported successfully!");
        } else {
            QMessageBox::warning(this, "Error", "Failed to import theme.");
        }
    }

    void on_export() {
        save_ui_to_theme();
        QString path = QFileDialog::getSaveFileName(this, "Export Theme", 
            current_theme_.name + ".json", "JSON Files (*.json)");
        if (path.isEmpty()) return;
        ThemeLibrary::instance().export_to_file(current_theme_.name, path);
        QMessageBox::information(this, "Success", "Theme exported!");
    }

    void on_apply() {
        save_ui_to_theme();
        ThemeEngine::instance().apply_theme(current_theme_);
        update_previews();
    }

    void on_save_and_close() {
        save_ui_to_theme();
        
        if (current_theme_.is_builtin) {
            if (!has_changes_) {
                ThemeEngine::instance().apply_theme(current_theme_);
                core::Config::instance().modify([this](core::AppSettings& s) {
                    s.theme = current_theme_.name.toStdString();
                });
                accept();
                return;
            }
            
            auto result = QMessageBox::question(this, "Save Theme",
                QString("'%1' is a built-in theme and cannot be overwritten.\n\n"
                        "Would you like to save it as a new custom theme?").arg(current_theme_.name),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                QMessageBox::Save);
            
            if (result == QMessageBox::Save) {
                QString name = QInputDialog::getText(this, "Save Theme As", 
                    "Enter a name for your custom theme:", QLineEdit::Normal, 
                    "My " + current_theme_.name);
                if (name.isEmpty()) return;
                
                if (name == "Dark" || name == "Light" || name == "Midnight" || 
                    name == "Ocean" || name == "Forest" || name == "Sunset" ||
                    name == "Rose" || name == "Cyber" || name == "Nord" || name == "Dracula") {
                    QMessageBox::warning(this, "Error", "Cannot use built-in theme names.");
                    return;
                }
                
                current_theme_.name = name;
                current_theme_.is_builtin = false;
                ThemeLibrary::instance().save_theme(current_theme_);
                
                core::Config::instance().modify([this](core::AppSettings& s) {
                    s.theme = current_theme_.name.toStdString();
                });
                
                ThemeEngine::instance().apply_theme(current_theme_);
                accept();
            } else if (result == QMessageBox::Discard) {
                ThemeEngine::instance().apply_theme(original_theme_);
                accept();
            }
            return;
        }
        
        if (!current_theme_.is_builtin) {
            ThemeLibrary::instance().save_theme(current_theme_);
        }
        
        core::Config::instance().modify([this](core::AppSettings& s) {
            s.theme = current_theme_.name.toStdString();
        });
        ThemeEngine::instance().apply_theme(current_theme_);
        accept();
    }

    void on_cancel() {
        if (has_changes_) {
            auto result = QMessageBox::question(this, "Unsaved Changes",
                "You have unsaved changes. Do you want to discard them?",
                QMessageBox::Discard | QMessageBox::Cancel,
                QMessageBox::Cancel);
            if (result == QMessageBox::Cancel) {
                return;
            }
            ThemeEngine::instance().apply_theme(original_theme_);
        }
        reject();
    }

    void on_value_changed() {
        if (loading_) return;
        save_ui_to_theme();
        update_previews();
        has_changes_ = true;
    }
    
    void on_layout_changed() {
        if (loading_) return;
        save_ui_to_theme();
        update_previews();
        has_changes_ = true;
    }

private:
    void setup_ui() {
        auto* main_layout = new QVBoxLayout(this);
        main_layout->setSpacing(16);
        main_layout->setContentsMargins(20, 20, 20, 20);

        auto* content = new QHBoxLayout();
        content->setSpacing(20);

        tabs_ = new QTabWidget(this);
        tabs_->addTab(create_themes_tab(), "Themes");
        tabs_->addTab(create_colors_tab(), "Colors");
        tabs_->addTab(create_fonts_tab(), "Typography");
        tabs_->addTab(create_layout_tab(), "Layout");
        tabs_->addTab(create_overlay_tab(), "Overlay");
        content->addWidget(tabs_, 3);

        auto* preview_panel = new QVBoxLayout();
        preview_panel->setSpacing(16);
        main_preview_ = new MainPreview(this);
        preview_panel->addWidget(main_preview_);
        
        auto* overlay_container = new QWidget(this);
        auto* overlay_layout = new QVBoxLayout(overlay_container);
        overlay_layout->setContentsMargins(0, 0, 0, 0);
        overlay_layout->setSpacing(4);
        auto* overlay_label = new QLabel("Overlay Preview", overlay_container);
        overlay_label->setStyleSheet("color: #64748b; font-size: 10px;");
        overlay_layout->addWidget(overlay_label);
        overlay_preview_ = new OverlayPreview(overlay_container);
        overlay_layout->addWidget(overlay_preview_);
        preview_panel->addWidget(overlay_container);
        preview_panel->addStretch();
        content->addLayout(preview_panel, 2);

        main_layout->addLayout(content, 1);

        auto* bottom_bar = new QHBoxLayout();
        bottom_bar->setSpacing(12);
        
        auto* delete_btn = new QPushButton("Delete Theme", this);
        delete_btn->setProperty("flat", true);
        delete_btn->setStyleSheet("color: #ef4444;");
        delete_btn->setToolTip("Delete the currently selected custom theme");
        connect(delete_btn, &QPushButton::clicked, this, &ThemeEditor::on_delete);
        bottom_bar->addWidget(delete_btn);

        bottom_bar->addStretch();

        auto* apply_btn = new QPushButton("Apply", this);
        apply_btn->setProperty("flat", true);
        apply_btn->setToolTip("Apply theme to see changes without saving");
        connect(apply_btn, &QPushButton::clicked, this, &ThemeEditor::on_apply);
        bottom_bar->addWidget(apply_btn);

        auto* cancel_btn = new QPushButton("Cancel", this);
        cancel_btn->setProperty("flat", true);
        cancel_btn->setToolTip("Discard changes and close");
        connect(cancel_btn, &QPushButton::clicked, this, &ThemeEditor::on_cancel);
        bottom_bar->addWidget(cancel_btn);

        auto* save_btn = new QPushButton("Save && Close", this);
        save_btn->setToolTip("Save theme and set as default for next startup");
        connect(save_btn, &QPushButton::clicked, this, &ThemeEditor::on_save_and_close);
        bottom_bar->addWidget(save_btn);

        main_layout->addLayout(bottom_bar);
    }

    QWidget* create_themes_tab() {
        auto* widget = new QWidget(this);
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(16);

        auto* info = new QLabel("Select a theme to edit or use as a starting point.\n"
            "Built-in themes cannot be modified, but you can save them with a new name.\n"
            "Note: Some changes may require a restart to fully take effect.", widget);
        info->setWordWrap(true);
        info->setStyleSheet("color: #94a3b8; font-size: 12px;");
        layout->addWidget(info);

        theme_list_ = new QListWidget(widget);
        theme_list_->setMinimumHeight(200);
        connect(theme_list_, &QListWidget::itemClicked, this, &ThemeEditor::on_theme_list_clicked);
        connect(theme_list_, &QListWidget::currentItemChanged, this, &ThemeEditor::on_theme_selection_changed);
        layout->addWidget(theme_list_, 1);

        auto* btn_row = new QHBoxLayout();
        btn_row->setSpacing(12);
        
        auto* import_btn = new QPushButton("Import...", widget);
        import_btn->setProperty("flat", true);
        import_btn->setToolTip("Import a theme from a .json file");
        connect(import_btn, &QPushButton::clicked, this, &ThemeEditor::on_import);
        btn_row->addWidget(import_btn);

        auto* export_btn = new QPushButton("Export...", widget);
        export_btn->setProperty("flat", true);
        export_btn->setToolTip("Export current theme to share with others");
        connect(export_btn, &QPushButton::clicked, this, &ThemeEditor::on_export);
        btn_row->addWidget(export_btn);

        auto* save_as_btn = new QPushButton("Save As...", widget);
        save_as_btn->setToolTip("Save current theme with a new name");
        connect(save_as_btn, &QPushButton::clicked, this, &ThemeEditor::on_save_as);
        btn_row->addWidget(save_as_btn);

        hide_btn_ = new QPushButton("👁 Hide", widget);
        hide_btn_->setProperty("flat", true);
        hide_btn_->setToolTip("Hide the selected theme from the list");
        connect(hide_btn_, &QPushButton::clicked, this, &ThemeEditor::on_hide_theme);
        btn_row->addWidget(hide_btn_);

        btn_row->addStretch();
        layout->addLayout(btn_row);

        return widget;
    }

    QWidget* create_colors_tab() {
        auto* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        
        auto* widget = new QWidget(scroll);
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(20);
        layout->setContentsMargins(4, 4, 4, 4);

        auto* essential = new QGroupBox("Main Colors", widget);
        essential->setToolTip("These affect the overall look of the app");
        auto* eg = new QGridLayout(essential);
        eg->setSpacing(12);
        eg->setContentsMargins(16, 20, 16, 16);

        int row = 0;
        auto add_color = [&](const QString& label, const QString& tip, ColorButton*& btn, int col = 0) {
            auto* lbl = new QLabel(label, essential);
            lbl->setToolTip(tip);
            eg->addWidget(lbl, row, col * 2);
            btn = new ColorButton(label, essential);
            btn->setToolTip(tip);
            connect(btn, &ColorButton::color_changed, this, &ThemeEditor::on_value_changed);
            eg->addWidget(btn, row, col * 2 + 1);
            if (col == 1) row++;
        };

        add_color("Background", "Main window background", color_bg_, 0);
        add_color("Surface", "Cards and panels", color_surface_, 1);
        row++;
        add_color("Primary", "Buttons and accents", color_primary_, 0);
        add_color("Text", "Main text color", color_text_, 1);
        row++;
        add_color("Success", "Positive/high certainty", color_success_, 0);
        add_color("Warning", "Caution indicators", color_warning_, 1);
        row++;
        add_color("Error", "Error/negative", color_error_, 0);
        add_color("Border", "Lines and borders", color_border_, 1);
        row++;
        add_color("Card", "Inner panel backgrounds", color_card_, 0);
        add_color("Accent", "Secondary highlights", color_accent_, 1);
        row++;
        add_color("Secondary", "Secondary buttons", color_secondary_, 0);
        add_color("Muted Text", "Dimmed text", color_text_muted_, 1);

        layout->addWidget(essential);

        auto* appearance = new QGroupBox("Corner Style", widget);
        auto* ag = new QHBoxLayout(appearance);
        ag->setContentsMargins(16, 20, 16, 16);
        ag->setSpacing(16);
        
        ag->addWidget(new QLabel("Corner Radius:", appearance));
        radius_slider_ = new QSlider(Qt::Horizontal, appearance);
        radius_slider_->setRange(0, 12);
        radius_slider_->setValue(8);
        radius_slider_->setToolTip("0 = square corners, 12 = very rounded");
        connect(radius_slider_, &QSlider::valueChanged, this, &ThemeEditor::on_value_changed);
        ag->addWidget(radius_slider_, 1);
        radius_label_ = new QLabel("8px", appearance);
        radius_label_->setFixedWidth(40);
        connect(radius_slider_, &QSlider::valueChanged, [this](int v) {
            radius_label_->setText(QString::number(v) + "px");
        });
        ag->addWidget(radius_label_);

        layout->addWidget(appearance);
        layout->addStretch();
        
        scroll->setWidget(widget);
        return scroll;
    }

    QWidget* create_fonts_tab() {
        auto* widget = new QWidget(this);
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(20);

        auto* group = new QGroupBox("Typography Settings", widget);
        auto* form = new QFormLayout(group);
        form->setSpacing(16);
        form->setContentsMargins(16, 20, 16, 16);

        font_family_ = new QFontComboBox(group);
        font_family_->setToolTip("Main font for all text in the app");
        connect(font_family_, &QFontComboBox::currentFontChanged, this, &ThemeEditor::on_value_changed);
        form->addRow("Primary Font:", font_family_);

        font_mono_ = new QFontComboBox(group);
        font_mono_->setFontFilters(QFontComboBox::MonospacedFonts);
        font_mono_->setToolTip("Font for coordinates and numbers");
        connect(font_mono_, &QFontComboBox::currentFontChanged, this, &ThemeEditor::on_value_changed);
        form->addRow("Monospace Font:", font_mono_);

        font_size_ = new QSpinBox(group);
        font_size_->setRange(11, 18);
        font_size_->setToolTip("Base text size - headers will scale accordingly");
        connect(font_size_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeEditor::on_value_changed);
        form->addRow("Base Font Size:", font_size_);

        layout->addWidget(group);
        layout->addStretch();
        return widget;
    }

    QWidget* create_layout_tab() {
        auto* widget = new QWidget(this);
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(20);

        auto* info = new QLabel("Drag and drop items to change the order of widgets in the main window.\n"
            "Note: This only affects the visual order, not functionality.", widget);
        info->setWordWrap(true);
        info->setStyleSheet("color: #94a3b8; font-size: 12px;");
        layout->addWidget(info);

        auto* group = new QGroupBox("Widget Order (drag to reorder)", widget);
        auto* gl = new QVBoxLayout(group);
        gl->setContentsMargins(16, 20, 16, 16);

        layout_list_ = new DragListWidget(group);
        layout_list_->addItem("Boat Status Indicator");
        layout_list_->addItem("Boat Mode Toggle Button");
        layout_list_->addItem("Result Display (coordinates)");
        layout_list_->addItem("Throw History List");
        layout_list_->item(0)->setData(Qt::UserRole, "boat_status");
        layout_list_->item(1)->setData(Qt::UserRole, "boat_mode_btn");
        layout_list_->item(2)->setData(Qt::UserRole, "result_display");
        layout_list_->item(3)->setData(Qt::UserRole, "throw_list");
        connect(layout_list_, &DragListWidget::order_changed, this, &ThemeEditor::on_layout_changed);
        gl->addWidget(layout_list_);

        layout->addWidget(group);

        auto* options = new QGroupBox("Visibility Options", widget);
        auto* ol = new QVBoxLayout(options);
        ol->setContentsMargins(16, 20, 16, 16);
        ol->setSpacing(12);

        show_boat_status_ = new QCheckBox("Show Boat Status Indicator", options);
        show_boat_status_->setChecked(true);
        show_boat_status_->setToolTip("Toggle the boat connection status bar visibility");
        connect(show_boat_status_, &QCheckBox::toggled, this, &ThemeEditor::on_layout_changed);
        ol->addWidget(show_boat_status_);

        show_status_bar_ = new QCheckBox("Show Bottom Status Bar", options);
        show_status_bar_->setChecked(true);
        show_status_bar_->setToolTip("Toggle the version/status bar at the bottom");
        connect(show_status_bar_, &QCheckBox::toggled, this, &ThemeEditor::on_layout_changed);
        ol->addWidget(show_status_bar_);

        layout->addWidget(options);
        layout->addStretch();
        return widget;
    }

    QWidget* create_overlay_tab() {
        auto* widget = new QWidget(this);
        auto* layout = new QVBoxLayout(widget);
        layout->setSpacing(20);

        auto* info = new QLabel("The overlay floats on top of your game window.\n"
            "By default it uses the main theme colors, but you can customize it separately.", widget);
        info->setWordWrap(true);
        info->setStyleSheet("color: #94a3b8; font-size: 12px;");
        layout->addWidget(info);

        overlay_custom_ = new QCheckBox("Use custom overlay style", widget);
        overlay_custom_->setToolTip("Enable to set different colors/style for the overlay window");
        connect(overlay_custom_, &QCheckBox::toggled, this, [this](bool on) {
            overlay_settings_->setEnabled(on);
            on_value_changed();
        });
        layout->addWidget(overlay_custom_);

        overlay_settings_ = new QGroupBox("Overlay Settings", widget);
        overlay_settings_->setEnabled(false);
        auto* og = new QFormLayout(overlay_settings_);
        og->setSpacing(14);
        og->setContentsMargins(16, 20, 16, 16);

        overlay_bg_ = new ColorButton("Background", overlay_settings_);
        overlay_bg_->setToolTip("Overlay background color");
        connect(overlay_bg_, &ColorButton::color_changed, this, &ThemeEditor::on_value_changed);
        og->addRow("Background:", overlay_bg_);

        overlay_text_ = new ColorButton("Text", overlay_settings_);
        overlay_text_->setToolTip("Overlay text color");
        connect(overlay_text_, &ColorButton::color_changed, this, &ThemeEditor::on_value_changed);
        og->addRow("Text Color:", overlay_text_);

        overlay_opacity_ = new QSlider(Qt::Horizontal, overlay_settings_);
        overlay_opacity_->setRange(50, 100);
        overlay_opacity_->setValue(96);
        overlay_opacity_->setToolTip("How solid vs transparent the overlay appears");
        connect(overlay_opacity_, &QSlider::valueChanged, this, &ThemeEditor::on_value_changed);
        og->addRow("Opacity:", overlay_opacity_);

        overlay_radius_ = new QSpinBox(overlay_settings_);
        overlay_radius_->setRange(0, 16);
        overlay_radius_->setValue(12);
        overlay_radius_->setToolTip("Corner roundness of overlay window");
        connect(overlay_radius_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThemeEditor::on_value_changed);
        og->addRow("Corner Radius:", overlay_radius_);

        layout->addWidget(overlay_settings_);
        layout->addStretch();
        return widget;
    }

    void update_theme_list() {
        theme_list_->clear();
        auto themes = ThemeLibrary::instance().list_all_themes();
        
        auto* default_header = new QListWidgetItem("─── Default Themes ───", theme_list_);
        default_header->setFlags(Qt::NoItemFlags);
        default_header->setForeground(QColor("#64748b"));
        default_header->setTextAlignment(Qt::AlignCenter);
        auto f = default_header->font();
        f.setPointSize(9);
        default_header->setFont(f);
        
        bool added_custom_header = false;
        
        for (const auto& t : themes) {
            if (ThemeLibrary::instance().is_hidden(t.name)) continue;
            
            if (!t.is_builtin && !added_custom_header) {
                auto* custom_header = new QListWidgetItem("─── Custom Themes ───", theme_list_);
                custom_header->setFlags(Qt::NoItemFlags);
                custom_header->setForeground(QColor("#64748b"));
                custom_header->setTextAlignment(Qt::AlignCenter);
                custom_header->setFont(f);
                added_custom_header = true;
            }
            
            auto* item = new QListWidgetItem(t.name, theme_list_);
            if (t.is_builtin) {
                item->setForeground(QColor("#94a3b8"));
            }
            if (t.name == current_theme_.name) {
                item->setSelected(true);
                theme_list_->setCurrentItem(item);
            }
        }
        
        auto hidden_names = ThemeLibrary::instance().get_hidden_theme_names();
        if (!hidden_names.empty()) {
            auto* hidden_header = new QListWidgetItem(
                hidden_expanded_ ? "▼ Hidden Themes (" + QString::number(hidden_names.size()) + ")" 
                                : "▶ Hidden Themes (" + QString::number(hidden_names.size()) + ")", 
                theme_list_);
            hidden_header->setFlags(Qt::ItemIsEnabled);
            hidden_header->setForeground(QColor("#64748b"));
            hidden_header->setFont(f);
            hidden_header->setData(Qt::UserRole, "__hidden_header__");
            
            if (hidden_expanded_) {
                for (const auto& name : hidden_names) {
                    auto* item = new QListWidgetItem("   " + name, theme_list_);
                    item->setForeground(QColor("#64748b"));
                    item->setData(Qt::UserRole, "__hidden__");
                    item->setData(Qt::UserRole + 1, name);
                }
            }
        }
    }

    void on_hide_theme() {
        auto* item = theme_list_->currentItem();
        if (!item) return;
        QString name = item->text();
        if (name.startsWith("───") || name.startsWith("▶") || name.startsWith("▼")) return;
        if (item->data(Qt::UserRole).toString() == "__hidden__") {
            QString real_name = item->data(Qt::UserRole + 1).toString();
            ThemeLibrary::instance().unhide_theme(real_name);
        } else {
            ThemeLibrary::instance().hide_theme(name);
        }
        update_theme_list();
    }

    void on_theme_selection_changed(QListWidgetItem* current, QListWidgetItem*) {
        if (!current || !hide_btn_) return;
        if (current->data(Qt::UserRole).toString() == "__hidden__") {
            hide_btn_->setText("👁 Show");
            hide_btn_->setToolTip("Unhide the selected theme");
        } else {
            hide_btn_->setText("👁 Hide");
            hide_btn_->setToolTip("Hide the selected theme from the list");
        }
    }

    void load_theme_to_ui() {
        const auto& c = current_theme_.colors;
        color_bg_->set_color(c.background);
        color_surface_->set_color(c.surface);
        color_primary_->set_color(c.primary);
        color_text_->set_color(c.text_primary);
        color_success_->set_color(c.success);
        color_warning_->set_color(c.warning);
        color_error_->set_color(c.error);
        color_border_->set_color(c.border);
        color_card_->set_color(c.card);
        color_accent_->set_color(c.accent);
        color_secondary_->set_color(c.secondary);
        color_text_muted_->set_color(c.text_muted);

        radius_slider_->setValue(current_theme_.sizes.border_radius);
        radius_label_->setText(QString::number(current_theme_.sizes.border_radius) + "px");

        const auto& f = current_theme_.fonts;
        font_family_->setCurrentFont(QFont(f.primary_family));
        font_mono_->setCurrentFont(QFont(f.mono_family));
        font_size_->setValue(f.base_size);

        const auto& l = current_theme_.layout;
        layout_list_->clear();
        for (const auto& id : l.main_panel_order) {
            QString name;
            if (id == "boat_status") name = "Boat Status Indicator";
            else if (id == "boat_mode_btn") name = "Boat Mode Toggle Button";
            else if (id == "result_display") name = "Result Display (coordinates)";
            else if (id == "throw_list") name = "Throw History List";
            else continue;
            auto* item = new QListWidgetItem(name, layout_list_);
            item->setData(Qt::UserRole, QString::fromStdString(id));
        }
        show_boat_status_->setChecked(l.show_boat_status);
        show_status_bar_->setChecked(l.show_status_bar);

        const auto& o = current_theme_.overlay;
        overlay_custom_->setChecked(o.use_custom);
        overlay_settings_->setEnabled(o.use_custom);
        overlay_bg_->set_color(o.colors.background);
        overlay_text_->set_color(o.colors.text_primary);
        overlay_opacity_->setValue(o.opacity);
        overlay_radius_->setValue(o.border_radius);
    }

    void save_ui_to_theme() {
        auto& c = current_theme_.colors;
        c.background = color_bg_->color();
        c.surface = color_surface_->color();
        c.surface_hover = color_surface_->color().lighter(110);
        c.primary = color_primary_->color();
        c.primary_hover = color_primary_->color().lighter(115);
        c.text_primary = color_text_->color();
        c.text_secondary = color_text_->color().darker(120);
        c.text_muted = color_text_muted_->color();
        c.success = color_success_->color();
        c.warning = color_warning_->color();
        c.error = color_error_->color();
        c.border = color_border_->color();
        c.card = color_card_->color();
        c.accent = color_accent_->color();
        c.secondary = color_secondary_->color();

        current_theme_.sizes.border_radius = radius_slider_->value();

        auto& f = current_theme_.fonts;
        f.primary_family = font_family_->currentFont().family();
        f.mono_family = font_mono_->currentFont().family();
        f.base_size = font_size_->value();
        f.header_size = f.base_size + 3;
        f.small_size = std::max(10, f.base_size - 2);

        auto& l = current_theme_.layout;
        l.main_panel_order.clear();
        for (int i = 0; i < layout_list_->count(); ++i) {
            QString id = layout_list_->item(i)->data(Qt::UserRole).toString();
            l.main_panel_order.push_back(id.toStdString());
        }
        l.show_boat_status = show_boat_status_->isChecked();
        l.show_status_bar = show_status_bar_->isChecked();

        auto& o = current_theme_.overlay;
        o.use_custom = overlay_custom_->isChecked();
        o.colors.background = overlay_bg_->color();
        o.colors.text_primary = overlay_text_->color();
        o.opacity = overlay_opacity_->value();
        o.border_radius = overlay_radius_->value();
    }

    void update_previews() {
        main_preview_->update_preview(current_theme_);
        overlay_preview_->update_preview(current_theme_);
    }

    ThemeSettings original_theme_;
    ThemeSettings current_theme_;
    bool has_changes_ = false;
    bool loading_ = false;
    bool hidden_expanded_ = false;

    QTabWidget* tabs_;
    MainPreview* main_preview_;
    OverlayPreview* overlay_preview_;

    QListWidget* theme_list_;
    QPushButton* hide_btn_;

    ColorButton* color_bg_;
    ColorButton* color_surface_;
    ColorButton* color_primary_;
    ColorButton* color_text_;
    ColorButton* color_success_;
    ColorButton* color_warning_;
    ColorButton* color_error_;
    ColorButton* color_border_;
    ColorButton* color_card_;
    ColorButton* color_accent_;
    ColorButton* color_secondary_;
    ColorButton* color_text_muted_;
    QSlider* radius_slider_;
    QLabel* radius_label_;

    QFontComboBox* font_family_;
    QFontComboBox* font_mono_;
    QSpinBox* font_size_;

    DragListWidget* layout_list_;
    QCheckBox* show_boat_status_;
    QCheckBox* show_status_bar_;

    QCheckBox* overlay_custom_;
    QGroupBox* overlay_settings_;
    ColorButton* overlay_bg_;
    ColorButton* overlay_text_;
    QSlider* overlay_opacity_;
    QSpinBox* overlay_radius_;
};

} // namespace dolbot::ui
