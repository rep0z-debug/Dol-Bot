#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QByteArray>
#include <functional>
#include <map>
#include "dolbot/domain/triangulator.hpp"

namespace dolbot::io {

class HttpServer : public QObject {
    Q_OBJECT
    
public:
    explicit HttpServer(QObject* parent = nullptr)
        : QObject(parent)
        , server_(new QTcpServer(this))
    {
        connect(server_, &QTcpServer::newConnection, this, &HttpServer::handle_connection);
    }
    
    bool start(quint16 port = 52533) {
        port_ = port;
        return server_->listen(QHostAddress::LocalHost, port);
    }
    
    void stop() {
        server_->close();
    }
    
    bool is_running() const { return server_->isListening(); }
    quint16 port() const { return port_; }
    
    void set_triangulator(domain::Triangulator* tri) { triangulator_ = tri; }
    void set_reset_callback(std::function<void()> cb) { reset_callback_ = std::move(cb); }
    void set_undo_callback(std::function<void()> cb) { undo_callback_ = std::move(cb); }

private:
    void handle_connection() {
        while (server_->hasPendingConnections()) {
            QTcpSocket* socket = server_->nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                handle_request(socket);
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
        }
    }
    
    void handle_request(QTcpSocket* socket) {
        // Simple line-based buffering
        while (socket->canReadLine()) {
            QByteArray data = socket->readLine();
            QString line = QString::fromUtf8(data).trimmed();

            if (line.isEmpty()) continue;

            QStringList parts = line.split(" ");
            if (parts.size() < 2) {
                // Not a valid request line, might be headers or body which we ignore for this simple API
                // But if it's the first line and invalid, it's an error.
                // For simplicity, we assume the first line we read is the request line.
                // If we get garbage, we might want to close or ignore.
                // Given the protocol is simple HTTP GET/POST, we just check if it looks like a request.
                 continue;
            }

            QString method = parts[0];
            QString path = parts[1];

            if (method == "GET" && path == "/api/v1/stronghold") {
                handle_stronghold_request(socket);
            } else if (method == "GET" && path == "/api/v1/status") {
                handle_status_request(socket);
            } else if (method == "POST" && path == "/api/v1/reset") {
                handle_reset_request(socket);
            } else if (method == "POST" && path == "/api/v1/undo") {
                handle_undo_request(socket);
            } else if (method == "GET" && path == "/api/v1/throws") {
                handle_throws_request(socket);
            } else {
                send_response(socket, 404, "Not Found", QJsonObject{{"error", "Endpoint not found"}});
            }

            // For this simple server, we process one request per connection and close it (or expect it to close)
            // But browsers might keep-alive. We can just process the first valid request line and ignore the rest/headers.
            // A more robust server would parse headers, content-length etc.
            // But this fix ensures we at least read a full line instead of readAll() partial packets.
            return;
        }
    }
    
    void handle_stronghold_request(QTcpSocket* socket) {
        QJsonObject response;
        
        if (!triangulator_) {
            send_response(socket, 500, "Internal Error", QJsonObject{{"error", "No triangulator"}});
            return;
        }
        
        const auto& result = triangulator_->result();
        response["success"] = result.has_result();
        response["throw_count"] = result.throw_count;
        response["locked"] = result.locked;
        
        QJsonArray predictions;
        for (const auto& pred : result.predictions) {
            QJsonObject p;
            p["chunk_x"] = pred.chunk.pos.x;
            p["chunk_z"] = pred.chunk.pos.z;
            p["certainty"] = pred.certainty;
            p["overworld_x"] = pred.chunk.stronghold_x();
            p["overworld_z"] = pred.chunk.stronghold_z();
            p["nether_x"] = pred.chunk.pos.nether_x();
            p["nether_z"] = pred.chunk.pos.nether_z();
            
            QJsonArray errors;
            for (double e : pred.angle_errors) {
                errors.append(e);
            }
            p["angle_errors"] = errors;
            
            predictions.append(p);
        }
        response["predictions"] = predictions;
        
        send_response(socket, 200, "OK", response);
    }
    
    void handle_status_request(QTcpSocket* socket) {
        QJsonObject response;
        response["version"] = "1.0.1";
        response["running"] = true;
        response["api_version"] = "1";
        
        if (triangulator_) {
            response["throw_count"] = triangulator_->throw_count();
            response["locked"] = triangulator_->is_locked();
            response["has_result"] = triangulator_->result().has_result();
        }
        
        send_response(socket, 200, "OK", response);
    }
    
    void handle_reset_request(QTcpSocket* socket) {
        if (reset_callback_) {
            reset_callback_();
            send_response(socket, 200, "OK", QJsonObject{{"success", true}});
        } else {
            send_response(socket, 500, "Internal Error", QJsonObject{{"error", "Reset not available"}});
        }
    }
    
    void handle_undo_request(QTcpSocket* socket) {
        if (undo_callback_) {
            undo_callback_();
            send_response(socket, 200, "OK", QJsonObject{{"success", true}});
        } else {
            send_response(socket, 500, "Internal Error", QJsonObject{{"error", "Undo not available"}});
        }
    }
    
    void handle_throws_request(QTcpSocket* socket) {
        QJsonObject response;
        
        if (!triangulator_) {
            send_response(socket, 500, "Internal Error", QJsonObject{{"error", "No triangulator"}});
            return;
        }
        
        QJsonArray throws;
        for (const auto& t : triangulator_->throws()) {
            QJsonObject obj;
            obj["x"] = t.position.x;
            obj["z"] = t.position.z;
            obj["angle"] = t.corrected_angle();
            obj["raw_angle"] = t.horizontal_angle;
            obj["boat_mode"] = t.is_boat_mode;
            obj["boat_error_limit"] = t.boat_error_limit;
            obj["boat_sensitivity"] = t.boat_sensitivity;
            obj["use_boat_sensitivity"] = t.use_boat_sensitivity;
            throws.append(obj);
        }
        response["throws"] = throws;
        response["count"] = static_cast<int>(triangulator_->throws().size());
        
        send_response(socket, 200, "OK", response);
    }
    
    void send_response(QTcpSocket* socket, int code, const QString& status, const QJsonObject& body) {
        QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);
        
        QString response = QString(
            "HTTP/1.1 %1 %2\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %3\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n"
            "\r\n"
        ).arg(code).arg(status).arg(json.size());
        
        socket->write(response.toUtf8());
        socket->write(json);
        socket->flush();
        socket->disconnectFromHost();
    }
    
    QTcpServer* server_;
    quint16 port_ = 52533;
    domain::Triangulator* triangulator_ = nullptr;
    std::function<void()> reset_callback_;
    std::function<void()> undo_callback_;
};

} // namespace dolbot::io
