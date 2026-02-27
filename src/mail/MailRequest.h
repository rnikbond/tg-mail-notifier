//----------------------------------------------------------
#ifndef MAILREQUEST_H
#define MAILREQUEST_H
//----------------------------------------------------------
#include "mail/IMailRequest.h"
//----------------------------------------------------------

class MailRequest : public IMailRequest {
public:

    MailRequest()  = default;
    ~MailRequest() = default;

    [[nodiscard]] UIDsOpt load_uids(const Email& email) const noexcept override;
    [[nodiscard]] UIDOpt last_uid(const Email& email) const noexcept override;
    [[nodiscard]] EmailMsgExp fetch_email(const Email& email, int64_t uid) const noexcept override;

private:

    Errors::Mail execute(const Email& email, const std::string& request, std::string& response) const;
    Errors::Mail execute_body(const Email& email, const std::string& request, std::string& response) const;

    void extract_text_gmime(const std::string& raw_email, std::string& sender, std::string& dt, std::string& title, std::string& body) const;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // MAILREQUEST_H
