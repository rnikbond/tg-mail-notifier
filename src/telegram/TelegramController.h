//----------------------------------------------------------
#ifndef TELEGRAMCONTROLLER_H
#define TELEGRAMCONTROLLER_H
//----------------------------------------------------------
#include <memory>
//----------------------------------------------------------
#include "nlohmann/json_fwd.hpp"
//----------------------------------------------------------
#include "repository/IRepository.h"
#include "telegram/TelegramAPI.h"
//----------------------------------------------------------
using json = nlohmann::json;
//----------------------------------------------------------

class TelegramController {

public:

    TelegramController(const std::string& token, std::shared_ptr<IRepository> repo);

    std::optional<TelegramRequest> process(const TelegramResponse&& response, int64_t& last_msg_id);

private:

    std::string                  m_token;
    std::shared_ptr<IRepository> m_repo;

private:

    int64_t find_chat_id(const json& body_js, int idx, const std::string_view tag);
    std::shared_ptr<const Chat> register_chat(int64_t chat_id, int idx, const json& body_js);
};
//----------------------------------------------------------------------------------------------------------------------

#endif // TELEGRAMCONTROLLER_H
