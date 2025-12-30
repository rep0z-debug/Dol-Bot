#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRegion>
#include <QScreen>
#include <QGuiApplication>
#include <QTimer>
#include "dolbot/domain/triangulator.hpp"
#include "dolbot/core/config.hpp"
#include "dolbot/ui/theme_engine.hpp"
#include "dolbot/ui/secure_label.hpp"
#include "dolbot/core/coords.hpp"

namespace dolbot::ui {

class OverlayWindow : public QWidget {
    Q_OBJECT

public:
    explicit OverlayWindow(QWidget* parent = nullptr)
        : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_NoSystemBackground, false);
        setMinimumSize(320, 140);
        setMaximumWidth(400);
        
        setup_ui();
        restore_position();
        apply_theme(ThemeEngine::instance().current_theme());
        theme_connection_ = ThemeEngine::instance().on_theme_changed(
            [this](const ThemeSettings& t) { apply_theme(t); }
        );
        hide();
    }
    
    ~OverlayWindow() override {
        save_position();
    }
    
    void set_enabled(bool enabled) {
        overlay_enabled_ = enabled;
        if (!enabled) {
            hide();
        } else if (has_result_) {
            show();
            raise();
        }
    }
    
    void update_result(const domain::TriangulationResult& result) {
        has_result_ = result.has_result();
        
        if (!overlay_enabled_) {
            hide();
            return;
        }
        
        const auto& c = get_overlay_colors();
        
        const auto* best = result.best();
        if (!has_result_ || !best) {
            coords_label_->setText("Waiting for throw...", "Waiting for throw...");
            certainty_label_->setText("—");
            certainty_label_->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: 600; background: transparent;")
                .arg(c.text_muted.name()));
            distance_label_->hide();
            nether_label_->hide();
            error_label_->hide();
            show();
            raise();
            return;
        }
        
        const auto& settings = core::Config::instance().settings();
        const auto& chunk = best->chunk;
        
        int display_x = chunk.stronghold_x(settings.mc_version);
        int display_z = chunk.stronghold_z(settings.mc_version);
        
        int real_display_x = display_x;
        int real_display_z = display_z;
        QString real_text;
        QString fake_text;

        int fake_display_x = display_x;
        int fake_display_z = display_z;
        if (settings.fake_coords_enabled) {
            auto [fx, fz] = core::FakeCoordGenerator::instance().apply(display_x, display_z);
            fake_display_x = fx;
            fake_display_z = fz;
        }

        switch (settings.display_mode) {
            case core::DisplayMode::EightEight:
                real_display_x = chunk.pos.x * 16 + 8;
                real_display_z = chunk.pos.z * 16 + 8;
                
                fake_display_x = real_display_x;
                fake_display_z = real_display_z;
                if (settings.fake_coords_enabled) {
                    auto [fx, fz] = core::FakeCoordGenerator::instance().apply(fake_display_x, fake_display_z);
                    fake_display_x = fx;
                    fake_display_z = fz;
                }
                
                real_text = QString("%1, %2").arg(real_display_x).arg(real_display_z);
                fake_text = QString("%1, %2").arg(fake_display_x).arg(fake_display_z);
                break;
                
            case core::DisplayMode::ChunkCoords:
                real_text = QString("Chunk (%1, %2)").arg(chunk.pos.x).arg(chunk.pos.z);
                fake_text = real_text; 
                break;
                
            default: 
                real_text = QString("%1, %2").arg(real_display_x).arg(real_display_z);
                fake_text = QString("%1, %2").arg(fake_display_x).arg(fake_display_z);
                break;
        }
        
        coords_label_->setPrivacyMode(settings.fake_coords_enabled);
        coords_label_->setText(real_text, fake_text);
        
        double cert = best->certainty * 100.0;
        certainty_label_->setText(QString("%1%").arg(cert, 0, 'f', 1));
        
        QColor color;
        if (cert >= 90) color = c.success;
        else if (cert >= 70) color = c.success.lighter(110);
        else if (cert >= 50) color = c.warning;
        else if (cert >= 30) color = QColor("#f97316");
        else color = c.error;
        
        certainty_label_->setStyleSheet(
            QString("color: %1; font-size: 16px; font-weight: 600; background: transparent;").arg(color.name())
        );
        
        if (best->distance.has_value()) {
            distance_label_->setText(QString("%1 blocks").arg(static_cast<int>(*best->distance)));
            distance_label_->show();
        } else {
            distance_label_->hide();
        }
        
        if (settings.show_nether_coords) {
            nether_label_->setPrivacyMode(settings.fake_coords_enabled);
            nether_label_->setText(
                QString("N: %1, %2").arg(real_display_x / 8).arg(real_display_z / 8),
                QString("N: %1, %2").arg(fake_display_x / 8).arg(fake_display_z / 8)
            );
            nether_label_->show();
        } else {
            nether_label_->hide();
        }
        
        if (settings.show_angle_errors && !best->angle_errors.empty()) {
            double max_err = best->max_error();
            error_label_->setText(QString("Error: %1°").arg(max_err, 0, 'f', 2));
            error_label_->setStyleSheet(
                QString("color: %1; font-size: 11px; background: transparent;")
                    .arg(max_err > 0.5 ? c.error.name() : c.text_muted.name()));
            error_label_->show();
        } else {
            error_label_->hide();
        }
        
        show();
        raise();
        repaint();
    }
    
    void set_boat_mode(bool enabled) {
        boat_indicator_->setVisible(enabled);
    }
    
    void clear() {
        has_result_ = false;
        const auto& c = get_overlay_colors();
        if (overlay_enabled_) {
            coords_label_->setText("Waiting for throw...", "Waiting for throw...");
            certainty_label_->setText("—");
            certainty_label_->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: 600; background: transparent;")
                .arg(c.text_muted.name()));
            distance_label_->hide();
            nether_label_->hide();
            error_label_->hide();
        } else {
            hide();
        }
    }

public slots:
    void apply_theme(const ThemeSettings& theme) {
        current_theme_ = theme;
        const auto& c = get_overlay_colors();
        const auto& f = theme.fonts;

        if (title_) {
            title_->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600; font-family: '%2'; background: transparent;")
                .arg(c.primary.name()).arg(f.primary_family));
        }

        if (boat_indicator_) {
            boat_indicator_->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: 700; font-family: '%2'; background: transparent;")
                .arg(c.accent.name()).arg(f.primary_family));
        }

        if (coords_label_) {
            coords_label_->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: 700; font-family: '%2'; background: %3;")
                .arg(c.text_primary.name()).arg(f.mono_family).arg(c.background.name()));
        }

        if (nether_label_) {
            nether_label_->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 600; font-family: '%2'; background: %3;")
                .arg(c.warning.name()).arg(f.mono_family).arg(c.background.name()));
        }

        if (distance_label_) {
            distance_label_->setStyleSheet(QString("color: %1; font-size: 12px; font-family: '%2'; background: transparent;")
                .arg(c.text_secondary.name()).arg(f.primary_family));
        }

        if (error_label_) {
            error_label_->setStyleSheet(QString("color: %1; font-size: 11px; font-family: '%2'; background: transparent;")
                .arg(c.text_muted.name()).arg(f.primary_family));
        }

        update();
        repaint();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        
        const auto& theme = current_theme_;
        const auto& c = get_overlay_colors();
        int opacity = theme.overlay.use_custom ? theme.overlay.opacity : 96;
        int r = theme.overlay.use_custom ? theme.overlay.border_radius : std::min(theme.sizes.border_radius, 12);
        
        QPainterPath path;
        path.addRoundedRect(rect().adjusted(0, 0, -1, -1), r, r);
        painter.setClipPath(path);
        
        QColor bgColor = c.background;
        bgColor.setAlpha(opacity * 255 / 100);
        
        painter.fillPath(path, bgColor);
        painter.setPen(QPen(c.border, 1));
        painter.drawPath(path);
    }
    
    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        int r = current_theme_.overlay.use_custom ? current_theme_.overlay.border_radius : 
                std::min(current_theme_.sizes.border_radius, 12);
        QRegion mask(rect());
        QPainterPath path;
        path.addRoundedRect(rect(), r, r);
        setMask(path.toFillPolygon().toPolygon());
    }
    
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            dragging_ = true;
            drag_start_ = event->globalPosition().toPoint();
            window_start_ = pos();
            event->accept();
        }
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        if (dragging_ && (event->buttons() & Qt::LeftButton)) {
            QPoint delta = event->globalPosition().toPoint() - drag_start_;
            QPoint new_pos = window_start_ + delta;
            move(new_pos);
            event->accept();
        }
    }
    
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && dragging_) {
            dragging_ = false;
            save_position();
            event->accept();
            show();
            raise();
            repaint();
        }
    }

private:
    const ThemeColors& get_overlay_colors() const {
        return current_theme_.overlay.use_custom ? current_theme_.overlay.colors : current_theme_.colors;
    }

    void setup_ui() {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 14, 16, 14);
        layout->setSpacing(8);
        
        auto* header = new QHBoxLayout();
        header->setSpacing(8);
        
        title_ = new QLabel("Dol Bot", this);
        header->addWidget(title_);
        
        boat_indicator_ = new QLabel("⛵ BOAT", this);
        boat_indicator_->hide();
        header->addWidget(boat_indicator_);
        
        header->addStretch();
        layout->addLayout(header);
        
        coords_label_ = new SecureLabel(this);
        coords_label_->setMinimumWidth(280);
        coords_label_->setMinimumHeight(32);
        coords_label_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        layout->addWidget(coords_label_);
        
        nether_label_ = new SecureLabel(this);
        nether_label_->setMinimumWidth(200);
        nether_label_->setMinimumHeight(20);
        layout->addWidget(nether_label_);
        
        auto* info_row = new QHBoxLayout();
        info_row->setSpacing(12);
        
        certainty_label_ = new QLabel("—", this);
        certainty_label_->setMinimumHeight(22);
        info_row->addWidget(certainty_label_);
        
        distance_label_ = new QLabel("", this);
        distance_label_->hide();
        info_row->addWidget(distance_label_);
        
        info_row->addStretch();
        layout->addLayout(info_row);
        
        error_label_ = new QLabel("", this);
        error_label_->hide();
        layout->addWidget(error_label_);
        
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    }
    
    void restore_position() {
        const auto& settings = core::Config::instance().settings();
        if (settings.overlay_x >= 0 && settings.overlay_y >= 0) {
            QPoint saved_pos(settings.overlay_x, settings.overlay_y);
            QScreen* screen = QGuiApplication::screenAt(saved_pos);
            if (screen) {
                move(saved_pos);
                return;
            }
        }
        if (QScreen* primary = QGuiApplication::primaryScreen()) {
            QRect screen_rect = primary->availableGeometry();
            move(screen_rect.right() - width() - 20, screen_rect.top() + 20);
        }
    }
    
    void save_position() {
        core::Config::instance().modify([this](core::AppSettings& s) {
            s.overlay_x = x();
            s.overlay_y = y();
        });
    }
    
    QLabel* title_;
    SecureLabel* coords_label_;
    SecureLabel* nether_label_;
    QLabel* certainty_label_;
    QLabel* distance_label_;
    QLabel* boat_indicator_;
    QLabel* error_label_;
    QPoint drag_start_;
    QPoint window_start_;
    bool dragging_ = false;
    bool overlay_enabled_ = false;
    bool has_result_ = false;
    ThemeSettings current_theme_;
    core::ConnectionHandle theme_connection_;
};

}
