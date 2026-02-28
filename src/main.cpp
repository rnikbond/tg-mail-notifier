//----------------------------------------------------------
#include <iostream>
#include <memory>
//----------------------------------------------------------
#include "core/logger.h"
//----------------------------------------------------------
#include "cache/Cache.h"
#include "core/Config.h"
#include "mail/MailManager.h"
#include "storage/memory/MemoryStorage.h"
#include "telegram/TelegramManager.h"
//----------------------------------------------------------

int main(int argc, char **argv) {

    Config &cfg = Config::get_instance();
    cfg.parse(argc, argv, "tg-mail-notifier.ini");
    cfg.setup_logger();

    log_info("started");

    std::unique_ptr<MemoryStorage>   m_storage;
    std::shared_ptr<Cache>           m_cache;
    std::shared_ptr<TelegramManager> m_tg_manager;
    std::shared_ptr<MailManager>     m_mail_manager;

    try {
        m_storage = std::make_unique<MemoryStorage>();
        m_cache   = std::make_shared<Cache>(std::move(m_storage));

        m_tg_manager   = std::make_shared<TelegramManager>(cfg.m_tg_token, cfg.m_tg_host_port, m_cache);
        m_mail_manager = std::make_shared<MailManager>(m_cache, cfg.m_tg_token);

        m_tg_manager->start();
        m_mail_manager->start();

    } catch (const std::exception &ex) {
        log_error("error starting: {}", ex.what());
    }

    std::string command;
    while (true) {
        std::cin >> command;

        if (command == "stop") {
            std::cout << "Принята команда на остановку..." << std::endl;

            m_mail_manager->stop();
            m_tg_manager->stop();

            break;
        } else {
            std::cout << "Неизвестная команда: " << command << ". Введите 'stop'." << std::endl;
        }
    }

    log_info("stopped");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
