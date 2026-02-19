//----------------------------------------------------------
#include <ranges>
//----------------------------------------------------------
#include <spdlog/spdlog.h>
//----------------------------------------------------------
#include "MemoryStorage.h"
//----------------------------------------------------------
namespace logger = spdlog;
//----------------------------------------------------------

MemoryStorage::MemoryStorage() {
}
//----------------------------------------------------------------------------------------------------------------------

void MemoryStorage::create(const Chat& chat) {

    if (m_data.contains(chat.chat_id)) {
        throw std::runtime_error(std::format("chat already exists. chat id: {}", chat.chat_id));
    }

    m_data[chat.chat_id] = chat;
}
//----------------------------------------------------------------------------------------------------------------------

void MemoryStorage::udpate(const Chat& chat) {

    if (!m_data.contains(chat.chat_id)) {
        throw std::runtime_error(std::format("chat not found. chat id: {}", chat.chat_id));
    }

    m_data[chat.chat_id] = chat;
}
//----------------------------------------------------------------------------------------------------------------------

std::optional<std::vector<Chat>> MemoryStorage::find(const std::vector<int64_t>& chat_ids) const noexcept {

    std::vector<Chat> chats;

    for (int64_t chat_id : chat_ids) {
        if (m_data.contains(chat_id)) {
            chats.push_back(m_data.at(chat_id));
        } else {
            logger::error("[ MemoryStorage::find] not found chat id: {}", chat_id);
        }
    }

    return chats;
}
//----------------------------------------------------------------------------------------------------------------------

std::vector<int64_t> MemoryStorage::chat_ids() const noexcept {

    std::vector<int64_t> ids;

    for (int64_t chat_id : m_data | std::views::keys) {
        ids.push_back(chat_id);
    }

    return {};
}
//----------------------------------------------------------------------------------------------------------------------
