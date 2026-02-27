//----------------------------------------------------------
#ifndef IMAILREQUEST_H
#define IMAILREQUEST_H
//----------------------------------------------------------
#include <cstdint>
#include <expected>
#include <vector>
//----------------------------------------------------------
#include "chat/Chat.h"
#include "errs/Errors.h"
//----------------------------------------------------------
using UIDsOpt = std::expected<std::vector<int64_t>, Errors::Mail>;
using UIDOpt  = std::expected<int64_t, Errors::Mail>;
using EmailMsgExp = std::expected<std::string, Errors::Mail>;
//----------------------------------------------------------

class IMailRequest {

public:

    IMailRequest()          = default;
    virtual ~IMailRequest() = default;

    /**
     * @brief Загрузка UID новых писем
     * @param email Структура для выполнения запроса
     * @return Список новых UIDs, или ошибку
     */
    [[nodiscard]] virtual UIDsOpt load_uids(const Email& email) const noexcept = 0;

    /**
     * @brief Загрузка последнего UID письма
     * @param email Структура для выполнения запроса
     * @return Последний UID или ошибку
     */
    [[nodiscard]] virtual UIDOpt last_uid(const Email& email) const noexcept = 0;

    /**
     * @brief Загрузка письма электронной почты
     * @param email Структура для выполнения запроса
     * @param uid   Идентификатор письма, которое нужно загрузить
     * @return Данные письма или ошибку
     */
    [[nodiscard]] virtual EmailMsgExp fetch_email(const Email& email, int64_t uid) const noexcept = 0;
};
//----------------------------------------------------------------------------------------------------------------------
#endif // IMAILREQUEST_H
