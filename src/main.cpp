//----------------------------------------------------------
#include <memory>
//----------------------------------------------------------
#include <spdlog/spdlog.h>
//----------------------------------------------------------
#include "cache/Cache.h"
#include "core/Config.h"
#include "storage/memory/MemoryStorage.h"
//----------------------------------------------------------
namespace logger = spdlog;
//----------------------------------------------------------

int main(int argc, char **argv) {

    Config &cfg = Config::get_instance();
    cfg.parse(argc, argv, "tg-mail-notifier.ini");
    cfg.setup_logger();

    logger::info("Создание объектов");

    std::unique_ptr<MemoryStorage> m_storage;
    std::shared_ptr<Cache>         m_cache;

    try {
        m_storage = std::make_unique<MemoryStorage>();
        m_cache   = std::make_shared<Cache>(std::move(m_storage));
    } catch (const std::exception &ex) {
        logger::error("error starting: {}", ex.what());
    }

    logger::info("Запуск приложения");

    logger::info("Приложение остановлен");
    return 0;
}
//----------------------------------------------------------------------------------------------------------------------
