//----------------------------------------------------------
#include "MailRequest.h"
//----------------------------------------------------------
#include "MailRequestFactory.h"
//----------------------------------------------------------

MailRequestFactory::MailObject MailRequestFactory::create() {
    return create_func()();
}
//----------------------------------------------------------------------------------------------------------------------

void MailRequestFactory::setCreator(MailCreator func) {
    create_func() = std::move(func);
}
//----------------------------------------------------------------------------------------------------------------------

void MailRequestFactory::resetCreator() {
    create_func() = []() { return std::make_unique<MailRequest>(); };
}
//----------------------------------------------------------------------------------------------------------------------

MailRequestFactory::MailCreator& MailRequestFactory::create_func() {

    static MailCreator func = []() { return std::make_unique<MailRequest>(); };
    return func;
}
//----------------------------------------------------------------------------------------------------------------------
