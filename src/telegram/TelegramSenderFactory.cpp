//----------------------------------------------------------
#include "TelegramSender.h"
//----------------------------------------------------------
#include "TelegramSenderFactory.h"
//----------------------------------------------------------

TelegramSenderFactory::TgSender TelegramSenderFactory::create() {

    return create_func()();
}
//----------------------------------------------------------------------------------------------------------------------

void TelegramSenderFactory::replace(SenderBuilder func) {
    create_func() = std::move(func);
}
//----------------------------------------------------------------------------------------------------------------------

void TelegramSenderFactory::restore() {

    create_func() = []() { return std::make_unique<TelegramSender>(); };
}
//----------------------------------------------------------------------------------------------------------------------

TelegramSenderFactory::SenderBuilder& TelegramSenderFactory::create_func() {

    static SenderBuilder func = []() { return std::make_unique<TelegramSender>(); };
    return func;
}
//----------------------------------------------------------------------------------------------------------------------
