//----------------------------------------------------------
#ifndef CACHE_H
#define CACHE_H
//----------------------------------------------------------
#include <memory>
//----------------------------------------------------------
#include "repository/IRepository.h"
#include "storage/IStorage.h"
//----------------------------------------------------------

/**
 * @brief Класс-кэш информации о чатах, реализующий интерфейс репозитория
 * 
 * Данный класс, помимо кэширования, также управляет данными в хранилище
 */
class Cache : public IRepository {

public:

    Cache(std::unique_ptr<IStorage> storage);
    ~Cache() = default;

public:

    [[nodiscard]] virtual ChatPtr create(const Chat& chat) noexcept override;
    [[nodiscard]] virtual bool update(const Chat& chat) noexcept override;
    [[nodiscard]] virtual ChatPtr find(int64_t chat_id) noexcept override;
    [[nodiscard]] virtual std::unordered_map<int64_t, Email> chats() noexcept override;

private:

    std::unique_ptr<IStorage> m_storage;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // CACHE_H
