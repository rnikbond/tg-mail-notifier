//----------------------------------------------------------
#ifndef CONFIG_H
#define CONFIG_H
//----------------------------------------------------------
#include <string>
//----------------------------------------------------------

/**
 * @brief Класс для разбора ini файла и аргументов командной строки
 * 
 * Данный класс реализован через паттер Singleton
 */
class Config {

    Config() = default;

public:

    static Config &get_instance();

public:

    void parse(int argc, char **argv, const std::string &path);
    void setup_logger();

public:

    std::string m_tg_host_port = {"https://api.telegram.org"};
    std::string m_tg_token;
    size_t      m_tg_timeout;

    std::string m_log_path  = {"tg-mail-notifier.log"};
    std::string m_log_level = {"warn"};
};
//----------------------------------------------------------------------------------------------------------------------

#endif // CONFIG_H
