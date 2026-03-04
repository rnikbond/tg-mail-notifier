//----------------------------------------------------------
#ifndef DATABASESTORAGE_H
#define DATABASESTORAGE_H
//----------------------------------------------------------
#include <memory>
//----------------------------------------------------------
#include "storage/IStorage.h"
//----------------------------------------------------------
struct sqlite3;
struct sqlite3_stmt;
//----------------------------------------------------------

/**
 * @brief Класс для хранения информации о чатах в базе данных, реализующий интерфейс IStorage
 */
class DatabaseStorage : public IStorage {
public:

    DatabaseStorage(const std::string& dsn);
    ~DatabaseStorage() = default;

public:

    virtual void create_chat(const ChatCipher& chat) override;
    [[nodiscard]] virtual std::optional<ChatCipher> find_chat(int64_t chat_id) const noexcept override;
    [[nodiscard]] virtual std::vector<ChatCipher> find_chats(const std::vector<int64_t>& chat_ids) const noexcept override;
    [[nodiscard]] virtual std::vector<int64_t> chat_ids() const noexcept override;

    virtual void append_email(int64_t chat_id, const EmailCipher& email) override;
    virtual void update_email(int64_t chat_id, const EmailCipher& email) override;
    virtual void delete_email(int64_t chat_id, int64_t email_id) override;

private:

    struct SQLiteDeleter
    {
        void operator()(sqlite3* db) const;
    };

    std::unique_ptr<sqlite3, SQLiteDeleter> m_db;

private:

    void set_text(sqlite3_stmt* stmt, int col, std::string& dest) const noexcept;
    void set_blob(sqlite3_stmt* stmt, int col, std::vector<uint8_t>& dest) const noexcept;

    void apply_migrations();
    void apply_migration(int migration_ver, const std::string& sql);

    int db_version();
    bool set_db_version(int version);
};
//----------------------------------------------------------------------------------------------------------------------

#endif // DATABASESTORAGE_H
