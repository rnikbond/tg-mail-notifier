//----------------------------------------------------------
#include "httplib.h"
//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "TelegramSender.h"
//----------------------------------------------------------

bool TelegramSender::send_msg(const TelegramRequest&& request) const noexcept {

    std::string host = "https://api.telegram.org";

    std::unique_ptr<httplib::Client> http = std::make_unique<httplib::Client>(host);

    auto res = http->Post(request.url, request.body, request.content_type);
    if (!res) {
        auto err = res.error();
        log_error("error send msg in telegram: {}", httplib::to_string(err));
        return false;
    }

    log_info("message was sent successfully");
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
