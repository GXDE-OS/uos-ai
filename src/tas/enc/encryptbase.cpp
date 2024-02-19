#include "encryptbase.h"

static const int EncryptRounds = 5;

EncryptBase::EncryptBase(const EVP_CIPHER *cipher)
    : m_cipher(cipher)
    , m_ctx(EVP_CIPHER_CTX_new())
{

}

EncryptBase::~EncryptBase()
{
    EVP_CIPHER_CTX_free(m_ctx);
}

void EncryptBase::setkey(const std::string &passowrd, const std::string &salt)
{
    m_key = std::vector<unsigned char> (passowrd.begin(), passowrd.end());
    m_iv = std::vector<unsigned char>(32, '0');
}

std::vector<unsigned char> EncryptBase::encrypt(const char *plaintext, int &len)
{
    if (m_key.empty() || m_iv.empty()) {
        len = 0;
        return {};
    }
    bool fOk = true;
    fOk = EVP_EncryptInit_ex(m_ctx, m_cipher, nullptr, &m_key[0], &m_iv[0]);
    int explen = 0, surlen = 0;
    std::vector<unsigned char> ciphertext = {0};
    if (fOk) explen = len + EVP_CIPHER_CTX_block_size(m_ctx);
    if (fOk) ciphertext = std::vector<unsigned char> (explen);
    if (fOk) fOk = EVP_EncryptUpdate(m_ctx, &ciphertext[0], &explen, (unsigned char *)plaintext, len);
    if (fOk) fOk = EVP_EncryptFinal_ex(m_ctx, (&ciphertext[0]) + explen, &surlen);
    EVP_CIPHER_CTX_cleanup(m_ctx);

    !fOk ? len = 0 : len = explen + surlen;

    return ciphertext;
}

std::vector<unsigned char> EncryptBase::decrypt(const char *ciphertext, int &len)
{
    if (m_key.empty() || m_iv.empty()) {
        len = 0;
        return {};
    }
    bool fOk = true;
    int explen = len, surlen = 0;
    std::vector<unsigned char> plaintext{0};
    fOk = EVP_DecryptInit_ex(m_ctx, m_cipher, nullptr, &m_key[0], &m_iv[0]);
    if (fOk)  plaintext = std::vector<unsigned char> (explen + EVP_CIPHER_CTX_block_size(m_ctx));
    if (fOk) fOk = EVP_DecryptUpdate(m_ctx, &plaintext[0], &explen, (unsigned char *)ciphertext, len);
    EVP_DecryptFinal_ex(m_ctx, (&plaintext[0]) + explen, &surlen);
    EVP_CIPHER_CTX_cleanup(m_ctx);

    !fOk ? len = 0 : len = explen + surlen;

    return plaintext;
}

bool EncryptBase::CheckSupportSM4()
{
#ifdef OPENSSL_VERSION_NUMBER
    if (OPENSSL_VERSION_NUMBER >= 0x10101001L) {
        return true;
    }
#endif
    return false;
}
