#ifndef CTRL_CENTER_ABILITY_H
#define CTRL_CENTER_ABILITY_H

#include <QString>

class IControlCenter
{
public:
    virtual ~IControlCenter() {}

    virtual int ShowModule(const QString &module) = 0;
    virtual int ShowPage(const QString &module, const QString &page) = 0;
    virtual int ShowPage(const QString &url) = 0;
};

#endif // CTRL_CENTER_ABILITY_H
