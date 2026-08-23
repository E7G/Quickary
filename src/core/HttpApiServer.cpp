#include "HttpApiServer.h"

#include "../providers/EverythingProvider.h"

#include <QDateTime>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>
#include <limits>

namespace quickary {
namespace {

QString kindName(ItemKind kind)
{
    switch (kind) {
    case ItemKind::File: return QStringLiteral("file");
    case ItemKind::Folder: return QStringLiteral("folder");
    case ItemKind::Application: return QStringLiteral("application");
    case ItemKind::Command: return QStringLiteral("command");
    case ItemKind::Web: return QStringLiteral("web");
    case ItemKind::Favorite: return QStringLiteral("favorite");
    }
    return QStringLiteral("unknown");
}

} // namespace

HttpApiServer::HttpApiServer(QObject* parent) : QObject(parent)
{
    provider_ = new EverythingProvider(this);
    connect(&server_, &QTcpServer::newConnection, this, &HttpApiServer::acceptConnections);
    connect(provider_, &EverythingProvider::resultsReady, this, &HttpApiServer::onSearchResults);
}

HttpApiServer::~HttpApiServer()
{
    stop();
}

bool HttpApiServer::start(quint16 requestedPort, const QString& token)
{
    stop();
    if (token.trimmed().isEmpty()) {
        emit statusChanged(false, QStringLiteral("HTTP API token is empty"));
        return false;
    }
    token_ = token.toUtf8();
    if (!server_.listen(QHostAddress::LocalHost, requestedPort)) {
        emit statusChanged(false, QStringLiteral("HTTP API could not listen on 127.0.0.1:%1: %2")
                                      .arg(requestedPort)
                                      .arg(server_.errorString()));
        return false;
    }
    emit statusChanged(true, QStringLiteral("HTTP API listening on http://127.0.0.1:%1").arg(server_.serverPort()));
    return true;
}

void HttpApiServer::stop()
{
    server_.close();
    queue_.clear();
    if (active_.socket) active_.socket->disconnectFromHost();
    active_ = {};
    queryInFlight_ = false;
}

bool HttpApiServer::isListening() const
{
    return server_.isListening();
}

quint16 HttpApiServer::port() const
{
    return server_.serverPort();
}

void HttpApiServer::acceptConnections()
{
    while (server_.hasPendingConnections()) {
        QTcpSocket* socket = server_.nextPendingConnection();
        socket->setParent(this);
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] { consumeRequest(socket); });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
}

void HttpApiServer::consumeRequest(QTcpSocket* socket)
{
    if (!socket || socket->property("quickaryRequestAccepted").toBool()) return;
    if (socket->bytesAvailable() > 16 * 1024) {
        sendError(socket, 413, "Payload Too Large", QStringLiteral("request headers are too large"));
        return;
    }
    const QByteArray buffer = socket->peek(socket->bytesAvailable());
    const int headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) return;

    const QByteArray request = socket->read(headerEnd + 4);
    const QList<QByteArray> lines = request.split('\n');
    if (lines.isEmpty()) {
        sendError(socket, 400, "Bad Request", QStringLiteral("missing request line"));
        return;
    }

    const QList<QByteArray> requestParts = lines.front().trimmed().split(' ');
    if (requestParts.size() != 3 || requestParts.at(0) != "GET") {
        sendError(socket, 405, "Method Not Allowed", QStringLiteral("only GET is supported"));
        return;
    }

    const QByteArray target = requestParts.at(1);
    const QUrl url = QUrl::fromEncoded(QByteArray("http://127.0.0.1") + target);
    if (!url.isValid()) {
        sendError(socket, 400, "Bad Request", QStringLiteral("invalid request URL"));
        return;
    }

    if (url.path() == QStringLiteral("/health")) {
        const QJsonObject health{
            {QStringLiteral("ok"), true},
            {QStringLiteral("service"), QStringLiteral("Quickary")},
            {QStringLiteral("version"), QStringLiteral("1.1.0")},
        };
        sendJson(socket, 200, "OK", QJsonDocument(health).toJson(QJsonDocument::Compact));
        return;
    }

    if (url.path() != QStringLiteral("/v1/search")) {
        sendError(socket, 404, "Not Found", QStringLiteral("unknown endpoint"));
        return;
    }
    if (!authorized(lines)) {
        sendError(socket, 401, "Unauthorized", QStringLiteral("missing or invalid API token"));
        return;
    }

    const QUrlQuery params(url);
    const QString query = params.queryItemValue(QStringLiteral("q"), QUrl::FullyDecoded).trimmed();
    if (query.isEmpty() || query.size() > 512) {
        sendError(socket, 400, "Bad Request", QStringLiteral("q must contain 1 to 512 characters"));
        return;
    }

    bool ok = false;
    int limit = params.queryItemValue(QStringLiteral("limit")).toInt(&ok);
    if (!ok) limit = 50;
    limit = qBound(1, limit, 200);
    socket->setProperty("quickaryRequestAccepted", true);
    queueSearch(socket, query, limit);
}

bool HttpApiServer::authorized(const QList<QByteArray>& lines) const
{
    const QByteArray bearer = QByteArray("Bearer ") + token_;
    for (int i = 1; i < lines.size(); ++i) {
        const QByteArray line = lines.at(i).trimmed();
        const QByteArray lower = line.toLower();
        if (lower.startsWith("authorization:")) {
            const QByteArray value = line.mid(sizeof("Authorization:") - 1).trimmed();
            if (value == bearer) return true;
        }
        if (lower.startsWith("x-quickary-token:")) {
            const QByteArray value = line.mid(sizeof("X-Quickary-Token:") - 1).trimmed();
            if (value == token_) return true;
        }
    }
    return false;
}

void HttpApiServer::queueSearch(QTcpSocket* socket, const QString& query, int limit)
{
    if (queue_.size() >= 8) {
        sendError(socket, 429, "Too Many Requests", QStringLiteral("search queue is full"));
        return;
    }
    PendingQuery pending;
    pending.socket = socket;
    pending.query = query;
    pending.limit = limit;
    pending.serial = nextSerial_++;
    queue_.enqueue(pending);
    startNextQuery();
}

void HttpApiServer::startNextQuery()
{
    if (queryInFlight_ || queue_.isEmpty()) return;
    active_ = queue_.dequeue();
    if (!active_.socket) {
        active_ = {};
        startNextQuery();
        return;
    }

    queryInFlight_ = true;
    SearchRequest request;
    request.rawQuery = active_.query;
    request.limit = active_.limit;
    request.deepSearch = false;
    request.serial = active_.serial;
    provider_->search(request);
}

void HttpApiServer::onSearchResults(const SearchBatch& batch)
{
    if (!queryInFlight_ || batch.serial != active_.serial) return;

    if (active_.socket) {
        QJsonArray results;
        const int count = qMin(active_.limit, batch.items.size());
        for (int i = 0; i < count; ++i) {
            const SearchItem& item = batch.items.at(i);
            const quint64 maxJsonInteger = static_cast<quint64>(std::numeric_limits<qint64>::max());
            QJsonObject object{
                {QStringLiteral("type"), kindName(item.kind)},
                {QStringLiteral("name"), item.title},
                {QStringLiteral("path"), item.path},
                {QStringLiteral("size"), static_cast<qint64>(qMin(item.size, maxJsonInteger))},
            };
            if (item.modified.isValid()) object.insert(QStringLiteral("modified"), item.modified.toString(Qt::ISODateWithMs));
            results.append(object);
        }
        const QJsonObject root{
            {QStringLiteral("query"), active_.query},
            {QStringLiteral("count"), results.size()},
            {QStringLiteral("results"), results},
        };
        sendJson(active_.socket, 200, "OK", QJsonDocument(root).toJson(QJsonDocument::Compact));
    }

    active_ = {};
    queryInFlight_ = false;
    startNextQuery();
}

void HttpApiServer::sendJson(QTcpSocket* socket, int status, const QByteArray& statusText, const QByteArray& json)
{
    if (!socket) return;
    QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + ' ' + statusText + "\r\n";
    response += "Content-Type: application/json; charset=utf-8\r\n";
    response += "Cache-Control: no-store\r\n";
    response += "Connection: close\r\n";
    response += "Content-Length: " + QByteArray::number(json.size()) + "\r\n\r\n";
    response += json;
    socket->write(response);
    socket->disconnectFromHost();
}

void HttpApiServer::sendError(QTcpSocket* socket, int status, const QByteArray& statusText, const QString& message)
{
    const QJsonObject root{{QStringLiteral("error"), message}};
    sendJson(socket, status, statusText, QJsonDocument(root).toJson(QJsonDocument::Compact));
}

} // namespace quickary
