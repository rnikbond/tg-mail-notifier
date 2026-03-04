//----------------------------------------------------------
#ifndef PASSWORDCRYPTOR_H
#define PASSWORDCRYPTOR_H
//----------------------------------------------------------
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
//----------------------------------------------------------
#include "chat/Chat.h"
//----------------------------------------------------------

/**
 * @brief Класс для шифрования и дешифрования паролей
 */
class PasswordCryptor {
public:

    PasswordCryptor()  = default;
    ~PasswordCryptor() = default;

    static void init(const std::string& key, const std::string& salt);

public:

    static PasswordCipher encrypt(const std::string& psw);
    static std::string decrypt(const PasswordCipher& cipher);

private:

    static std::string          m_salt;
    static std::vector<uint8_t> m_key;

    static std::vector<uint8_t> generate_iv(size_t size = 12);
};
//----------------------------------------------------------------------------------------------------------------------

#endif // PASSWORDCRYPTOR_H
