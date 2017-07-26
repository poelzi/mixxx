#include "util/notifications.h"

#include <QtDebug>
#include <QSystemTrayIcon>


namespace mixxx {


NotificationHelper::NotificationHelper()
    : s_enabled(0) {
        this->s_icon = QSystemTrayIcon::QSystemTrayIcon(parent = Q_NULLPTR);

}

NotificationHelper::~NotificationHelper() {
    if(this->s_icon != Q_NULLPTR) {
        this->s_icon.hide();
    }
}

}
