#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVersionNumber>
#include <QTimer>
#include "dolbot/core/signal.hpp"

namespace dolbot::io {

struct VersionInfo {
    QString version;
    QString download_url;
    QString release_notes;
    bool is_newer = false;
};

class UpdateChecker : public QObject {
    Q_OBJECT
    
public:
    static constexpr const char* CURRENT_VERSION = "1.0.0";
    static constexpr const char* RELEASES_URL = "https://api.github.com/repos/rep0z-debug/Dol-Bot/releases/latest";
    static constexpr int TIMEOUT_MS = 10000;
    
    explicit UpdateChecker(QObject* parent = nullptr)
        : QObject(parent)
        , manager_(new QNetworkAccessManager(this))
        , timeout_timer_(new QTimer(this))
    {
        timeout_timer_->setSingleShot(true);
        connect(timeout_timer_, &QTimer::timeout, this, [this]() {
            if (current_reply_) {
                current_reply_->abort();
                check_complete_.fire(false, "Update check timed out");
            }
        });
    }
    
    void check_for_updates() {
        QNetworkRequest request{QUrl{RELEASES_URL}};
        request.setHeader(QNetworkRequest::UserAgentHeader, "DolBot/" + QString(CURRENT_VERSION));
        request.setRawHeader("Accept", "application/vnd.github.v3+json");
        
        current_reply_ = manager_->get(request);
        timeout_timer_->start(TIMEOUT_MS);
        
        connect(current_reply_, &QNetworkReply::finished, this, [this]() {
            timeout_timer_->stop();
            handle_response(current_reply_);
            current_reply_->deleteLater();
            current_reply_ = nullptr;
        });
        
        connect(current_reply_, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError error) {
            Q_UNUSED(error);
            timeout_timer_->stop();
        });
    }
    
    core::ConnectionHandle on_update_available(std::function<void(const VersionInfo&)> callback) {
        return update_available_.connect(std::move(callback));
    }
    
    core::ConnectionHandle on_check_complete(std::function<void(bool, const QString&)> callback) {
        return check_complete_.connect(std::move(callback));
    }
    
    static bool is_newer_version(const QString& remote, const QString& current) {
        QVersionNumber remote_v = QVersionNumber::fromString(remote.startsWith("v") ? remote.mid(1) : remote);
        QVersionNumber current_v = QVersionNumber::fromString(current);
        return remote_v > current_v;
    }

private:
    void handle_response(QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) {
            QString error_msg;
            int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            
            if (status_code == 404) {
                error_msg = "No releases found yet";
            } else if (status_code == 403) {
                error_msg = "Rate limited - try again later";
            } else if (reply->error() == QNetworkReply::OperationCanceledError) {
                error_msg = "Update check timed out";
            } else if (reply->error() == QNetworkReply::HostNotFoundError) {
                error_msg = "No internet connection";
            } else {
                error_msg = reply->errorString();
            }
            
            check_complete_.fire(false, error_msg);
            return;
        }
        
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            check_complete_.fire(false, "Invalid response format");
            return;
        }
        
        QJsonObject obj = doc.object();
        
        if (obj.contains("message")) {
            QString msg = obj["message"].toString();
            if (msg.contains("Not Found")) {
                check_complete_.fire(false, "No releases found yet");
                return;
            }
        }
        
        VersionInfo info;
        info.version = obj["tag_name"].toString();
        info.release_notes = obj["body"].toString();
        
        if (info.version.isEmpty()) {
            check_complete_.fire(false, "No version info in release");
            return;
        }
        
        QJsonArray assets = obj["assets"].toArray();
        for (const auto& asset : assets) {
            QJsonObject a = asset.toObject();
            QString name = a["name"].toString();
            if (name.endsWith(".exe") || name.endsWith(".zip")) {
                info.download_url = a["browser_download_url"].toString();
                break;
            }
        }
        
        if (info.download_url.isEmpty()) {
            info.download_url = obj["html_url"].toString();
        }
        
        info.is_newer = is_newer_version(info.version, CURRENT_VERSION);
        
        if (info.is_newer) {
            update_available_.fire(info);
        }
        
        check_complete_.fire(true, "");
    }
    
    QNetworkAccessManager* manager_;
    QTimer* timeout_timer_;
    QNetworkReply* current_reply_ = nullptr;
    core::Signal<const VersionInfo&> update_available_;
    core::Signal<bool, const QString&> check_complete_;
};

} // namespace dolbot::io

