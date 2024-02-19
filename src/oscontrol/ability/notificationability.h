#ifndef NOTIFICATION_ABILITY_H
#define NOTIFICATION_ABILITY_H

#define NOTIFICATION_ABILITY_H

#include <QVariant>

class INotification
{
public:
    virtual ~INotification() {}
    virtual int SetSystemInfo(int param, QVariant data) = 0;
};

#endif //NOTIFICATION_ABILITY_H
