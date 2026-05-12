#include "modelinfo.h"

using namespace uos_ai;

ModelAccount::ModelAccount() : QSharedData() {}

ModelAccount::ModelAccount(const ModelAccount &other) : QSharedData(other)
  , id(other.id)
  , account(other.account)
  , model(other.model)
  , params(other.params)
  , network(other.network)
{

}
