//----------------------------------------------------------
#include "httplib.h"
#include "nlohmann/json.hpp"
//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "TelegramSender.h"
//----------------------------------------------------------
using json = nlohmann::json;
//----------------------------------------------------------

TelegramSender::TelegramSender(const std::string &host, const std::string &token)
    : m_host(host)
    , m_token(token) {

    if (m_host.empty()) {
        throw std::runtime_error("telegram host is empty");
    }

    if (m_token.empty()) {
        throw std::runtime_error("telegram token is empty");
    }
}
//----------------------------------------------------------------------------------------------------------------------

bool TelegramSender::send_msg(int64_t chat_id, const std::string &body) const noexcept {

    constexpr std::string_view url = "/bot{}/sendMessage";

    json js_body;
    js_body["chat_id"]    = chat_id;
    js_body["text"]       = body;
    js_body["parse_mode"] = "HTML";

    std::unique_ptr<httplib::Client> http = std::make_unique<httplib::Client>(m_host);

    //: Иногда почему-то сообщение не отправляется в telegram с 1-го раза.
    //: Делаем 3 попытки
    const int max_retries = 3;
    for (int attempt = 1; attempt <= max_retries; attempt++) {
        auto res = http->Post(std::format(url, m_token), js_body.dump(), "application/json");
        if (res) {
            log_info("message was sent successfully in telegram. chat_id: {}", chat_id);
            return true;
        }

        auto err = res.error();
        log_warn("failed send msg in telegram. chat_id: {}, attempt {}/{}. {}", attempt, max_retries, chat_id, httplib::to_string(err));
    }

    return false;
}
//----------------------------------------------------------------------------------------------------------------------
