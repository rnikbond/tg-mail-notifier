//----------------------------------------------------------
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
//----------------------------------------------------------
#include "../src/telegram/TelegramSenderFactory.h"
#include "MailFactory.h"
//----------------------------------------------------------
#include "MailManager.h"
//----------------------------------------------------------
namespace logger = spdlog;
//----------------------------------------------------------

MailManager::MailManager(std::shared_ptr<IRepository> repo, const std::string& tg_token)
    : m_repo(repo)
    , m_tg_token(tg_token) {
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

        bool is_stop = m_cond_wait.wait_for(lock, std::chrono::seconds(15), [&]() { return m_request_stop; });
        if (is_stop) {
            logger::info("[MailManager::run] request to stop");
            break;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

void MailManager::scan_emails() {

    auto chats_map = m_repo->chats();

    //: Удаление чатов, в которых не настроена почта
    std::erase_if(chats_map, [](const std::pair<int64_t, Email>& item) {
        const auto& [chat_id, email] = item;
        return email.address.empty() || email.password.empty() || email.last_uid < 0;
    });

    if (chats_map.empty()) {
        logger::info("[MailManager::scan_emails] emails list is empty");
        return;
    }

    auto tg_sender = TelegramSenderFactory::create();

    for (auto& [chat_id, email] : chats_map) {

        //: Загрузка UIDs новых писем
        auto uids_opt = load_uids(email);
        if (!uids_opt.has_value()) {
            continue;
        }

        std::vector<int64_t> uids = std::move(uids_opt.value());
        if (uids.empty()) {
            logger::info("[MailManager::scan_emails] no new messages. email: {}", email.address);
            continue;
        }

        //: Проверка, есть ли новые письма
        if (uids.back() == email.last_uid) {
            //logger::warn("[MailManager::scan_emails] last uid is equal to the last processed. email: {}", email.address);
            continue;
        }

        //: Если среди загруженных появились UID, которые меньше последнего загруженного - удаляем
        std::erase_if(uids, [uid_now = email.last_uid](int64_t uid) { return uid <= uid_now; });
        if (uids.empty()) {
            continue;
        }

        //: Загрузка сообщений по новым UIDs
        for (int64_t uid : uids) {
            auto msg_opt = load_email_msg(email, uid);
            if (!msg_opt.has_value()) {
                continue;
            }

            logger::info("[MailManager::scan_emails] loaded new message. email: {}, UID: {}\n{}", email.address, uid, msg_opt.value());

            json js_body;
            js_body["chat_id"]    = chat_id;
            js_body["text"]       = std::move(msg_opt.value());
            js_body["parse_mode"] = "HTML";

            constexpr std::string_view url = "/bot{}/sendMessage";

            TelegramRequest request;
            request.url          = std::format(url, m_tg_token);
            request.body         = js_body.dump();
            request.content_type = "application/json";

            tg_sender->send_msg(std::move(request));
        }

        //: Обновление последнего обработанного сообщения
        email.last_uid = uids.back();
        if (!m_repo->update_email(chat_id, email)) {
            logger::error("[MailManager::scan_emails] failed update last UID in repository. email: {}, uid: {}", email.address, email.last_uid);
        } else {
            logger::info("[MailManager::scan_emails] last UID updated in repository. email: {}, uid: {}", email.address, email.last_uid);
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------

std::optional<std::vector<int64_t>> MailManager::load_uids(const Email& email) {

    auto loader = MailFactory::create();

    auto res = loader->load_uids(email);
    if (!res.has_value()) {
        logger::error("[MailManager::load_uids] failed load uids. email: {}, last_uid: {}, error: {}",
                      email.address,
                      email.last_uid,
                      static_cast<int>(res.error()));
        return std::nullopt;
    }

    return res.value();
}
//----------------------------------------------------------------------------------------------------------------------

std::optional<std::string> MailManager::load_email_msg(const Email& email, int64_t uid) {

    auto loader = MailFactory::create();

    auto res = loader->fetch_email(email, uid);
    if (!res.has_value()) {
        logger::error("[MailManager::load_email_msg] failed load msg. email: {}, uid: {}, error: {}", email.address, uid, static_cast<int>(res.error()));
        return std::nullopt;
    }

    return res.value();
}
//----------------------------------------------------------------------------------------------------------------------
