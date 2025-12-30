#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPainter>
#include <QScrollArea>
#include <cmath>
#include "dolbot/ui/secure_label.hpp"
#include "dolbot/domain/triangulator.hpp"
#include "dolbot/ui/theme_engine.hpp"
#include "dolbot/core/config.hpp"

namespace dolbot::ui {


class ModeIndicatorBar : public QWidget {
    Q_OBJECT
    
public:
    explicit ModeIndicatorBar(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);
        
        hidden_badge_ = new QLabel("🔒 Hidden", this);
        hidden_badge_->hide();
        layout->addWidget(hidden_badge_);
        
        locked_badge_ = new QLabel("🔐 Locked", this);
        locked_badge_->hide();
        layout->addWidget(locked_badge_);
        
        fossil_badge_ = new QLabel("🦴 Fossil", this);
        fossil_badge_->hide();
        layout->addWidget(fossil_badge_);
        
        fake_badge_ = new QLabel("Fake", this);
        fake_badge_->setToolTip("Displaying fake coordinates");
        fake_badge_->hide();
        layout->addWidget(fake_badge_);
        
        apply_theme(ThemeEngine::instance().current_theme());
        theme_connection_ = ThemeEngine::instance().on_theme_changed(
            [this](const ThemeSettings& t) { apply_theme(t); }
        );
    }
    
    void set_hidden(bool is_hidden) {
        hidden_badge_->setVisible(is_hidden);
    }
    
    void set_locked(bool is_locked) {
        locked_badge_->setVisible(is_locked);
    }
    
    void set_fossil(bool is_fossil) {
        fossil_badge_->setVisible(is_fossil);
    }
    
    void set_fake(bool is_fake) {
        fake_badge_->setVisible(is_fake);
    }

private:
    void apply_theme(const ThemeSettings& theme) {
        const auto& c = theme.colors;
        const auto& f = theme.fonts;
        int r = std::min(theme.sizes.border_radius, 3);
        
        QString base_style = QString(
            "background: %1; "
            "color: %2; "
            "font-size: 10px; "
            "font-weight: 600; "
            "font-family: '%3'; "
            "padding: 2px 6px; "
            "border-radius: %4px;"
        ).arg(c.surface_hover.name()).arg(c.text_primary.name()).arg(f.primary_family).arg(r);
        
        hidden_badge_->setStyleSheet(base_style);
        
        locked_badge_->setStyleSheet(QString(
            "background: %1; "
            "color: %2; "
            "font-size: 10px; "
            "font-weight: 600; "
            "font-family: '%3'; "
            "padding: 2px 6px; "
            "border-radius: %4px;"
        ).arg(c.surface_hover.name()).arg(c.warning.name()).arg(f.primary_family).arg(r));
        
        fossil_badge_->setStyleSheet(QString(
            "background: %1; "
            "color: %2; "
            "font-size: 10px; "
            "font-weight: 600; "
            "font-family: '%3'; "
            "padding: 2px 6px; "
            "border-radius: %4px;"
        ).arg(c.surface_hover.name()).arg(c.accent.name()).arg(f.primary_family).arg(r));
        
        fake_badge_->setStyleSheet(QString(
            "background: %1; "
            "color: %2; "
            "font-size: 10px; "
            "font-weight: 600; "
            "font-family: '%3'; "
            "padding: 2px 6px; "
            "border-radius: %4px;"
        ).arg(c.surface_hover.name()).arg(c.secondary.name()).arg(f.primary_family).arg(r));
    }

    QLabel* hidden_badge_;
    QLabel* locked_badge_;
    QLabel* fossil_badge_;
    QLabel* fake_badge_;
    core::ConnectionHandle theme_connection_;
};

class CertaintyBar : public QWidget {
    Q_OBJECT

public:
    explicit CertaintyBar(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedHeight(6);
        apply_theme(ThemeEngine::instance().current_theme());
        theme_connection_ = ThemeEngine::instance().on_theme_changed(
            [this](const ThemeSettings& t) { apply_theme(t); }
        );
    }
    
    void set_value(double certainty) {
        certainty_ = std::clamp(certainty, 0.0, 100.0);
        update();
    }

public slots:
    void apply_theme(const ThemeSettings& theme) {
        theme_ = theme;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        const auto& c = theme_.colors;
        
        p.setBrush(c.surface);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect(), 4, 4);
        
        if (certainty_ > 0) {
            int fill_width = static_cast<int>((certainty_ / 100.0) * width());
            fill_width = std::max(fill_width, 4);
            
            QColor color;
            if (certainty_ >= 90) color = c.success;
            else if (certainty_ >= 70) color = c.success.lighter(110);
            else if (certainty_ >= 50) color = c.warning;
            else if (certainty_ >= 30) color = QColor("#f97316");
            else color = c.error;
            
            p.setBrush(color);
            p.drawRoundedRect(0, 0, fill_width, height(), 4, 4);
        }
    }

private:
    double certainty_ = 0.0;
    ThemeSettings theme_;
    core::ConnectionHandle theme_connection_;
};

class CompactPredictionCard : public QWidget {
    Q_OBJECT
    
public:
    explicit CompactPredictionCard(int rank, QWidget* parent = nullptr)
        : QWidget(parent)
        , rank_(rank)
    {
        setup_ui();
        apply_theme(ThemeEngine::instance().current_theme());
        theme_connection_ = ThemeEngine::instance().on_theme_changed(
            [this](const ThemeSettings& t) { apply_theme(t); }
        );
    }
    
    void set_prediction(const domain::PredictionResult* pred, bool is_primary) {
        if (!pred) {
            hide();
            return;
        }
        
        show();
        is_primary_ = is_primary;
        
        const auto& settings = core::Config::instance().settings();
        const auto& c = pred->chunk;
        double certainty = pred->certainty * 100.0;
        
        int display_x = c.stronghold_x(settings.mc_version);
        int display_z = c.stronghold_z(settings.mc_version);
        
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
                real_display_x = c.pos.x * 16 + 8;
                real_display_z = c.pos.z * 16 + 8;
                
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
                real_text = QString("Chunk (%1, %2)").arg(c.pos.x).arg(c.pos.z);
                fake_text = real_text; 
                break;
                
            default: 
                real_text = QString("%1, %2").arg(real_display_x).arg(real_display_z);
                fake_text = QString("%1, %2").arg(fake_display_x).arg(fake_display_z);
                break;
        }
        
        coords_label_->setPrivacyMode(settings.fake_coords_enabled);
        coords_label_->setText(real_text, fake_text);
        certainty_label_->setText(QString("%1%").arg(certainty, 0, 'f', 1));
        certainty_bar_->set_value(certainty);
        
        if (settings.show_nether_coords) {
            QString real_nether = QString("N: %1, %2").arg(real_display_x / 8).arg(real_display_z / 8);
            QString fake_nether = QString("N: %1, %2").arg(fake_display_x / 8).arg(fake_display_z / 8);
            
            nether_label_->setPrivacyMode(settings.fake_coords_enabled);
            nether_label_->setText(real_nether, fake_nether);
            nether_label_->show();
        } else {
            nether_label_->hide();
        }

        if (pred->distance.has_value()) {
            dist_label_->setText(QString("%1 blocks").arg(static_cast<int>(*pred->distance)));
            dist_label_->show();
        } else {
            dist_label_->hide();
        }
        
        if (settings.portal_linking_warning && pred->last_throw_pos.has_value() && pred->certainty > 0.6) {
            int sh_x = c.stronghold_x(settings.mc_version);
            int sh_z = c.stronghold_z(settings.mc_version);
            int sh_nether_x = sh_x / 8;
            int sh_nether_z = sh_z / 8;
            double player_ow_x = pred->last_throw_pos->x;
            double player_ow_z = pred->last_throw_pos->z;
            int player_nether_x = static_cast<int>(std::floor(player_ow_x / 8.0));
            int player_nether_z = static_cast<int>(std::floor(player_ow_z / 8.0));
            int dx = std::abs(sh_nether_x - player_nether_x);
            int dz = std::abs(sh_nether_z - player_nether_z);
            bool would_link = (dx <= 128 && dz <= 128);
            if (!would_link) {
                int nether_dist = static_cast<int>(std::sqrt(dx*dx + dz*dz));
                portal_warning_->setText(QString("⚠ No link (%1 blocks in nether)").arg(nether_dist));
                portal_warning_->show();
            } else {
                portal_warning_->hide();
            }
        } else {
            portal_warning_->hide();
        }

        if (settings.mismeasure_warning_enabled && pred->has_warning()) {
            QString warning_text;
            if (pred->likely_mismeasure && pred->poor_triangulation) {
                warning_text = "⚠ Input inconsistency detected";
            } else if (pred->likely_mismeasure) {
                warning_text = "⚠ Likely mismeasure";
            } else {
                warning_text = "⚠ Poor triangulation";
            }
            mismeasure_warning_->setText(warning_text);
            mismeasure_warning_->setToolTip(QString::fromStdString(pred->warning_message));
            mismeasure_warning_->show();
        } else {
            mismeasure_warning_->hide();
        }

        update_style(certainty);
    }

public slots:
    void apply_theme(const ThemeSettings& theme) {
        const auto& c = theme.colors;
        const auto& f = theme.fonts;
        int r = std::min(theme.sizes.border_radius, 8);
        
        rank_label_->setStyleSheet(QString("font-weight: 700; color: %1; font-family: '%2'; background: transparent;")
            .arg(rank_ == 1 ? c.primary.name() : c.text_muted.name())
            .arg(f.primary_family));
        
        dist_label_->setStyleSheet(QString("font-size: %1px; color: %2; font-family: '%3'; background: transparent;")
            .arg(f.small_size).arg(c.text_muted.name()).arg(f.primary_family));
        
        nether_label_->setStyleSheet(QString("font-size: %1px; color: %2; font-family: '%3'; background: %4;")
            .arg(f.small_size).arg(c.warning.name()).arg(f.primary_family).arg(c.surface.name()));
        
        portal_warning_->setStyleSheet(QString("font-size: %1px; color: %2; font-weight: 700; font-family: '%3'; background: transparent;")
            .arg(f.small_size - 1).arg(c.error.name()).arg(f.primary_family));
        
        mismeasure_warning_->setStyleSheet(QString("font-size: %1px; color: %2; font-weight: 700; background: %3; padding: 1px 4px; border-radius: %4px; font-family: '%5';")
            .arg(f.small_size - 1)
            .arg(c.warning.name())
            .arg(c.surface_hover.name())
            .arg(std::max(r - 4, 2))
            .arg(f.primary_family));
    }
    
private:
    void setup_ui() {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 8, 10, 8);
        layout->setSpacing(10);
        
        rank_label_ = new QLabel(QString("#%1").arg(rank_), this);
        rank_label_->setFixedWidth(24);
        layout->addWidget(rank_label_);
        
        auto* info = new QWidget(this);
        auto* info_layout = new QVBoxLayout(info);
        info_layout->setContentsMargins(0, 0, 0, 0);
        info_layout->setSpacing(2);
        
        coords_label_ = new SecureLabel(info);
        info_layout->addWidget(coords_label_);

        auto* sub_info_layout = new QHBoxLayout();
        sub_info_layout->setContentsMargins(0, 0, 0, 0);
        sub_info_layout->setSpacing(8);
        
        dist_label_ = new QLabel("", info);
        sub_info_layout->addWidget(dist_label_);
        
        nether_label_ = new SecureLabel(info);
        nether_label_->hide();
        sub_info_layout->addWidget(nether_label_);
        
        sub_info_layout->addStretch();
        info_layout->addLayout(sub_info_layout);
        
        certainty_bar_ = new CertaintyBar(info);
        info_layout->addWidget(certainty_bar_);
        
        layout->addWidget(info, 1);
        
        auto* right = new QWidget(this);
        auto* right_layout = new QVBoxLayout(right);
        right_layout->setContentsMargins(0, 0, 0, 0);
        right_layout->setSpacing(0);
        right_layout->setAlignment(Qt::AlignRight);
        
        certainty_label_ = new QLabel("—", right);
        certainty_label_->setAlignment(Qt::AlignRight);
        right_layout->addWidget(certainty_label_);
        
        portal_warning_ = new QLabel("⚠ Link", right);
        portal_warning_->setToolTip("Stronghold might not link from current Nether position");
        portal_warning_->hide();
        right_layout->addWidget(portal_warning_);
        
        mismeasure_warning_ = new QLabel("⚠ Warning", right);
        mismeasure_warning_->hide();
        right_layout->addWidget(mismeasure_warning_);
        
        layout->addWidget(right);
    }
    
    void update_style(double certainty) {
        const auto& c = ThemeEngine::instance().colors();
        const auto& f = ThemeEngine::instance().fonts();
        
        QColor color;
        if (certainty >= 90) color = c.success;
        else if (certainty >= 70) color = c.success.lighter(110);
        else if (certainty >= 50) color = c.warning;
        else if (certainty >= 30) color = QColor("#f97316");
        else color = c.error;
        
        certainty_label_->setStyleSheet(QString("font-size: %1px; font-weight: 700; color: %2; font-family: '%3'; background: transparent;")
            .arg(f.header_size).arg(color.name()).arg(f.primary_family));
        
        if (is_primary_) {
            coords_label_->setStyleSheet(QString("font-size: %1px; font-weight: 700; color: %2; font-family: '%3'; background: %4;")
                .arg(f.header_size + 2)
                .arg(certainty >= 50 ? c.text_primary.name() : c.text_muted.name())
                .arg(f.primary_family)
                .arg(c.surface.name())); 
        } else {
            coords_label_->setStyleSheet(QString("font-size: %1px; font-weight: 600; color: %2; font-family: '%3'; background: %4;")
                .arg(f.header_size)
                .arg(c.text_muted.name())
                .arg(f.primary_family)
                .arg(c.surface.name())); 
        }
    }
    
    int rank_;
    bool is_primary_ = true;
    QLabel* rank_label_;
    SecureLabel* coords_label_;
    QLabel* dist_label_;
    QLabel* certainty_label_;
    CertaintyBar* certainty_bar_;
    SecureLabel* nether_label_;
    QLabel* portal_warning_;
    QLabel* mismeasure_warning_;
    core::ConnectionHandle theme_connection_;
};

class ResultDisplay : public QWidget {
    Q_OBJECT

public:
    explicit ResultDisplay(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setup_ui();
        apply_theme(ThemeEngine::instance().current_theme());
        theme_connection_ = ThemeEngine::instance().on_theme_changed(
            [this](const ThemeSettings& t) { apply_theme(t); }
        );
    }
    
    void update_result(const domain::TriangulationResult& result) {
        const auto& settings = core::Config::instance().settings();
        int count = settings.prediction_count;
        
        if (!result.has_result()) {
            show_empty_state(result.locked, result.has_divine);
            return;
        }
        
        empty_state_->hide();
        cards_container_->show();
        
        for (int i = 0; i < 3; ++i) {
            if (i < count && i < static_cast<int>(result.predictions.size())) {
                cards_[i]->set_prediction(&result.predictions[i], i == 0);
            } else {
                cards_[i]->hide();
            }
        }
        
        mode_indicator_->set_locked(result.locked);
        mode_indicator_->set_fake(settings.fake_coords_enabled);
        
        update_combined_certainty(result);
        update_throw_advice(result);
    }
    
    void set_distance(std::optional<int>) {}
    void set_angle(std::optional<double>) {}

    void set_hidden(bool is_hidden) {
        mode_indicator_->set_hidden(is_hidden);
    }
    
    void set_fossil(bool is_fossil) {
        mode_indicator_->set_fossil(is_fossil);
    }

public slots:
    void apply_theme(const ThemeSettings& theme) {
        const auto& c = theme.colors;
        const auto& f = theme.fonts;
        int r = std::min(theme.sizes.border_radius, 8);


        title_label_->setStyleSheet(QString("font-size: %1px; font-weight: 600; color: %2; font-family: '%3'; background: transparent;")
            .arg(f.base_size).arg(c.text_secondary.name()).arg(f.primary_family));
        

        
        cards_container_->setStyleSheet(QString("background: %1; border-radius: %2px;")
            .arg(c.surface.name()).arg(r));
        
        combined_info_->setStyleSheet(QString("background: %1; border-radius: %2px; padding: 8px;")
            .arg(c.card.name()).arg(std::max(r - 2, 4)));
        
        combined_label_->setStyleSheet(QString("font-size: %1px; color: %2; font-family: '%3'; background: transparent;")
            .arg(f.small_size).arg(c.text_muted.name()).arg(f.primary_family));
        
        combined_value_->setStyleSheet(QString("font-size: %1px; font-weight: 600; color: %2; font-family: '%3'; background: transparent;")
            .arg(f.small_size).arg(c.accent.name()).arg(f.primary_family));
        
        advice_widget_->setStyleSheet(QString("background: %1; border-radius: %2px;")
            .arg(c.surface_hover.name()).arg(std::max(r - 2, 4)));
        
        advice_header_->setStyleSheet(QString("font-size: %1px; font-weight: 600; color: %2; font-family: '%3'; background: transparent;")
            .arg(f.small_size - 1).arg(c.accent.name()).arg(f.primary_family));
        
        advice_label_->setStyleSheet(QString("font-size: %1px; color: %2; font-family: '%3'; background: transparent;")
            .arg(f.base_size - 1).arg(c.text_primary.name()).arg(f.primary_family));
        
        
        empty_state_->setStyleSheet(QString("background: %1; border: 1px solid %2; border-radius: %3px;")
            .arg(c.surface.name())
            .arg(c.border.name())
            .arg(r));
        
        empty_title_->setStyleSheet(QString("font-size: %1px; font-weight: 600; color: %2; font-family: '%3'; background: transparent;")
            .arg(f.header_size).arg(c.text_muted.name()).arg(f.primary_family));
        
        empty_desc_->setStyleSheet(QString("font-size: %1px; color: %2; font-family: '%3'; background: transparent;")
            .arg(f.small_size).arg(c.text_muted.darker(120).name()).arg(f.primary_family));
        
        for (int i = 0; i < 3; ++i) {
            if (cards_[i]) cards_[i]->apply_theme(theme);
        }
    }

private:
    void setup_ui() {
        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(8);
        layout->setContentsMargins(0, 0, 0, 0);
        
        auto* header = new QHBoxLayout();
        header->setContentsMargins(2, 0, 2, 0);
        
        title_label_ = new QLabel("Stronghold", this);
        header->addWidget(title_label_);
        
        header->addStretch();
        
        mode_indicator_ = new ModeIndicatorBar(this);
        header->addWidget(mode_indicator_);
        
        layout->addLayout(header);
        
        cards_container_ = new QWidget(this);
        auto* cards_layout = new QVBoxLayout(cards_container_);
        cards_layout->setSpacing(1);
        cards_layout->setContentsMargins(0, 0, 0, 0);
        
        for (int i = 0; i < 3; ++i) {
            cards_[i] = new CompactPredictionCard(i + 1, cards_container_);
            cards_[i]->hide();
            cards_layout->addWidget(cards_[i]);
        }
        
        layout->addWidget(cards_container_);
        
        combined_info_ = new QWidget(this);
        auto* combined_layout = new QHBoxLayout(combined_info_);
        combined_layout->setContentsMargins(10, 6, 10, 6);
        
        combined_label_ = new QLabel("", combined_info_);
        combined_layout->addWidget(combined_label_);
        
        combined_layout->addStretch();
        
        combined_value_ = new QLabel("", combined_info_);
        combined_layout->addWidget(combined_value_);
        
        combined_info_->hide();
        layout->addWidget(combined_info_);
        
        advice_widget_ = new QWidget(this);
        auto* advice_layout = new QVBoxLayout(advice_widget_);
        advice_layout->setContentsMargins(10, 8, 10, 8);
        advice_layout->setSpacing(4);
        
        advice_header_ = new QLabel("💡 Throw Suggestion", advice_widget_);
        advice_layout->addWidget(advice_header_);
        
        advice_label_ = new QLabel("", advice_widget_);
        advice_label_->setWordWrap(true);
        advice_layout->addWidget(advice_label_);
        
        advice_widget_->hide();
        layout->addWidget(advice_widget_);
        
        empty_state_ = new QWidget(this);
        auto* empty_layout = new QVBoxLayout(empty_state_);
        empty_layout->setContentsMargins(16, 20, 16, 20);
        empty_layout->setSpacing(4);
        
        empty_title_ = new QLabel("No throws yet", empty_state_);
        empty_title_->setAlignment(Qt::AlignCenter);
        empty_layout->addWidget(empty_title_);
        
        empty_desc_ = new QLabel("Press F3+C while looking at an ender eye", empty_state_);
        empty_desc_->setAlignment(Qt::AlignCenter);
        empty_layout->addWidget(empty_desc_);
        
        layout->addWidget(empty_state_);
        
        show_empty_state(false, core::Config::instance().settings().enable_divine);
    }
    
    void show_empty_state(bool locked = false, bool has_divine = false) {
        for (int i = 0; i < 3; ++i) {
            cards_[i]->hide();
        }
        cards_container_->hide();
        empty_state_->show();
        mode_indicator_->set_locked(locked);
        mode_indicator_->set_fake(core::Config::instance().settings().fake_coords_enabled);
        mode_indicator_->set_fossil(has_divine);
        combined_info_->hide();
        advice_widget_->hide();
    }
    
    void update_combined_certainty(const domain::TriangulationResult& result) {
        if (result.predictions.size() < 2) {
            combined_info_->hide();
            return;
        }
        
        const auto& pred1 = result.predictions[0];
        const auto& pred2 = result.predictions[1];

        if (pred1.certainty > 0.95) {
            combined_info_->hide();
            return;
        }
        
        int dx = std::abs(pred1.chunk.pos.x - pred2.chunk.pos.x);
        int dz = std::abs(pred1.chunk.pos.z - pred2.chunk.pos.z);
        
        if (dx > 1 || dz > 1) {
            combined_info_->hide();
            return;
        }
        
        double combined = pred1.certainty + pred2.certainty;
        if (combined < 0.80) {
            combined_info_->hide();
            return;
        }
        
        int nether_x = pred1.chunk.pos.x + pred2.chunk.pos.x; 
        int nether_z = pred1.chunk.pos.z + pred2.chunk.pos.z; 
        
        combined_label_->setText(QString("Combined (%1, %2):").arg(nether_x).arg(nether_z));
        combined_value_->setText(QString("%1%").arg(combined * 100, 0, 'f', 1));
        combined_info_->show();
    }
    
    void update_throw_advice(const domain::TriangulationResult& result) {
        if (!core::Config::instance().settings().show_throw_suggestions) {
            advice_widget_->hide();
            return;
        }

        if (result.predictions.empty()) {
            advice_widget_->hide();
            return;
        }
        
        double certainty = result.predictions[0].certainty;
        
        if (certainty >= 0.85) {
            advice_widget_->hide();
            return;
        }
        
        if (certainty >= 0.70) {
            advice_label_->setText("Good confidence. One more throw to confirm?");
        } else if (certainty >= 0.30) {
            advice_label_->setText("Moderate confidence. Another throw from a different angle would help.");
        } else {
            advice_label_->setText("Low confidence. Throw again from a different position.");
        }
        
        advice_widget_->show();
    }
    
    QLabel* title_label_;
    QWidget* cards_container_;
    CompactPredictionCard* cards_[3];
    QWidget* empty_state_;
    QLabel* empty_title_;
    QLabel* empty_desc_;
    ModeIndicatorBar* mode_indicator_;
    QWidget* combined_info_;
    QLabel* combined_label_;
    QLabel* combined_value_;
    QWidget* advice_widget_;
    QLabel* advice_header_;
    QLabel* advice_label_;
    core::ConnectionHandle theme_connection_;
};

} // namespace dolbot::ui
