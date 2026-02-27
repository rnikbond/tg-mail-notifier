//----------------------------------------------------------
#ifndef MAILFACTORY_H
#define MAILFACTORY_H
//----------------------------------------------------------
#include <functional>
#include <memory>
//----------------------------------------------------------
#include "mail/IMailRequest.h"
//----------------------------------------------------------

class MailFactory {

    using MailObject  = std::unique_ptr<IMailRequest>;
    using MailCreator = std::function<std::unique_ptr<IMailRequest>()>;

public:

    MailFactory()  = default;
    ~MailFactory() = default;

public:

    static MailObject create();
    static void setCreator(MailCreator func);
    static void resetCreator();

private:

    static MailCreator& create_func();
};
//----------------------------------------------------------------------------------------------------------------------

#endif // MAILFACTORY_H
