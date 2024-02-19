#ifndef UOSACCOUNTENCODER_H
#define UOSACCOUNTENCODER_H
#include "tasdef.h"

#include <QSharedPointer>

class EncryptBase;

class UosAccountEncoder
{
public:
    UosAccountEncoder();

    UosAccountEncoder(const QString &passwd);

    // ​generator account
    QVector<UosFreeAccount> makeUosFreeAccount(const UosFreeAccount &seed, const int count = 1000);

    //encode:（key ; id ; type）AES ecb 128
    QString encrypt(const QString &key, const QString &index, int type);

    //decode:（key ; id ; type）AES ecb 128
    std::tuple<QString, QString, int> decrypt(const QString &ciphertext);

private:
    QSharedPointer<EncryptBase> m_encryptor;

    QString m_passwd;

};

#endif // UOSACCOUNTENCODER_H
