//----------------------------------------------------------
#ifndef CHAT_H
#define CHAT_H
//----------------------------------------------------------
#include <chrono>
#include <set>
#include <string>
#include <unordered_map>
//----------------------------------------------------------

/**
 * @brief Структура для работы с интервалом времени
 */
struct TimeImterval
{
    int start_secs = {}; ///< Начало интервала в секундах. Например, 08:00 =  8 * 3'600 = 28'800
    int end_secs   = {}; ///< Конец интервала в секундах.  Например, 22:00 = 22 * 3'600 = 79'200

    /// \brief Пороверка корректности интервала
    bool is_valid() const {
        return (start_secs != 0 || end_secs != 0) && (start_secs != end_secs);
    }

    /// @brief Пороверка входа времени в интервал
    bool is_in_range(std::chrono::system_clock::time_point dt) const {

        if (!is_valid()) {
            return true;
        }

        auto dt_secs = std::chrono::duration_cast<std::chrono::seconds>(dt.time_since_epoch()).count();

        auto dt_start_day_secs = dt_secs - (dt_secs % (24 * 3600));
        auto dt_start_secs     = dt_start_day_secs + start_secs;
        auto dt_end_secs       = dt_start_day_secs + end_secs;
        if (start_secs > end_secs) {
            dt_end_secs += 24 * 3600;
        }

        return (dt_secs >= dt_start_secs && dt_secs <= dt_end_secs);
    }
};
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Структура, описывающая настройки чата
 */
struct ChatExt
{
    int id = {-1}; ///< Идентификато настроек

    TimeImterval silent_interval; ///< Интервал, когда сообщения должны приходить без звука
};
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Структура, 
 */
struct EmailExt
{
    int id = {-1}; ///< Идентификатор настроек

    std::set<std::string> addr_filter_rules;  ///< Фильтры по отправителям. Например: {"info@service.ru", "*@sales.ru"}
    std::set<std::string> title_filter_rules; ///< Фильтры по заголовку письма
    std::set<std::string> body_filter_rules;  ///< Фильтры по телу письма
};
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Структура зашифрованного пароля
 */
struct PasswordCipher
{
    std::vector<uint8_t> data; ///< Пароль для подключения к почте
    std::vector<uint8_t> iv;   ///< Верктор инициализации при шифровании пароля
    std::vector<uint8_t> tag;  ///< Тег аутентификации при шифровании
};
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Структура информации email
 */
struct Email
{
    int64_t     id = {-1};       ///< Идентификатор записи
    std::string address;         ///< Адрес. Например: "ololoev@mail.ru"
    std::string password;        ///< Пароль для подключения к почте
    int64_t     last_uid = {-1}; ///< UID последнего обработанного письма
    EmailExt    extensions;      ///< Доп. настройки почты

    /// @brief Пороверка корректности почты
    bool ok() const {
        return !address.empty() && !password.empty() && last_uid >= 0;
    }
};
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Структура инфомации о чате
 */
struct Chat
{
    int64_t      id = {};      ///< Идетнификатор чата из telegram
    std::string  username;     ///< Логин из telegram
    std::string  first_name;   ///< Имя пользователя из telegram
    std::string  last_name;    ///< Фамилия пользователя из telegram
    ChatExt      extensions;   ///< Доп. настройки чата

    std::unordered_map<int64_t, Email> emails; ///< <email_id, Email> Данные об электронной почте
};
//----------------------------------------------------------------------------------------------------------------------
#endif // CHAT_H
