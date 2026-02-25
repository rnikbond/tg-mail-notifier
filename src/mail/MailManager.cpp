//----------------------------------------------------------
#include "spdlog/spdlog.h"
//----------------------------------------------------------
#include "MailManager.h"
//----------------------------------------------------------
namespace logger = spdlog;
//----------------------------------------------------------

// Callback для записи данных в строку
size_t WriteCallbackMail(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    s->append((char*) contents, newLength);
    return newLength;
}
//----------------------------------------------------------------------------------------------------------------------

MailManager::MailManager(std::shared_ptr<IRepository> repo)
    : m_repo(repo) {
}
//----------------------------------------------------------------------------------------------------------------------

void MailManager::start() {

    logger::info("[MailManager::start] start");

    m_request_stop = false;
    m_thread       = std::thread(&MailManager::run, this);
}
//----------------------------------------------------------------------------------------------------------------------

void MailManager::stop() {

    logger::info("[MailManager::stop] stopping...");

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_request_stop = true;
    }

    m_cond_wait.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }

    logger::info("[MailManager::stop] stopped");
}
//----------------------------------------------------------------------------------------------------------------------

void MailManager::run() {

    while (true) {

        std::unique_lock<std::mutex> lock(m_mutex);

        scan_emails();

        bool is_stop = m_cond_wait.wait_for(lock, std::chrono::seconds(1), [&]() { return m_request_stop; });
        if (is_stop) {
            logger::info("[MailManager::run] request to stop");
            break;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

void MailManager::scan_emails() {

    return;

    auto chats_map = m_repo->chats();
    std::erase_if(chats_map, [](const std::pair<int64_t, Email>& item) {
        const auto& [chat_id, email] = item;
        return email.address.empty();
    });

    if (chats_map.empty()) {
        return;
    }

    for (auto& [chat_id, email] : chats_map) {

        std::vector<int64_t> uids = load_uids(email);
        if (uids.empty()) {
            continue;
        }

        if (uids.back() == email.last_uid) {
            continue;
        }

        if (email.last_uid == 0) {
            email.last_uid = uids.back();
            if (m_repo->update_email(chat_id, email)) {
                logger::info("[MailManager::scan_emails] inint email uid. email: {}, uid: {}", email.address, email.last_uid);
            }
            continue;
        }

        std::erase_if(uids, [uid_now = email.last_uid](int64_t uid) { return uid <= uid_now; });
    }
}
//----------------------------------------------------------------------------------------------------------------------
