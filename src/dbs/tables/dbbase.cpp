#include "dbbase.h"
#include "daoclient.h"

DbBase::DbBase()
    : m_daoClient(&DaoClient::getInstance())
{

}

DbBase::~DbBase()
{

}

bool DbBase::save()
{
    return 0;
}

bool DbBase::update()
{
    return 0;
}

bool DbBase::remove()
{
    return 0;
}
