//----------------------------------------------------------
#ifndef CONFIG_H
#define CONFIG_H
//----------------------------------------------------------
#include <string>
//----------------------------------------------------------

/*!
 * \brief Класс для разбора ini файла и аргументов командной строки
 */
class Config {
public:

    Config();
    void parse(int argc, char **argv, const std::string &path);

public:

    std::string m_tg_token;
    std::string m_log_path  = {"tg-mail-notifier.log"};
    std::string m_log_level = {"warn"};
};
//----------------------------------------------------------------------------------------------------------------------

#endif // CONFIG_H
