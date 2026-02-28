//----------------------------------------------------------
#include "../src/core/logger.h"
//----------------------------------------------------------
#include "Cache.h"
//----------------------------------------------------------

/*!
 * @brief Конструктор кэша
 * @param storage Указатель на объект, реализующий интерфейс хранилища
 */
Cache::Cache(std::unique_ptr<IStorage> storage)
    : m_storage(std::move(storage)) {
}
//----------------------------------------------------------------------------------------------------------------------

ChatOpt Cache::create(const Chat& chat) noexcept {

    if (chat.chat_id == 0 || chat.username.empty()) {
        log_error("chat not have id or username. chat_id = {}, username = {}", chat.chat_id, chat.username);
        return std::nullopt;
    }

    std::unique_lock lock(m_mutex);

    try {
        m_storage->create(chat);
    } catch (const std::exception& ex) {
        log_error("failed create in storage: {}. chat_id = {}", ex.what(), chat.chat_id);
        return std::nullopt;
    }

    if (!m_data.contains(chat.chat_id)) {
        auto chat_shared     = std::make_shared<Chat>(chat);
        m_data[chat.chat_id] = chat_shared;
        return chat_shared;
    }

    return m_data.at(chat.chat_id);
}
//----------------------------------------------------------------------------------------------------------------------

bool Cache::update(const Chat& chat) noexcept {

    if (chat.chat_id == 0 || chat.username.empty()) {
        log_error("chat not have id or username. chat_id = {}, username = {}", chat.chat_id, chat.username);
        return false;
    }

    std::unique_lock lock(m_mutex);

    try {
        m_storage->update(chat);
    } catch (const std::exception& ex) {
        log_error("failed update in storage: {}. chat id = {}", ex.what(), chat.chat_id);
        return false;
    }

    m_data[chat.chat_id] = std::make_shared<Chat>(chat);
    return true;
}
//----------------------------------------------------------------------------------------------------------------------

bool Cache::update_email(int64_t chat_id, const Email& email) noexcept {

    auto res = find({chat_id});
    if (!res.has_value() || res.value().size() != 1) {
        log_error("not found chat: {}", chat_id);
        return false;
    }

    std::unique_lock lock(m_mutex);

    Chat chat  = *res.value().at(0);
    chat.email = email;

    try {
        m_storage->update(chat);
    } catch (const std::exception& ex) {
        log_error("failed update in storage: {}. chat id = {}", ex.what(), chat.chat_id);
        return false;
    }

    m_data[chat.chat_id] = std::make_shared<Chat>(chat);
    return true;
}
//----------------------------------------------------------------------------------------------------------------------

ChatsOpt Cache::find(const std::vector<int64_t>& chat_ids) noexcept {

    std::vector<std::shared_ptr<const Chat>> chats;
    std::vector<int64_t>                     load_chats_ids;

    {
        std::shared_lock lock(m_mutex);

        for (int64_t chat_id : chat_ids) {
            if (m_data.contains(chat_id)) {
                chats.push_back(m_data.at(chat_id));
            } else {
                load_chats_ids.push_back(chat_id);
            }
        }
    }

    if (load_chats_ids.size() != 0) {

        std::unique_lock lock(m_mutex);

        auto res = m_storage->find(load_chats_ids);
        if (res.has_value()) {
            auto chats_storage = res.value();
            for (const Chat& chat : chats_storage) {
                auto chat_shared     = std::make_shared<Chat>(chat);
                m_data[chat.chat_id] = chat_shared;
                chats.push_back(chat_shared);
            }
        } else {
            log_error("not found chats in storage: {}", fmt::join(load_chats_ids, ","));
        }
    }

    return chats;
}
//----------------------------------------------------------------------------------------------------------------------

std::unordered_map<int64_t, Email> Cache::chats() noexcept {

    auto chats_ids = m_storage->chat_ids();
    auto res       = find(chats_ids);
    if (!res.has_value()) {
        log_error("not found chats in: {}", fmt::join(chats_ids, ","));
        return {};
    }

    std::unordered_map<int64_t, Email> chats_email;

    auto chats_map = res.value();
    for (const auto& chat : chats_map) {
        chats_email[chat->chat_id] = chat->email;
    }

    return chats_email;
}
//----------------------------------------------------------------------------------------------------------------------
