//----------------------------------------------------------
#ifndef TELEGRAMCONTROLLER_H
#define TELEGRAMCONTROLLER_H
//----------------------------------------------------------
#include <memory>
//----------------------------------------------------------
#include "nlohmann/json_fwd.hpp"
//----------------------------------------------------------
#include "repository/IRepository.h"
#include "telegram/TelegramAPI.h"
//----------------------------------------------------------
using json = nlohmann::json;
//----------------------------------------------------------
using RequestOpt = std::optional<TelegramRequest>;
//----------------------------------------------------------

class TelegramController {

public:

    TelegramController(const std::string& token, std::shared_ptr<IRepository> repo);

    RequestOpt process(const TelegramResponse&& response, int64_t& last_msg_id);

private:

    std::string                  m_token;
    std::shared_ptr<IRepository> m_repo;

private:

    int64_t find_chat_id(const json& body_js, int idx, const std::string_view tag);

    std::shared_ptr<const Chat> register_chat(int64_t chat_id, int idx, const json& body_js);

    RequestOpt handle_reply_on_cmd(const json& body_js, int idx, std::shared_ptr<const Chat> chat);
    RequestOpt handle_cmd(const json& body_js, int idx, std::shared_ptr<const Chat> chat);

    TelegramRequest prepare_request_about(std::shared_ptr<const Chat> chat) const noexcept;
    TelegramRequest prepare_request_status(std::shared_ptr<const Chat> chat) const noexcept;
    TelegramRequest prepare_request_email(std::shared_ptr<const Chat> chat) const noexcept;
    TelegramRequest prepare_request_password(std::shared_ptr<const Chat> chat) const noexcept;

    TelegramRequest prepare_request_text(std::shared_ptr<const Chat> chat, std::string_view msg) const noexcept;
    TelegramRequest prepare_request_json(std::shared_ptr<const Chat> chat, const json& js_body) const noexcept;

    TelegramRequest process_cmd_value_email(const json& body_js, int idx, std::shared_ptr<const Chat> chat);
    TelegramRequest process_cmd_value_password(const json& body_js, int idx, std::shared_ptr<const Chat> chat);

    void strip_whitespace(std::string_view& text) const noexcept;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // TELEGRAMCONTROLLER_H
