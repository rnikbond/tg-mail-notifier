//----------------------------------------------------------
#ifndef MAILREQUEST_H
#define MAILREQUEST_H
//----------------------------------------------------------
#include "mail/IMailRequest.h"
//----------------------------------------------------------

class MailRequest : public IMailRequest {
public:

    MailRequest();

    [[nodiscard]] UIDsOpt load_uids(const Email& email) const noexcept override;
    [[nodiscard]] UIDOpt last_uid(const Email& email) const noexcept override;

private:

    Errors::Mail execute(const Email& email, const std::string& request, std::string& response) const;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // MAILREQUEST_H
