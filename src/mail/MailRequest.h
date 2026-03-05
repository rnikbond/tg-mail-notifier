//----------------------------------------------------------
#ifndef MAILREQUEST_H
#define MAILREQUEST_H
//----------------------------------------------------------
#include "mail/IMailRequest.h"
//----------------------------------------------------------
class MailRequestFactory;
//----------------------------------------------------------
typedef void CURL;
//----------------------------------------------------------

/**
 * @brief Класс для запросов к почтовому серверу, реализующий интерфейс IMailRequest
 * 
 * Данный класс должен создаваться через фибрику объектов.
 * @sa MailRequestFactory
 */
class MailRequest : public IMailRequest {

private:

    MailRequest() = default;
    friend class MailRequestFactory;

public:

    ~MailRequest() = default;

public:

    [[nodiscard]] UIDsResult load_uids(const Email& email) const noexcept override;
    [[nodiscard]] UIDResult last_uid(const Email& email) const noexcept override;
    [[nodiscard]] MailMsgResult fetch_email(const Email& email, int64_t uid) const noexcept override;

private:

    Errors::Mail execute_request(const Email& email, const std::string& request, std::string& response) const noexcept;
    Errors::Mail execute_url(const Email& email, const std::string& url, std::string& response) const noexcept;

    int execute_curl(CURL* curl) const noexcept;

    void extract_text_gmime(const std::string& raw_email, std::string& sender, std::string& dt, std::string& title, std::string& body) const noexcept;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // MAILREQUEST_H
