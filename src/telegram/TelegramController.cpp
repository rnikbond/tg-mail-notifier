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
    AddEmail,       ///< Команда "/add_email"
    ChangePassword, ///< Команда "/change_password"
    ClearEmail,     ///< Команда "/clear_email"
    Buttons,        ///< Команда "/buttons"
};
//----------------------------------------------------------------------------------------------------------------------

const std::unordered_map<std::string, Commands> g_commands_map = {
    {"/start", Commands::Start},
    {"/about", Commands::About},
    {"/status", Commands::Status},
    {"/add_email", Commands::AddEmail},
    {"/change_password", Commands::ChangePassword},
    {"/clear_email", Commands::ClearEmail},
    {"/buttons", Commands::Buttons},
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

const std::string MARKER_EMAIL       = "\xE2\x80\x8B\xE2\x80\x8B"; // Невидимая метка для почты
const std::string MARKER_PASS        = "\xE2\x80\x8D\xE2\x80\x8D"; // Невидимая метка для пароля
const std::string MARKER_DEL_EMAIL   = "\xE2\x81\xA3";
const std::string MARKER_CHANGE_PASS = "\xE2\x80\x8C\xE2\x81\xA0";

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
                {{"command", cmd_text(Commands::AddEmail)      }, {"description", "📧 Добавить email"}},
                {{"command", cmd_text(Commands::ChangePassword)}, {"description", "🔑 Изменить пароль"}},
                {{"command", cmd_text(Commands::ClearEmail)    }, {"description", "❌ Удалить email"}},
                {{"command", cmd_text(Commands::Status)        }, {"description", "ℹ️ Статус"}},
                {{"command", cmd_text(Commands::About)         }, {"description", "❔ Обо мне"}},
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

        //log_info(body_js.dump(4));

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

        int64_t chat_id = -1;

        if (body_js["result"][idx].contains("callback_query")) {
            chat_id = body_js["result"][idx]["callback_query"]["message"]["chat"]["id"];
        } else {
            chat_id = body_js["result"][idx]["message"]["chat"]["id"];
        }

        std::shared_ptr<const Chat> chat;
        { //: Поиск/регистрация чата
            auto chat_res = m_repo->find_chat(chat_id);
            if (chat_res.has_value()) {
                chat = std::move(chat_res.value());
            } else {
                chat = register_chat(chat_id, idx, body_js);
                log_info("new chat registered. chat_id={}, username={}", chat->id, chat->username);
            }
        }

        if (body_js["result"][idx].contains("callback_query")) {
            return handle_button_click(body_js, idx, chat);
        } else if (body_js["result"][idx]["message"].contains("reply_to_message")) {
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
    chat.id = chat_id;

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

    if (reply_text.starts_with(MARKER_EMAIL)) {
        return process_reply_email(body_js, idx, chat);
    }

    if (reply_text.starts_with(MARKER_PASS)) {
        return process_reply_set_password(body_js, idx, chat);
    }

    if (reply_text.starts_with(MARKER_CHANGE_PASS)) {
        return process_reply_change_password(body_js, idx, chat);
    }

    if (reply_text.starts_with(MARKER_DEL_EMAIL)) {
        return process_reply_clear_email(body_js, idx, chat);
    }

    return prepare_request_unknown(chat);
}
//----------------------------------------------------------------------------------------------------------------------

RequestOpt TelegramController::handle_button_click(const json& body_js, int idx, std::shared_ptr<const Chat> chat) {

    std::string button_data = body_js["result"][idx]["callback_query"]["data"];

    if (button_data.starts_with(MARKER_CHANGE_PASS)) {

        auto email_opt = value_after_marker(MARKER_EMAIL, MARKER_EMAIL, button_data);
        if (!email_opt.has_value()) {
            return prepare_request_text(chat, std::format("не определил email"));
        }

        return prepare_request_set_password(chat, email_opt.value());

    } else if (button_data.starts_with(MARKER_DEL_EMAIL)) {

        auto email_opt = value_after_marker(MARKER_EMAIL, MARKER_EMAIL, button_data);
        if (!email_opt.has_value()) {
            return prepare_request_text(chat, std::format("не определил email"));
        }

        int64_t email_id = chat->find_email_id(email_opt.value());
        if (email_id < 0) {
            return prepare_request_text(chat, std::format("не определил email"));
        }

        auto err = Errors::Repository::OK;
        bool ok  = m_repo->delete_email(chat->id, email_id);
        if (ok) {
            return prepare_request_text(chat, "✅ email удалён");
        } else {
            return prepare_request_text(chat, "❗️ Такой email не найден");
        }
    }

    return prepare_request_unknown(chat);
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
        case Commands::AddEmail:

            if (chat->emails.size() == 5) {
                return prepare_request_text(chat, "Добавлено максимально количество email");
            }

            return prepare_request_add_email(chat);
        case Commands::ChangePassword:
            return prepare_request_buttons_change_email(chat);
        case Commands::ClearEmail:
            return prepare_request_buttons_clear_email(chat);
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
    email.address = value;

    auto err = Errors::Repository::OK;
    auto res = m_repo->append_email(chat->id, email);
    if (res.has_value()) {
        log_info("email address successfully added. chat_id={}, email={}", chat->id, email.address);
    } else {
        err = std::move(res.error());
    }

    switch (err) {
        case Errors::Repository::OK:
            break;
        case Errors::Repository::AlreadyExists:
            return prepare_request_text(chat, "❗️ Такое email уже добавлен");
        case Errors::Repository::InvalidEmail:
            return prepare_request_text(chat, "❗️ Некорректный email");
        default:
            log_error("failed update email. chat id={}, error: {}", chat->id, Errors::to_string(err));
            return prepare_request_internal_err(chat);
    }

    //: Отправляем сразу запрос на ввод пароля, если ввели только email
    return prepare_request_set_password(chat, email.address);
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
TelegramRequest TelegramController::process_reply_set_password(const json& body_js, int idx, std::shared_ptr<const Chat> chat) {

    if (chat->emails.empty()) {
        std::string text = std::format("Сначала нужно указать Email через команду: {}\n", find_command_text(Commands::AddEmail));
        return prepare_request_text(chat, text);
    }

    std::string reply_text = body_js["result"][idx]["message"]["reply_to_message"]["text"];
    auto        email_opt  = value_after_marker(MARKER_EMAIL, MARKER_EMAIL, reply_text);
    if (!email_opt.has_value()) {
        return prepare_request_text(chat, std::format("не определил email"));
    }

    int64_t email_id = chat->find_email_id(email_opt.value());
    if (email_id < 0) {
        return prepare_request_text(chat, std::format("не определил email"));
    }

    std::string      text  = body_js["result"][idx]["message"]["text"];
    std::string_view value = text;
    strip_whitespace(value);

    Email email    = chat->emails.at(email_id);
    email.password = value;
    if (!email.password.empty()) {
        auto res = check_email_auth(chat->id, email);
        if (!res.has_value()) {
            std::string text = std::format("❌ Ошибка авторизации: <b>Некорректный email или пароль</b>"
                                           "\n\n"
                                           "email: {}"
                                           "\n\n"
                                           "Удалите email {} или измените пароль {}",
                                           email.address,
                                           find_command_text(Commands::ClearEmail),
                                           find_command_text(Commands::ChangePassword));
            return prepare_request_text(chat, text);
        }

        email.last_uid = res.value();

        log_info("email successfully registered: email={}, last_uid={}", email.address, email.last_uid);
    }

    auto res = m_repo->update_email(chat->id, email);
    if (!res.has_value()) {
        log_error("failed update password for email. chat id={}, error: {}", chat->id, Errors::to_string(res.error()));
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

TelegramRequest TelegramController::process_reply_change_password(const json& body_js, int idx, std::shared_ptr<const Chat> chat) {

    if (chat->emails.empty()) {
        std::string text = std::format("Сначала нужно указать Email через команду: {}\n", find_command_text(Commands::AddEmail));
        return prepare_request_text(chat, text);
    }

    std::string      text  = body_js["result"][idx]["message"]["text"];
    std::string_view value = text;
    strip_whitespace(value);

    int64_t email_id = chat->find_email_id(std::string(value));
    if (email_id < 0) {
        return prepare_request_text(chat, std::format("не определил email"));
    }

    Email email = chat->emails.at(email_id);
    return prepare_request_set_password(chat, email.address);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Обработка ответа на команду "/clear_email"
 * @param body_js Данные в виде JSON объекта
 * @param idx     Индекс сообщения из массива JSON: ["result"]
 * @param chat    Указатель на чат
 * @return Данные для отправки в telegram или nullopt
 *
 * @throw json::out_of_range Выбрасывается, если нет одного из ключей: result/i/message/text
 */
TelegramRequest TelegramController::process_reply_clear_email(const json& body_js, int idx, std::shared_ptr<const Chat> chat) {

    std::string      text  = body_js["result"][idx]["message"]["text"];
    std::string_view value = text;
    strip_whitespace(value);

    int64_t email_id = chat->find_email_id(std::string(value));
    if (email_id < 0) {
        return prepare_request_text(chat, "❗️ Такой email не найден");
    }

    Email email;
    email.address = value;

    auto err = Errors::Repository::OK;
    bool ok  = m_repo->delete_email(chat->id, email_id);
    if (ok) {
        return prepare_request_text(chat, "✅ email удалён");
    } else {
        return prepare_request_text(chat, "❗️ Такой email не найден");
    }
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Обработка ответа на команду "/clear_email_auth"
 * @param chat Указатель на чат
 * @return Данные для отправки в telegram или nullopt
 */
TelegramRequest TelegramController::process_cmd_clear_email_auth(std::shared_ptr<const Chat> chat) const noexcept {

    for (int64_t email_id : chat->emails | std::views::keys) {
        bool ok = m_repo->delete_email(chat->id, email_id);
        if (!ok) {
            log_error("failed clear email in repository. chat id={}", chat->id);
            return prepare_request_internal_err(chat);
        }
    }

    log_info("success clear email in repoisitory. chat_id={}", chat->id);
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
                                         find_command_text(Commands::AddEmail));

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

            if (!status_emails.empty()) {
                status_emails += "\n-------\n";
            }

            status_emails += std::format("📧 Почта: {}\n🔑 Пароль: {}\n👁‍🗨 Авторизация: {}",
                                         email.address,
                                         (email.password.empty() ? "❌" : "✅"),
                                         email.last_uid < 0 ? "❌" : "✅");
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
TelegramRequest TelegramController::prepare_request_add_email(std::shared_ptr<const Chat> chat) const noexcept {

    json js_body;
    js_body["chat_id"]      = chat->id;
    js_body["text"]         = std::format("{}Введите Email:", MARKER_EMAIL);
    js_body["reply_markup"] = {{"force_reply", true}, {"input_field_placeholder", "example@mail.com"}};

    return prepare_request_json(chat, js_body);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Подготовка запроса на команду "/password"
 * @param chat Указатель на чат
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_change_password(std::shared_ptr<const Chat> chat) const noexcept {

    json js_body;
    js_body["chat_id"]      = chat->id;
    js_body["text"]         = std::format("{}Выберете email для смены пароля:", MARKER_CHANGE_PASS);
    js_body["reply_markup"] = {{"force_reply", true}};

    return prepare_request_json(chat, js_body);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Подготовка запроса на команду "/about"
 * @param chat Указатель на чат
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_set_password(std::shared_ptr<const Chat> chat, const std::string& email) const noexcept {

    json js_body;
    js_body["chat_id"]      = chat->id;
    js_body["text"]         = std::format("{}Введите пароль для: {}{}{}", MARKER_PASS, MARKER_EMAIL, email, MARKER_EMAIL);
    js_body["reply_markup"] = {{"force_reply", true}};

    return prepare_request_json(chat, js_body);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Подготовка запроса на команду "/clear_email"
 * @param chat Указатель на чат
 * @return Структуру с данными для запроса
 */
TelegramRequest TelegramController::prepare_request_clear_email(std::shared_ptr<const Chat> chat) const noexcept {

    json js_body;
    js_body["chat_id"]      = chat->id;
    js_body["text"]         = std::format("{}Введите email, который нужно удалить:", MARKER_DEL_EMAIL);
    js_body["reply_markup"] = {{"force_reply", true}};

    return prepare_request_json(chat, js_body);
}
//----------------------------------------------------------------------------------------------------------------------

TelegramRequest TelegramController::prepare_request_buttons_change_email(std::shared_ptr<const Chat> chat) const noexcept {

    json keyboard = json::array();

    for (const auto& [_, email] : chat->emails) {
        keyboard.push_back(json::array(
            {{{"text", email.address}, {"callback_data", std::format("{}{}{}{}", MARKER_CHANGE_PASS, MARKER_EMAIL, email.address, MARKER_EMAIL)}}}));
    }

    json js_body;
    js_body["chat_id"]                         = chat->id;
    js_body["text"]                            = "Выберете Email для смены пароля";
    js_body["reply_markup"]["inline_keyboard"] = keyboard;

    return prepare_request_json(chat, js_body);
}
//----------------------------------------------------------------------------------------------------------------------

TelegramRequest TelegramController::prepare_request_buttons_clear_email(std::shared_ptr<const Chat> chat) const noexcept {

    json keyboard = json::array();

    for (const auto& [_, email] : chat->emails) {
        keyboard.push_back(
            json::array({{{"text", email.address}, {"callback_data", std::format("{}{}{}{}", MARKER_DEL_EMAIL, MARKER_EMAIL, email.address, MARKER_EMAIL)}}}));
    }

    json js_body;
    js_body["chat_id"]                         = chat->id;
    js_body["text"]                            = "Выберете Email для удаления";
    js_body["reply_markup"]["inline_keyboard"] = keyboard;

    log_info("send buttons:\n{}", js_body.dump(4));

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
    js_body["chat_id"]      = chat->id;
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
    request.chat_id      = chat->id;

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
    request.chat_id      = chat->id;

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

std::optional<std::string> TelegramController::value_after_marker(const std::string& marker_start, const std::string& marker_end,
                                                                  const std::string& text) const noexcept {
    // 1. Ищем начало первого маркера
    size_t start_pos = text.find(marker_start);
    if (start_pos == std::string::npos) {
        return std::nullopt;
    }

    // Смещаемся на длину первого маркера, чтобы оказаться в начале искомого значения
    size_t value_start = start_pos + marker_start.length();

    // 2. Ищем второй маркер, начиная поиск ПОСЛЕ первого
    size_t end_pos = text.find(marker_end, value_start);
    if (end_pos == std::string::npos) {
        return std::nullopt;
    }

    // 3. Вырезаем строку между ними
    std::string result = text.substr(value_start, end_pos - value_start);

    // Можно добавить проверку на пустоту, если пустая строка между маркерами не нужна
    if (result.empty()) {
        return std::nullopt;
    }

    return result;
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
