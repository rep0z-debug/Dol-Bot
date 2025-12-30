#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <optional>
#include "dolbot/domain/fossil_divine.hpp"
#include "dolbot/ui/theme_engine.hpp"

namespace dolbot::ui {

class DivinePanel : public QWidget {
    Q_OBJECT
    
public:
    explicit DivinePanel(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_ui();
        apply_theme(ThemeEngine::instance().current_theme());
        theme_connection_ = ThemeEngine::instance().on_theme_changed(
            [this](const ThemeSettings& t) { apply_theme(t); }
        );
        hide();
    }
    
    void set_fossil(int segment, double x, double z) {
        current_fossil_ = domain::FossilLocation{x, z, segment};
        auto result = domain::FossilDivine::compute(*current_fossil_);
        
        if (result) {
            divine_result_ = *result;
            update_display();
            show();
        }
    }
    
    void clear() {
        current_fossil_.reset();
        hide();
    }
    
    bool has_divine() const { return current_fossil_.has_value(); }
    
    std::optional<domain::DivineResult> get_result() const {
        return divine_result_;
    }

signals:
    void divine_changed(const domain::DivineResult& result);

public slots:
    void apply_theme(const ThemeSettings& theme) {
        const auto& c = theme.colors;
        const auto& f = theme.fonts;
        int r = std::min(theme.sizes.border_radius, 8);
        
        setStyleSheet(QString(R"(
            DivinePanel {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                    stop:0 %1, stop:1 %2);
                border-radius: %3px;
                border: 1px solid %4;
            }
        )").arg(c.primary.darker(150).name()).arg(c.background.name()).arg(r).arg(c.primary.name()));
        
        if (icon_) {
            icon_->setStyleSheet("font-size: 20px; background: transparent;");
        }
        
        if (title_) {
            title_->setStyleSheet(QString("font-size: 14px; font-weight: bold; color: %1; font-family: '%2'; background: transparent;")
                .arg(c.accent.name()).arg(f.primary_family));
        }
        
        if (close_btn_) {
            close_btn_->setStyleSheet(QString(R"(
                QPushButton {
                    background: transparent;
                    color: %1;
                    border: none;
                    font-size: 16px;
                    font-weight: bold;
                }
                QPushButton:hover { color: %2; }
            )").arg(c.text_muted.name()).arg(c.error.name()));
        }
        
        if (dir_label_) {
            dir_label_->setStyleSheet(QString("color: %1; font-size: 10px; font-family: '%2'; background: transparent;")
                .arg(c.text_muted.name()).arg(f.primary_family));
        }
        
        if (direction_label_) {
            direction_label_->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold; font-family: '%2'; background: transparent;")
                .arg(c.text_primary.name()).arg(f.mono_family));
        }
        
        if (seg_label_) {
            seg_label_->setStyleSheet(QString("color: %1; font-size: 10px; font-family: '%2'; background: transparent;")
                .arg(c.text_muted.name()).arg(f.primary_family));
        }
        
        if (segment_label_) {
            segment_label_->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold; font-family: '%2'; background: transparent;")
                .arg(c.text_primary.name()).arg(f.mono_family));
        }
        
        if (ring_title_) {
            ring_title_->setStyleSheet(QString("color: %1; font-size: 10px; font-family: '%2'; background: transparent;")
                .arg(c.text_muted.name()).arg(f.primary_family));
        }
        
        if (ring_label_) {
            ring_label_->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold; font-family: '%2'; background: transparent;")
                .arg(c.text_primary.name()).arg(f.mono_family));
        }
        
        if (coords_label_) {
            coords_label_->setStyleSheet(QString("color: %1; font-size: 11px; font-family: '%2'; background: transparent;")
                .arg(c.accent.name()).arg(f.mono_family));
        }
    }

private:
    void setup_ui() {
        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(8);
        layout->setContentsMargins(12, 12, 12, 12);
        
        auto* header = new QHBoxLayout();
        
        icon_ = new QLabel("🦴");
        header->addWidget(icon_);
        
        title_ = new QLabel("Fossil Divine");
        header->addWidget(title_);
        
        header->addStretch();
        
        close_btn_ = new QPushButton("×");
        close_btn_->setFixedSize(24, 24);
        connect(close_btn_, &QPushButton::clicked, this, &DivinePanel::clear);
        header->addWidget(close_btn_);
        
        layout->addLayout(header);
        
        auto* info_layout = new QHBoxLayout();
        
        auto* dir_group = new QWidget();
        auto* dir_layout = new QVBoxLayout(dir_group);
        dir_layout->setContentsMargins(0, 0, 0, 0);
        
        dir_label_ = new QLabel("Direction");
        dir_layout->addWidget(dir_label_);
        
        direction_label_ = new QLabel("---");
        dir_layout->addWidget(direction_label_);
        
        info_layout->addWidget(dir_group);
        
        auto* seg_group = new QWidget();
        auto* seg_layout = new QVBoxLayout(seg_group);
        seg_layout->setContentsMargins(0, 0, 0, 0);
        
        seg_label_ = new QLabel("Segment");
        seg_layout->addWidget(seg_label_);
        
        segment_label_ = new QLabel("---");
        seg_layout->addWidget(segment_label_);
        
        info_layout->addWidget(seg_group);
        
        auto* ring_group = new QWidget();
        auto* ring_layout = new QVBoxLayout(ring_group);
        ring_layout->setContentsMargins(0, 0, 0, 0);
        
        ring_title_ = new QLabel("Est. Ring");
        ring_layout->addWidget(ring_title_);
        
        ring_label_ = new QLabel("---");
        ring_layout->addWidget(ring_label_);
        
        info_layout->addWidget(ring_group);
        
        layout->addLayout(info_layout);
        
        coords_label_ = new QLabel("");
        layout->addWidget(coords_label_);
    }
    
    void update_display() {
        if (!current_fossil_ || !divine_result_) return;
        
        direction_label_->setText(QString("%1°").arg(divine_result_->direction_angle, 0, 'f', 1));
        segment_label_->setText(QString::number(current_fossil_->segment + 1) + "/16");
        ring_label_->setText(QString::number(divine_result_->ring_index + 1));
        
        auto coords = domain::FossilDivine::get_divine_coords(*current_fossil_, 1500);
        coords_label_->setText(QString("Divine coords: %1, %2")
            .arg(static_cast<int>(coords.x))
            .arg(static_cast<int>(coords.z)));
        
        emit divine_changed(*divine_result_);
    }
    
    QLabel* icon_;
    QLabel* title_;
    QPushButton* close_btn_;
    QLabel* dir_label_;
    QLabel* direction_label_;
    QLabel* seg_label_;
    QLabel* segment_label_;
    QLabel* ring_title_;
    QLabel* ring_label_;
    QLabel* coords_label_;
    
    std::optional<domain::FossilLocation> current_fossil_;
    std::optional<domain::DivineResult> divine_result_;
    core::ConnectionHandle theme_connection_;
};

} // namespace dolbot::ui
