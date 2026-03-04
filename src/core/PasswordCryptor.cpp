//----------------------------------------------------------
#include <memory>
//----------------------------------------------------------
#include <openssl/evp.h>
#include <openssl/rand.h>
//----------------------------------------------------------
#include "PasswordCryptor.h"
//----------------------------------------------------------

std::string          PasswordCryptor::m_salt = "";
std::vector<uint8_t> PasswordCryptor::m_key  = {};
//----------------------------------------------------------

/*!
 * @brief Шифрование пароля
 * @param psw Пароль, который нужно зашифровать
 * @return Зашифрованный пароль
 */
PasswordCipher PasswordCryptor::encrypt(const std::string& psw) {

    std::string psw_with_salt = psw + m_salt;

    PasswordCipher cipher;
    cipher.iv = generate_iv();
    cipher.data.resize(psw_with_salt.size());

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);

    // Инициализация AES-256-GCM
    EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), NULL, m_key.data(), cipher.iv.data());

    // Шифрование
    int len;
    EVP_EncryptUpdate(ctx.get(), cipher.data.data(), &len, (uint8_t*) psw_with_salt.data(), psw_with_salt.size());

    // Финализация (в GCM режиме записывает остаток)
    int final_len;
    EVP_EncryptFinal_ex(ctx.get(), cipher.data.data() + len, &final_len);

    // Получение тега аутентификации (нужен для расшифровки)
    cipher.tag.resize(16);
    EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, 16, cipher.tag.data());

    return cipher;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Расшифровка пароля
 * @param psw Пароль, который нужно расшифровать
 * @return Расшифрованный пароль
 */
std::string PasswordCryptor::decrypt(const PasswordCipher& cipher) {

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);

    std::vector<uint8_t> plaintext(cipher.data.size());
    int                  len, final_len;

    // 1. Инициализация
    EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), NULL, m_key.data(), cipher.iv.data());

    // 2. Расшифровка тела
    EVP_DecryptUpdate(ctx.get(), plaintext.data(), &len, cipher.data.data(), cipher.data.size());

    // 3. Установка ожидаемого тега (обязательно для GCM!)
    if (!EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, 16, (void*) cipher.tag.data())) {
        throw std::runtime_error("decrypt: failed set TAG");
    }

    // 4. Финализация. Если тег не совпал, функция вернет ошибку <= 0
    if (EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + len, &final_len) <= 0) {
        throw std::runtime_error("decrypt: password was forged");
    }

    std::string psw = std::string(plaintext.begin(), plaintext.end());

    psw.resize(0, psw.length() - m_salt.length());
    return psw;
}
//----------------------------------------------------------------------------------------------------------------------

std::vector<uint8_t> PasswordCryptor::generate_iv(size_t size) {
    std::vector<uint8_t> iv(size);
    if (RAND_bytes(iv.data(), size) != 1) {
        throw std::runtime_error("failed generate  IV");
    }
    return iv;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Инициализация
 * @param key  Закрытый ключ шифрования
 * @param salt Соль
 */
void PasswordCryptor::init(const std::vector<uint8_t>& key, const std::string& salt) {
    m_salt = salt;
    m_key  = key;
}
//----------------------------------------------------------------------------------------------------------------------
