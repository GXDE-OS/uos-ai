// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCREENSHOTSERVICE_H
#define SCREENSHOTSERVICE_H

#include <QObject>

class QDBusPendingCallWatcher;

namespace uos_ai {

/**
 * @brief Service layer for screenshot functionality.
 *
 * Handles all D-Bus communication with the screenshot service.
 * Manages window hiding, D-Bus calls, signal connections, and restoration.
 */
class ScreenshotService : public QObject
{
    Q_OBJECT
public:
    static ScreenshotService *instance();

    /**
     * @brief Check screenshot availability.
     * @return -1: Screenshot button not shown
     *          0: Screenshot functionality available
     *          1: Need to upgrade to higher version screenshot
     *          2: Screen recording in progress
     */
    int checkAvailability();

public slots:
    void start();

signals:
    void done(const QString &imagePath);
    void canceled();

private slots:
    void onCustomDone(const QString &path);
    void onCallFinished(QDBusPendingCallWatcher *watcher);

private:
    explicit ScreenshotService(QObject *parent = nullptr);
};

} // namespace uos_ai

#endif // SCREENSHOTSERVICE_H