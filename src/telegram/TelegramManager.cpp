//----------------------------------------------------------
#include "TelegramSenderFactory.h"
#include "logger.h"
//----------------------------------------------------------
#include "TelegramManager.h"
//----------------------------------------------------------

/**
 * @brief Конструктор класса
 * @param host    Хост telegram сервера
 * @param token   Токен бота
 * @param timeout Время удержания соединения для ожидания ответа в сек.
 * @param repo    Указатель на репозиторий
 */
TelegramManager::TelegramManager(const std::string& host, const std::string& token, size_t timeout, std::shared_ptr<IRepository> repo)
    : m_token(token)
    , m_timeout(timeout)
    , m_repo(repo)
    , m_http(std::make_unique<httplib::Client>(host))
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
 * 
 * @throw std::runtime_error Выбрасывается, если не удалось установить команды для бота
 */
void TelegramManager::start() {

    log_info("start");

    { //: Инициализация команд
        auto request  = m_controller->commands();
        auto response = m_http->Post(request->url, request->body, request->content_type);
        if (!response) {
            throw std::runtime_error("failed init bot commands: response is nullptr");
        }

        if (response->status != httplib::OK_200) {
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

    constexpr std::string_view url_template = "/bot{}/getUpdates?offset={}&timeout={}";

    auto tg_sender = TelegramSenderFactory::create();

    while (true) {

        std::unique_lock<std::mutex> lock(m_mutex);

        int64_t     upd_id = (m_last_chat_update_id == -1) ? -1 : m_last_chat_update_id + 1;
        std::string url    = std::format(url_template, m_token, upd_id, m_timeout);
        httplib::Result res;

        try {
            httplib::Result res = m_http->Get(url);
            if (!res) {
                auto err = res.error();
                log_error(std::format("http::Get({}) returned error: {}", url, httplib::to_string(err)));
                continue;
            }

            TelegramResponse response;
            response.body = res->body;

            auto request = m_controller->process(std::move(response), m_last_chat_update_id);
            if (request.has_value()) {
                tg_sender->send_msg(request.value());
            }

        } catch (const std::exception& ex) {
            log_error(std::format("exception 'getUpdates': {}", ex.what()));
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
