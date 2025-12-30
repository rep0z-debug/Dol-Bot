#pragma once

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include "dolbot/ui/theme_engine.hpp"

namespace dolbot::ui {

class StatusBar : public QFrame {
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setFixedHeight(28);
        setup_ui();
        apply_theme(ThemeEngine::instance().current_theme());
        theme_connection_ = ThemeEngine::instance().on_theme_changed(
            [this](const ThemeSettings& t) { apply_theme(t); }
        );
    }
    
    void set_status(const QString& text) {
        status_label_->setText(text);
    }
    
    
    void set_version(const QString& version) {
        version_label_->setText(version);
    }
    
    void set_throws(int count) {
        throws_label_->setText(QString("%1 throw%2").arg(count).arg(count == 1 ? "" : "s"));
    }

public slots:
    void apply_theme(const ThemeSettings& theme) {
        const auto& c = theme.colors;
        const auto& f = theme.fonts;

        setStyleSheet(QString(R"(
            StatusBar {
                background-color: %1;
                border-top: 1px solid %2;
            }
        )").arg(c.card.name()).arg(c.border.name()));

        if (status_label_) {
            status_label_->setStyleSheet(QString("color: %1; font-size: 11px; font-family: '%2'; background: transparent;")
                .arg(c.text_muted.name()).arg(f.primary_family));
        }

        if (throws_label_) {
            throws_label_->setStyleSheet(QString("color: %1; font-size: 11px; font-family: '%2'; background: transparent;")
                .arg(c.text_muted.darker(120).name()).arg(f.primary_family));
        }

        if (version_label_) {
            version_label_->setStyleSheet(QString("color: %1; font-size: 10px; font-family: '%2'; background: transparent;")
                .arg(c.text_muted.darker(140).name()).arg(f.primary_family));
        }
    }

private:
    void setup_ui() {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 0, 12, 0);
        layout->setSpacing(16);
        
        status_label_ = new QLabel("Ready", this);
        layout->addWidget(status_label_);
        
        layout->addStretch();
        
        throws_label_ = new QLabel("0 throws", this);
        layout->addWidget(throws_label_);
        
        version_label_ = new QLabel("v1.0.0", this);
        layout->addWidget(version_label_);
    }
    
    QLabel* status_label_;
    QLabel* version_label_;
    QLabel* throws_label_;
    core::ConnectionHandle theme_connection_;
};

} // namespace dolbot::ui
