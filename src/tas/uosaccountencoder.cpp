#include "uosaccountencoder.h"
#include "enc/encryptbase.h"

#include <QDebug>
#include <sstream>
#include <iomanip>

UosAccountEncoder::UosAccountEncoder()
    : m_encryptor(new EncryptBase(EVP_aes_128_ecb()))
    , m_passwd("f5739d638610d5e1")
{
    m_encryptor->setkey(m_passwd.toStdString().c_str(), "");
}

UosAccountEncoder::UosAccountEncoder(const QString &passwd)
    : m_encryptor(new EncryptBase(EVP_aes_128_ecb()))
    , m_passwd(passwd)
{
    m_encryptor->setkey(m_passwd.toStdString().c_str(), "");
}

QVector<UosFreeAccount> UosAccountEncoder::makeUosFreeAccount(const UosFreeAccount &seed, const int count)
{
    QVector<UosFreeAccount> uosFreeAccount;
    for (int i = 1; i <= count; i++) {
        UosFreeAccount ufa;
        ufa.appid = encrypt(seed.appid, QString::number(i), seed.type);
        ufa.appkey = encrypt(seed.appkey, QString::number(i), seed.type);
        ufa.appsecret = encrypt(seed.appsecret, QString::number(i), seed.type);
        uosFreeAccount.append(ufa);
    }
    return uosFreeAccount;
}

QString UosAccountEncoder::encrypt(const QString &key, const QString &index, int type)
{
    QString plaintext = QString("%1;%2;%3").arg(key).arg(index).arg(type);
    int len = plaintext.length();
    std::vector<unsigned char> secret = m_encryptor->encrypt(plaintext.toStdString().c_str(), len);
    QByteArray data = QByteArray((const char *)secret.data(), len);
    return data.toBase64();
}

std::tuple<QString, QString, int> UosAccountEncoder::decrypt(const QString &ciphertext)
{
    QByteArray bcipher = QByteArray::fromBase64(ciphertext.toLocal8Bit());
    int len = bcipher.length();
    std::vector<unsigned char> plaintext = m_encryptor->decrypt(bcipher.data(), len);
    QByteArray data = QByteArray((const char *)plaintext.data(), len);
    auto plainList = data.split(';');
    if (plainList.size() == 3) {
        return std::make_tuple(plainList.at(0), plainList.at(1), plainList.at(2).toInt());
    }
    return std::make_tuple("", "", 0);
}
