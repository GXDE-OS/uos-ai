#include "bailianmessageprotocol.h"
#include "global_key_define.h"

#include <QJsonObject>

using namespace uos_ai;

BailianMessageProtocol::BailianMessageProtocol()
    : OaiMessageProtocol()
{

}

BailianMessageProtocol::~BailianMessageProtocol()
{

}

QJsonObject BailianMessageProtocol::params(const QVariantHash &args)
{
    QJsonObject ret = OaiMessageProtocol::params(args);

    if (args.contains(STR_KEY_THINKING)) {
        bool enableThinking = args.value(STR_KEY_THINKING).toBool();
        ret.insert("enable_thinking", enableThinking);
    }

    return ret;
}
