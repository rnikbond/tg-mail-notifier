//----------------------------------------------------------
#ifndef TELEGRAMSENDER_H
#define TELEGRAMSENDER_H
//----------------------------------------------------------
#include "httplib.h"
//----------------------------------------------------------
#include "telegram/ITelegramSender.h"
#include "telegram/TelegramAPI.h"
//----------------------------------------------------------

/**
 * @brief Класс для отправки сообщения в telegram, реализающий интерфейс ITelegramSender
 * 
 * Данный класс должен создаваться через фибрику объектов.
 * @sa TelegramSenderFactory
 */
class TelegramSender : public ITelegramSender {

private:

    TelegramSender(const std::string& host, const std::string& token);
    friend class TelegramSenderFactory;

public:

    ~TelegramSender() = default;

public:

    virtual bool send_msg(int64_t chat_id, const std::string& body) const noexcept override;
    virtual bool send_msg(const TelegramRequest& request) const noexcept override;

private:

    std::string m_host;
    std::string m_token;

private:

    std::optional<httplib::Error> execute(const std::string& url, const std::string& body, const std::string& content_type, int64_t chat_id) const;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // TELEGRAMSENDER_H
