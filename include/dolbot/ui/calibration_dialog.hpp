#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QListWidget>
#include <QProgressBar>
#include <QGroupBox>
#include <vector>
#include <cmath>
#include "dolbot/core/coords.hpp"
#include "dolbot/domain/eye_throw.hpp"
#include "dolbot/io/clipboard_watcher.hpp"

namespace dolbot::ui {

class CalibrationDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit CalibrationDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Calibrate Standard Deviation");
        setMinimumSize(500, 500);
        setup_ui();
        setup_clipboard_watcher();
    }
    
    ~CalibrationDialog() override {
        if (clipboard_watcher_) {
            clipboard_watcher_->stop();
        }
    }
    
    double calculated_std_dev() const { return calculated_std_; }
    
private:
    void setup_clipboard_watcher() {
        clipboard_watcher_ = std::make_unique<io::ClipboardWatcher>(this);
        
        clip_conn_ = clipboard_watcher_->on_throw_detected([this](const domain::EyeThrow& t) {
            if (t.is_looking_down()) {
                status_label_->setText("Looking down - throw ignored");
                status_label_->setStyleSheet("color: #f59e0b;");
                return;
            }
            
            add_throw(t);
            status_label_->setText(QString("Throw detected! Angle: %1°").arg(t.corrected_angle(), 0, 'f', 2));
            status_label_->setStyleSheet("color: #22c55e;");
        });
        
        clipboard_watcher_->start();
    }
    
    void setup_ui() {
        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(16);
        layout->setContentsMargins(20, 20, 20, 20);
        
        auto* intro = new QLabel(
            "<b>How to calibrate:</b><br>"
            "1. Find a stronghold and enter its coordinates below<br>"
            "2. Stand at different positions and throw ender eyes at the stronghold<br>"
            "3. Press F3+C after each throw to record it<br>"
            "4. After 5+ throws, the calculated std. dev. will be shown"
        );
        intro->setWordWrap(true);
        layout->addWidget(intro);
        
        status_label_ = new QLabel("Waiting for F3+C input...");
        status_label_->setStyleSheet("color: #64748b; font-style: italic;");
        layout->addWidget(status_label_);
        
        auto* loc_group = new QGroupBox("Known Stronghold Location");
        auto* loc_layout = new QFormLayout(loc_group);
        
        stronghold_x_ = new QDoubleSpinBox();
        stronghold_x_->setRange(-30000000, 30000000);
        stronghold_x_->setDecimals(0);
        stronghold_x_->setToolTip("Enter the X coordinate of the stronghold staircase");
        loc_layout->addRow("X:", stronghold_x_);
        
        stronghold_z_ = new QDoubleSpinBox();
        stronghold_z_->setRange(-30000000, 30000000);
        stronghold_z_->setDecimals(0);
        stronghold_z_->setToolTip("Enter the Z coordinate of the stronghold staircase");
        loc_layout->addRow("Z:", stronghold_z_);
        
        layout->addWidget(loc_group);
        
        auto* throws_group = new QGroupBox("Recorded Throws");
        auto* throws_layout = new QVBoxLayout(throws_group);
        
        throw_list_ = new QListWidget();
        throw_list_->setMinimumHeight(120);
        throws_layout->addWidget(throw_list_);
        
        auto* throw_controls = new QHBoxLayout();
        
        auto* clear_btn = new QPushButton("Clear All");
        connect(clear_btn, &QPushButton::clicked, this, &CalibrationDialog::clear_throws);
        throw_controls->addWidget(clear_btn);
        
        throw_controls->addStretch();
        
        throw_count_label_ = new QLabel("0 throws recorded");
        throw_controls->addWidget(throw_count_label_);
        
        throws_layout->addLayout(throw_controls);
        layout->addWidget(throws_group);
        
        auto* results_group = new QGroupBox("Calibration Results");
        auto* results_layout = new QFormLayout(results_group);
        
        std_result_label_ = new QLabel("---");
        std_result_label_->setStyleSheet("font-weight: bold; font-size: 16px;");
        results_layout->addRow("Calculated Std Dev:", std_result_label_);
        
        rms_label_ = new QLabel("---");
        results_layout->addRow("RMS Error:", rms_label_);
        
        max_error_label_ = new QLabel("---");
        results_layout->addRow("Max Error:", max_error_label_);
        
        layout->addWidget(results_group);
        
        auto* buttons = new QHBoxLayout();
        
        auto* apply_btn = new QPushButton("Apply to Settings");
        apply_btn->setEnabled(false);
        connect(apply_btn, &QPushButton::clicked, this, [this]() {
            accept();
        });
        apply_btn_ = apply_btn;
        buttons->addWidget(apply_btn);
        
        auto* cancel_btn = new QPushButton("Cancel");
        connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);
        buttons->addWidget(cancel_btn);
        
        layout->addLayout(buttons);
    }
    
    void add_throw(const domain::EyeThrow& t) {
        if (stronghold_x_->value() == 0 && stronghold_z_->value() == 0) {
            status_label_->setText("Enter stronghold coordinates first!");
            status_label_->setStyleSheet("color: #ef4444;");
            return;
        }
        
        throws_.push_back(t);
        recalculate();
        update_throw_list();
    }
    
    void clear_throws() {
        throws_.clear();
        throw_list_->clear();
        throw_count_label_->setText("0 throws recorded");
        std_result_label_->setText("---");
        rms_label_->setText("---");
        max_error_label_->setText("---");
        calculated_std_ = 0.0;
        apply_btn_->setEnabled(false);
        status_label_->setText("Cleared. Waiting for F3+C input...");
        status_label_->setStyleSheet("color: #64748b; font-style: italic;");
    }
    
    void recalculate() {
        if (throws_.size() < 2) {
            std_result_label_->setText("Need at least 2 throws");
            apply_btn_->setEnabled(false);
            return;
        }
        
        std::vector<double> errors;
        double sum_sq = 0.0;
        double max_err = 0.0;
        
        double sh_x = stronghold_x_->value();
        double sh_z = stronghold_z_->value();
        
        for (const auto& t : throws_) {
            double dx = sh_x - t.position.x;
            double dz = sh_z - t.position.z;
            double expected_angle = -180.0 / core::coords::PI * std::atan2(dx, dz);
            double error = std::abs(core::coords::normalize_angle(t.corrected_angle() - expected_angle));
            
            errors.push_back(error);
            sum_sq += error * error;
            max_err = std::max(max_err, error);
        }
        
        double rms = std::sqrt(sum_sq / errors.size());
        calculated_std_ = rms;
        
        std_result_label_->setText(QString::number(rms, 'f', 4));
        rms_label_->setText(QString::number(rms, 'f', 4) + "°");
        max_error_label_->setText(QString::number(max_err, 'f', 4) + "°");
        
        apply_btn_->setEnabled(true);
    }
    
    void update_throw_list() {
        throw_list_->clear();
        
        double sh_x = stronghold_x_->value();
        double sh_z = stronghold_z_->value();
        
        for (size_t i = 0; i < throws_.size(); ++i) {
            const auto& t = throws_[i];
            double dx = sh_x - t.position.x;
            double dz = sh_z - t.position.z;
            double expected_angle = -180.0 / core::coords::PI * std::atan2(dx, dz);
            double error = core::coords::normalize_angle(t.corrected_angle() - expected_angle);
            
            QString text = QString("#%1: Angle %2° (Error: %3°)")
                .arg(i + 1)
                .arg(t.corrected_angle(), 0, 'f', 2)
                .arg(error, 0, 'f', 4);
            
            throw_list_->addItem(text);
        }
        
        throw_count_label_->setText(QString("%1 throws recorded").arg(throws_.size()));
    }
    
    std::unique_ptr<io::ClipboardWatcher> clipboard_watcher_;
    core::ConnectionHandle clip_conn_;
    
    QDoubleSpinBox* stronghold_x_;
    QDoubleSpinBox* stronghold_z_;
    QListWidget* throw_list_;
    QLabel* throw_count_label_;
    QLabel* std_result_label_;
    QLabel* rms_label_;
    QLabel* max_error_label_;
    QLabel* status_label_;
    QPushButton* apply_btn_;
    
    std::vector<domain::EyeThrow> throws_;
    double calculated_std_ = 0.0;
};

} // namespace dolbot::ui

