//----------------------------------------------------------
#ifndef ITELEGRAMSENDER_H
#define ITELEGRAMSENDER_H
//----------------------------------------------------------
#include "TelegramAPI.h"
//----------------------------------------------------------

class ITelegramSender {

public:

    ITelegramSender()          = default;
    virtual ~ITelegramSender() = default;

    /**
     * @brief Отправка сообщения
     * @param request Данные запроса
     * @return TRUE, если сообщение отправлено. Иначе FALSE.
     */
    virtual bool send_msg(const TelegramRequest&& request) const noexcept = 0;
};
//----------------------------------------------------------------------------------------------------------------------
#endif // ITELEGRAMSENDER_H
