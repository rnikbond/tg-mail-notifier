//----------------------------------------------------------
#include <memory>
#include <regex>
//----------------------------------------------------------
#include <arpa/nameser.h>
#include <curl/curl.h>
#include <netinet/in.h>
#include <resolv.h>
//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "ImapDiscoveryTool.h"
//----------------------------------------------------------
CURLcode execute_curl(CURL* curl) noexcept;
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s);
std::string extract_imap_host(const std::string& xml);
//----------------------------------------------------------

/**
 * @brief Получение информации о почтовом сервере по адресу электронной почты
 * @param email Адрес почты
 * @return Структуру с данными для IMAP или ошибку, если не удалось сформировать URL
 */
std::expected<MailServer, Errors::ImapDiscover> ImapDiscoveryTool::detect_mail_server(const std::string& email) {

    if (!is_correct_email(email)) {
        return std::unexpected(Errors::ImapDiscover::EmailSyntax);
    }

    auto domain_res = domain_from_email(email);
    if (!domain_res) {
        log_error("failed find domain in email. email={}", email);
        return std::unexpected(Errors::ImapDiscover::EmailSyntax);
    }

    MailServer info;
    info.domain = std::move(domain_res.value());

    //: 1. Пробудем достучаться через домен, указанный в почте
    auto imap_url_res = url_by_domain(info.domain);
    if (imap_url_res) {
        log_info("mail server detected. method=url_by_domain(), domain: {}", info.domain);
        info.url = imap_url_res.value();
        return info;
    }

    //: 2. Просто по домену из почты не получилось.
    //:    Пробудем через mx запись
    imap_url_res = url_via_mx(info.domain);
    if (imap_url_res) {
        log_info("mail server detected. method=url_via_mx(), domain: {}", info.domain);
        info.url = imap_url_res.value();
        return info;
    }

    //: 3. Через mx-запись не получилось.
    //:    Пробудем через DNS SRV
    imap_url_res = url_via_dns_srv(info.domain);
    if (imap_url_res) {
        log_info("mail server detected. method=url_via_dns_srv(), domain: {}", info.domain);
        info.url = imap_url_res.value();
        return info;
    }

    //: 4. Через DNS SRV не получилось.
    //:    Пробудем через Mozilla Autoconfig
    imap_url_res = url_via_autoconf(email, info.domain);
    if (imap_url_res) {
        log_info("mail server detected. method=url_via_autoconf(), domain: {}", info.domain);
        info.url = imap_url_res.value();
        return info;
    }

    //: Не удалось опеределить IMAP URL
    return std::unexpected(Errors::ImapDiscover::DomainNotFound);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Получение домена из адреса почты
 * @param email Адрес почты
 * @return Домен, который идет после '@'
 */
std::expected<std::string, Errors::ImapDiscover> ImapDiscoveryTool::domain_from_email(const std::string& email) {

    size_t pos = email.find('@');
    if (pos == std::string::npos) {
        return std::unexpected(Errors::ImapDiscover::EmailSyntax);
    }

    return email.substr(pos + 1, email.length() - pos - 1);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Проверка корректности адреса электронной почты
 * @param email Адрес почты
 * @return TRUE, если почта корректна с точки зрения синтаксиса. Иначе FALSE.
 */
bool ImapDiscoveryTool::is_correct_email(const std::string_view& email) {

    const std::regex pattern(R"(^[\w\.-]+@[\w\.-]+\.\w{2,4}$)");
    if (!std::regex_match(static_cast<std::string>(email), pattern)) {
        return false;
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Проверка доступности URL по IMAP
 * @param imap_url URL, который нужно проверить
 * @return TRUE, если URL доступен. FASLE, если URL недоступен. Ошибка, если не удалось выполнить проверку.
 */
std::expected<bool, Errors::ImapDiscover> ImapDiscoveryTool::check_imap_server(const std::string& imap_url) {

    CURL* curl_raw = curl_easy_init();
    if (!curl_raw) {
        log_error("failed curl_easy_init()");
        return std::unexpected(Errors::ImapDiscover::Internal);
    }

    auto deleter = [](CURL* curl) { curl_easy_cleanup(curl); };

    std::unique_ptr<CURL, decltype(deleter)> curl(curl_raw, deleter);

    // Указываем URL (например, "imaps://imap.example.com")
    curl_easy_setopt(curl.get(), CURLOPT_URL, imap_url.c_str());

    // Устанавливаем таймаут на подключение (в секундах)
    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 5L);

    // Режим "только подключение" — curl проверит TCP/SSL соединение и остановится
    curl_easy_setopt(curl.get(), CURLOPT_CONNECT_ONLY, 1L);

    // Включаем подробный вывод для отладки (необязательно)
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    CURLcode res = execute_curl(curl.get());
    return (res == CURLE_OK);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Получение URL IMAP из домена
 * @param domain Домен
 * @return URL IMAP в виде: imaps://<domain>
 */
std::expected<std::string, Errors::ImapDiscover> ImapDiscoveryTool::url_by_domain(const std::string& domain) {

    std::string imap_url = std::format("imaps://imap.{}", domain);

    auto res = check_imap_server(imap_url);
    if (!res.has_value()) {
        return std::unexpected(res.error());
    }

    if (!res.value()) {
        return std::unexpected(Errors::ImapDiscover::DomainNotFound);
    }

    return imap_url;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Получение URL IMAP через MX-запись.
 * @param domain Домен
 * @return URL IMAP или ошибку, если не удалось сформировать URL.
 * 
 * Функция обращается к DNS-серверу и спрашивает: «Кто отвечает за почту ololo.ru».
 * DNS возвращает структуру, содержащую приоритет и имя хоста. Например: mail.ololo.ru.
 * dn_expand распаковывает сжатое DNS-имя в обычную строку mail.ololo.ru.
 * В результате формируется URL IMAP, который возвращается. Например: imaps://mail.ololo.ru.
 */
std::expected<std::string, Errors::ImapDiscover> ImapDiscoveryTool::url_via_mx(const std::string& domain) {

    unsigned char response[4096];

    // Выполняем запрос MX-записи
    int len = res_query(domain.c_str(), C_IN, T_MX, response, sizeof(response));
    if (len < 0) {
        return std::unexpected(Errors::ImapDiscover::DomainNotFound);
    }

    ns_msg msg;
    if (ns_initparse(response, len, &msg) < 0) {
        return std::unexpected(Errors::ImapDiscover::DomainNotFound);
    }

    // Количество записей в секции Answer
    int count = ns_msg_count(msg, ns_s_an);

    for (int i = 0; i < count; i++) {
        ns_rr rr;
        if (ns_parserr(&msg, ns_s_an, i, &rr) < 0)
            continue;

        if (ns_rr_type(rr) == ns_t_mx) {
            char host[MAXDNAME];
            // Данные MX записи начинаются с 2 байт приоритета (пропускаем их: +2)
            const unsigned char* data = ns_rr_rdata(rr);
            if (dn_expand(response, response + len, data + 2, host, sizeof(host)) > 0) {

                std::string mx_host = host;

                // 1. Проверка на крупных провайдеров
                if (mx_host.find("yandex.net") != std::string::npos || mx_host.find("yandex.ru") != std::string::npos) {
                    return "imaps://imap.yandex.ru";
                }
                if (mx_host.find("google.com") != std::string::npos || mx_host.find("googlemail.com") != std::string::npos) {
                    return "imaps://imap.gmail.com";
                }
                if (mx_host.find("mail.ru") != std::string::npos) {
                    return "imaps://imap.mail.ru";
                }

                std::vector<std::string> prefixes = {"mx.", "inmx."};
                for (auto prefix : prefixes) {
                    if (mx_host.substr(0, prefix.length()) == prefix) {
                        return "imap." + mx_host.substr(prefix.length());
                    }
                }

                return std::format("imaps://{}", mx_host.substr(3));
            }
        }
    }

    return std::unexpected(Errors::ImapDiscover::DomainNotFound);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Получение IMAP URL через DNS SRV
 * @param domain Домен
 * @return URL IMAP или sошибку, если не удалось сформировать URL.
 */
std::expected<std::string, Errors::ImapDiscover> ImapDiscoveryTool::url_via_dns_srv(const std::string& domain) {

    unsigned char answer[1024];
    // Ищем защищенный IMAPS (_imaps._tcp.domain)
    std::string query = "_imaps._tcp." + domain;

    int len = res_search(query.c_str(), C_IN, T_SRV, answer, sizeof(answer));
    if (len < 0) {
        return std::unexpected(Errors::ImapDiscover::DomainNotFound);
    }

    ns_msg handle;
    ns_initparse(answer, len, &handle);
    ns_rr rr;

    // Берем первую найденную запись (AN - Answer section)
    if (ns_parserr(&handle, ns_s_an, 0, &rr) == 0) {
        const unsigned char* rdata = ns_rr_rdata(rr);
        // Структура SRV: Priority(2b), Weight(2b), Port(2b), Target(Name)
        int  port = (rdata[4] << 8) + rdata[5];
        char target[MAXDNAME];
        dn_expand(answer, answer + len, rdata + 6, target, sizeof(target));

        return std::format("imaps://{}:{}", target, port);
    }

    return std::unexpected(Errors::ImapDiscover::DomainNotFound);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Получение IMAP URL через протокол Mozilla Autoconfig.
 * @param email  Адрес почты
 * @param domain Домен из почты
 * @return URL IMAP или ошибку, если не удалось сформировать URL.
 */
std::expected<std::string, Errors::ImapDiscover> ImapDiscoveryTool::url_via_autoconf(const std::string& email, const std::string& domain) {

    CURL* curl_raw = curl_easy_init();
    if (!curl_raw) {
        log_error("failed curl_easy_init()");
        return std::unexpected(Errors::ImapDiscover::Internal);
    }

    auto deleter = [](CURL* curl) { curl_easy_cleanup(curl); };

    std::unique_ptr<CURL, decltype(deleter)> curl(curl_raw, deleter);

    // URL автоконфига Mozilla
    std::string url = std::format("https://autoconfig.{}/mail/config-v1.1.xml?emailaddress={}", domain, email);

    std::string response_data;
    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 5L);        // Таймаут на выпорлнение всего запроса = 5 сек
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L); // Следовать редиректам

    //: Из-за самоподписанных сертификатов могут быть ошибки - отключаем
    curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = execute_curl(curl.get());
    if (res != CURLE_OK) {
        log_error("failed CURL repform. code={}, error: {}", static_cast<int>(res), curl_easy_strerror(res));
        return std::unexpected(Errors::ImapDiscover::Internal);
    }

    std::string host = extract_imap_host(response_data);
    if (host.empty()) {
        return std::unexpected(Errors::ImapDiscover::DomainNotFound);
    }

    return std::format("imaps://{}", host);
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Выполнение запроса CURL
 * @param curl Объект
 * @return \a CURLcode
 */
CURLcode execute_curl(CURL* curl) noexcept {

    auto start = std::chrono::steady_clock::now();
    log_info("CURL starting request");

    CURLcode res = curl_easy_perform(curl);

    auto end     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    log_info("CURL request finished at: {}ms", elapsed);

    return res;
}
//----------------------------------------------------------------------------------------------------------------------

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t new_size = size * nmemb;
    s->append((char*) contents, new_size);
    return new_size;
}
//----------------------------------------------------------------------------------------------------------------------

std::string extract_imap_host(const std::string& xml) {
    // Ищем блок <incomingServer type="imap">
    size_t imap_pos = xml.find("type=\"imap\"");
    if (imap_pos == std::string::npos)
        return "";

    // Внутри этого блока ищем <hostname>
    size_t host_start = xml.find("<hostname>", imap_pos);
    if (host_start == std::string::npos)
        return "";
    host_start += 10; // Длина тега <hostname>

    size_t host_end = xml.find("</hostname>", host_start);
    if (host_end == std::string::npos)
        return "";

    return xml.substr(host_start, host_end - host_start);
}
//----------------------------------------------------------------------------------------------------------------------
