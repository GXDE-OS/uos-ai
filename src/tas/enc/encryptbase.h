#ifndef ENCRYPTBASE_H
#define ENCRYPTBASE_H

#include <vector>
#include <string>
#include <openssl/evp.h>

class EncryptBase
{
public:
    EncryptBase(const EVP_CIPHER *cipher);

    ~EncryptBase();

    void setkey(const std::string &passowrd, const std::string &salt);

    std::vector<unsigned char> encrypt(const char *plaintext, int &len);

    std::vector<unsigned char> decrypt(const char *ciphertext, int &len);

    static bool CheckSupportSM4();

protected:
    const EVP_CIPHER *m_cipher;

    EVP_CIPHER_CTX *m_ctx;

    std::vector<unsigned char> m_key;

    std::vector<unsigned char> m_iv;
};

#endif // ENCRYPTBASE_H
