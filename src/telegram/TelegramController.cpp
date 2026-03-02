//----------------------------------------------------------
#define JSON_DIAGNOSTICS 1 //: Включение детальной информации json::exception
#include "nlohmann/json.hpp"
//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "../mail/MailRequestFactory.h"
#include "chat/Chat.h"
//----------------------------------------------------------
#include "TelegramController.h"
//----------------------------------------------------------

namespace {
/**
 * @brief  Поддерживаемые команды
 */
enum class Commands {
    None,           ///< Отсутствие занчения
    Start,          ///< Команда "/start"
    About,          ///< Команда "/about"
    Status,         ///< Команда "/status"
    Email,          ///< Команда "/email"
    Password,       ///< Команда "/password"
    ClearEmailAuth, ///< Команда "/clear_email_auth"
};
//----------------------------------------------------------------------------------------------------------------------

const std::unordered_map<std::string, Commands> g_commands_map = {
    {"/start", Commands::Start},
    {"/about", Commands::About},
    {"/status", Commands::Status},
    {"/email", Commands::Email},
    {"/password", Commands::Password},
    {"/clear_email_auth", Commands::ClearEmailAuth},
};
//----------------------------------------------------------------------------------------------------------------------

/**
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

/**
 * @brief Конструктор контроллера для работы с telegram
 * @param token Токен бота
 * @param repo  Указатель на репозиторий
 * 
 * @throw std::invalid_argument Выбрасывается, если \a token пуст или \a repo не создан
 */
TelegramController::TelegramController(const std::string& token, std::shared_ptr<IRepository> repo)
    : m_repo(repo)
    , m_token(token) {

    if (token.empty()) {
        throw std::invalid_argument("telegram token is empty");
    }

    if (!repo) {
        throw std::invalid_argument("invalid repository");
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Получение запроса для отправки команд в telegram бот
 * @return Запрос в telegram для инициализации списка команд
 */
RequestOpt TelegramController::commands() const {

    auto cmd_text = [&](Commands cmd) -> std::string {
        std::string text = find_command_text(cmd);

        text = text.substr(1, text.length() - 1);
        return text;
    };

    // clang-format off
    json js_body = {
        { "commands", {
                {{"command", cmd_text(Commands::Status)        }, {"description", "ℹ️ Статус"}},
                {{"command", cmd_text(Commands::Email)         }, {"description", "📧 Изменить email"}},
                {{"command", cmd_text(Commands::Password)      }, {"description", "🔑 Изменить пароль Email"}},
                {{"command", cmd_text(Commands::About)         }, {"description", "❔ Обо мне"}},
                {{"command", cmd_text(Commands::ClearEmailAuth)}, {"description", "🗑 Очистить Email и пароль"}},
            }
        }
    };
    // clang-format on

    constexpr std::string_view url = "/bot{}/setMyCommands";

    TelegramRequest request;
    request.url          = std::format(url, m_token);
    request.body         = js_body.dump();
    request.content_type = "application/json";
    request.chat_id      = 0;

    return request;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Обработка ответа из telegram
 * @param[in]  response    Данные ответа
 * @param[out] last_msg_id Идентификатор последнего прочитанного сообщения
 * @return Данные для отправки запроса в telegram, или nullopt, если это сообщение проигнорировано
 * 
 * @throw std::runtime_error Выбрасывается в случаях:
 *  - ошибки при чтении JSON из response
 *  - ошибке работы с репозиторием
 */
RequestOpt TelegramController::process(const TelegramResponse&& response, int64_t& last_msg_id) {

    try {
        json body_js = json::parse(response.body);

        //: Обрабатка только последнего сообщения
        int idx = body_js["result"].size() - 1;
        if (idx < 0) {
            return std::nullopt;
        }

        int64_t update_id = body_js["result"][idx]["update_id"];
        if (last_msg_id == update_id) {
            log_info("now new messege from telegram");
            return std::nullopt;
        }

        //: Запоминаем идентификатор сообщения
        last_msg_id = update_id;

        int64_t chat_id = body_js["result"][idx]["message"]["chat"]["id"];

        std::shared_ptr<const Chat> chat;
        { //: Поиск/регистрация чата
            auto chat_res = m_repo->find_chat(chat_id);
            if (chat_res.has_value()) {
                chat = std::move(chat_res.value());
            } else {
                chat = register_chat(chat_id, idx, body_js);
                log_info("new chat registered. chat_id={}, username={}", chat->chat_id, chat->username);
            }
        }

        if (body_js["result"][idx]["message"].contains("reply_to_message")) {
            return handle_reply_on_cmd(body_js, idx, chat);
        } else {
            return handle_cmd(body_js, idx, chat);
        }
    } catch (const json::out_of_range& ex) {
        log_error("JSON key not exists: {}", ex.what());
        throw std::runtime_error("invalid JSON");
    } catch (const json::exception& ex) {
        log_error("error parse JSON: {}", ex.what());
        throw std::runtime_error("invalid JSON");
    } catch (const std::exception& ex) {
        log_error("error process: {}", ex.what());
        throw std::runtime_error("failed process response");
    }

    return std::nullopt;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Регистрация нового чата
 * @param chat_id Идентификатор чата
 * @param idx     Индекс сообщения из массива JSON: ["result"]
 * @param body_js Данные в виде JSON объекта
 * @return Указатель на созданный чат
 * 
 * @throw std::runtime_error Выбрасывается в случае ошибки при чтении JSON или при ошибке регистрации чата в репозитории
 */
std::shared_ptr<const Chat> TelegramController::register_chat(int64_t chat_id, int idx, const json& body_js) {

    Chat chat;
    chat.chat_id = chat_id;

    auto chat_json = body_js["result"][idx]["message"]["chat"];

    if (chat_json.contains("username")) {
        chat.username = chat_json["username"];
    }
    if (chat_json.contains("first_name")) {
        chat.first_name = chat_json["first_name"];
    }
    if (chat_json.contains("last_name")) {
        chat.last_name = chat_json["last_name"];
    }

    auto res = m_repo->create_chat(chat);
    if (!res.has_value()) {
        throw std::runtime_error(std::format("failed create chat in repository: {}", Errors::to_string(res.error())));
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
 * @throw std::runtime_error Выбрасывается в случае ошибки при чтении JSON
 * 
 * Изменение данных реализовано через ответ на сообщение
 */
RequestOpt TelegramController::handle_reply_on_cmd(const json& body_js, int idx, std::shared_ptr<const Chat> chat) {

    std::string reply_text = body_js["result"][idx]["message"]["reply_to_message"]["text"];

    size_t pos = reply_text.find('\n');
    if (pos == std::string::npos) {
        return prepare_request_unknown(chat);
    }

    std::string command_text = reply_text.substr(0, pos);
    if (!g_commands_map.contains(command_text)) {
        return prepare_request_unknown(chat);
    }

    TelegramRequest request;
    Commands cmd = g_commands_map.at(command_text);
    switch (cmd) {
        case Commands::Email:
            return process_reply_email(body_js, idx, chat);
        case Commands::Password:
            return process_reply_password(body_js, idx, chat);
        default:
            log_error("no case for command: {}", find_command_text(cmd));
            return prepare_request_unknown(chat);
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Обработка команды из telegram
 * @param body_js Данные в виде JSON объекта
 * @param idx     Индекс сообщения из массива JSON: ["result"]
 * @param chat    Указатель на чат
 * @return Данные для отправки в telegram или nullopt
 * 
 * @throw std::runtime_error Выбрасывается в случае ошибки при чтении JSON
 */
RequestOpt TelegramController::handle_cmd(const json& body_js, int idx, std::shared_ptr<const Chat> chat) {

    std::string command_text = body_js["result"][idx]["message"]["text"];
    if (!g_commands_map.contains(command_text)) {
        return prepare_request_unknown(chat);
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

            if (chat->emails.empty() || chat->emails.at(0).address.empty()) {
                std::string text = std::format("Сначала нужно указать Email через команду: {}\n", find_command_text(Commands::Email));
                return prepare_request_text(chat, text);
            }

            return prepare_request_password(chat);
        case Commands::ClearEmailAuth:
            return process_cmd_clear_email_auth(chat);
        default:
            log_error("no case for command: {}", command_text);
            return prepare_request_unknown(chat);
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
 * 
 * @throw json::out_of_range Выбрасывается, если нет одного из ключей: result/i/message/text
 */
TelegramRequest TelegramController::process_reply_email(const json& body_js, int idx, std::shared_ptr<const Chat> chat) {

    std::string      text  = body_js["result"][idx]["message"]["text"];
    std::string_view value = text;
    strip_whitespace(value);

    Email email;
    if (!chat->emails.empty()) {
        email = chat->emails.at(0);
    }

    email.address = value;
    if (!email.address.empty() && !email.password.empty()) {
        auto res = check_email_auth(chat->chat_id, email);
        if (!res) {
            return prepare_request_text(chat, "❌ Ошибка авторизации на почте: некорректный email или пароль");
        }

        email.last_uid = res.value();
        log_info("email successfully registered. email={}, last_uid={}", email.address, email.last_uid);
    }

    auto err = Errors::Repository::OK;

    if (email.id >= 0) {
        auto res = m_repo->update_email(chat->chat_id, email);
        if (res.has_value()) {
            log_info("email address successfully updated. chat_id={}, email={}", chat->chat_id, email.address);
        } else {
            err = std::move(res.error());
        }
    } else {
        auto res = m_repo->append_email(chat->chat_id, email);
        if (res.has_value()) {
            log_info("email address successfully added. chat_id={}, email={}", chat->chat_id, email.address);
        } else {
            err = std::move(res.error());
        }
    }

    switch (err) {
        case Errors::Repository::OK:
            break;
        case Errors::Repository::AlreadyExists:
            return prepare_request_text(chat, "❗️ Такое email уже добавлен");
        case Errors::Repository::InvalidEmail:
            return prepare_request_text(chat, "❗️ Некорректный email");
        default:
            log_error("failed update email. chat id={}, error: {}", chat->chat_id, Errors::to_string(err));
            return prepare_request_internal_err(chat);
    }

    //: Отправляем сразу запрос на ввод пароля, если ввели только email
    if (!email.address.empty() && email.password.empty()) {
        return prepare_request_password(chat);
    }

    if (!email.address.empty() && !email.password.empty()) {
        std::string text = std::format("✅ Email настроен.\n"
                                       "Как появятся новые письма, буду пересылать их в этот чат"
                                       "\n\n"
                                       "Для проверки состояния отправь мне команду: {}",
                                       find_command_text(Commands::Status));
        return prepare_request_text(chat, text);
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
 *
 * @throw json::out_of_range Выбрасывается, если нет одного из ключей: result/i/message/text
 */
TelegramRequest TelegramController::process_reply_password(const json& body_js, int idx, std::shared_ptr<const Chat> chat) {

    if (chat->emails.empty() || chat->emails.at(0).address.empty()) {
        std::string text = std::format("Сначала нужно указать Email через команду: {}\n", find_command_text(Commands::Email));
        return prepare_request_text(chat, text);
    }

    std::string      text  = body_js["result"][idx]["message"]["text"];
    std::string_view value = text;
    strip_whitespace(value);

    Email email    = chat->emails.at(0);
    email.password = value;
    if (!email.password.empty()) {
        auto res = check_email_auth(chat->chat_id, email);
        if (!res.has_value()) {
            std::string text = std::format("❌ Ошибка авторизации на почте: некорректный пароль."
                                           "\n\n"
                                           "Исправьте {} или выполните команду {} снова",
                                           find_command_text(Commands::Email),
                                           find_command_text(Commands::Password));
            return prepare_request_text(chat, text);
        }

        email.last_uid = res.value();

        log_info("email successfully registered: email={}, last_uid={}", email.address, email.last_uid);
    }

    auto res = m_repo->update_email(chat->chat_id, email);
    if (!res.has_value()) {
        log_error("failed update password for email. chat id={}, error: {}", chat->chat_id, Errors::to_string(res.error()));
        return prepare_request_internal_err(chat);
    }

    std::string msg_text = std::format("✅ Email настроен.\n"
                                       "Как появятся новые письма, буду пересылать их в этот чат"
                                       "\n\n"
                                       "Для проверки состояния отправь мне команду: {}",
                                       find_command_text(Commands::Status));
    return prepare_request_text(chat, msg_text);
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Обработка ответа на команду "/clear_email_auth"
 * @param chat Указатель на чат
 * @return Данные для отправки в telegram или nullopt
 */
TelegramRequest TelegramController::process_cmd_clear_email_auth(std::shared_ptr<const Chat> chat) const noexcept {

    for (int64_t email_id : chat->emails | std::views::keys) {
        bool ok = m_repo->delete_email(chat->chat_id, email_id);
        if (!ok) {
            log_error("failed clear email in repository. chat id={}", chat->chat_id);
            return prepare_request_internal_err(chat);
        }
    }

    log_info("success clear email in repoisitory. chat_id={}", chat->chat_id);
    return prepare_request_text(chat, "✅ Данные email очищены");
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Подготовка запроса на команду "/about"
 * @param chat Указатель на чат
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_about(std::shared_ptr<const Chat> chat) const noexcept {

    const std::string text = std::format("<b>Привет! Я твой персональный почтовый фильтр</b> 📨"
                                         "\n\n"
                                         "Я буду следить за твоим ящиком и мгновенно пришлю уведомление, "
                                         "как только придет письмо с важными для тебя словами.\n"
                                         "Больше никакого спама, только то, что ты ждешь!\n\n"
                                         "Что мне нужно для старта:\n"
                                         "📧 <b>Почта</b> — адрес, который будем мониторить.\n"
                                         "🔑 <b>Пароль</b> — это пароль приложения.\n"
                                         "<i>Это не Ваш пароль от почты. Мне он не подойдет.\n"
                                         "Нужно создать специальный \"пароль приложения\" в личном кабинете вашей почты.</i>\n"
                                         "\n\n"
                                         "Для настройки или изменения данных используй меню команд."
                                         "\n"
                                         "Например: {}",
                                         find_command_text(Commands::Email));

    return prepare_request_text(chat, text);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Подготовка запроса на команду "/about"
 * @param chat Указатель на чат
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_status(std::shared_ptr<const Chat> chat) const noexcept {

    std::string status_emails;

    if (chat->emails.empty()) {
        status_emails = "❌ Почта не указана";
    } else {
        for (const Email& email : chat->emails | std::views::values) {
            status_emails += std::format("📧 Почта: {}\n🔑 Пароль: {}\n👁‍🗨 Статус: {}",
                                         email.address,
                                         (email.password.empty() ? "❌" : "✅"),
                                         email.last_uid >= 0 ? "✅ Сканирование работает" : "❌ Сканирование не работает");
        }
    }

    std::string text = std::format("<pre>{}</pre>", status_emails);
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
 * @brief Подготовка запроса о неизвестном вводе
 * @param chat Указатель на чат
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_unknown(std::shared_ptr<const Chat> chat) const noexcept {

    return prepare_request_text(chat, "Я понимаю только команды из меню и ответы на команды\n\nИспользуй доступные команды в меню");
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Подготовка запроса о внутренней ошибе
 * @param chat Указатель на чат
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_internal_err(std::shared_ptr<const Chat> chat) const noexcept {
    return prepare_request_text(chat, "Что-то поломалось ☹️\nУже чинят");
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
    js_body["chat_id"]      = chat->chat_id;
    js_body["text"]         = msg;
    js_body["parse_mode"]   = "HTML";
    js_body["reply_markup"] = {{"remove_keyboard", true}};

    // if (!is_notify) {
    //     body["disable_notification"] = true;
    // }

    TelegramRequest request;
    request.url          = std::format(url, m_token);
    request.body         = js_body.dump();
    request.content_type = "application/json";
    request.chat_id      = chat->chat_id;

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
    request.chat_id      = chat->chat_id;

    return request;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Проверка авторизации email
 * @param chat_id Идентификатор чата
 * @param email   Данные Email
 * @return UID последнего сообщения или nullopt, если не удалось получить
 */
std::optional<int64_t> TelegramController::check_email_auth(int64_t chat_id, const Email& email) const noexcept {

    auto res = MailRequestFactory::create()->last_uid(email);
    if (res.has_value()) {
        log_info("success load last mail UID. chat_id={}, email={}, UID: {}", chat_id, email.address, res.value());
        return res.value();
    }

    switch (res.error()) {
        case Errors::Mail::Auth:
            log_info("failed load last mail UID. chat_id={}, email={}, error: {}", chat_id, email.address, Errors::to_string(res.error()));
            break;
        default:
            log_warn("failed load last mail UID. chat_id={}, email={}, error: {}", chat_id, email.address, Errors::to_string(res.error()));
            break;
    }

    return std::nullopt;
}
//----------------------------------------------------------------------------------------------------------------------

/**
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
