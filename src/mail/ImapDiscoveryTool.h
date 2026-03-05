//----------------------------------------------------------
#ifndef IMAPDISCOVERYTOOL_H
#define IMAPDISCOVERYTOOL_H
//----------------------------------------------------------
#include <expected>
#include <string>
//----------------------------------------------------------
#include "chat/Chat.h"
#include "errs/Errors.h"
//----------------------------------------------------------

class ImapDiscoveryTool {

public:

    ImapDiscoveryTool()  = default;
    ~ImapDiscoveryTool() = default;

public:

    static std::expected<std::string, Errors::ImapDiscover> domain_from_email(const std::string& email);
    static bool is_correct_email(const std::string_view& email);

    static std::expected<MailServer, Errors::ImapDiscover> detect_mail_server(const std::string& email);

private:

    static std::expected<bool, Errors::ImapDiscover> check_imap_server(const std::string& imap_url);

    static std::expected<std::string, Errors::ImapDiscover> url_by_domain(const std::string& domain);
    static std::expected<std::string, Errors::ImapDiscover> url_via_mx(const std::string& domain);
    static std::expected<std::string, Errors::ImapDiscover> url_via_dns_srv(const std::string& domain);
    static std::expected<std::string, Errors::ImapDiscover> url_via_autoconf(const std::string& email, const std::string& domain);
};
//----------------------------------------------------------------------------------------------------------------------

#endif // IMAPDISCOVERYTOOL_H
