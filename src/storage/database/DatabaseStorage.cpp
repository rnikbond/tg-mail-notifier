//----------------------------------------------------------
#include <charconv>
//----------------------------------------------------------
#include <sqlite3.h>
//----------------------------------------------------------
#include "generated_migrations.h"
//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "DatabaseStorage.h"
//----------------------------------------------------------
#define LATEST_DB_VERSION 1
//----------------------------------------------------------

void DatabaseStorage::SQLiteDeleter::operator()(sqlite3* db) const {
    if (db) {
        sqlite3_close(db);
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Конструктор класса
 * @param dsn Строка подключения к БД (Data Source Name)
 * 
 * @throw std::runtime_error Выбрасывается, если:
 * - Не удалось подключиться в БД
 * - Не удалось накатить миграции
 */
DatabaseStorage::DatabaseStorage(const std::string& dsn) {

    sqlite3* db_raw;
    int      err = sqlite3_open(dsn.c_str(), &db_raw);
    if (err != SQLITE_OK) {
        sqlite3_close(db_raw);
        throw std::runtime_error(std::format("failed database connection: DSN={}, error: {}", dsn, sqlite3_errmsg(db_raw)));
    }

    m_db.reset(db_raw);
    apply_migrations();
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Создание нового чата
 * @param chat Данные чата
 */
void DatabaseStorage::create_chat(const Chat& chat) {
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Поиск чата
 * @param chat_id Идентификатор чата
 * @return Данные чата, если он найден
 */
std::optional<Chat> DatabaseStorage::find_chat(int64_t chat_id) const noexcept {
    return std::nullopt;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Поиск чатов
 * @param chat_ids Список идентификаторов чатов
 * @return Данные найденных чатов
 * 
 * Если какие-либо чаты не найдены, вернётся список только найденных
 */
std::vector<Chat> DatabaseStorage::find_chats(const std::vector<int64_t>& chat_ids) const noexcept {
    return {};
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Получение идетнификторов всех чатов
 * @return Список идентификаторов чатов
 */
std::vector<int64_t> DatabaseStorage::chat_ids() const noexcept {
    return {};
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Добавление новой почты
 * @param chat_id Идентификатор чата
 * @param email   Данные электронной почты
 */
void DatabaseStorage::append_email(int64_t chat_id, const Email& email) {
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Обновление данных об электронной почте
 * @param chat Данные чата
 */
void DatabaseStorage::update_email(int64_t chat_id, const Email& email) {
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Удаление данных об электронной почте
 * @param chat_id  Идентификатор чата
 * @param email_id Идентификатор электронной почты
 */
void DatabaseStorage::delete_email(int64_t chat_id, int64_t email_id) {
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Применение миграций базы данных
 */
void DatabaseStorage::apply_migrations() {

    int version = db_version();
    if (LATEST_DB_VERSION == version) {
        log_info("database is up to date. version: {}", version);
        return;
    }

    log_info("database is outdated. version: {}, latest: {}", version, LATEST_DB_VERSION);

    for (const auto& [name, sql] : migration_files) {

        if (!name.ends_with(".up.sql")) {
            continue;
        }

        int migration_ver = 0;

        { //: Парсим имя файла, чтоб получить версию
            size_t pos = name.find("_");
            if (pos == std::string::npos) {
                log_error("invalid migration: {}", name);
                break;
            }

            auto [_, err] = std::from_chars(name.data(), name.data() + pos, migration_ver);
            if (err != std::errc()) {
                log_error("invalid migration: {}", name);
                break;
            }
        }

        if (migration_ver <= version) {
            continue;
        }

        log_info("applying migration : {}", name);
        apply_migration(migration_ver, sql);
        log_info("migration successfully applied. migration: {}", name);

        if (migration_ver == LATEST_DB_VERSION) {
            break;
        }
    }

    version = db_version();
    if (LATEST_DB_VERSION == version) {
        log_info("database is up to date. version: {}", version);
    } else {
        throw std::runtime_error(std::format("database is not up to date. version: {}, latest: {}", version, LATEST_DB_VERSION));
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief DatabaseStorage::apply_migration
 * @param migration_ver Версия миграции
 * @param sql           SQL запрос
 * 
 * @throw std::runtime_error Выбрасывается в случае ошибки при применении миграции
 */
void DatabaseStorage::apply_migration(int migration_ver, const std::string& sql) {

    int err = sqlite3_exec(m_db.get(), "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (err != SQLITE_OK) {
        throw std::runtime_error(std::format("BEGIN TRANSACTION failed. migration={}", migration_ver));
    }

    err = sqlite3_exec(m_db.get(), sql.c_str(), nullptr, nullptr, nullptr);
    if (err != SQLITE_OK) {
        std::string text = std::format("sql exec failed. migration={}", migration_ver);
        throw std::runtime_error(text);
    }

    bool ok = set_db_version(migration_ver);
    if (!ok) {
        throw std::runtime_error(std::format("change db version failed. migration: {}", migration_ver));
    }

    err = sqlite3_exec(m_db.get(), "COMMIT;", nullptr, nullptr, nullptr);
    if (err != SQLITE_OK) {
        std::string text = std::format("COMMIT failed. migration={}", migration_ver);
        throw std::runtime_error(text);
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
