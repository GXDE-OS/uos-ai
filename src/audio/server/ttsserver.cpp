#include "ttsserver.h"

#include <QRegularExpression>

TtsServer::TtsServer(const QString &id, QObject *parent)
    : QObject(parent)
    , m_id(id)
{

}

TtsServer::~TtsServer()
{

}

QString TtsServer::id() const
{
    return m_id;
}

void TtsServer::setModel(int model)
{
    m_model = model;
}

int TtsServer::model() const
{
    return m_model;
}

QStringList TtsServer::splitString(const QString &inputString)
{
    QStringList results;
    QRegularExpression re(R"([\s,，.。;；?!！]+)"); // 匹配非中文非英文的字符
    QStringList chunks = inputString.split(re);

    QString chunk;
    int charCount = 0;

    for (const QString &str : chunks) {
        if (chunk.isEmpty()) {
            chunk = str;
            charCount = str.length();
        } else if (charCount + str.length() + 1 <= chunkSize) { // 加1是为了考虑分隔符的长度
            chunk += " " + str;
            charCount += str.length() + 1;
        } else {
            results << chunk;
            chunk = str;
            charCount = str.length();
        }
    }

    if (!chunk.isEmpty()) {
        results << chunk;
    }

    return results;
}
