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

class IMailRequest {

public:

    IMailRequest()          = default;
    virtual ~IMailRequest() = default;

public:

    using UIDsResult    = std::expected<std::vector<int64_t>, Errors::Mail>;
    using UIDResult     = std::expected<int64_t, Errors::Mail>;
    using MailMsgResult = std::expected<std::string, Errors::Mail>;

public:

    /**
     * @brief Загрузка идентификаторов (UID) новых писем
     * @param email Информация об электронной почте для выполнения запроса
     * @return Отсортированный список идентификаторов новых писем, или ошибку
     */
    [[nodiscard]] virtual UIDsResult load_uids(const Email& email) const noexcept = 0;

    /**
     * @brief Загрузка идентификатора последнего письма
     * @param email Информация об электронной почте для выполнения запроса
     * @return Идентификатор последнего письма или ошибку
     */
    [[nodiscard]] virtual UIDResult last_uid(const Email& email) const noexcept = 0;

    /**
     * @brief Загрузка письма электронной почты
     * @param email Информация об электронной почте для выполнения запроса
     * @param uid   Идентификатор письма, которое нужно загрузить
     * @return Данные письма или ошибку
     */
    [[nodiscard]] virtual MailMsgResult fetch_email(const Email& email, int64_t uid) const noexcept = 0;
};
//----------------------------------------------------------------------------------------------------------------------
#endif // IMAILREQUEST_H
