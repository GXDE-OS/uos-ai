#ifndef TAS_H
#define TAS_H

// Test Auditing Service (TAS)
#include "tasdef.h"

#include <QObject>
#include <QByteArray>
#include <QSharedPointer>

class HttpAccessmanager;

class TAS : public QObject
{
    Q_OBJECT
public:
    TAS();

    virtual ~TAS();

    virtual TextAuditResult doTextAuditing(const QByteArray &data) = 0;

};

#endif // TAS_H
