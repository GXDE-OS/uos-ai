#ifndef DEEPSEEKMESSAGEPROTOCOL_H
#define DEEPSEEKMESSAGEPROTOCOL_H

#include "../openai/oaimessageprotocol.h"

namespace uos_ai {

class DeepSeekMessageProtocol : public OaiMessageProtocol
{
public:
    explicit DeepSeekMessageProtocol();
    ~DeepSeekMessageProtocol() override;

    QJsonArray messages() const override;
};

} // namespace uos_ai

#endif // DEEPSEEKMESSAGEPROTOCOL_H
