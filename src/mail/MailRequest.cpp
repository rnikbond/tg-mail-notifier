//----------------------------------------------------------
#include <regex>
//----------------------------------------------------------
#include <curl/curl.h>
#include <gmime/gmime.h>
//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "MailRequest.h"
//----------------------------------------------------------

/**
 * @brief Callback для CURL, с помощью которой записывается результат запроса
 * @param[in]  contents Данные
 * @param[in]  size     Размер данных
 * @param[in]  nmemb    Количество блоков
 * @param[out] s       Строка, куда будут записаны данные
 * @return Размер записанного блока
 */
size_t write_callback_response(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    s->append((char*) contents, newLength);
    return newLength;
}
//----------------------------------------------------------------------------------------------------------------------

/**
* @brief Загрузка идентификаторов (UID) новых писем
* @param email Информация об электронной почте для выполнения запроса
* @return Отсортированный список идентификаторов новых писем, или ошибку
*/
IMailRequest::UIDsResult MailRequest::load_uids(const Email& email) const noexcept {

    std::string request = std::format("UID FETCH {}:* (FLAGS)", email.last_uid);
    std::string response;

    auto err = execute_request(email, request, response);
    if (err != Errors::Mail::OK) {
        return std::unexpected(err);
    }

    std::vector<int64_t> uids;

    { //: Разбор ответа
        std::string       word;
        std::stringstream stream(response);

        while (stream >> word) {
            if (word == "(UID") {
                int uid = 0;
                if (stream >> uid) {
                    uids.push_back(uid);
                }
            }
        }
    }

    std::sort(uids.begin(), uids.end());
    return uids;
}
//----------------------------------------------------------------------------------------------------------------------

/**
* @brief Загрузка идентификатора последнего письма
* @param email Информация об электронной почте для выполнения запроса
* @return Идентификатор последнего письма или ошибку
*/
IMailRequest::UIDResult MailRequest::last_uid(const Email& email) const noexcept {

    std::string request = "UID SEARCH ALL";
    std::string response;

    auto err = execute_request(email, request, response);
    if (err != Errors::Mail::OK) {
        return std::unexpected(err);
    }

    int64_t uid = -1;

    { //: Разбор ответа
        std::string       word;
        std::stringstream stream(response);

        while (stream >> word) {
            try {
                uid = std::stoi(word);
            } catch (...) {
                continue;
            }
        }
    }

    return uid;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Загрузка письма электронной почты
 * @param email Информация об электронной почте для выполнения запроса
 * @param uid   Идентификатор письма, которое нужно загрузить
 * @return Данные письма или ошибку
 */
IMailRequest::MailMsgResult MailRequest::fetch_email(const Email& email, int64_t uid) const noexcept {

    std::string url = std::format("imaps://imap.yandex.ru/INBOX/;UID={}", uid);
    std::string response;

    auto err = execute_url(email, url, response);
    if (err != Errors::Mail::OK) {
        return std::unexpected(err);
    }

    return response;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Выполнение запроса
 * @param[in]  email    Информация об электронной почте для выполнения запроса
 * @param[in]  request  Данные запроса
 * @param[out] response Данные ответа на запрос
 * @return Ошибку выполнения запроса
 */
Errors::Mail MailRequest::execute_request(const Email& email, const std::string& request, std::string& response) const noexcept {

    auto deleter = [](CURL* curl) { curl_easy_cleanup(curl); };

    std::unique_ptr<CURL, decltype(deleter)> curl(curl_easy_init(), deleter);
    if (!curl) {
        log_error("failed init CURL");
        return Errors::Mail::Internal;
    }

    curl_easy_setopt(curl.get(), CURLOPT_USERNAME, email.address.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_PASSWORD, email.password.c_str());

    curl_easy_setopt(curl.get(), CURLOPT_URL, "imaps://imap.yandex.ru/INBOX");
    curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, request.c_str());

    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_callback_response);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);

    //: Иногда CURL возвращает код ошибки 100, но следующий запрос выполняется успешно.
    //: Делаем 3 попытки, если получаем код ошибки != CURLE_LOGIN_DENIED
    const int max_retries = 3;
    for (int attempt = 1; attempt <= max_retries; attempt++) {
        CURLcode res = curl_easy_perform(curl.get());
        switch (res) {
            case CURLE_OK:
                return Errors::Mail::OK;
            case CURLE_LOGIN_DENIED:
                return Errors::Mail::Auth;
            default:
                log_warn("error CURL. attempt={}/{}. email={}, error: [code={}] {}",
                         attempt,
                         max_retries,
                         email.address,
                         static_cast<int>(res),
                         curl_easy_strerror(res));
                //: Активация детального вывода полсле получения неизвестной ошибки
                curl_easy_setopt(curl.get(), CURLOPT_VERBOSE, 1L);
                break;
        }
    }

    return Errors::Mail::Internal;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Выполнение запроса по указанному URL
 * @param[in]  email    Информация об электронной почте для выполнения запроса
 * @param[in]  url      URL запроса
 * @param[out] response Данные ответа на запрос
 * @return Ошибку выполнения запроса
 */
Errors::Mail MailRequest::execute_url(const Email& email, const std::string& url, std::string& response) const noexcept {

    auto deleter = [](CURL* curl) { curl_easy_cleanup(curl); };

    std::unique_ptr<CURL, decltype(deleter)> curl(curl_easy_init(), deleter);
    if (!curl) {
        log_error("failed init CURL");
        return Errors::Mail::Internal;
    }

    curl_easy_setopt(curl.get(), CURLOPT_USERNAME, email.address.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_PASSWORD, email.password.c_str());

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(curl.get(), CURLOPT_NOBODY, 0L);

    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_callback_response);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);

    //: Иногда CURL возвращает код ошибки 100, но следующий запрос выполняется успешно.
    //: Делаем 3 попытки, если получаем код ошибки != CURLE_LOGIN_DENIED
    const int max_retries = 3;
    bool      is_ok       = false;
    for (int attempt = 1; !is_ok && attempt <= max_retries; attempt++) {
        CURLcode res = curl_easy_perform(curl.get());
        switch (res) {
            case CURLE_OK:
                is_ok = true;
                break;
            case CURLE_LOGIN_DENIED:
                return Errors::Mail::Auth;
            default:
                log_warn("error CURL. attempt={}/{}. email:={}, error: [code={}] {}",
                         attempt,
                         max_retries,
                         email.address,
                         static_cast<int>(res),
                         curl_easy_strerror(res));
                //: Активация детального вывода полсле получения неизвестной ошибки
                curl_easy_setopt(curl.get(), CURLOPT_VERBOSE, 1L);
                break;
        }
    }

    std::string sender;
    std::string dt;
    std::string title;
    std::string body;

    auto stripHTML = [](std::string html) {
        // 1. Удаляем содержимое тегов <script> и <style> полностью
        html = std::regex_replace(html, std::regex("<(script|style)[^>]*>[\\s\\S]*?<\\/\\1>"), "");

        // 2. Удаляем все остальные теги
        html = std::regex_replace(html, std::regex("<[^>]*>"), " ");

        // 3. Заменяем HTML-сущности (базово)
        html = std::regex_replace(html, std::regex("&nbsp;"), " ");
        html = std::regex_replace(html, std::regex("&lt;"), "<");
        html = std::regex_replace(html, std::regex("&gt;"), ">");
        html = std::regex_replace(html, std::regex("&amp;"), "&");

        // 4. Убираем лишние пробелы и переносы
        html = std::regex_replace(html, std::regex("\\s{2,}"), " ");

        return html;
    };

    extract_text_gmime(response, sender, dt, title, body);
    sender = stripHTML(sender);
    dt     = stripHTML(dt);
    title  = stripHTML(title);
    body   = stripHTML(body);

    response = std::format("{}\n{}\n{}\n{}", sender, dt, title, body);
    if (response.length() > 3072) {
        response.resize(3072);
    }

    return Errors::Mail::OK;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Получение данных письма электронной почты
 * @param[in]  raw_email Сырые данные письма
 * @param[out] sender Отправитель
 * @param[out] dt     Время письма
 * @param[out] title  Заголовок письма
 * @param[out] body   Тело письма
 */
void MailRequest::extract_text_gmime(const std::string& raw_email, std::string& sender, std::string& dt, std::string& title, std::string& body) const noexcept {

    g_mime_init();

    GMimeStream* stream = g_mime_stream_mem_new_with_buffer(raw_email.c_str(), raw_email.length());
    GMimeParser* parser = g_mime_parser_new_with_stream(stream);

    GMimeMessage* message = g_mime_parser_construct_message(parser, nullptr);

    InternetAddressList* from_list = g_mime_message_get_from(message);
    if (from_list && internet_address_list_length(from_list) > 0) {
        InternetAddress* addr = internet_address_list_get_address(from_list, 0);
        const char*      name = internet_address_get_name(addr); // Имя (например, "Иван Иванов")
        if (name)
            sender = name;
    }

    const char* subject = g_mime_message_get_subject(message);
    if (subject) {
        title = subject;
    }

    GDateTime* date = g_mime_message_get_date(message);
    if (date) {
        char* date_str = g_date_time_format(date, "%Y-%m-%d %H:%M:%S");

        dt = date_str;
        g_free(date_str);
    }

    GMimeObject* mime_part = g_mime_message_get_mime_part(message);
    if (GMIME_IS_TEXT_PART(mime_part)) {
        char* text = g_mime_text_part_get_text((GMimeTextPart*) mime_part);
        if (text) {
            body = text;
            g_free(text);
        }
    } else if (GMIME_IS_MULTIPART(mime_part)) {
        GMimeMultipart* multipart = (GMimeMultipart*) mime_part;

        int count = g_mime_multipart_get_count(multipart);
        for (int i = 0; i < count; i++) {
            GMimeObject* part = g_mime_multipart_get_part(multipart, i);
            if (GMIME_IS_TEXT_PART(part)) {
                char* text = g_mime_text_part_get_text((GMimeTextPart*) part);
                if (text) {
                    body = text;
                    g_free(text);
                    break;
                }
            }
        }
    }

    g_object_unref(message);
    g_object_unref(parser);
    g_object_unref(stream);
    g_mime_shutdown();
}
//----------------------------------------------------------------------------------------------------------------------
