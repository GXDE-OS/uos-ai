#include "iatserver.h"

IatServer::IatServer(QObject *parent)
    : QObject{parent}
{

}

IatServer::~IatServer()
{

}

void IatServer::setModel(int model)
{
    m_model = model;
}

int IatServer::model() const
{
    return m_model;
}
