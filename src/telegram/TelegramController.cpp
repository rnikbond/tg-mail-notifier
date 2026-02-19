//----------------------------------------------------------
#include "nlohmann/json.hpp"
#include <spdlog/spdlog.h>
//----------------------------------------------------------
#include "TelegramController.h"
//----------------------------------------------------------
namespace logger = spdlog;
//----------------------------------------------------------

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
std::optional<TelegramRequest> TelegramController::process(const TelegramResponse&& response, int64_t& last_msg_id) {

    json body_js = json::parse(response.body);

    if (!body_js.contains("result")) {
        throw std::runtime_error("[TelegramController] invalid JSON: does not contains 'result'");
    }

    //: Обрабатывае только последнее сообщение
    int idx = body_js["result"].size() - 1;
    if (idx < 0) {
        return std::nullopt;
    }

    if (!body_js["result"][idx].contains("update_id")) {
        throw std::runtime_error("[TelegramController] invalid JSON: does not contains 'update_id' in [result][i]");
    }

    int64_t update_id = body_js["result"][idx]["update_id"];
    if (last_msg_id == update_id) {
        //: Это сообщение уже обрабатывалось
        return std::nullopt;
    }

    //: Запоминаем идентификатор обработанного сообщения
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
        logger::warn(std::format("[TelegramController] unknown JSON: \n{}", body_js.dump(4)));
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

    constexpr std::string_view url = "/bot{}/sendMessage";

    json req_body;
    req_body["chat_id"]    = chat->chat_id;
    req_body["text"]       = "Привет";
    req_body["parse_mode"] = "HTML";

    TelegramRequest request;
    request.url          = std::format(url, m_token);
    request.body         = req_body.dump();
    request.content_type = "application/json";

    return request;
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
        throw std::runtime_error("[TelegramController] invalid JSON: does not contains 'chat' in [result][i][edited_message]");
    }
    if (!body_js["result"][idx][tag]["chat"].contains("id")) {
        throw std::runtime_error("[TelegramController] invalid JSON: does not contains 'id' in [result][i][edited_message][chat]");
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
        throw std::runtime_error("[TelegramController] invalid JSON: does not contains 'username' in [result][i][message][chat]");
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
        throw std::runtime_error("failed create chat in repository");
    }

    return res.value();
}
//----------------------------------------------------------------------------------------------------------------------
