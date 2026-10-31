/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QHash>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

/**
 * Minimal HTTP server answering every request with a canned JSON body.
 */
class FakeHttpServer : public QTcpServer
{
public:
    FakeHttpServer()
    {
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *socket = nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
                    m_buffers[socket] += socket->readAll();
                    const QByteArray &buffer = m_buffers[socket];
                    const qsizetype headerEnd = buffer.indexOf("\r\n\r\n");
                    if (headerEnd < 0) {
                        return;
                    }
                    const qsizetype lengthStart = buffer.toLower().indexOf("content-length:");
                    const int length =
                        lengthStart < 0 ? 0 : buffer.mid(lengthStart + 15, buffer.indexOf("\r\n", lengthStart) - lengthStart - 15).trimmed().toInt();
                    if (buffer.size() < headerEnd + 4 + length) {
                        return;
                    }
                    requests.append(buffer);
                    QTimer::singleShot(responseDelay, socket, [this, socket] {
                        socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: "
                                      + QByteArray::number(response.size()) + "\r\n\r\n" + response);
                        socket->disconnectFromHost();
                    });
                });
            }
        });
        listen(QHostAddress::LocalHost);
    }

    QUrl url() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1").arg(serverPort()));
    }

    QByteArray response;
    int responseDelay = 0;
    QList<QByteArray> requests;

private:
    QHash<QTcpSocket *, QByteArray> m_buffers;
};
