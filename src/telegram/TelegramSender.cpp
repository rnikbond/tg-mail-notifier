//----------------------------------------------------------
#include "httplib.h"
#include <spdlog/spdlog.h>
//----------------------------------------------------------
#include "TelegramSender.h"
//----------------------------------------------------------
namespace logger = spdlog;
//----------------------------------------------------------

bool TelegramSender::send_msg(const TelegramRequest&& request) const noexcept {

    std::string host = "https://api.telegram.org";

    std::unique_ptr<httplib::Client> http = std::make_unique<httplib::Client>(host);

    auto res = http->Post(request.url, request.body, request.content_type);
    if (!res) {
        auto err = res.error();
        logger::error("[TelegramSender::send_msg] error send msg in telegram: {}", httplib::to_string(err));
        return false;
    }

    logger::info("[TelegramSender::send_msg] message was sent successfully");
    return true;
}
//----------------------------------------------------------------------------------------------------------------------
