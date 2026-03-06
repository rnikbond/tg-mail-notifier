//----------------------------------------------------------
#ifndef ITELEGRAMSENDER_H
#define ITELEGRAMSENDER_H
//----------------------------------------------------------
#include <string>
//----------------------------------------------------------
#include "telegram/TelegramAPI.h"
//----------------------------------------------------------

class ITelegramSender {

public:

    ITelegramSender()          = default;
    virtual ~ITelegramSender() = default;

    /**
     * @brief Запрос на отправку сообщения
     * @param chat_id Идентификатор telegram чата
     * @param body    Текстовое тело сообщения
     * @return TRUE, если сообщение успешно отправлено. Иначе FALSE.
     */
    virtual bool send_msg(int64_t chat_id, const std::string& body) const noexcept = 0;

    /**
     * @brief Запрос на отправку сообщения
     * @param request Заполненная структура для отправки сообщения
     * @return TRUE, если сообщение отправлено. Иначе FALSE.
     */
    virtual bool send_msg(const TelegramRequest& request) const noexcept = 0;

    /**
     * @brief Запрос на пересылку сообщения из электронной почты
     * @param chat_id Идентификатор telegram чата
     * @param body    Содержимое письма
     * @return TRUE, если сообщение успешно отправлено. Иначе FALSE.
     */
    virtual bool send_email_msg(int64_t chat_id, const std::string& body) const noexcept = 0;
};
//----------------------------------------------------------------------------------------------------------------------
#endif // ITELEGRAMSENDER_H
