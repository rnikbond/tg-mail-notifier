//----------------------------------------------------------
#include <curl/curl.h>
#include <spdlog/spdlog.h>
//----------------------------------------------------------
#include "MailRequest.h"
//----------------------------------------------------------
namespace logger = spdlog;
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

Errors::Mail MailRequest::execute(const Email& email, const std::string& request, std::string& response) const {

    auto deleter = [](CURL* curl) { curl_easy_cleanup(curl); };

    std::unique_ptr<CURL, decltype(deleter)> curl(curl_easy_init(), deleter);
    if (!curl) {
        logger::error("[MailManager::execute] failed create CURL");
        return Errors::Mail::Internal;
    }

    enum {
        REQ_TYPE_ALL,
        REQ_TYPE_FROM,
    };

    curl_easy_setopt(curl.get(), CURLOPT_USERNAME, email.address.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_PASSWORD, email.password.c_str());

    curl_easy_setopt(curl.get(), CURLOPT_URL, "imaps://imap.yandex.ru/INBOX");
    curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, request.c_str());

    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_callback_response);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl.get());
    switch (res) {
        case CURLE_OK:
            return Errors::Mail::OK;

        case CURLE_LOGIN_DENIED:
            logger::error("[MailManager::execute] invalid email or password: {}", email.address);
            return Errors::Mail::Auth;

        default:
            logger::error("[MailManager::execute] error CURL: {}", curl_easy_strerror(res));
            return Errors::Mail::Internal;
    }
}
//----------------------------------------------------------------------------------------------------------------------
