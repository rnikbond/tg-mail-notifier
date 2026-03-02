//----------------------------------------------------------
#include "nlohmann/json.hpp"
//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "TelegramSender.h"
//----------------------------------------------------------
using json = nlohmann::json;
//----------------------------------------------------------

/*!
 * @brief Конструкток класса
 * @param host  Хост telegram сервера
 * @param token Токен бота
 * 
 * @throw std::runtime_error Выбрасывается, если \a host или \a token пусты
 */
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

/**
 * @brief Запрос на отправку сообщения
 * @param chat_id Идентификатор telegram чата
 * @param body    Текстовое тело сообщения
 * @return TRUE, если сообщение успешно отправлено. Иначе FALSE.
 */
bool TelegramSender::send_msg(int64_t chat_id, const std::string &body) const noexcept {

    constexpr std::string_view url_template = "/bot{}/sendMessage";

    std::string url = std::format(url_template, m_token);

    json js_body;
    js_body["chat_id"]    = chat_id;
    js_body["text"]       = body;
    js_body["parse_mode"] = "HTML";

    try {
        auto err = execute(url, js_body.dump(), "application/json", chat_id);
        if (err) {
            return false;
        }
    } catch (const std::exception &ex) {
        log_error("failed on send msg in telegram. chat_id={}, exception: {}", chat_id, ex.what());
        return false;
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Запрос на отправку сообщения
 * @param request Заполненная структура для отправки сообщения
 * @return TRUE, если сообщение отправлено. Иначе FALSE.
 */
bool TelegramSender::send_msg(const TelegramRequest &request) const noexcept {

    try {
        auto err = execute(request.url, request.body, request.content_type, request.chat_id);
        if (err) {
            return false;
        }
    } catch (const std::exception &ex) {
        log_error("failed on send msg in telegram. url={}, exception: {}", request.url, ex.what());
        return false;
    }

    return false;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Выполнение запроса на отправку сообщения в telegram
 * @param url          URL для отправки
 * @param body         Тело сообщения
 * @param content_type Заголовок Content-Type
 * @param chat_id      Идентификатор чата
 * @return std::nullopt, если сообщение успешно отправлено. Или ошибку httplib.
 * 
 * @throw std::runtime_error Выбрасывается при перезвате исключения из httplib
 */
std::optional<httplib::Error> TelegramSender::execute(const std::string &url, const std::string &body, const std::string &content_type, int64_t chat_id) const {

    httplib::Error                   err;
    std::unique_ptr<httplib::Client> http = std::make_unique<httplib::Client>(m_host);

    try {
        //: Иногда почему-то сообщение не отправляется в telegram с 1-го раза.
        //: Делаем 3 попытки
        const int max_retries = 3;
        for (int attempt = 1; attempt <= max_retries; attempt++) {

            log_info("send msg in telegram. chat_id={}, attempt={}/{}", chat_id, attempt, max_retries);

            auto res = http->Post(url, body, content_type);
            if (res) {
                return std::nullopt;
            }

            err = std::move(res.error());
            log_warn("failed sent msg in telegram. chat_id={}, error: {}", chat_id, httplib::to_string(err));
        }
    } catch (const std::exception &ex) {
        throw std::runtime_error(std::format("exception on send msg in telegram: chat_id={}, {}", chat_id, ex.what()));
    }

    return err;
}
//----------------------------------------------------------------------------------------------------------------------
