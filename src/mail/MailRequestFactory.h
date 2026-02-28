//----------------------------------------------------------
#ifndef MAILREQUESTFACTORY_H
#define MAILREQUESTFACTORY_H
//----------------------------------------------------------
#include <functional>
#include <memory>
//----------------------------------------------------------
#include "mail/IMailRequest.h"
//----------------------------------------------------------

class MailRequestFactory {

    using MailObject  = std::unique_ptr<IMailRequest>;
    using MailCreator = std::function<std::unique_ptr<IMailRequest>()>;

public:

    MailRequestFactory()  = default;
    ~MailRequestFactory() = default;

public:

    static MailObject create();
    static void setCreator(MailCreator func);
    static void resetCreator();

private:

    static MailCreator& create_func();
};
//----------------------------------------------------------------------------------------------------------------------

#endif // MAILREQUESTFACTORY_H
