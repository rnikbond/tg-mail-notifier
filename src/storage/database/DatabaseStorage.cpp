//----------------------------------------------------------
#include <sqlite3.h>
//----------------------------------------------------------
#include "generated_migrations.h"
//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "DatabaseStorage.h"
//----------------------------------------------------------

void DatabaseStorage::SQLiteDeleter::operator()(sqlite3* db) const {
    if (db) {
        sqlite3_close(db);
    }
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Конструктор класса
 * @param db_name Имя базы данных
 */
DatabaseStorage::DatabaseStorage(const char* db_name) {

    // sqlite3* db_raw;
    // int      err = sqlite3_open(db_name, &db_raw);
    // if (err != SQLITE_OK) {
    //     sqlite3_close(db_raw);
    //     throw std::runtime_error(std::format("failed database connection: db_name={}, error: {}", db_name, sqlite3_errmsg(db_raw)));
    // }

    // m_db.reset(db_raw);
    apply_migrations();
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Применение миграций базы данных
 */
void DatabaseStorage::apply_migrations() {

    for (const auto& [name, sql] : migration_files) {
        log_info("rcc sql file: {}", name);
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Получение текущей версии базы данных
 * @return Номер версии
 */
int DatabaseStorage::db_version() {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(m_db.get(), "PRAGMA user_version", -1, &stmt, nullptr);
    sqlite3_step(stmt);
    int version = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return version;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Изменение версии базы данных
 * @param version Новая версия
 * @return TRUE, если версия изменена. Иначе FALSE.
 */
bool DatabaseStorage::set_db_version(int version) {

    std::string sql = std::format("PRAGMA user_version = {};", version);

    int err = sqlite3_exec(m_db.get(), sql.c_str(), nullptr, nullptr, nullptr);
    if (err != SQLITE_OK) {
        log_error("failed set shema version: {}", sqlite3_errmsg(m_db.get()));
        return false;
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------
