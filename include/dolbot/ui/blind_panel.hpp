#pragma once

#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include "dolbot/domain/blind_evaluator.hpp"
#include "dolbot/ui/theme_engine.hpp"

namespace dolbot::ui {

class BlindPanel : public QFrame {
    Q_OBJECT

public:
    explicit BlindPanel(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setup_ui();
        apply_theme(ThemeEngine::instance().current_theme());
        theme_connection_ = ThemeEngine::instance().on_theme_changed(
            [this](const ThemeSettings& t) { apply_theme(t); }
        );
    }
    
    void update_result(const domain::BlindResult& result) {
        last_result_ = result;
        coords_label_->setText(QString("Nether: (%1, %2)")
            .arg(static_cast<int>(result.x))
            .arg(static_cast<int>(result.z)));
        
        double prob_pct = result.highroll_probability * 100.0;
        probability_label_->setText(QString("%1%").arg(prob_pct, 0, 'f', 1));
        
        update_probability_style(prob_pct);
        rating_label_->setText(QString::fromStdString(result.rating()));
        
        distance_label_->setText(QString("Avg distance: %1 blocks")
            .arg(static_cast<int>(result.avg_distance)));
        
        if (result.improvement_distance > 50) {
            improvement_label_->setText(QString("Better spot %1° away (%2 blocks)")
                .arg(result.improvement_direction, 0, 'f', 0)
                .arg(static_cast<int>(result.improvement_distance)));
            improvement_label_->show();
        } else {
            improvement_label_->hide();
        }
        
        setVisible(true);
    }
    
    void clear() {
        coords_label_->setText("—");
        probability_label_->setText("—");
        rating_label_->setText("");
        distance_label_->setText("");
        improvement_label_->hide();
    }

public slots:
    void apply_theme(const ThemeSettings& theme) {
        const auto& c = theme.colors;
        const auto& f = theme.fonts;
        int r = std::min(theme.sizes.border_radius, 8);
        
        setStyleSheet(QString("BlindPanel { background: %1; border: 1px solid %2; border-radius: %3px; }")
            .arg(c.surface.name()).arg(c.border.name()).arg(r));
        
        if (header_) {
            header_->setStyleSheet(QString("font-size: 12px; font-weight: 600; color: %1; font-family: '%2'; background: transparent;")
                .arg(c.warning.name()).arg(f.primary_family));
        }
        
        if (coords_label_) {
            coords_label_->setStyleSheet(QString("font-size: 14px; color: %1; font-family: '%2'; background: transparent;")
                .arg(c.text_primary.name()).arg(f.mono_family));
        }
        
        if (prob_title_) {
            prob_title_->setStyleSheet(QString("font-size: 13px; color: %1; font-family: '%2'; background: transparent;")
                .arg(c.text_secondary.name()).arg(f.primary_family));
        }
        
        if (rating_label_) {
            rating_label_->setStyleSheet(QString("font-size: 11px; color: %1; font-family: '%2'; background: transparent;")
                .arg(c.text_muted.name()).arg(f.primary_family));
        }
        
        if (distance_label_) {
            distance_label_->setStyleSheet(QString("font-size: 12px; color: %1; font-family: '%2'; background: transparent;")
                .arg(c.text_muted.name()).arg(f.primary_family));
        }
        
        if (improvement_label_) {
            improvement_label_->setStyleSheet(QString("font-size: 12px; color: %1; font-family: '%2'; background: transparent;")
                .arg(c.accent.name()).arg(f.primary_family));
        }
        
        if (last_result_.highroll_probability > 0) {
            update_probability_style(last_result_.highroll_probability * 100.0);
        }
    }

private:
    void update_probability_style(double prob_pct) {
        const auto& c = ThemeEngine::instance().colors();
        const auto& f = ThemeEngine::instance().fonts();
        
        QColor color;
        if (prob_pct >= 20) color = c.success;
        else if (prob_pct >= 10) color = c.success.lighter(120);
        else if (prob_pct >= 5) color = c.warning;
        else color = c.error;
        
        probability_label_->setStyleSheet(
            QString("font-size: 24px; font-weight: 700; color: %1; font-family: '%2'; background: transparent;")
                .arg(color.name()).arg(f.mono_family)
        );
    }

    void setup_ui() {
        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(8);
        layout->setContentsMargins(12, 12, 12, 12);
        
        header_ = new QLabel("Blind Evaluation", this);
        layout->addWidget(header_);
        
        coords_label_ = new QLabel("—", this);
        layout->addWidget(coords_label_);
        
        auto* prob_row = new QHBoxLayout();
        
        prob_title_ = new QLabel("Highroll:", this);
        prob_row->addWidget(prob_title_);
        
        probability_label_ = new QLabel("—", this);
        prob_row->addWidget(probability_label_);
        
        rating_label_ = new QLabel("", this);
        prob_row->addWidget(rating_label_);
        
        prob_row->addStretch();
        layout->addLayout(prob_row);
        
        distance_label_ = new QLabel("", this);
        layout->addWidget(distance_label_);
        
        improvement_label_ = new QLabel("", this);
        improvement_label_->hide();
        layout->addWidget(improvement_label_);
    }
    
    QLabel* header_;
    QLabel* coords_label_;
    QLabel* probability_label_;
    QLabel* prob_title_;
    QLabel* rating_label_;
    QLabel* distance_label_;
    QLabel* improvement_label_;
    domain::BlindResult last_result_;
    core::ConnectionHandle theme_connection_;
};

} // namespace dolbot::ui
