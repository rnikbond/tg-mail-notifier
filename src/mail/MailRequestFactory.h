//----------------------------------------------------------
#ifndef MAILREQUESTFACTORY_H
#define MAILREQUESTFACTORY_H
//----------------------------------------------------------
#include <functional>
#include <memory>
//----------------------------------------------------------
#include "mail/IMailRequest.h"
//----------------------------------------------------------

/**
 * @brief Класс-фабрика для создания объектов для выполнения запросов к серверу электронной почты
 */
class MailRequestFactory {

public:

    MailRequestFactory()  = default;
    ~MailRequestFactory() = default;

public:

    using MailObject  = std::unique_ptr<IMailRequest>;
    using MailCreator = std::function<std::unique_ptr<IMailRequest>()>;

public:

    static MailObject create();
    static void setCreator(MailCreator func);
    static void resetCreator();

private:

    static MailCreator default_func;
    static MailCreator& create_func();
};
//----------------------------------------------------------------------------------------------------------------------

#endif // MAILREQUESTFACTORY_H
