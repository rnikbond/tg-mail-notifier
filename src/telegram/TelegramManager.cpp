//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "TelegramManager.h"
//----------------------------------------------------------

TelegramManager::TelegramManager(const std::string& token, const std::string& host_port, std::shared_ptr<IRepository> repo)
    : m_token(token)
    , m_repo(repo)
    , m_http(std::make_unique<httplib::Client>(host_port))
    , m_controller(std::make_unique<TelegramController>(token, repo)) {

    if (m_token.empty()) {
        throw std::runtime_error("telegram token is empty");
    }

    if (!m_repo) {
        throw std::runtime_error("repository is not initialized");
    }
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

    log_info("start");

    { //: Инициализация команд
        auto request  = m_controller->commands();
        auto response = m_http->Post(request->url, request->body, request->content_type);
        if (!response || response->status != httplib::OK_200) {
            auto err = response.error();
            throw std::runtime_error(std::format("failed init bot commands: {}, {}", response->status, httplib::to_string(err)));
        }
    }

    m_request_stop = false;
    m_thread       = std::thread(&TelegramManager::run, this);
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Остановка работы telegram менеджера
 */
void TelegramManager::stop() noexcept {

    log_info("stopping...");

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_request_stop = true;
    }

    m_wait_cond.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }

    log_info("stopped");
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Цикл обработки сообщений из telegram
 */
void TelegramManager::run() {

    constexpr std::string_view url_template = "/bot{}/getUpdates?offset={}&timeout=10";

    while (true) {

        std::unique_lock<std::mutex> lock(m_mutex);

        int64_t         upd_id = (m_last_chat_update_id == -1) ? -1 : m_last_chat_update_id + 1;
        std::string     url    = std::format(url_template, m_token, upd_id);
        httplib::Result msg = m_http->Get(url);
        if (!msg) {
            auto err = msg.error();
            log_error(std::format("http::Get({}) returned error: {}", url, httplib::to_string(err)));
            continue;
        }

        try {
            TelegramResponse response;
            response.body = msg->body;

            auto request = m_controller->process(std::move(response), m_last_chat_update_id);
            if (request.has_value()) {
                send_msg(std::move(request.value()));
            }

        } catch (const std::exception& ex) {
            log_error(std::format("error process telegram response: {}", ex.what()));
        }

        bool is_stop = m_wait_cond.wait_for(lock, std::chrono::seconds(1), [&]() { return m_request_stop; });
        if (is_stop) {
            log_info("request to stop");
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
