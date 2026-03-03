//----------------------------------------------------------
#ifndef DATABASESTORAGE_H
#define DATABASESTORAGE_H
//----------------------------------------------------------
#include <memory>
//----------------------------------------------------------
#include "storage/IStorage.h"
//----------------------------------------------------------
struct sqlite3;
//----------------------------------------------------------

/**
 * @brief Класс для хранения информации о чатах в базе данных, реализующий интерфейс IStorage
 */
class DatabaseStorage : public IStorage {
public:

    DatabaseStorage(const std::string& dsn);
    ~DatabaseStorage() = default;

public:

    virtual void create_chat(const Chat& chat) override;
    [[nodiscard]] virtual std::optional<Chat> find_chat(int64_t chat_id) const noexcept override;
    [[nodiscard]] virtual std::vector<Chat> find_chats(const std::vector<int64_t>& chat_ids) const noexcept override;
    [[nodiscard]] virtual std::vector<int64_t> chat_ids() const noexcept override;

    virtual void append_email(int64_t chat_id, const Email& email) override;
    virtual void update_email(int64_t chat_id, const Email& email) override;
    virtual void delete_email(int64_t chat_id, int64_t email_id) override;

private:

    struct SQLiteDeleter
    {
        void operator()(sqlite3* db) const;
    };

    std::unique_ptr<sqlite3, SQLiteDeleter> m_db;

private:

    void apply_migrations();
    void apply_migration(int migration_ver, const std::string& sql);

    int db_version();
    bool set_db_version(int version);
};
//----------------------------------------------------------------------------------------------------------------------

#endif // DATABASESTORAGE_H
