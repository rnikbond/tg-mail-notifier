//----------------------------------------------------------
#include <spdlog/spdlog.h>
//----------------------------------------------------------
#include "TelegramManager.h"
//----------------------------------------------------------
namespace logger = spdlog;
//----------------------------------------------------------

TelegramManager::TelegramManager(const std::string& token, const std::string& host_port, std::shared_ptr<IRepository> repo)
    : m_token(token)
    , m_repo(repo)
    , m_http(std::make_unique<httplib::Client>(host_port))
    , m_controller(std::make_unique<TelegramController>(token, repo)) {
}
//----------------------------------------------------------------------------------------------------------------------

TelegramManager::~TelegramManager() {

    if (m_thread.joinable()) {
        stop();
    }
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Запуск telegram менеджера
 */
void TelegramManager::start() {

    if (m_token.empty()) {
        throw std::runtime_error("telegram token is empty");
    }

    if (!m_repo) {
        throw std::runtime_error("repository is not initialized");
    }

    m_request_stop = false;
    m_thread       = std::thread(&TelegramManager::run, this);

    logger::info("[TelegramManager] start");
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Остановка работы telegram менеджера
 */
void TelegramManager::stop() noexcept {

    logger::info("[TelegramManager] stopping...");

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_request_stop = true;
    }

    m_wait_cond.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }

    logger::info("[TelegramManager] stopped");
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Цикл обработки сообщений из telegram
 */
void TelegramManager::run() {

    constexpr std::string_view url_template = "/bot{}/getUpdates?offset={}";

    while (true) {

        std::unique_lock<std::mutex> lock(m_mutex);

        std::string     url = std::format(url_template, m_token, m_last_chat_update_id);
        httplib::Result msg = m_http->Get(url);
        if (!msg) {
            auto err = msg.error();
            logger::error(std::format("[TelegramManager] http::Get({}) returned error: {}", url, httplib::to_string(err)));
            break;
        }

        try {
            TelegramResponse response;
            response.body = msg->body;

            auto request = m_controller->process(std::move(response), m_last_chat_update_id);
            if (request.has_value()) {
                send_msg(std::move(request.value()));
            }

        } catch (const std::exception& ex) {
            logger::error(std::format("error process telegram response: {}", ex.what()));
        }

        bool is_stop = m_wait_cond.wait_for(lock, std::chrono::seconds(1), [&]() { return m_request_stop; });
        if (is_stop) {
            logger::info("[TelegramManager] request to stop");
            break;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Отправка сообщения в telegram чат
 * @param request Данные для отправки
 */
void TelegramManager::send_msg(const TelegramRequest&& request) {

    m_http->Post(request.url, request.body, request.content_type);
}
//----------------------------------------------------------------------------------------------------------------------
