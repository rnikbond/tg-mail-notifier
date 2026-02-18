//----------------------------------------------------------
#include <filesystem>
//----------------------------------------------------------
#include <CLI/CLI.hpp>
//----------------------------------------------------------
#include "Config.h"
//----------------------------------------------------------

Config::Config() {
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * \brief Разбор параметров
 * \param argc Количество аргументов командной строки
 * \param argv Сами аргументы командной строки
 * \param path Путь к ini файлу
 * 
 * Приоритет отдается аргументам командной строки.
 */
void Config::parse(int argc, char **argv, const std::string &path) {

    CLI::App app{"Телеграм бот для получения уведомлений об email"};
    app.set_config("--config", path);

    //: Секция [tg]
    auto tg = app.add_option_group("tg", "Телеграм");
    tg->add_option("--tg-token", m_tg_token, "Токен telegram бот");

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
