#include "iatserver.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAudio)


using namespace uos_ai;

IatServer::IatServer(QObject *parent)
    : QObject{parent}
{
}

IatServer::~IatServer()
{
}

void IatServer::setModel(int model)
{
    qCDebug(logAudio) << "Setting model to:" << model;
    m_model = model;
}

int IatServer::model() const
{
    qCDebug(logAudio) << "Getting current model:" << m_model;
    return m_model;
}
