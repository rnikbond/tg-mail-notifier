//----------------------------------------------------------
#ifndef MEMORYSTORAGE_H
#define MEMORYSTORAGE_H
//----------------------------------------------------------
#include <unordered_map>
//----------------------------------------------------------
#include "storage/IStorage.h"
//----------------------------------------------------------

/**
 * @brief Класс для хранения информации о чатах в памяти, реализующий интерфейс хранилища
 */
class MemoryStorage : public IStorage {

public:

    MemoryStorage();

public:

    virtual void create_chat(const Chat& chat) override;
    virtual std::optional<Chat> find_chat(int64_t chat_id) const noexcept override;
    virtual std::vector<Chat> find_chats(const std::vector<int64_t>& chat_ids) const noexcept override;
    virtual std::vector<int64_t> chat_ids() const noexcept override;

    virtual void append_email(int64_t chat_id, const Email& email) override;
    virtual void update_email(int64_t chat_id, const Email& email) override;
    virtual void delete_email(int64_t chat_id, int64_t id) override;

private:

    std::unordered_map<int64_t, Chat> m_data;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // MEMORYSTORAGE_H
