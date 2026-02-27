//----------------------------------------------------------
#include "MailRequest.h"
//----------------------------------------------------------
#include "MailFactory.h"
//----------------------------------------------------------

MailFactory::MailObject MailFactory::create() {
    return create_func()();
}
//----------------------------------------------------------------------------------------------------------------------

void MailFactory::setCreator(MailCreator func) {
    create_func() = std::move(func);
}
//----------------------------------------------------------------------------------------------------------------------

void MailFactory::resetCreator() {
    create_func() = []() { return std::make_unique<MailRequest>(); };
}
//----------------------------------------------------------------------------------------------------------------------

MailFactory::MailCreator& MailFactory::create_func() {

    static MailCreator func = []() { return std::make_unique<MailRequest>(); };
    return func;
}
//----------------------------------------------------------------------------------------------------------------------
