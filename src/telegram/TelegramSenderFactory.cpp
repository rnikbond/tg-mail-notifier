//----------------------------------------------------------
#include "TelegramSender.h"
//----------------------------------------------------------
#include "TelegramSenderFactory.h"
//----------------------------------------------------------

std::string TelegramSenderFactory::m_host  = "";
std::string TelegramSenderFactory::m_token = "";
//----------------------------------------------------------------------------------------------------------------------

TelegramSenderFactory::TgSender TelegramSenderFactory::create() {

    return create_func()();
}
//----------------------------------------------------------------------------------------------------------------------

void TelegramSenderFactory::replace(SenderBuilder func) {
    create_func() = std::move(func);
}
//----------------------------------------------------------------------------------------------------------------------

void TelegramSenderFactory::restore() {

    create_func() = []() { return std::make_unique<TelegramSender>(m_host, m_token); };
}
//----------------------------------------------------------------------------------------------------------------------

TelegramSenderFactory::SenderBuilder& TelegramSenderFactory::create_func() {

    static SenderBuilder func = []() { return std::make_unique<TelegramSender>(m_host, m_token); };
    return func;
}
//----------------------------------------------------------------------------------------------------------------------

void TelegramSenderFactory::configure(const std::string& host, const std::string& token) {
    m_host  = host;
    m_token = token;
}
//----------------------------------------------------------------------------------------------------------------------
