//----------------------------------------------------------
#ifndef DATABASESTORAGE_H
#define DATABASESTORAGE_H
//----------------------------------------------------------
#include <memory>
//----------------------------------------------------------
struct sqlite3;
//----------------------------------------------------------

/**
 * @brief Класс для хранения информации о чатах в базе данных, реализующий интерфейс IStorage
 */
class DatabaseStorage {
public:

    DatabaseStorage(const char* db_name);
    ~DatabaseStorage() = default;

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
