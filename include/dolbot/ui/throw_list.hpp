#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include "dolbot/domain/eye_throw.hpp"
#include "dolbot/domain/triangulator.hpp"
#include "dolbot/ui/theme_engine.hpp"

namespace dolbot::ui {

class ThrowListItem : public QFrame {
    Q_OBJECT

public:
    explicit ThrowListItem(const domain::EyeThrow& throw_data, int index, 
                          double angle_error = 0.0, QWidget* parent = nullptr)
        : QFrame(parent)
        , throw_data_(throw_data)
        , index_(index)
        , angle_error_(angle_error)
    {
        setMinimumHeight(52);
        setup_ui();
        apply_theme(ThemeEngine::instance().current_theme());
    }

signals:
    void remove_requested(int index);

public slots:
    void apply_theme(const ThemeSettings& theme) {
        const auto& c = theme.colors;
        const auto& f = theme.fonts;
        int r = std::min(theme.sizes.border_radius, 10);

        setObjectName("throwItem");
        setStyleSheet(QString(R"(
            #throwItem {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 %1, stop:1 %2);
                border: 1px solid %3;
                border-radius: %4px;
            }
            #throwItem:hover {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 %5, stop:1 %1);
                border-color: %6;
            }
        )").arg(c.surface.name())
           .arg(c.card.name())
           .arg(c.border.name())
           .arg(r)
           .arg(c.surface_hover.name())
           .arg(c.primary.name()));

        if (num_badge_) {
            num_badge_->setStyleSheet(QString(R"(
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1, 
                    stop:0 %1, stop:1 %2);
                color: white;
                font-weight: 700;
                font-size: 12px;
                font-family: '%3';
                border-radius: %4px;
            )").arg(c.primary.name()).arg(c.secondary.name()).arg(f.primary_family).arg(r));
        }

        if (coords_) {
            coords_->setStyleSheet(QString("color: %1; font-weight: 600; font-size: 13px; background: transparent; font-family: '%2';")
                .arg(c.text_primary.name()).arg(f.primary_family));
        }

        if (dim_) {
            dim_->setStyleSheet(QString("color: %1; font-size: 11px; background: transparent; font-family: '%2';")
                .arg(c.text_muted.name()).arg(f.primary_family));
        }

        if (angle_) {
            QString angle_color;
            if (throw_data_.is_boat_error()) {
                angle_color = c.error.name();
            } else if (throw_data_.is_boat_mode) {
                angle_color = c.success.name();
            } else {
                angle_color = c.text_secondary.name();
            }
            angle_->setStyleSheet(QString("color: %1; font-weight: 600; font-size: 14px; font-family: '%2'; background: transparent;")
                .arg(angle_color).arg(f.mono_family));
        }

        if (remove_btn_) {
            remove_btn_->setStyleSheet(QString(R"(
                QPushButton {
                    background: transparent;
                    border: 1px solid transparent;
                    border-radius: %4px;
                    font-size: 14px;
                    color: %1;
                }
                QPushButton:hover {
                    background: %2;
                    border-color: %3;
                    color: %3;
                }
            )").arg(c.text_muted.name())
               .arg(c.card.name())
               .arg(c.error.name())
               .arg(r));
        }
    }

private:
    void setup_ui() {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(14, 10, 14, 10);
        layout->setSpacing(12);
        
        num_badge_ = new QLabel(QString::number(index_ + 1), this);
        num_badge_->setFixedSize(26, 26);
        num_badge_->setAlignment(Qt::AlignCenter);
        layout->addWidget(num_badge_);
        
        QString type_icon;
        QString type_color;
        bool show_label = true;
        const auto& c = ThemeEngine::instance().colors();
        int r = std::min(ThemeEngine::instance().sizes().border_radius, 8);
        
        switch (throw_data_.effective_type()) {
            case domain::ThrowType::Boat: 
                type_icon = "B"; 
                type_color = c.success.name();
                break;
            case domain::ThrowType::Manual: 
                type_icon = "M"; 
                type_color = c.warning.name();
                break;
            case domain::ThrowType::Nether: 
                type_icon = "N"; 
                type_color = c.error.name();
                break;
            default: 
                show_label = false;
                break;
        }
        
        if (show_label) {
            type_label_ = new QLabel(type_icon, this);
            type_label_->setStyleSheet(QString("font-size: 11px; font-weight: 700; background: %1; color: white; padding: 2px 6px; border-radius: %2px;").arg(type_color).arg(std::min(r, 8)));
            type_label_->setToolTip(get_type_tooltip());
            layout->addWidget(type_label_);
        }
        
        auto* info_container = new QWidget(this);
        auto* info_layout = new QVBoxLayout(info_container);
        info_layout->setContentsMargins(0, 0, 0, 0);
        info_layout->setSpacing(2);
        
        auto pos = throw_data_.overworld_position();
        coords_ = new QLabel(
            QString("%1, %2")
                .arg(static_cast<int>(pos.x))
                .arg(static_cast<int>(pos.z)),
            info_container
        );
        info_layout->addWidget(coords_);
        
        QString dim_text;
        switch (throw_data_.dimension) {
            case domain::Dimension::Overworld: dim_text = "Overworld"; break;
            case domain::Dimension::Nether: dim_text = "Nether"; break;
            case domain::Dimension::End: dim_text = "The End"; break;
        }
        
        dim_ = new QLabel(dim_text, info_container);
        info_layout->addWidget(dim_);
        
        layout->addWidget(info_container, 1);
        
        auto* angle_container = new QWidget(this);
        auto* angle_layout = new QVBoxLayout(angle_container);
        angle_layout->setContentsMargins(0, 0, 0, 0);
        angle_layout->setSpacing(1);
        angle_layout->setAlignment(Qt::AlignRight);
        
        angle_ = new QLabel(
            QString("%1°").arg(throw_data_.corrected_angle(), 0, 'f', 2),
            angle_container
        );
        angle_->setAlignment(Qt::AlignRight);
        angle_layout->addWidget(angle_);
        
        if (throw_data_.correction != 0.0) {
            QString sign = throw_data_.correction > 0 ? "+" : "";
            corr_ = new QLabel(
                QString("%1%2°").arg(sign).arg(throw_data_.correction, 0, 'f', 2),
                angle_container
            );
            QString corr_color = throw_data_.correction > 0 ? c.success.name() : c.warning.name();
            corr_->setStyleSheet(QString("color: %1; font-size: 10px; background: transparent;").arg(corr_color));
            corr_->setAlignment(Qt::AlignRight);
            angle_layout->addWidget(corr_);
        } else if (std::abs(angle_error_) > 0.001) {
            error_label_ = new QLabel(
                QString("Δ %1°").arg(angle_error_, 0, 'f', 3),
                angle_container
            );
            QString error_color = get_error_color(angle_error_);
            error_label_->setStyleSheet(QString("color: %1; font-size: 10px; background: transparent;").arg(error_color));
            error_label_->setAlignment(Qt::AlignRight);
            error_label_->setToolTip("Deviation from predicted stronghold angle");
            angle_layout->addWidget(error_label_);
        }
        
        layout->addWidget(angle_container);
        
        remove_btn_ = new QPushButton("", this);
        remove_btn_->setFixedSize(28, 28);
        remove_btn_->setCursor(Qt::PointingHandCursor);
        remove_btn_->setToolTip("Remove this throw");
        remove_btn_->setText("✕");
        connect(remove_btn_, &QPushButton::clicked, this, [this]() {
            emit remove_requested(index_);
        });
        layout->addWidget(remove_btn_);
    }
    
    QString get_type_tooltip() const {
        switch (throw_data_.effective_type()) {
            case domain::ThrowType::Boat: return "Boat throw (10× precision)";
            case domain::ThrowType::Manual: return "Manual entry";
            case domain::ThrowType::Nether: return "Nether throw";
            default: return "Standard eye throw";
        }
    }
    
    QString get_error_color(double error) const {
        const auto& c = ThemeEngine::instance().colors();
        double abs_err = std::abs(error);
        if (abs_err < 0.01) return c.success.name();
        if (abs_err < 0.05) return c.success.lighter(120).name();
        if (abs_err < 0.1) return c.warning.name();
        if (abs_err < 0.2) return c.warning.darker(110).name();
        return c.error.name();
    }
    
    domain::EyeThrow throw_data_;
    int index_;
    double angle_error_;
    
    QLabel* num_badge_ = nullptr;
    QLabel* type_label_ = nullptr;
    QLabel* coords_ = nullptr;
    QLabel* dim_ = nullptr;
    QLabel* angle_ = nullptr;
    QLabel* corr_ = nullptr;
    QLabel* error_label_ = nullptr;
    QPushButton* remove_btn_ = nullptr;
};

class ThrowList : public QFrame {
    Q_OBJECT

public:
    explicit ThrowList(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setup_ui();
        apply_theme(ThemeEngine::instance().current_theme());
        theme_connection_ = ThemeEngine::instance().on_theme_changed(
            [this](const ThemeSettings& t) { apply_theme(t); }
        );
    }
    
    void update_throws(const std::vector<domain::EyeThrow>& throws, 
                       const std::vector<double>& errors = {}) {
        clear();
        
        for (size_t i = 0; i < throws.size(); ++i) {
            double error = (i < errors.size()) ? errors[i] : 0.0;
            auto* item = new ThrowListItem(throws[i], static_cast<int>(i), error, list_layout_->parentWidget());
            connect(item, &ThrowListItem::remove_requested, this, &ThrowList::on_remove_throw);
            list_layout_->insertWidget(list_layout_->count() - 1, item);
        }
        
        if (throws.empty()) {
            empty_state_->show();
        } else {
            empty_state_->hide();
        }
        
        count_label_->setText(QString("%1 throw%2")
            .arg(throws.size())
            .arg(throws.size() == 1 ? "" : "s"));
    }
    
    void clear() {
        QLayoutItem* child;
        while ((child = list_layout_->takeAt(0)) != nullptr) {
            if (child->widget() && child->widget() != empty_state_) {
                delete child->widget();
            }
            delete child;
        }
        if (empty_state_) {
            list_layout_->insertWidget(0, empty_state_);
        }
        list_layout_->addStretch();
    }

signals:
    void undo_requested();
    void redo_requested();
    void reset_requested();
    void remove_throw_requested(int index);

public slots:
    void apply_theme(const ThemeSettings& theme) {
        const auto& c = theme.colors;
        const auto& f = theme.fonts;
        int r = std::min(theme.sizes.border_radius, 10);

        setStyleSheet("background: transparent; border: none;");

        if (title_) {
            title_->setStyleSheet(QString("font-size: 14px; font-weight: 600; color: %1; background: transparent; font-family: '%2';")
                .arg(c.text_secondary.name()).arg(f.primary_family));
        }

        if (count_label_) {
            count_label_->setStyleSheet(QString("font-size: 12px; color: %1; background: %2; padding: 4px 10px; border-radius: %3px; font-family: '%4';")
                .arg(c.text_muted.name()).arg(c.surface.name()).arg(r).arg(f.primary_family));
        }

        if (empty_state_) {
            empty_state_->setStyleSheet(QString(R"(
                #emptyThrows {
                    background: %1;
                    border: 1px solid %2;
                    border-radius: %3px;
                }
            )").arg(c.surface.name()).arg(c.border.name()).arg(r));
        }

        if (empty_text_) {
            empty_text_->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 500; background: transparent; font-family: '%2';")
                .arg(c.text_muted.name()).arg(f.primary_family));
        }

        if (empty_hint_) {
            empty_hint_->setStyleSheet(QString("color: %1; font-size: 11px; background: transparent; font-family: '%2';")
                .arg(c.text_muted.darker(120).name()).arg(f.primary_family));
        }

        if (hotkey_hint_) {
            hotkey_hint_->setStyleSheet(QString("color: %1; font-size: 10px; background: transparent; font-family: '%2';")
                .arg(c.text_muted.darker(120).name()).arg(f.primary_family));
        }

        QString scrollStyle = QString(R"(
            QScrollArea { background: transparent; border: none; }
            QScrollBar:vertical { 
                width: 10px; 
                background: transparent;
                margin: 0;
            }
            QScrollBar::handle:vertical { 
                background: %1; 
                border-radius: 5px; 
                min-height: 40px;
                margin: 2px;
            }
            QScrollBar::handle:vertical:hover {
                background: %2;
            }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
            QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
        )").arg(c.border.name()).arg(c.primary.name());
        
        if (scroll_area_) {
            scroll_area_->setStyleSheet(scrollStyle);
        }

        update_button_styles(theme);
        
        if (list_layout_) {
            for (int i = 0; i < list_layout_->count(); ++i) {
                if (auto* item = qobject_cast<ThrowListItem*>(list_layout_->itemAt(i)->widget())) {
                    item->apply_theme(theme);
                }
            }
        }
    }

private slots:
    void on_remove_throw(int index) {
        emit remove_throw_requested(index);
    }

private:
    void update_button_styles(const ThemeSettings& theme) {
        const auto& c = theme.colors;
        const auto& f = theme.fonts;
        int r = std::min(theme.sizes.border_radius, 8);

        if (undo_btn_) {
            undo_btn_->setStyleSheet(QString(R"(
                QPushButton {
                    background: %1;
                    color: white;
                    border: none;
                    border-radius: %2px;
                    padding: 10px 16px;
                    font-weight: 600;
                    font-size: 12px;
                    font-family: '%3';
                }
                QPushButton:hover {
                    background: %4;
                }
            )").arg(c.surface_hover.name()).arg(r).arg(f.primary_family).arg(c.surface_hover.lighter(110).name()));
        }

        if (redo_btn_) {
            redo_btn_->setStyleSheet(QString(R"(
                QPushButton {
                    background: %1;
                    color: white;
                    border: none;
                    border-radius: %2px;
                    padding: 10px 16px;
                    font-weight: 600;
                    font-size: 12px;
                    font-family: '%3';
                }
                QPushButton:hover {
                    background: %4;
                }
            )").arg(c.surface_hover.name()).arg(r).arg(f.primary_family).arg(c.surface_hover.lighter(110).name()));
        }

        if (reset_btn_) {
            reset_btn_->setStyleSheet(QString(R"(
                QPushButton {
                    background: %1;
                    color: white;
                    border: none;
                    border-radius: %2px;
                    padding: 10px 16px;
                    font-weight: 600;
                    font-size: 12px;
                    font-family: '%3';
                }
                QPushButton:hover {
                    background: %4;
                }
            )").arg(c.error.darker(150).name()).arg(r).arg(f.primary_family).arg(c.error.darker(120).name()));
        }
    }

    void setup_ui() {
        setMinimumHeight(180);
        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(0, 0, 0, 0);
        
        auto* header = new QHBoxLayout();
        header->setContentsMargins(4, 0, 4, 0);
        
        title_ = new QLabel("Eye Throws", this);
        header->addWidget(title_);
        
        header->addStretch();
        
        count_label_ = new QLabel("0 throws", this);
        header->addWidget(count_label_);
        
        layout->addLayout(header);
        
        scroll_area_ = new QScrollArea(this);
        scroll_area_->setMinimumHeight(100);
        scroll_area_->setWidgetResizable(true);
        scroll_area_->setFrameShape(QFrame::NoFrame);
        scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        
        auto* scroll_content = new QWidget();
        scroll_content->setStyleSheet("background: transparent;");
        list_layout_ = new QVBoxLayout(scroll_content);
        list_layout_->setSpacing(8);
        list_layout_->setContentsMargins(0, 0, 6, 0);
        
        empty_state_ = new QFrame(scroll_content);
        empty_state_->setObjectName("emptyThrows");
        
        auto* empty_layout = new QVBoxLayout(empty_state_);
        empty_layout->setContentsMargins(20, 24, 20, 24);
        empty_layout->setSpacing(8);
        
        auto* empty_icon = new QLabel("", empty_state_);
        empty_icon->setStyleSheet("font-size: 14px; background: transparent;");
        empty_icon->setAlignment(Qt::AlignCenter);
        empty_layout->addWidget(empty_icon);
        
        empty_text_ = new QLabel("No throws yet", empty_state_);
        empty_text_->setAlignment(Qt::AlignCenter);
        empty_layout->addWidget(empty_text_);
        
        empty_hint_ = new QLabel("Press F3+C while looking\nat an ender eye", empty_state_);
        empty_hint_->setAlignment(Qt::AlignCenter);
        empty_layout->addWidget(empty_hint_);
        
        list_layout_->addWidget(empty_state_);
        list_layout_->addStretch();
        
        scroll_area_->setWidget(scroll_content);
        layout->addWidget(scroll_area_, 1);
        
        auto* buttons = new QHBoxLayout();
        buttons->setSpacing(8);
        buttons->setContentsMargins(0, 4, 0, 0);
        
        undo_btn_ = new QPushButton("↩ Undo", this);
        undo_btn_->setCursor(Qt::PointingHandCursor);
        connect(undo_btn_, &QPushButton::clicked, this, &ThrowList::undo_requested);
        buttons->addWidget(undo_btn_, 1);
        
        redo_btn_ = new QPushButton("↪ Redo", this);
        redo_btn_->setCursor(Qt::PointingHandCursor);
        connect(redo_btn_, &QPushButton::clicked, this, &ThrowList::redo_requested);
        buttons->addWidget(redo_btn_, 1);
        
        reset_btn_ = new QPushButton("✕ Reset", this);
        reset_btn_->setCursor(Qt::PointingHandCursor);
        connect(reset_btn_, &QPushButton::clicked, this, &ThrowList::reset_requested);
        buttons->addWidget(reset_btn_, 1);
        
        layout->addLayout(buttons);
        
        hotkey_hint_ = new QLabel(
            "Ctrl+Z Undo | Ctrl+Y Redo | Ctrl+R Reset",
            this
        );
        hotkey_hint_->setAlignment(Qt::AlignCenter);
        layout->addWidget(hotkey_hint_);
    }
    
    QVBoxLayout* list_layout_;
    QFrame* empty_state_;
    QLabel* count_label_;
    QLabel* title_;
    QLabel* empty_text_;
    QLabel* empty_hint_;
    QLabel* hotkey_hint_;
    QScrollArea* scroll_area_;
    QPushButton* undo_btn_;
    QPushButton* redo_btn_;
    QPushButton* reset_btn_;
    core::ConnectionHandle theme_connection_;
};

} // namespace dolbot::ui
