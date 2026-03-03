//----------------------------------------------------------
#include <iostream>
#include <memory>
//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "cache/Cache.h"
#include "core/Config.h"
#include "mail/MailManager.h"
#include "storage/database/DatabaseStorage.h"
#include "storage/memory/MemoryStorage.h"
#include "telegram/TelegramManager.h"
#include "telegram/TelegramSenderFactory.h"
//----------------------------------------------------------

int main(int argc, char **argv) {

    Config &cfg = Config::get_instance();
    cfg.parse(argc, argv, "tg-mail-notifier.ini");
    cfg.setup_logger();

    log_info("started");

    if (cfg.m_tg_timeout == 0) {
        log_warn({"telegram session timeout == 0"});
    }

    if (cfg.m_storage == Config::StorageTypes::Database) {
        if (cfg.m_db_dsn.empty()) {
            log_error("DSN is empty");
            return 1;
        }
    }

    TelegramSenderFactory::configure(cfg.m_tg_host_port, cfg.m_tg_token);

    std::unique_ptr<MemoryStorage>   m_memory_storage;
    std::unique_ptr<IStorage>        m_storage;
    std::shared_ptr<Cache>           m_cache;
    std::shared_ptr<TelegramManager> m_tg_manager;
    std::shared_ptr<MailManager>     m_mail_manager;

    try {
        switch (cfg.m_storage) {
            case Config::StorageTypes::Database:
                log_info("used database as storage");
                m_storage = std::make_unique<DatabaseStorage>(cfg.m_db_dsn);
                break;
            default:
                log_info("used memory as storage");
                m_storage = std::make_unique<MemoryStorage>();
                break;
        }

        m_cache        = std::make_shared<Cache>(std::move(m_storage));
        m_tg_manager   = std::make_shared<TelegramManager>(cfg.m_tg_host_port, cfg.m_tg_token, cfg.m_tg_timeout, m_cache);
        m_mail_manager = std::make_shared<MailManager>(m_cache);

        m_tg_manager->start();
        m_mail_manager->start();

    } catch (const std::exception &ex) {
        log_error("error starting: {}", ex.what());
        return 1;
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
