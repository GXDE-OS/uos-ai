#ifndef MINIMAXMESSAGEPROTOCOL_H
#define MINIMAXMESSAGEPROTOCOL_H

#include "openai/oaimessageprotocol.h"

namespace uos_ai {

class MiniMaxMessageProtocol : public OaiMessageProtocol
{
public:
    explicit MiniMaxMessageProtocol();
    ~MiniMaxMessageProtocol() override;

    QJsonObject params(const QVariantHash &args) override;
    bool parseChunk(const QByteArray &chunkData, QVariantHash &content) override;
    bool parseResponse(const QByteArray &data, QVariantHash &content) override;
};

} // namespace uos_ai

#endif // MINIMAXMESSAGEPROTOCOL_H
