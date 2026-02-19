//----------------------------------------------------------
#include <iostream>
#include <memory>
//----------------------------------------------------------
#include <spdlog/spdlog.h>
//----------------------------------------------------------
#include "cache/Cache.h"
#include "core/Config.h"
#include "storage/memory/MemoryStorage.h"
#include "telegram/TelegramManager.h"
//----------------------------------------------------------
namespace logger = spdlog;
//----------------------------------------------------------

int main(int argc, char **argv) {

    Config &cfg = Config::get_instance();
    cfg.parse(argc, argv, "tg-mail-notifier.ini");
    cfg.setup_logger();

    logger::info("[main] started");

    std::unique_ptr<MemoryStorage>   m_storage;
    std::shared_ptr<Cache>           m_cache;
    std::shared_ptr<TelegramManager> m_tg_manager;

    try {
        m_storage    = std::make_unique<MemoryStorage>();
        m_cache      = std::make_shared<Cache>(std::move(m_storage));
        m_tg_manager = std::make_shared<TelegramManager>(cfg.m_tg_token, cfg.m_tg_host_port, m_cache);

        m_tg_manager->start();

    } catch (const std::exception &ex) {
        logger::error("error starting: {}", ex.what());
    }

    std::string command;
    std::cout << "Введите 'stop' для завершения: " << std::endl;
    while (true) {
        std::cout << "> ";
        std::cin >> command;

        if (command == "stop") {
            std::cout << "Принята команда на остановку..." << std::endl;

            m_tg_manager->stop();

            break;
        } else {
            std::cout << "Неизвестная команда: " << command << ". Введите 'stop'." << std::endl;
        }
    }

    logger::info("[main] stopped");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
