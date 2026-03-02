//----------------------------------------------------------
#include "logger.h"
#include <ranges>
//----------------------------------------------------------
#include "../src/telegram/TelegramSenderFactory.h"
#include "MailRequestFactory.h"
//----------------------------------------------------------
#include "MailManager.h"
//----------------------------------------------------------

/**
 * @brief Конструктор менеджера электронной почты
 * @param repo Указатель на репозиторий
 */
MailManager::MailManager(std::shared_ptr<IRepository> repo)
    : m_repo(repo) {
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Запуск менеджера электронной почты
 */
void MailManager::start() {

    log_info("start");

    m_request_stop = false;
    m_thread       = std::thread(&MailManager::run, this);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Остановка работы менеджера электронной почты
 */
void MailManager::stop() {

    log_info("stopping...");

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_request_stop = true;
    }

    m_cond_wait.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }

    log_info("stopped");
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Основной цикл работы менеджера электронной почты
 */
void MailManager::run() {

    while (true) {

        std::unique_lock<std::mutex> lock(m_mutex);

        scan_mails();

        bool is_stop = m_cond_wait.wait_for(lock, std::chrono::seconds(60), [&]() { return m_request_stop; });
        if (is_stop) {
            log_info("request to stop");
            break;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Сканирование новых писем
 */
void MailManager::scan_mails() {

    auto chats = m_repo->chats();

    if (chats.empty()) {
        log_info("chats list is empty");
        return;
    }

    auto tg_sender   = TelegramSenderFactory::create();
    auto mail_loader = MailRequestFactory::create();

    for (auto& chat : chats) {
        for (const Email& email : chat->emails | std::views::values) {

            if (!email.ok()) {
                log_info("mail is not ready for scan. chat_id={}, email={}, last_UID={}", chat->chat_id, email.address, email.last_uid);
                continue;
            }

            //: Загрузка UIDs новых писем
            auto uids_res = mail_loader->load_uids(email);
            if (!uids_res.has_value()) {
                log_error("failed load new mail UIDs. chat_id={}, email={}, last_UID={}, error: ",
                          chat->chat_id,
                          email.address,
                          email.last_uid,
                          Errors::to_string(uids_res.error()));
                continue;
            }

            std::vector<int64_t> uids = std::move(uids_res.value());
            //: Если среди загруженных появились UID, которые меньше последнего загруженного - удаляем
            std::erase_if(uids, [uid_now = email.last_uid](int64_t uid) { return uid <= uid_now; });

            if (uids.empty()) {
                log_info("no new mail msg on the email. chat_id={}, email={}, last_UID={}", chat->chat_id, email.address, email.last_uid);
                continue;
            }

            //: Загрузка сообщений по новым UIDs и отправка их в telegram
            int64_t last_uid = email.last_uid;
            for (int64_t uid : uids) {

                auto msg_res = mail_loader->fetch_email(email, uid);
                if (!msg_res.has_value()) {
                    log_error("failed fetch mail msg. chat_id={}, email={}, UID={}, error: ",
                              chat->chat_id,
                              email.address,
                              uid,
                              Errors::to_string(msg_res.error()));
                    continue;
                }

                log_info("send mail msg in telegram. chat_id:={}, email={}, UID={}", chat->chat_id, email.address, uid);

                tg_sender->send_msg(chat->chat_id, std::move(msg_res.value()));
                last_uid = uid;
            }

            if (last_uid <= email.last_uid) {
                continue;
            }

            auto res = m_repo->update_email_uid(chat->chat_id, email.id, last_uid);
            if (res.has_value()) {
                log_info("last mail uid success updated. chat_id:={}, email={}, last_UID={}", chat->chat_id, email.address, last_uid);
            } else {
                log_error("failed update email last uid. chat_id:={}, email={}, error: {}", chat->chat_id, email.address, Errors::to_string(res.error()));
            }
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
