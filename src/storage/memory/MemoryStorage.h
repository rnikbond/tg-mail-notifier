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

    virtual void create(const Chat& chat) override;
    virtual void udpate(const Chat& chat) override;
    virtual std::optional<std::vector<Chat>> find(const std::set<int64_t>& chat_ids) const noexcept override;
    virtual std::set<int64_t> chat_ids() const noexcept override;

private:

    std::unordered_map<int64_t, Chat> m_data;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // MEMORYSTORAGE_H
