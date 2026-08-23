#pragma once

#include "SearchTypes.h"

#include <QObject>
#include <QPointer>
#include <QQueue>
#include <QTcpServer>

class QTcpSocket;

namespace quickary {

class EverythingProvider;

class HttpApiServer final : public QObject {
    Q_OBJECT
public:
    explicit HttpApiServer(QObject* parent = nullptr);
    ~HttpApiServer() override;

    bool start(quint16 port, const QString& token);
    void stop();
    bool isListening() const;
    quint16 port() const;

signals:
    void statusChanged(bool listening, const QString& detail);

private:
    struct PendingQuery {
        QPointer<QTcpSocket> socket;
        QString query;
        int limit{50};
        quint64 serial{0};
    };

    void acceptConnections();
    void consumeRequest(QTcpSocket* socket);
    void queueSearch(QTcpSocket* socket, const QString& query, int limit);
    void startNextQuery();
    void onSearchResults(const SearchBatch& batch);
    bool authorized(const QList<QByteArray>& lines) const;
    void sendJson(QTcpSocket* socket, int status, const QByteArray& statusText, const QByteArray& json);
    void sendError(QTcpSocket* socket, int status, const QByteArray& statusText, const QString& message);

    QTcpServer server_;
    EverythingProvider* provider_{};
    QQueue<PendingQuery> queue_;
    PendingQuery active_;
    QByteArray token_;
    quint64 nextSerial_{1};
    bool queryInFlight_{false};
};

} // namespace quickary
