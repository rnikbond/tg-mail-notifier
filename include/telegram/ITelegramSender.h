//----------------------------------------------------------
#ifndef ITELEGRAMSENDER_H
#define ITELEGRAMSENDER_H
//----------------------------------------------------------
#include <string>
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
    virtual bool send_msg(int64_t chat_id, const std::string& body) const noexcept = 0;
};
//----------------------------------------------------------------------------------------------------------------------
#endif // ITELEGRAMSENDER_H
