#include "tas.h"
#include "tasdef.h"
#include "httpaccessmanager.h"

#include <QDebug>

TAS::TAS()
    : QObject(nullptr)
{
    qRegisterMetaType<TextAuditResult>("TextAuditResult");
    qRegisterMetaType<QSharedPointer<TextAuditResult> >("QSharedPointer<TextAuditResult>");
}

TAS::~TAS()
{

}
