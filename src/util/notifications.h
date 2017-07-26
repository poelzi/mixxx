#ifndef MIXXX_SCREENSAVER_H
#define MIXXX_NOTIFICATION_H

#include <QSystemTrayIcon>
#include <QtCore>

namespace mixxx {

// Code related to interacting with desktop notifications.
//
// Main use is to send notifications to the desktop.
//
class NotificationHelper {
public:
    NotificationHelper();
    ~NotificationHelper();

    void showMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information, int millisecondsTimeoutHint = 10000);

private:
    bool s_enabled;
    QSystemTrayIcon s_icon;
};

}

#endif // MIXXX_NOTIFICATION_H
