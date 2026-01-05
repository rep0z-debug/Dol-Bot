#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>

namespace dolbot::ui {

class SecureLabel : public QWidget {
    Q_OBJECT
    
public:
    explicit SecureLabel(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setLayout(new QHBoxLayout(this));
        layout()->setContentsMargins(0, 0, 0, 0);
        layout()->setSpacing(0);
        
        label_ = new QLabel(this);
        label_->setTextInteractionFlags(Qt::NoTextInteraction);
        layout()->addWidget(label_);
    }
    
    void setText(const QString& real_text, const QString& fake_text) {
        real_text_ = real_text;
        fake_text_ = fake_text;
        label_->setText(privacy_mode_ ? fake_text_ : real_text_);
    }
    
    void setPrivacyMode(bool enabled) {
        privacy_mode_ = enabled;
        label_->setText(privacy_mode_ ? fake_text_ : real_text_);
    }
    
    void setStyleSheet(const QString& style) {
        QWidget::setStyleSheet(style);
        label_->setStyleSheet(style);
    }
    
    void setAlignment(Qt::Alignment align) {
        label_->setAlignment(align);
    }
    
private:
    QLabel* label_ = nullptr;
    QString real_text_;
    QString fake_text_;
    bool privacy_mode_ = false;
};

}
