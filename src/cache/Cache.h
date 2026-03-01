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
 * @brief Класс-кэш информации о чатах, реализующий интерфейс репозитория
 * 
 * Данный класс, помимо кэширования, также управляет данными в хранилище
 */
class Cache : public IRepository {

public:

    Cache(std::unique_ptr<IStorage> storage);
    ~Cache() = default;

public:

    [[nodiscard]] virtual ChatOpt create(const Chat& chat) noexcept override;
    [[nodiscard]] virtual bool update(const Chat& chat) noexcept override;
    [[nodiscard]] virtual bool update_email(int64_t chat_id, const Email& email) noexcept override;
    [[nodiscard]] virtual ChatsOpt find(const std::vector<int64_t>& chat_ids) noexcept override;
    [[nodiscard]] virtual std::unordered_map<int64_t, Email> chats() noexcept override;

private:

    std::shared_mutex m_mutex;

    std::unique_ptr<IStorage> m_storage;
    std::unordered_map<int64_t, std::shared_ptr<Chat>> m_data;

private:

    std::shared_ptr<Chat> refresh(int64_t chat_id);
};
//----------------------------------------------------------------------------------------------------------------------

#endif // CACHE_H
