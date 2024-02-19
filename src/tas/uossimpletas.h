#ifndef UOSSIMPLETAS_H
#define UOSSIMPLETAS_H

#include "tas.h"

class HttpEventLoop;

class UosSimpleTas: public TAS
{
public:
    UosSimpleTas();

    virtual TextAuditResult doTextAuditing(const QByteArray &data);

    QString hostUrl() const;
};

#endif // UOSSIMPLETAS_H
