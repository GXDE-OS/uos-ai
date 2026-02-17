#include "transerver.h"
UOSAI_USE_NAMESPACE

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAudio)

TranServer::TranServer(QObject *parent)
    : QObject{parent}
{
}

TranServer::~TranServer()
{
}

void TranServer::setModel(int model)
{
    qCDebug(logAudio) << "Setting model to:" << model;
    m_model = model;
}

int TranServer::model() const
{
    qCDebug(logAudio) << "Getting current model:" << m_model;
    return m_model;
}
