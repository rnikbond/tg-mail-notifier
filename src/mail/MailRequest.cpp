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

// Callback для записи данных в строку
size_t write_callback_response(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    s->append((char*) contents, newLength);
    return newLength;
}
//----------------------------------------------------------------------------------------------------------------------

UIDsOpt MailRequest::load_uids(const Email& email) const noexcept {

    std::string request = std::format("UID FETCH {}:* (FLAGS)", email.last_uid);
    std::string response;

    auto err = execute(email, request, response);
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

UIDOpt MailRequest::last_uid(const Email& email) const noexcept {

    std::string request = "UID SEARCH ALL";
    std::string response;

    auto err = execute(email, request, response);
    if (err != Errors::Mail::OK) {
        return std::unexpected(err);
    }

    std::vector<int64_t> uids;

    { //: Разбор ответа
        std::string       word;
        std::stringstream stream(response);

        while (stream >> word) {
            try {
                uids.push_back(std::stoi(word));
            } catch (...) {
                continue;
            }
        }
    }

    std::sort(uids.begin(), uids.end());
    return uids.back();
}
//----------------------------------------------------------------------------------------------------------------------

EmailMsgExp MailRequest::fetch_email(const Email& email, int64_t uid) const noexcept {

    std::string url = std::format("imaps://imap.yandex.ru/INBOX/;UID={}", uid);
    std::string response;

    auto err = execute_body(email, url, response);
    if (err != Errors::Mail::OK) {
        return std::unexpected(err);
    }

    return response;
}
//----------------------------------------------------------------------------------------------------------------------

Errors::Mail MailRequest::execute(const Email& email, const std::string& request, std::string& response) const {

    auto deleter = [](CURL* curl) { curl_easy_cleanup(curl); };

    std::unique_ptr<CURL, decltype(deleter)> curl(curl_easy_init(), deleter);
    if (!curl) {
        log_error("failed create CURL");
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
                log_error("invalid email or password: {}", email.address);
                return Errors::Mail::Auth;

            default:
                log_warn("failed load last email UID. attempt {}/{}. email: {}", attempt, max_retries, email.address);
                break;
        }
    }

    return Errors::Mail::Internal;
}
//----------------------------------------------------------------------------------------------------------------------

Errors::Mail MailRequest::execute_body(const Email& email, const std::string& url, std::string& response) const {

    auto deleter = [](CURL* curl) { curl_easy_cleanup(curl); };

    std::unique_ptr<CURL, decltype(deleter)> curl(curl_easy_init(), deleter);
    if (!curl) {
        log_error("failed create CURL");
        return Errors::Mail::Internal;
    }

    curl_easy_setopt(curl.get(), CURLOPT_USERNAME, email.address.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_PASSWORD, email.password.c_str());

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(curl.get(), CURLOPT_NOBODY, 0L);

    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_callback_response);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl.get());
    switch (res) {
        case CURLE_OK:
            break;

        case CURLE_LOGIN_DENIED:
            log_error("invalid email or password: {}", email.address);
            return Errors::Mail::Auth;

        default:
            log_error("error CURL: {}", curl_easy_strerror(res));
            return Errors::Mail::Internal;
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
    if (response.length() > 3500) {
        response = response.substr(0, 3500);
    }

    return Errors::Mail::OK;
}
//----------------------------------------------------------------------------------------------------------------------

void MailRequest::extract_text_gmime(const std::string& raw_email, std::string& sender, std::string& dt, std::string& title, std::string& body) const {

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
