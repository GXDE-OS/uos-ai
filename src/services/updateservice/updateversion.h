#ifndef UPDATEVERSION_H
#define UPDATEVERSION_H

#include <QList>
#include <QString>
#include <QVersionNumber>

namespace uos_ai {

class UpdateVersion
{
public:
    static QString stripPackageRevision(const QString &version);
    static QVersionNumber parseNumber(const QString &version);
    static QString normalize(const QString &version);
    static int compare(const QString &left, const QString &right);
};

}

#endif // UPDATEVERSION_H
