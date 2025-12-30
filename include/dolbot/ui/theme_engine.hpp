#pragma once

#include <QWidget>
#include <QString>
#include <QColor>
#include <QPalette>
#include <QApplication>
#include <QStyle>
#include <QFile>
#include "dolbot/ui/theme_data.hpp"
#include "dolbot/core/signal.hpp"

namespace dolbot::ui {

class ThemeEngine {
public:
    static ThemeEngine& instance() {
        static ThemeEngine inst;
        return inst;
    }

    void apply_dark_theme() {
        current_ = ThemeSettings::dark_preset();
        apply_to_app();
        theme_changed_.fire(current_);
    }

    void apply_light_theme() {
        current_ = ThemeSettings::light_preset();
        apply_to_app();
        theme_changed_.fire(current_);
    }

    void apply_midnight_theme() {
        current_ = ThemeSettings::midnight_preset();
        apply_to_app();
        theme_changed_.fire(current_);
    }

    void apply_theme(const ThemeSettings& settings) {
        current_ = settings;
        apply_to_app();
        theme_changed_.fire(current_);
    }

    [[nodiscard]] const ThemeColors& colors() const { return current_.colors; }
    [[nodiscard]] const FontSettings& fonts() const { return current_.fonts; }
    [[nodiscard]] const SizeSettings& sizes() const { return current_.sizes; }
    [[nodiscard]] const LayoutSettings& layout() const { return current_.layout; }
    [[nodiscard]] const OverlayOverride& overlay() const { return current_.overlay; }
    [[nodiscard]] const ThemeSettings& current_theme() const { return current_; }

    [[nodiscard]] QString stylesheet() const {
        return generate_stylesheet(current_.colors, current_.fonts, current_.sizes);
    }

    core::ConnectionHandle on_theme_changed(std::function<void(const ThemeSettings&)> callback) {
        return theme_changed_.connect(std::move(callback));
    }

private:
    ThemeEngine() {
        current_ = ThemeSettings::dark_preset();
    }

    QString generate_stylesheet(const ThemeColors& t, const FontSettings& f, const SizeSettings& s) const {
        int r = std::clamp(s.border_radius, 0, 16);
        QString font = f.primary_family;
        font.replace("'", "");
        int fontSize = std::clamp(f.base_size, 10, 20);
        return QString(R"(
            * {
                font-family: '%16', 'Inter', -apple-system, sans-serif;
                font-size: %17px;
                background-color: transparent;
                color: %2;
            }
            
            QWidget {
                background-color: %1;
                color: %2;
            }
            
            QDialog {
                background-color: %1;
            }
            
            QMainWindow {
                background-color: %1;
            }
            
            QScrollArea {
                background-color: %1;
                border: none;
            }
            
            QScrollArea > QWidget > QWidget {
                background-color: %1;
            }
            
            QTabWidget::pane {
                border: 1px solid %4;
                border-radius: %18px;
                background-color: %1;
                padding: 8px;
            }
            
            QTabWidget > QWidget {
                background-color: %1;
            }
            
            QTabBar::tab {
                background-color: transparent;
                color: %8;
                padding: 10px 16px;
                margin-right: 4px;
                border-top-left-radius: %18px;
                border-top-right-radius: %18px;
            }
            
            QTabBar::tab:hover {
                background-color: %7;
                color: %2;
            }
            
            QTabBar::tab:selected {
                background-color: %1;
                color: %5;
                font-weight: 600;
                border: 1px solid %4;
                border-bottom: none;
            }
            
            QFrame {
                background-color: transparent;
                border: none;
            }

            QFrame[frameShape="1"], QFrame[frameShape="4"], QFrame[frameShape="5"] {
                background-color: %14;
                border: 1px solid %4;
                border-radius: %18px;
            }
            
            QMenuBar {
                background-color: %1;
                color: %2;
                border: none;
                border-bottom: 1px solid %4;
                padding: 4px 0;
            }

            QMenuBar::item {
                background-color: transparent;
                color: %2;
                padding: 8px 12px;
                border-radius: %18px;
            }

            QMenuBar::item:selected {
                background-color: %7;
            }

            QMenuBar::item:pressed {
                background-color: %5;
                color: white;
            }

            QStatusBar {
                background-color: %1;
                color: %8;
                border-top: 1px solid %4;
                padding: 4px 8px;
            }

            QStatusBar::item {
                border: none;
            }

            QStatusBar QLabel {
                color: %8;
                background-color: transparent;
            }
            
            QGroupBox {
                font-weight: 600;
                border: 1px solid %4;
                border-radius: %18px;
                margin-top: 12px;
                padding: 14px;
                padding-top: 24px;
                background-color: %3;
            }
            
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                left: 12px;
                padding: 4px 10px;
                background-color: %5;
                color: white;
                border-radius: %18px;
            }
            
            QPushButton {
                background-color: %5;
                color: white;
                border: none;
                border-radius: %18px;
                padding: 10px 18px;
                font-weight: 600;
            }
            
            QPushButton:hover {
                background-color: %6;
            }
            
            QPushButton:pressed {
                background-color: %5;
                padding-top: 12px;
                padding-bottom: 8px;
            }
            
            QPushButton:disabled {
                background-color: %7;
                color: %8;
            }
            
            QPushButton[flat="true"] {
                background-color: %3;
                border: 1px solid %4;
                color: %2;
            }
            
            QPushButton[flat="true"]:hover {
                background-color: %7;
            }
            
            QLabel {
                background-color: transparent;
                border: none;
                padding: 0;
            }
            
            QLineEdit, QSpinBox, QDoubleSpinBox, QKeySequenceEdit {
                background-color: %3;
                border: 1px solid %4;
                border-radius: %18px;
                padding: 8px 12px;
                color: %2;
                selection-background-color: %5;
            }
            
            QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QKeySequenceEdit:focus {
                border-color: %5;
                background-color: %7;
            }
            
            QSpinBox::up-button, QDoubleSpinBox::up-button,
            QSpinBox::down-button, QDoubleSpinBox::down-button {
                background-color: transparent;
                border: none;
                width: 20px;
            }
            
            QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
                border-left: 4px solid transparent;
                border-right: 4px solid transparent;
                border-bottom: 4px solid %8;
                width: 0; height: 0;
            }
            
            QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
                border-left: 4px solid transparent;
                border-right: 4px solid transparent;
                border-top: 4px solid %8;
                width: 0; height: 0;
            }
            
            QComboBox {
                background-color: %3;
                border: 1px solid %4;
                border-radius: %18px;
                padding: 8px 12px;
                padding-right: 28px;
                color: %2;
            }
            
            QComboBox:hover {
                border-color: %8;
            }
            
            QComboBox:focus {
                border-color: %5;
            }
            
            QComboBox::drop-down {
                border: none;
                width: 28px;
                background-color: transparent;
            }
            
            QComboBox::down-arrow {
                border-left: 4px solid transparent;
                border-right: 4px solid transparent;
                border-top: 5px solid %8;
                width: 0; height: 0;
                margin-right: 8px;
            }
            
            QComboBox QAbstractItemView {
                background-color: %3;
                border: 1px solid %4;
                border-radius: %18px;
                padding: 6px;
                selection-background-color: %5;
                outline: none;
            }
            
            QComboBox QAbstractItemView::item {
                padding: 8px 12px;
                border-radius: %18px;
                background-color: %3;
                color: %2;
            }
            
            QComboBox QAbstractItemView::item:hover {
                background-color: %7;
            }
            
            QComboBox QAbstractItemView::item:selected {
                background-color: %5;
                color: white;
            }
            
            QCheckBox {
                spacing: 10px;
                background-color: transparent;
            }
            
            QCheckBox::indicator {
                width: 20px;
                height: 20px;
                border: 2px solid %4;
                border-radius: %18px;
                background-color: %3;
            }
            
            QCheckBox::indicator:hover {
                border-color: %8;
            }
            
            QCheckBox::indicator:checked {
                background-color: %5;
                border-color: %5;
            }
            
            QScrollBar:vertical {
                background-color: transparent;
                width: 10px;
                margin: 0;
            }
            
            QScrollBar::handle:vertical {
                background-color: %4;
                border-radius: 5px;
                min-height: 40px;
                margin: 2px;
            }
            
            QScrollBar::handle:vertical:hover {
                background-color: %8;
            }
            
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
            QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
                background-color: transparent;
                height: 0;
            }
            
            QScrollBar:horizontal {
                background-color: transparent;
                height: 10px;
            }
            
            QScrollBar::handle:horizontal {
                background-color: %4;
                border-radius: 5px;
                min-width: 40px;
                margin: 2px;
            }
            
            QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
                width: 0;
            }
            
            QToolTip {
                background-color: %3;
                color: %2;
                border: 1px solid %4;
                padding: 8px 12px;
                border-radius: %18px;
            }
            
            QMessageBox {
                background-color: %1;
            }
            
            QMessageBox QLabel {
                background-color: transparent;
                color: %2;
            }
            
            QMessageBox QPushButton {
                min-width: 80px;
            }
            
            QMenu {
                background-color: %3;
                border: 1px solid %4;
                border-radius: %18px;
                padding: 8px;
            }
            
            QMenu::item {
                padding: 8px 24px;
                border-radius: %18px;
                background-color: transparent;
            }
            
            QMenu::item:selected {
                background-color: %5;
                color: white;
            }
            
            QMenu::separator {
                height: 1px;
                background-color: %4;
                margin: 6px 12px;
            }
            
            QListWidget, QListView, QTreeView, QTableView {
                background-color: %3;
                border: 1px solid %4;
                border-radius: %18px;
                outline: none;
            }
            
            QListWidget::item, QListView::item, QTreeView::item {
                padding: 8px;
                background-color: transparent;
            }
            
            QListWidget::item:hover, QListView::item:hover, QTreeView::item:hover {
                background-color: %7;
            }
            
            QListWidget::item:selected, QListView::item:selected, QTreeView::item:selected {
                background-color: %5;
                color: white;
            }
            
            QHeaderView::section {
                background-color: %3;
                color: %2;
                padding: 10px;
                border: none;
                border-bottom: 1px solid %4;
            }
            
            QFileDialog {
                background-color: %1;
            }
            
            QFileDialog QWidget {
                background-color: %1;
                color: %2;
            }
            
            QFileDialog QTreeView, QFileDialog QListView {
                background-color: %3;
            }
            
            QFontComboBox {
                background-color: %3;
                border: 1px solid %4;
                border-radius: %18px;
                padding: 8px 12px;
                color: %2;
            }
            
            QSlider::groove:horizontal {
                border: 1px solid %4;
                height: 6px;
                background: %3;
                border-radius: 3px;
            }
            
            QSlider::handle:horizontal {
                background: %5;
                border: none;
                width: 16px;
                margin: -5px 0;
                border-radius: 8px;
            }
            
            QSlider::handle:horizontal:hover {
                background: %6;
            }
            
            QColorDialog {
                background-color: %1;
            }
            
            QColorDialog QWidget {
                background-color: %1;
                color: %2;
            }
        )")
        .arg(t.background.name())
        .arg(t.text_primary.name())
        .arg(t.surface.name())
        .arg(t.border.name())
        .arg(t.primary.name())
        .arg(t.primary_hover.name())
        .arg(t.surface_hover.name())
        .arg(t.text_muted.name())
        .arg(t.success.name())
        .arg(t.warning.name())
        .arg(t.secondary.name())
        .arg(t.error.name())
        .arg(t.text_secondary.name())
        .arg(t.card.name())
        .arg(t.accent.name())
        .arg(font)
        .arg(fontSize)
        .arg(r);
    }

    void apply_to_app() {
        QString ss = stylesheet();
        qApp->setStyleSheet("");
        qApp->setStyleSheet(ss);
        for (auto* w : qApp->topLevelWidgets()) {
            polish_recursive(w);
        }
    }
    
    void polish_recursive(QWidget* widget) {
        if (!widget) return;
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();
        for (auto* child : widget->findChildren<QWidget*>()) {
            child->style()->unpolish(child);
            child->style()->polish(child);
            child->update();
        }
    }

    ThemeSettings current_;
    core::Signal<const ThemeSettings&> theme_changed_;
};

} // namespace dolbot::ui
