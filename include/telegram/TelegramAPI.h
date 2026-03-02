//----------------------------------------------------------
#ifndef TELEGRAMAPI_H
#define TELEGRAMAPI_H
//----------------------------------------------------------
#include <string>
//----------------------------------------------------------
#include "nlohmann/json_fwd.hpp"
//----------------------------------------------------------
using json = nlohmann::json;
//----------------------------------------------------------

/**
 * @brief Структура, описывающая ответ Telegram сервера на запрос
 * 
 * То, что возвращает Telegram
 */
struct TelegramResponse
{
    std::string_view body;
};
//----------------------------------------------------------

/**
 * @brief Структура, описывающая запрос Telegram серверу
 * 
 * То, что будет отправлено в Telegram
 */
struct TelegramRequest
{
    int64_t     chat_id = {-1};
    std::string url;
    std::string content_type;
    std::string body;
};
//----------------------------------------------------------

#endif // TELEGRAMAPI_H
