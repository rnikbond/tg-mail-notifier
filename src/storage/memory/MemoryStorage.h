//----------------------------------------------------------
#ifndef MEMORYSTORAGE_H
#define MEMORYSTORAGE_H
//----------------------------------------------------------
#include <unordered_map>
//----------------------------------------------------------
#include "storage/IStorage.h"
//----------------------------------------------------------

/**
 * @brief Класс для хранения информации о чатах в памяти, реализующий интерфейс IStorage
 */
class MemoryStorage : public IStorage {

public:

    MemoryStorage() = default;

public:

    virtual void create_chat(const ChatCipher& chat) override;
    virtual std::optional<ChatCipher> find_chat(int64_t chat_id) const noexcept override;
    virtual std::vector<ChatCipher> find_chats(const std::vector<int64_t>& chat_ids) const noexcept override;
    virtual std::vector<int64_t> chat_ids() const noexcept override;

    virtual void append_email(int64_t chat_id, const EmailCipher& email) override;
    virtual void update_email(int64_t chat_id, const EmailCipher& email) override;
    virtual void delete_email(int64_t chat_id, int64_t id) override;

    virtual void append_mail_server(const MailServer& server) override;
    virtual std::vector<MailServer> mail_servers() override;

private:

    std::unordered_map<int64_t, ChatCipher> m_data;
    std::unordered_map<std::string, std::string> m_mail_servers;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // MEMORYSTORAGE_H
