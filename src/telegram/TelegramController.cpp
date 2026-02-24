//----------------------------------------------------------
#include <regex>
//----------------------------------------------------------
#include "nlohmann/json.hpp"
#include <spdlog/spdlog.h>
//----------------------------------------------------------
#include "TelegramController.h"
//----------------------------------------------------------
namespace logger = spdlog;
//----------------------------------------------------------

namespace {

/**
 * @brief  Поддерживаемые команды
 */
enum Commands {
    None,     ///< Отсутствие занчения
    Start,    ///< Команда "/start"
    About,    ///< Команда "/about"
    Status,   ///< Команда "/status"
    Email,    ///< Команда "/email"
    Password, ///< Команда "/password"
};
//----------------------------------------------------------------------------------------------------------------------

const std::unordered_map<std::string, Commands> g_commands_map = {
    {"/start", Commands::Start},
    {"/about", Commands::About},
    {"/status", Commands::Status},
    {"/email", Commands::Email},
    {"/password", Commands::Password},
};
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Поиск текстового значения команды
 * @param cmd Идентификатор команды
 * @return Текстовое занчение команды
 */
std::string find_command_text(Commands cmd) {
    std::string text;
    for (auto& [cmd_text, cmd_id] : g_commands_map) {
        if (cmd_id == cmd) {
            text = cmd_text;
            break;
        }
    }
    return text;
}
//----------------------------------------------------------------------------------------------------------------------

} // namespace
//----------------------------------------------------------------------------------------------------------------------

TelegramController::TelegramController(const std::string& token, std::shared_ptr<IRepository> repo)
    : m_repo(repo)
    , m_token(token) {

    if (token.empty()) {
        throw std::runtime_error("telegram token is empty");
    }

    if (!repo) {
        throw std::runtime_error("invalid repository");
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Обработка ответа из telegram
 * @param[in]  response    Данные ответа
 * @param[out] last_msg_id Идентификатор последнего прочитанного сообщения
 * @return Данные для отправки запроса в telegram, или nullopt, если это сообщение проигнорировано
 */
RequestOpt TelegramController::process(const TelegramResponse&& response, int64_t& last_msg_id) {

    json body_js;
    try {
        body_js = json::parse(response.body);
    } catch (json::parse_error& e) {
        logger::error("[TelegramController::process] failed parse JSON:\n"
                      "error: {}\n"
                      "error id: {}\n,"
                      "position: {}",
                      e.what(),
                      e.id,
                      e.byte);
        throw;
    }

    logger::debug(body_js.dump(4));

    if (!body_js.contains("result")) {
        throw std::runtime_error("[TelegramController::process] invalid JSON: does not contains 'result'");
    }

    //: Обрабатывае только последнее сообщение
    int idx = body_js["result"].size() - 1;
    if (idx < 0) {
        return std::nullopt;
    }

    if (!body_js["result"][idx].contains("update_id")) {
        throw std::runtime_error("[TelegramController::process] invalid JSON: does not contains 'update_id' in [result][i]");
    }

    int64_t update_id = body_js["result"][idx]["update_id"];
    if (last_msg_id == update_id) {
        //: Это сообщение уже обрабатывалось
        return std::nullopt;
    }

    //: Запоминаем идентификатор обработанного сообщения
    // if (last_msg_id == 0) {
    //     last_msg_id = update_id;
    //     return std::nullopt;
    // }
    last_msg_id = update_id;

    int64_t chat_id;

    if (body_js["result"][idx].contains("message")) {
        //: Обработка нового сообщения, которое написал пользователь
        chat_id = find_chat_id(body_js, idx, "message");
    } else if (body_js["result"][idx].contains("edited_message")) {
        //: Обработка отредактированного сообщения
        chat_id = find_chat_id(body_js, idx, "edited_message");
    } else {
        //: непонятно, как обрабатывать
        logger::warn("[TelegramController::process] unknown JSON: \n{}", body_js.dump(4));
        return std::nullopt;
    }

    std::shared_ptr<const Chat> chat;

    { //: Поиск/регистрация чата
        auto res = m_repo->find({chat_id});

        if (res.has_value()) {
            auto chats_vec = res.value();
            if (chats_vec.size() > 0) {
                chat = res.value().at(0);
            }
        }

        if (!chat) {
            chat = register_chat(chat_id, idx, body_js);
        }
    }

    if (body_js["result"][idx]["message"].contains("reply_to_message")) {
        return handle_reply_on_cmd(body_js, idx, chat);
    } else {
        return handle_cmd(body_js, idx, chat);
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Поиск идентификтаора чата в json для тега
 * @param body_js JSON объект
 * @param idx     Индекс сообщения из массива JSON: ["result"]
 * @param tag     Тег
 * @return Идентификатор чата
 */
int64_t TelegramController::find_chat_id(const json& body_js, int idx, const std::string_view tag) {
    if (!body_js["result"][idx][tag].contains("chat")) {
        throw std::runtime_error("[TelegramController::find_chat_id] invalid JSON: does not contains 'chat' in [result][i][edited_message]");
    }
    if (!body_js["result"][idx][tag]["chat"].contains("id")) {
        throw std::runtime_error("[TelegramController::find_chat_id] invalid JSON: does not contains 'id' in [result][i][edited_message][chat]");
    }

    return body_js["result"][idx][tag]["chat"]["id"];
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Регистрация нового чата
 * @param chat_id Идентификатор чата
 * @param idx     Индекс сообщения из массива JSON: ["result"]
 * @param body_js Данные в виде JSON объекта
 * @return Указатель на созданный чат
 */
std::shared_ptr<const Chat> TelegramController::register_chat(int64_t chat_id, int idx, const json& body_js) {

    if (!body_js["result"][idx]["message"]["chat"].contains("username")) {
        throw std::runtime_error("[TelegramController::register_chat] invalid JSON: does not contains 'username' in [result][i][message][chat]");
    }

    Chat chat;
    chat.chat_id = chat_id;

    if (body_js["result"][idx]["message"]["chat"].contains("username")) {
        chat.username = body_js["result"][idx]["message"]["chat"]["username"];
    }
    if (body_js["result"][idx]["message"]["chat"].contains("first_name")) {
        chat.first_name = body_js["result"][idx]["message"]["chat"]["first_name"];
    }
    if (body_js["result"][idx]["message"]["chat"].contains("last_name")) {
        chat.last_name = body_js["result"][idx]["message"]["chat"]["last_name"];
    }

    auto res = m_repo->create(chat);
    if (!res.has_value()) {
        throw std::runtime_error("[TelegramController::register_chat] failed create chat in repository");
    }

    return res.value();
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Обработка ответа на сообщение
 * @param body_js Данные в виде JSON объекта
 * @param idx     Индекс сообщения из массива JSON: ["result"]
 * @param chat    Указатель на чат
 * @return Данные для отправки в telegram или nullopt
 * 
 * Изменение данных реализовано через ответ на сообщение
 */
RequestOpt TelegramController::handle_reply_on_cmd(const json& body_js, int idx, std::shared_ptr<const Chat> chat) {

    if (!body_js["result"][idx]["message"]["reply_to_message"].contains("text")) {
        throw std::runtime_error("[TelegramController::handle_reply_on_cmd] invalid JSON: does not contains 'text' in [result][i][message][reply_to_message]");
    }

    if (!body_js["result"][idx]["message"].contains("text")) {
        throw std::runtime_error("[TelegramController::handle_reply_on_cmd] invalid JSON: does not contains 'text' in [result][i][message]");
    }

    std::string reply_text = body_js["result"][idx]["message"]["reply_to_message"]["text"];

    size_t pos = reply_text.find('\n');
    if (pos == std::string::npos) {
        throw std::runtime_error(std::format("[TelegramController::handle_reply_on_cmd] invalid reply: {}", reply_text));
    }

    std::string command_text = reply_text.substr(0, pos);
    if (!g_commands_map.contains(command_text)) {
        throw std::runtime_error(std::format("[TelegramController::handle_reply_on_cmd] unknown command: {}", command_text));
    }

    Commands cmd = g_commands_map.at(command_text);
    switch (cmd) {
        case Commands::Email:
            return process_cmd_value_email(body_js, idx, chat);
        case Commands::Password:
            return process_cmd_value_password(body_js, idx, chat);
        default:
            throw std::runtime_error(std::format("[TelegramController::handle_cmd] no case for command: {}", command_text));
    }

    return std::nullopt;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Обработка команды из telegram
 * @param body_js Данные в виде JSON объекта
 * @param idx     Индекс сообщения из массива JSON: ["result"]
 * @param chat    Указатель на чат
 * @return Данные для отправки в telegram или nullopt
 */
RequestOpt TelegramController::handle_cmd(const json& body_js, int idx, std::shared_ptr<const Chat> chat) {

    if (!body_js["result"][idx]["message"].contains("text")) {
        throw std::runtime_error("[TelegramController::handle_cmd] invalid JSON: does not contains 'text' in [result][i][message]");
    }

    std::string command_text = body_js["result"][idx]["message"]["text"];
    if (!g_commands_map.contains(command_text)) {
        if (command_text.length() > 1 && command_text[0] == '/') {
            return prepare_request_text(chat, "Такого я ещё не умею ☹️");
        } else {
            return prepare_request_text(chat, "Это не похоже на команду 😞");
        }
    }

    Commands cmd = g_commands_map.at(command_text);
    switch (cmd) {
        case Commands::Start:
        case Commands::About:
            return prepare_request_about(chat);
        case Commands::Status:
            return prepare_request_status(chat);
        case Commands::Email:
            return prepare_request_email(chat);
        case Commands::Password:
            return prepare_request_password(chat);
        default:
            logger::error("[TelegramController::handle_cmd] no case for command: {}", command_text);
            throw std::runtime_error(std::format("[TelegramController::handle_cmd] no case command: {}", command_text));
    }

    return std::nullopt;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Обработка ответа на команду "/email"
 * @param body_js Данные в виде JSON объекта
 * @param idx     Индекс сообщения из массива JSON: ["result"]
 * @param chat    Указатель на чат
 * @return Данные для отправки в telegram или nullopt
 */
TelegramRequest TelegramController::process_cmd_value_email(const json& body_js, int idx, std::shared_ptr<const Chat> chat) {

    std::string      text  = body_js["result"][idx]["message"]["text"];
    std::string_view value = text;
    strip_whitespace(value);

    const std::regex pattern(R"(^[\w\.-]+@[\w\.-]+\.\w{2,4}$)");
    if (!std::regex_match(static_cast<std::string>(value), pattern)) {
        return prepare_request_text(chat, "Некорректный Email");
    }

    Chat chat_edit          = *chat;
    chat_edit.email.address = value;

    bool ok = m_repo->update(chat_edit);
    if (!ok) {
        throw std::runtime_error(std::format("[TelegramController::process_cmd_value_email] failed update chat. chat id: {}", chat->chat_id));
    }

    return prepare_request_text(chat, "✅ Записал");
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Обработка ответа на команду "/password"
 * @param body_js Данные в виде JSON объекта
 * @param idx     Индекс сообщения из массива JSON: ["result"]
 * @param chat    Указатель на чат
 * @return Данные для отправки в telegram или nullopt
 */
TelegramRequest TelegramController::process_cmd_value_password(const json& body_js, int idx, std::shared_ptr<const Chat> chat) {

    std::string      text  = body_js["result"][idx]["message"]["text"];
    std::string_view value = text;
    strip_whitespace(value);

    Chat chat_edit           = *chat;
    chat_edit.email.password = value;

    bool ok = m_repo->update(chat_edit);
    if (!ok) {
        throw std::runtime_error(std::format("[TelegramController::process_cmd_value_password] failed update chat. chat id: {}", chat->chat_id));
    }

    return prepare_request_text(chat, "✅ Записал");
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Подготовка запроса на команду "/about"
 * @param chat Указатель на чат
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_about(std::shared_ptr<const Chat> chat) const noexcept {

    constexpr std::string_view text = "<b>Привет! Я твой персональный почтовый фильтр</b> 📨"
                                      "\n\n"
                                      "Я буду следить за твоим ящиком и мгновенно пришлю уведомление, "
                                      "как только придет письмо с важными для тебя словами.\n"
                                      "Больше никакого спама, только то, что ты ждешь!\n\n"
                                      "Что мне нужно для старта:\n"
                                      "📧 <b>Email</b> — адрес, который будем мониторить.\n"
                                      "🔑 <b>Токен OAuth2 (пароль приложения)</b> — твой ключ безопасности.\n"
                                      "Например, для Яндекс.Почты нужно создать «Пароль приложения».\n"
                                      "🏷 <b>Ключевые слова</b> — Если оставить поле пустым, я буду присылать вообще все "
                                      "письма.\n\n"
                                      "Для настройки или изменения данных используй меню команд.";

    return prepare_request_text(chat, text);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Подготовка запроса на команду "/about"
 * @param chat Указатель на чат
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_status(std::shared_ptr<const Chat> chat) const noexcept {

    std::string text = std::format("<b>Cтатус</b>\n\n"
                                   "email: {}\n"
                                   "password: {}\n",
                                   (chat->email.address.empty() ? "❗️не указан" : chat->email.address),
                                   (chat->email.password.empty() ? "❗️не указан" : "✅"),
                                   "💱");

    return prepare_request_text(chat, text);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Подготовка запроса на команду "/about"
 * @param chat Указатель на чат
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_email(std::shared_ptr<const Chat> chat) const noexcept {

    json js_body;
    js_body["chat_id"]      = chat->chat_id;
    js_body["text"]         = std::format("{}\nВведите Email:", find_command_text(Commands::Email));
    js_body["reply_markup"] = {{"force_reply", true}, {"input_field_placeholder", "example@mail.com"}};

    return prepare_request_json(chat, js_body);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Подготовка запроса на команду "/about"
 * @param chat Указатель на чат
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_password(std::shared_ptr<const Chat> chat) const noexcept {

    json js_body;
    js_body["chat_id"]      = chat->chat_id;
    js_body["text"]         = std::format("{}\nВведите пароль:", find_command_text(Commands::Password));
    js_body["reply_markup"] = {{"force_reply", true}};

    return prepare_request_json(chat, js_body);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Подготовка запроса на основе текста
 * @param chat Указатель на чат
 * @param msg  Текст сообщений, который будет отправлен в запроса
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_text(std::shared_ptr<const Chat> chat, std::string_view msg) const noexcept {

    constexpr std::string_view url = "/bot{}/sendMessage";

    json js_body;
    js_body["chat_id"]    = chat->chat_id;
    js_body["text"]       = msg;
    js_body["parse_mode"] = "HTML";

    // if (!is_notify) {
    //     body["disable_notification"] = true;
    // }

    TelegramRequest request;
    request.url          = std::format(url, m_token);
    request.body         = js_body.dump();
    request.content_type = "application/json";

    return request;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Подготовка запроса на основе JSON
 * @param chat    Указатель на чат
 * @param js_body JSON
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_json(std::shared_ptr<const Chat> chat, const json& js_body) const noexcept {

    constexpr std::string_view url = "/bot{}/sendMessage";

    TelegramRequest request;
    request.url          = std::format(url, m_token);
    request.body         = js_body.dump();
    request.content_type = "application/json";

    return request;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Удаление пробелов из текста
 * @param[in/out] text Текст
 */
void TelegramController::strip_whitespace(std::string_view& text) const noexcept {

    const char* whitespace = " ";

    size_t start_pos_no_space = text.find_first_not_of(whitespace);
    if (start_pos_no_space != std::string::npos) {
        text.remove_prefix(start_pos_no_space);
    }

    size_t end_pos_no_space = text.find_last_not_of(whitespace);
    text.remove_suffix(text.size() - (end_pos_no_space + 1));
}
//----------------------------------------------------------------------------------------------------------------------
