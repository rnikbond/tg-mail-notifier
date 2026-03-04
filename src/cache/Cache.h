//----------------------------------------------------------
#ifndef CACHE_H
#define CACHE_H
//----------------------------------------------------------
#include <memory>
#include <shared_mutex>
//----------------------------------------------------------
#include "repository/IRepository.h"
#include "storage/IStorage.h"
//----------------------------------------------------------

/**
 * @brief Класс-кэш информации о чатах, реализующий интерфейс IRepository
 * 
 * Данный класс, помимо кэширования, также управляет данными в хранилище
 */
class Cache : public IRepository {

public:

    Cache(std::unique_ptr<IStorage> storage);
    ~Cache() = default;

public:

    virtual ChatResult create_chat(const Chat& chat) noexcept override;
    [[nodiscard]]
    virtual ChatResult find_chat(int64_t chat_id) noexcept override;
    [[nodiscard]]
    virtual ChatsResult find_chats(const std::vector<int64_t>& chat_ids) noexcept override;
    [[nodiscard]]
    virtual Chats chats() noexcept override;

    virtual ChatResult append_email(int64_t chat_id, const Email& email) noexcept override;
    virtual ChatResult update_email(int64_t chat_id, const Email& email) noexcept override;
    virtual ChatResult update_email_uid(int64_t chat_id, int64_t email_id, int64_t uid) noexcept override;
    virtual bool delete_email(int64_t chat_id, int64_t email_id) noexcept override;

private:

    std::shared_mutex m_mutex;

    std::unique_ptr<IStorage> m_storage;
    std::unordered_map<int64_t, std::shared_ptr<Chat>> m_cache_data;

private:

    void reload_from_storage();
    std::unordered_map<int64_t, std::shared_ptr<Chat>>::iterator append(int64_t chat_id);
    std::vector<std::shared_ptr<Chat>> append(const std::vector<int64_t>& chat_ids);
    std::shared_ptr<Chat> refresh(int64_t chat_id);

private:

    bool is_correct_email_addr(const std::string_view& addr) const noexcept;

    ChatCipher convert_chat_to_cipher(const Chat& chat) const;
    Chat convert_chat_from_cipher(const ChatCipher& chat_cipher) const;
    EmailCipher convert_email_to_cipher(const Email& email) const;
    Email convert_email_from_cipher(const EmailCipher& email_cipher) const;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // CACHE_H
