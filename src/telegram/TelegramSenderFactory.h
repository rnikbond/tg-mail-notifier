//----------------------------------------------------------
#ifndef TELEGRAMSENDERFACTORY_H
#define TELEGRAMSENDERFACTORY_H
//----------------------------------------------------------
#include <functional>
//----------------------------------------------------------
#include "telegram/ITelegramSender.h"
//----------------------------------------------------------

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

private:

    static SenderBuilder& create_func();
};
//----------------------------------------------------------------------------------------------------------------------

#endif // TELEGRAMSENDERFACTORY_H
