//----------------------------------------------------------
#ifndef TELEGRAMSENDERFACTORY_H
#define TELEGRAMSENDERFACTORY_H
//----------------------------------------------------------
#include <functional>
#include <memory>
#include <string>
//----------------------------------------------------------
#include "telegram/ITelegramSender.h"
//----------------------------------------------------------

/**
 * @brief Класс-фабрика для создания объектов для отправки сообщений в telegram чат
 */
class TelegramSenderFactory {

    using SenderBuilder = std::function<std::unique_ptr<ITelegramSender>()>;
    using TgSender      = std::unique_ptr<ITelegramSender>;

public:

    TelegramSenderFactory()  = default;
    ~TelegramSenderFactory() = default;

public:

    static TgSender create();
    static void replace(SenderBuilder func);
    static void restore();

    static void configure(const std::string& host, const std::string& token);

private:

    static SenderBuilder& create_func();

    static std::string m_host;
    static std::string m_token;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // TELEGRAMSENDERFACTORY_H
