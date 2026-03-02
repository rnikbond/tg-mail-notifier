//----------------------------------------------------------
#ifndef TELEGRAMSENDER_H
#define TELEGRAMSENDER_H
//----------------------------------------------------------
#include "telegram/ITelegramSender.h"
//----------------------------------------------------------

/**
 * @brief Класс для отправки сообщения в telegram, реализающий интерфейс ITelegramSender
 * 
 * Данный класс должен создаваться через фибрику объектов.
 * @sa TelegramSenderFactory
 */
class TelegramSender : public ITelegramSender {
public:

    TelegramSender(const std::string& host, const std::string& token);
    ~TelegramSender() = default;

    virtual bool send_msg(int64_t chat_id, const std::string& body) const noexcept override;

private:

    std::string m_host;
    std::string m_token;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // TELEGRAMSENDER_H
