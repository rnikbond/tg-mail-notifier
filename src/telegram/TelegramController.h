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

/**
 * @brief Контроллер для работы с telegram ботом
 * 
 * Данный контроллер обрабатывает сообщения из telegram и возвращает структуру с данными для отправки в telegram чат.
 * Также этот класс предоставляет список доступных команд для telegram бота.
 */
class TelegramController {

public:

    TelegramController(const std::string& token, std::shared_ptr<IRepository> repo);

    RequestOpt commands() const;
    RequestOpt menu_buttons(int64_t chat_id) const;
    RequestOpt process(const TelegramResponse&& response, int64_t& last_msg_id);

private:

    std::string m_token;
    std::shared_ptr<IRepository> m_repo;

private:

    std::shared_ptr<const Chat> register_chat(int64_t chat_id, int idx, const json& body_js);

    RequestOpt handle_reply_on_cmd(const json& body_js, int idx, std::shared_ptr<const Chat> chat);
    RequestOpt handle_cmd(const json& body_js, int idx, std::shared_ptr<const Chat> chat);
    RequestOpt handle_button_click(const json& body_js, int idx, std::shared_ptr<const Chat> chat);

    [[nodiscard]] TelegramRequest prepare_request_about(std::shared_ptr<const Chat> chat) const noexcept;
    [[nodiscard]] TelegramRequest prepare_request_status(std::shared_ptr<const Chat> chat) const noexcept;
    [[nodiscard]] TelegramRequest prepare_request_add_email(std::shared_ptr<const Chat> chat) const noexcept;
    [[nodiscard]] TelegramRequest prepare_request_change_password(std::shared_ptr<const Chat> chat) const noexcept;
    [[nodiscard]] TelegramRequest prepare_request_set_password(std::shared_ptr<const Chat> chat, const std::string& email) const noexcept;
    [[nodiscard]] TelegramRequest prepare_request_clear_email(std::shared_ptr<const Chat> chat) const noexcept;

    [[nodiscard]] TelegramRequest prepare_request_buttons_change_email(std::shared_ptr<const Chat> chat) const noexcept;
    [[nodiscard]] TelegramRequest prepare_request_buttons_clear_email(std::shared_ptr<const Chat> chat) const noexcept;

    [[nodiscard]] TelegramRequest prepare_request_unknown(std::shared_ptr<const Chat> chat) const noexcept;
    [[nodiscard]] TelegramRequest prepare_request_internal_err(std::shared_ptr<const Chat> chat) const noexcept;

    [[nodiscard]] TelegramRequest prepare_request_text(std::shared_ptr<const Chat> chat, std::string_view msg) const noexcept;
    [[nodiscard]] TelegramRequest prepare_request_json(std::shared_ptr<const Chat> chat, const json& js_body) const noexcept;

    [[nodiscard]] TelegramRequest process_reply_email(const json& body_js, int idx, std::shared_ptr<const Chat> chat);
    [[nodiscard]] TelegramRequest process_reply_set_password(const json& body_js, int idx, std::shared_ptr<const Chat> chat);
    [[nodiscard]] TelegramRequest process_reply_change_password(const json& body_js, int idx, std::shared_ptr<const Chat> chat);
    [[nodiscard]] TelegramRequest process_reply_clear_email(const json& body_js, int idx, std::shared_ptr<const Chat> chat);
    [[nodiscard]] TelegramRequest process_cmd_clear_email_auth(std::shared_ptr<const Chat> chat) const noexcept;

    [[nodiscard]] std::optional<int64_t> check_email_auth(int64_t chat_id, const Email& email) const noexcept;

    std::optional<std::string> value_after_marker(const std::string& marker_start, const std::string& marker_end, const std::string& text) const noexcept;
    void strip_whitespace(std::string_view& text) const noexcept;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // TELEGRAMCONTROLLER_H
