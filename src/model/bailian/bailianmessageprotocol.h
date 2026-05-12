#ifndef BAILIANMESSAGEPROTOCOL_H
#define BAILIANMESSAGEPROTOCOL_H

#include "openai/oaimessageprotocol.h"

namespace uos_ai {

class BailianMessageProtocol : public OaiMessageProtocol
{
public:
    explicit BailianMessageProtocol();
    ~BailianMessageProtocol() override;

    QJsonObject params(const QVariantHash &args) override;
};

}

#endif // BAILIANMESSAGEPROTOCOL_H
