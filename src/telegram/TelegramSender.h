//----------------------------------------------------------
#ifndef TELEGRAMSENDER_H
#define TELEGRAMSENDER_H
//----------------------------------------------------------
#include "telegram/ITelegramSender.h"
//----------------------------------------------------------

class TelegramSender : public ITelegramSender {
public:

    TelegramSender()  = default;
    ~TelegramSender() = default;

    virtual bool send_msg(const TelegramRequest&& request) const noexcept override;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // TELEGRAMSENDER_H
