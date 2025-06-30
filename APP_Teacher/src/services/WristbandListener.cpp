#include "WristbandListener.h"
#include <QThreadPool>
#include <QMutexLocker>
#include <QPointer>
#include "WristbandClientHandler.h"
#include "WristbandData.h"


WristbandListener::WristbandListener(QObject* parent)
    : QTcpServer(parent) {
    QThreadPool::globalInstance()->setMaxThreadCount(10);
}

WristbandListener::~WristbandListener() {
    stopServer();
}

bool WristbandListener::startServer(const quint16 port) {
    //直接调用QTcpServer的listen监听port端口
    if (!listen(QHostAddress::Any, port)) {
        return false;
    }
    emit listeningChanged(isListening());
    return true;
}

void WristbandListener::stopServer() {
    close();
    // QMutexLocker locker(&m_mutex);
    // for (const auto& handler : m_activeHandlers) {
    //     if (handler)
    //         emit handler->requestDisconnect();
    // }
    // m_activeHandlers.clear();
    emit listeningChanged(isListening());
}

void WristbandListener::incomingConnection(const qintptr socketDescriptor) {
    QPointer handler = new WristbandClientHandler(socketDescriptor);

    connect(
        handler, &WristbandClientHandler::errorOccurred,
        this, [&](const QString& id, const QString& error) {
            QVariantMap map = {
                {"id", id},
                {"msg", error}
            };
            emit errorOccurred(map);

            QMutexLocker locker(&m_mutex);
            m_activeHandlers.remove(socketDescriptor);
        },
        Qt::QueuedConnection
    );

    connect(
        handler, &WristbandClientHandler::clientConnected,
        this, [=](const QString& id) {
            emit clientConnected(id);

            QMutexLocker locker(&m_mutex);
            m_activeHandlers.insert(socketDescriptor, handler);
        },
        Qt::QueuedConnection
    );

    connect(
        handler, &WristbandClientHandler::dataReceived,
        this, [=](const QString& id, const WristbandData& data) {
            emit dataReceived(id, data);
        },
        Qt::QueuedConnection
    );

    connect(
        handler, &WristbandClientHandler::clientDisconnected,
        this, [&](const QString& id) {
            emit clientDisconnected(id);

            QMutexLocker locker(&m_mutex);
            m_activeHandlers.remove(socketDescriptor);
        },
        Qt::QueuedConnection
    );

    QThreadPool::globalInstance()->start(handler);
}
