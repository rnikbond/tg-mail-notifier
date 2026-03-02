//----------------------------------------------------------
#include <filesystem>
//----------------------------------------------------------
#include <CLI/CLI.hpp>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
//----------------------------------------------------------
#include "Config.h"
//----------------------------------------------------------

/**
 * @brief Получение ссылки на объект конфигурации
 * @return Ссылку на объект конфигурации
 */
Config &Config::get_instance() {
    static Config cfg;
    return cfg;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Разбор параметров
 * @param argc Количество аргументов командной строки
 * @param argv Сами аргументы командной строки
 * @param path Путь к ini файлу
 * 
 * Приоритет отдается аргументам командной строки.
 */
void Config::parse(int argc, char **argv, const std::string &path) {

    CLI::App app{"Телеграм бот для получения уведомлений об email"};
    app.set_config("--config", path);

    //: Секция [tg]
    auto tg = app.add_option_group("tg", "Телеграм");
    tg->add_option("--tg-token", m_tg_token, "Токен telegram бот");
    tg->add_option("--tg-host-port", m_tg_host_port, "URL telegram сервера");
    tg->add_option("--tg-timeout", m_tg_timeout, "Время удержания сессии с telegram");

    //: Секция [log]
    auto log = app.add_option_group("log", "Логирование");
    log->add_option("--log-path", m_log_path, "Путь к лог-файлу")->default_val(m_log_path);
    log->add_option("--log-level", m_log_level, "Уровень: trace, debug, info, warn, err, critical, off")->default_val(m_log_level);

    //: Парсинг аргументов командной строки.
    //: В случае, если указатели аргумента, которых нет, CLI11 кинет эксепшн и напишет об этом.
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        std::exit(app.exit(e));
    }

    //: Создание конфига, если раньше его не было
    if (!std::filesystem::exists(path)) {
        std::ofstream out(path);
        out << app.config_to_str(true, true);
        out.close();
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Настройка логера
 * 
 * Логирование происходит на консоль и в файл.
 * На консоль выводится с уровнем trace.
 * В файл выводится с уровнем, указанным в \a m_log_level.
 */
void Config::setup_logger() {

    const char *log_pattern = "%^%Y-%m-%d %H:%M:%S.%e|%-7l|th:%t|%-25s|%-26!| %v%$";

    try {
        //: Создание папки для логов, если её нет
        std::filesystem::path log_file(m_log_path);
        if (log_file.has_parent_path()) {
            std::filesystem::create_directories(log_file.parent_path());
        }

        //: Настраиваем "раковины" (sinks)
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern(log_pattern);

        //: Файловый логгер: макс 5МБ, храним 3 старых файла
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(m_log_path, 1024 * 1024 * 5, 3);
        file_sink->set_level(spdlog::level::from_str(m_log_level));
        file_sink->set_pattern(log_pattern);

        //: Собираем логгер из двух sink‑ов
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

        //: Глобальная настройка логера
        auto logger = std::make_shared<spdlog::logger>("global_logger", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        logger->set_pattern(log_pattern);

        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::trace);

    } catch (const spdlog::spdlog_ex &e) {
        std::cerr << "log initialization failed: " << e.what() << std::endl;
        std::exit(1);
    }
}
//----------------------------------------------------------------------------------------------------------------------
