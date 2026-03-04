//----------------------------------------------------------
#include <charconv>
#include <ranges>
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
 * 
 * @throw std::runtime_error Выбрасывается в случае ошибки выполнения SQL запроса
 */
void DatabaseStorage::create_chat(const ChatCipher& chat) {

    constexpr const char* sql_insert_chats = R"(INSERT INTO chats VALUES(?, ?, ?, ?);)";
    constexpr const char* sql_insert_email = R"(INSERT INTO emails VALUES(?, ?, ?, ?, ?, ?, ?);)";

    int err = sqlite3_exec(m_db.get(), "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (err != SQLITE_OK) {
        throw std::runtime_error(std::format("failed begin transaction. sql error: {}", sqlite3_errmsg(m_db.get())));
    }

    auto deleter = [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); };

    std::unique_ptr<sqlite3_stmt, decltype(deleter)> stmt_chats;

    {
        sqlite3_stmt* stmt_chats_raw;
        err = sqlite3_prepare_v2(m_db.get(), sql_insert_chats, -1, &stmt_chats_raw, nullptr);

        stmt_chats.reset(stmt_chats_raw);
        if (err != SQLITE_OK) {
            throw std::runtime_error(std::format("failed prepare chats. sql error: {}", sqlite3_errmsg(m_db.get())));
        }
    }

    //: Вставка в chats
    sqlite3_bind_int64(stmt_chats.get(), 1, chat.id);                             //: id
    sqlite3_bind_text(stmt_chats.get(), 2, chat.username.c_str(), -1, nullptr);   //: username
    sqlite3_bind_text(stmt_chats.get(), 3, chat.first_name.c_str(), -1, nullptr); //: first_name
    sqlite3_bind_text(stmt_chats.get(), 4, chat.last_name.c_str(), -1, nullptr);  //: last_name
    if (sqlite3_step(stmt_chats.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::format("failed insert chat. sql error: {}", sqlite3_errmsg(m_db.get())));
    }

    if (!chat.emails.empty()) {

        std::unique_ptr<sqlite3_stmt, decltype(deleter)> stmt_emails;

        {
            sqlite3_stmt* stmt_emails_raw;
            err = sqlite3_prepare_v2(m_db.get(), sql_insert_email, -1, &stmt_emails_raw, nullptr);

            stmt_emails.reset(stmt_emails_raw);
            if (err != SQLITE_OK) {
                throw std::runtime_error(std::format("failed prepare emails. sql error: {}", sqlite3_errmsg(m_db.get())));
            }
        }

        //: Вставка email
        for (const EmailCipher& email : chat.emails | std::views::values) {
            sqlite3_bind_null(stmt_emails.get(), 1);                                                                  //: id
            sqlite3_bind_int64(stmt_emails.get(), 2, chat.id);                                                        //: chat_id
            sqlite3_bind_text(stmt_emails.get(), 3, email.address.c_str(), -1, nullptr);                              //: address
            sqlite3_bind_int64(stmt_emails.get(), 4, email.last_uid);                                                 //: last_uid
            sqlite3_bind_blob(stmt_emails.get(), 5, email.password.data.data(), email.password.data.size(), nullptr); //: password
            sqlite3_bind_blob(stmt_emails.get(), 6, email.password.iv.data(), email.password.iv.size(), nullptr);     //: password_iv
            sqlite3_bind_blob(stmt_emails.get(), 7, email.password.tag.data(), email.password.tag.size(), nullptr);   //: password_tag

            if (sqlite3_step(stmt_emails.get()) != SQLITE_DONE) {
                throw std::runtime_error(std::format("failed insert email. sql error: {}", sqlite3_errmsg(m_db.get())));
            }

            sqlite3_reset(stmt_emails.get());
        }
    }

    err = sqlite3_exec(m_db.get(), "COMMIT;", nullptr, nullptr, nullptr);
    if (err != SQLITE_OK) {
        throw std::runtime_error(std::format("failed commit. sql error: {}", sqlite3_errmsg(m_db.get())));
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Поиск чата
 * @param chat_id Идентификатор чата
 * @return Данные чата, если он найден
 */
std::optional<ChatCipher> DatabaseStorage::find_chat(int64_t chat_id) const noexcept {

    constexpr const char* sql_chats  = "SELECT * FROM chats WHERE id = ?;";
    constexpr const char* sql_emails = "SELECT id, address, last_uid, password, password_iv, password_tag FROM emails WHERE chat_id = ?;";

    auto deleter = [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); };

    std::unique_ptr<sqlite3_stmt, decltype(deleter)> stmt_chats;
    std::unique_ptr<sqlite3_stmt, decltype(deleter)> stmt_emails;

    sqlite3_stmt* stmt_chats_raw;
    sqlite3_stmt* stmt_emails_raw;

    int err = sqlite3_prepare_v2(m_db.get(), sql_chats, -1, &stmt_chats_raw, nullptr);

    stmt_chats.reset(stmt_chats_raw);
    if (err != SQLITE_OK) {
        log_error("sql error on chats: {}", sqlite3_errmsg(m_db.get()));
        return std::nullopt;
    }

    err = sqlite3_prepare_v2(m_db.get(), sql_emails, -1, &stmt_emails_raw, nullptr);

    stmt_emails.reset(stmt_emails_raw);
    if (err != SQLITE_OK) {
        log_error("sql error on emails: {}", sqlite3_errmsg(m_db.get()));
        return std::nullopt;
    }

    //: chats
    sqlite3_bind_int64(stmt_chats.get(), 1, chat_id); //: chats.id

    //: emails
    sqlite3_bind_int64(stmt_emails.get(), 1, chat_id); //: emails.chat_id

    //: Выполнение запроса к chats
    int rc_chats = sqlite3_step(stmt_chats.get());
    if (rc_chats != SQLITE_ROW) {
        return std::nullopt;
    }

    ChatCipher chat;
    chat.id = sqlite3_column_int64(stmt_chats.get(), 0);
    set_text(stmt_chats.get(), 1, chat.username);
    set_text(stmt_chats.get(), 2, chat.first_name);
    set_text(stmt_chats.get(), 3, chat.last_name);

    //: Выполнение запроса к emails
    while (sqlite3_step(stmt_emails.get()) == SQLITE_ROW) {
        EmailCipher email;
        email.id = sqlite3_column_int64(stmt_emails.get(), 0);
        set_text(stmt_emails.get(), 1, email.address);
        email.last_uid = sqlite3_column_int64(stmt_emails.get(), 2);
        set_blob(stmt_emails.get(), 3, email.password.data);
        set_blob(stmt_emails.get(), 4, email.password.iv);
        set_blob(stmt_emails.get(), 5, email.password.tag);

        chat.emails[email.id] = std::move(email);
    }

    return chat;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Поиск чатов
 * @param chat_ids Список идентификаторов чатов
 * @return Данные найденных чатов
 * 
 * Если какие-либо чаты не найдены, вернётся список только найденных
 */
std::vector<ChatCipher> DatabaseStorage::find_chats(const std::vector<int64_t>& chat_ids) const noexcept {

    constexpr const char* sql_create_tmp   = "CREATE TEMP TABLE ids (id INT);";
    constexpr const char* sql_insert_tmp   = "INSERT INTO ids VALUES (?);";
    constexpr const char* sql_select_chats = R"(SELECT chats.*
                                             FROM chats
                                             JOIN ids ON chats.id = ids.id;)";
    constexpr const char* sql_emails       = R"(SELECT id, address, last_uid, password, password_iv, password_tag
                                                FROM emails
                                                WHERE chat_id = ?;)";

    auto deleter = [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); };

    { //: Вставка во временную таблицу

        int rc = sqlite3_exec(m_db.get(), "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            log_error("failed BEGIN TRANSACTION. error: {}", sqlite3_errmsg(m_db.get()));
            return {};
        }

        rc = sqlite3_exec(m_db.get(), sql_create_tmp, nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            log_error("failed create temp table. error: {}", sqlite3_errmsg(m_db.get()));
            return {};
        }

        std::unique_ptr<sqlite3_stmt, decltype(deleter)> stmt;

        {
            sqlite3_stmt* stmt_raw;
            rc = sqlite3_prepare_v2(m_db.get(), sql_insert_tmp, -1, &stmt_raw, nullptr);
            stmt.reset(stmt_raw);
            if (rc != SQLITE_OK) {
                log_error("failed prepare insert in temp table. error: {}", sqlite3_errmsg(m_db.get()));
                return {};
            }
        }

        for (int64_t chat_id : chat_ids) {
            sqlite3_bind_int64(stmt.get(), 1, chat_id);
            sqlite3_step(stmt.get());
            sqlite3_reset(stmt.get());
        }

        rc = sqlite3_exec(m_db.get(), "COMMIT;", nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            log_error("failed COMMIT. error: {}", sqlite3_errmsg(m_db.get()));
            return {};
        }
    }

    std::unique_ptr<sqlite3_stmt, decltype(deleter)> stmt_chats;

    {
        sqlite3_stmt* stmt_raw;

        int rc = sqlite3_prepare_v2(m_db.get(), sql_select_chats, -1, &stmt_raw, nullptr);
        stmt_chats.reset(stmt_raw);
        if (rc != SQLITE_OK) {
            log_error("failed prepare select chats. error: {}", sqlite3_errmsg(m_db.get()));
            return {};
        }
    }

    std::vector<ChatCipher> chats;

    while (sqlite3_step(stmt_chats.get()) == SQLITE_ROW) {

        ChatCipher chat;
        chat.id = sqlite3_column_int64(stmt_chats.get(), 0);
        set_text(stmt_chats.get(), 1, chat.username);
        set_text(stmt_chats.get(), 2, chat.first_name);
        set_text(stmt_chats.get(), 3, chat.last_name);

        std::unique_ptr<sqlite3_stmt, decltype(deleter)> stmt_emails;

        {
            sqlite3_stmt* stmt_raw;

            int rc = sqlite3_prepare_v2(m_db.get(), sql_emails, -1, &stmt_raw, nullptr);
            stmt_emails.reset(stmt_raw);
            if (rc != SQLITE_OK) {
                log_error("failed prepare select emails. error: {}", sqlite3_errmsg(m_db.get()));
                return {};
            }
        }

        sqlite3_bind_int64(stmt_emails.get(), 1, chat.id);

        //: Выполнение запроса к emails
        while (sqlite3_step(stmt_emails.get()) == SQLITE_ROW) {
            EmailCipher email_cipher;
            email_cipher.id = sqlite3_column_int64(stmt_emails.get(), 0);
            set_text(stmt_emails.get(), 1, email_cipher.address);
            email_cipher.last_uid = sqlite3_column_int64(stmt_emails.get(), 2);
            set_blob(stmt_emails.get(), 3, email_cipher.password.data);
            set_blob(stmt_emails.get(), 4, email_cipher.password.iv);
            set_blob(stmt_emails.get(), 5, email_cipher.password.tag);

            chat.emails[email_cipher.id] = std::move(email_cipher);
        }

        chats.push_back(std::move(chat));
    }

    return chats;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Получение идетнификторов всех чатов
 * @return Список идентификаторов чатов
 */
std::vector<int64_t> DatabaseStorage::chat_ids() const noexcept {

    constexpr const char* sql = R"(SELECT id FROM chats;)";

    auto deleter = [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); };

    std::unique_ptr<sqlite3_stmt, decltype(deleter)> stmt;

    sqlite3_stmt* stmt_raw;
    int           err = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt_raw, nullptr);
    stmt.reset(stmt_raw);

    if (err != SQLITE_OK) {
        log_error("sql error: {}", sqlite3_errmsg(m_db.get()));
        return {};
    }

    std::vector<int64_t> ids;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        int64_t chat_id = sqlite3_column_int64(stmt.get(), 0);
        ids.push_back(chat_id);
    }

    return ids;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Добавление новой почты
 * @param chat_id    Идентификатор чата
 * @param email      Данные электронной почты
 */
void DatabaseStorage::append_email(int64_t chat_id, const EmailCipher& email) {

    constexpr const char* sql = "INSERT INTO emails VALUES(?, ?, ?, ?, ?, ?, ?)";

    auto deleter = [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); };

    std::unique_ptr<sqlite3_stmt, decltype(deleter)> stmt;

    sqlite3_stmt* stmt_raw;
    int           err = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt_raw, nullptr);
    stmt.reset(stmt_raw);

    if (err != SQLITE_OK) {
        throw std::runtime_error(std::format("sql error: {}", sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_null(stmt.get(), 1);                                     //: id
    sqlite3_bind_int64(stmt.get(), 2, chat_id);                           //: chat_id
    sqlite3_bind_text(stmt.get(), 3, email.address.c_str(), -1, nullptr); //: address
    sqlite3_bind_int64(stmt.get(), 4, email.last_uid);                    //: last_uid

    //: password
    if (email.password.data.size() > 0) {
        sqlite3_bind_blob(stmt.get(), 5, email.password.data.data(), email.password.data.size(), nullptr);
    } else {
        sqlite3_bind_null(stmt.get(), 5);
    }

    //: password_iv
    if (email.password.data.size() > 0) {
        sqlite3_bind_blob(stmt.get(), 6, email.password.iv.data(), email.password.iv.size(), nullptr);
    } else {
        sqlite3_bind_null(stmt.get(), 6);
    }

    //: password_tag
    if (email.password.tag.size() > 0) {
        sqlite3_bind_blob(stmt.get(), 7, email.password.tag.data(), email.password.tag.size(), nullptr);
    } else {
        sqlite3_bind_null(stmt.get(), 7);
    }

    err = sqlite3_step(stmt.get());
    if (err != SQLITE_DONE) {
        throw std::runtime_error(
            std::format("failed append email. chat_id={}, email.address={}, error: {}", chat_id, email.address, sqlite3_errmsg(m_db.get())));
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Обновление данных об электронной почте
 * @param chat_id    Идентификатор чата
 * @param email      Данные электронной почты
 */
void DatabaseStorage::update_email(int64_t chat_id, const EmailCipher& email) {

    constexpr const char* sql = "UPDATE emails SET address=?, last_uid=?, password=?, password_iv=?, password_tag=? WHERE id = ?";

    auto deleter = [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); };

    std::unique_ptr<sqlite3_stmt, decltype(deleter)> stmt;

    sqlite3_stmt* stmt_raw;
    int           err = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt_raw, nullptr);
    stmt.reset(stmt_raw);

    if (err != SQLITE_OK) {
        throw std::runtime_error(std::format("sql error: {}", sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_text(stmt.get(), 1, email.address.c_str(), -1, nullptr); //: address
    sqlite3_bind_int64(stmt.get(), 2, email.last_uid);                    //: last_uid

    //: password
    if (email.password.data.size() > 0) {
        sqlite3_bind_blob(stmt.get(), 3, email.password.data.data(), email.password.data.size(), nullptr);
    } else {
        sqlite3_bind_null(stmt.get(), 3);
    }

    //: password_iv
    if (email.password.data.size() > 0) {
        sqlite3_bind_blob(stmt.get(), 4, email.password.iv.data(), email.password.iv.size(), nullptr);
    } else {
        sqlite3_bind_null(stmt.get(), 4);
    }

    //: password_tag
    if (email.password.tag.size() > 0) {
        sqlite3_bind_blob(stmt.get(), 5, email.password.tag.data(), email.password.tag.size(), nullptr);
    } else {
        sqlite3_bind_null(stmt.get(), 5);
    }

    sqlite3_bind_int64(stmt.get(), 6, email.id); //: emails.id

    err = sqlite3_step(stmt.get());
    if (err != SQLITE_DONE) {
        throw std::runtime_error(std::format("failed update email. chat_id={}, email.id={}, email.address={}, error: {}",
                                             chat_id,
                                             email.id,
                                             email.address,
                                             sqlite3_errmsg(m_db.get())));
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Удаление данных об электронной почте
 * @param chat_id  Идентификатор чата
 * @param email_id Идентификатор электронной почты
 */
void DatabaseStorage::delete_email(int64_t chat_id, int64_t email_id) {

    constexpr const char* sql = "DELETE FROM emails WHERE id = ?";

    auto deleter = [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); };

    std::unique_ptr<sqlite3_stmt, decltype(deleter)> stmt;

    sqlite3_stmt* stmt_raw;
    int           err = sqlite3_prepare_v2(m_db.get(), sql, -1, &stmt_raw, nullptr);
    stmt.reset(stmt_raw);
    if (err != SQLITE_OK) {
        throw std::runtime_error(std::format("sql error: {}", sqlite3_errmsg(m_db.get())));
    }

    sqlite3_bind_int64(stmt.get(), 1, email_id); //: email.id

    err = sqlite3_step(stmt.get());
    if (err != SQLITE_DONE) {
        throw std::runtime_error(std::format("failed delete email. id={}, chat_id={}, error: {}", email_id, chat_id, sqlite3_errmsg(m_db.get())));
    }
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

/**
 * @brief Получение из запроса текстового значения
 * @param[in]  stmt Запрос
 * @param[in]  col  Колонка
 * @param[out] dest Куда записать
 */
void DatabaseStorage::set_text(sqlite3_stmt* stmt, int col, std::string& dest) const noexcept {
    const unsigned char* text_raw = sqlite3_column_text(stmt, col);
    if (text_raw) {
        dest = reinterpret_cast<const char*>(text_raw);
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Получение из запроса данных типа BLOB
 * @param[in]  stmt Запрос
 * @param[in]  col  Колонка
 * @param[out] dest Куда записать
 */
void DatabaseStorage::set_blob(sqlite3_stmt* stmt, int col, std::vector<uint8_t>& dest) const noexcept {
    const void* raw_data = sqlite3_column_blob(stmt, col);
    // Получаем размер данных в байтах
    int bytes = sqlite3_column_bytes(stmt, col);
    if (raw_data && bytes > 0) {
        // Копируем данные в вектор
        const uint8_t* byte_ptr = static_cast<const uint8_t*>(raw_data);
        dest.assign(byte_ptr, byte_ptr + bytes);
    }
}
//----------------------------------------------------------------------------------------------------------------------
