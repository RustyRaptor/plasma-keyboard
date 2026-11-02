// SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

// Saves a screenshot of a KWin session through its ScreenShot2 D-Bus interface.
// KWin must run with KWIN_SCREENSHOT_NO_PERMISSION_CHECKS=1.
//
//   screenshot FILE.png

#include <QCoreApplication>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QImage>

#include <cstdio>
#include <unistd.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 2) {
        qWarning("usage: screenshot FILE.png");
        return 1;
    }

    int fds[2];
    if (pipe(fds) != 0) {
        return 1;
    }

    QDBusInterface screenshot(QStringLiteral("org.kde.KWin"), QStringLiteral("/org/kde/KWin/ScreenShot2"), QStringLiteral("org.kde.KWin.ScreenShot2"));
    const QDBusReply<QVariantMap> reply =
        screenshot.call(QStringLiteral("CaptureWorkspace"), QVariantMap{}, QVariant::fromValue(QDBusUnixFileDescriptor(fds[1])));
    close(fds[1]);
    if (!reply.isValid()) {
        fprintf(stderr, "screenshot failed: %s\n", qPrintable(reply.error().message()));
        return 1;
    }

    QByteArray data;
    char buffer[65536];
    ssize_t count;
    while ((count = read(fds[0], buffer, sizeof(buffer))) > 0) {
        data.append(buffer, count);
    }
    close(fds[0]);

    const QVariantMap metadata = reply.value();
    const QImage image(reinterpret_cast<const uchar *>(data.constData()),
                       metadata.value(QStringLiteral("width")).toInt(),
                       metadata.value(QStringLiteral("height")).toInt(),
                       metadata.value(QStringLiteral("stride")).toInt(),
                       QImage::Format(metadata.value(QStringLiteral("format")).toInt()));
    if (!image.save(QString::fromLocal8Bit(argv[1]))) {
        fprintf(stderr, "could not save %zd bytes of %s\n", size_t(data.size()), qPrintable(QVariant(metadata).toString()));
        return 1;
    }
    return 0;
}
