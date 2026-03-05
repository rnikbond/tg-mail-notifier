//----------------------------------------------------------
#include <ranges>
#include <regex>
//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "../core/PasswordCryptor.h"
#include "../mail/ImapDiscoveryTool.h"
//----------------------------------------------------------
#include "Cache.h"
//----------------------------------------------------------

/**
 * @brief Конструктор кэша
 * @param storage Указатель на объект, реализующий интерфейс хранилища
 */
Cache::Cache(std::unique_ptr<IStorage> storage)
    : m_storage(std::move(storage)) {
    reload_from_storage();
}
//----------------------------------------------------------------------------------------------------------------------

/**
* @brief Создание нового чата
* @param chat Данные нового чата
* @return Указатель на созданный чат или ошибку, если не удалось создать
*/
IRepository::ChatResult Cache::create_chat(const Chat& chat) noexcept {

    if (chat.id == 0 || chat.username.empty()) {
        return std::unexpected(Errors::Repository::InvalidChat);
    }

    { //: Проверка наличия такого чата
        std::shared_lock lock(m_mutex);
        if (m_cache_data.contains(chat.id)) {
            return std::unexpected(Errors::Repository::AlreadyExists);
        }
    }

    std::unique_lock lock(m_mutex);

    //: Создание в хранилище
    try {
        auto chat_cipher = convert_chat_to_cipher(chat);
        m_storage->create_chat(chat_cipher);
    } catch (const std::logic_error& ex) {
        return std::unexpected(Errors::Repository::AlreadyExists);
    } catch (const std::exception& ex) {
        log_error("failed create chat in storage: {}. chat_id = {}", ex.what(), chat.id);
        return std::unexpected(Errors::Repository::Internal);
    }

    //: Добавление в кэш
    try {
        return refresh(chat.id);
    } catch (const std::exception& ex) {
        log_error("failed refresh chat in cache. chat_id = {}, error: {}", chat.id, ex.what());
        return std::unexpected(Errors::Repository::Internal);
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
* @brief Поиск чата
* @param chat_id Идентификатор чата
* @return Данные чата, если он найден или ошибку
*/
IRepository::ChatResult Cache::find_chat(int64_t chat_id) noexcept {

    { //: Поиск чата в кэше
        std::shared_lock lock(m_mutex);

        if (auto it = m_cache_data.find(chat_id); it != m_cache_data.end()) {
            return it->second;
        }
    }

    //: Раз оказались здесь - значит в кэше чата нет.
    //: Пробуем загрузить из хранилища
    std::unique_lock lock(m_mutex);

    auto chat_opt = m_storage->find_chat(chat_id);
    if (!chat_opt) {
        return std::unexpected(Errors::Repository::NotFound);
    }

    auto chat     = convert_chat_from_cipher(chat_opt.value());
    auto chat_ptr = std::make_shared<Chat>(std::move(chat));

    m_cache_data[chat_id] = chat_ptr;
    return chat_ptr;
}
//----------------------------------------------------------------------------------------------------------------------

/**
* @brief Поиск чатов
* @param chat_ids Список идентификаторов чатов
* @return Найденные чаты
* 
* Если какие-либо чаты не найдены, будет обращение к хранилиущ для их поиска.
* Вернётся только список найденных чатов.
*/
IRepository::ChatsResult Cache::find_chats(const std::vector<int64_t>& chat_ids) noexcept {

    std::vector<std::shared_ptr<const Chat>> chats;
    chats.reserve(chat_ids.size());

    std::vector<int64_t> missing_ids;

    { //: Добавление чатов из кэша
        std::shared_lock lock(m_mutex);

        for (int64_t chat_id : chat_ids) {
            if (auto it = m_cache_data.find(chat_id); it != m_cache_data.end()) {
                chats.push_back(it->second);
                continue;
            }

            missing_ids.push_back(chat_id);
        }
    }

    //: Загрузка их хранилища чатов, которых нет в кэше
    if (missing_ids.size() != 0) {
        std::unique_lock lock(m_mutex);

        auto new_chats = append(missing_ids);
        for (auto& chat_ptr : new_chats) {
            chats.push_back(std::move(chat_ptr));
        }
    }

    return chats;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Получение всех чатов, которые есть в хранилище
 * @return Список всех чатов
 */
IRepository::Chats Cache::chats() noexcept {

    std::shared_lock read_lock(m_mutex);

    std::vector<std::shared_ptr<const Chat>> chats;
    chats.reserve(m_cache_data.size());

    for (auto& [_, chat_ptr] : m_cache_data) {
        chats.push_back(chat_ptr);
    }
    return chats;
}
//----------------------------------------------------------------------------------------------------------------------

/**
* @brief Добавление новой почты
* @param chat_id Идентификатор чата
* @param email   Данные электронной почты
* @return Чат с обновленными данными или ошибку, если не удалось добавить
*/
IRepository::ChatResult Cache::append_email(int64_t chat_id, const Email& email) noexcept {

    if (!ImapDiscoveryTool::is_correct_email(email.address)) {
        return std::unexpected(Errors::Repository::InvalidEmail);
    }

    std::unique_lock lock(m_mutex);

    auto it = m_cache_data.find(chat_id);
    if (it == m_cache_data.end()) {
        it = append(chat_id);
    }
    if (it == m_cache_data.end()) {
        return std::unexpected(Errors::Repository::NotFound);
    }

    auto domain_opt = update_domain(email.address);
    if (domain_opt) {
        //: Что-то не то с доменом
        return std::unexpected(domain_opt.value());
    }

    auto& emails = it->second->emails;

    //: Проверка дублирования адресов
    for (const auto& [id, email_] : emails) {
        if (email_.address == email.address && email_.id != email.id) {
            return std::unexpected(Errors::Repository::AlreadyExists);
        }
    }

    try {
        auto email_cipher = convert_email_to_cipher(email);
        m_storage->append_email(chat_id, email_cipher);
    } catch (const std::out_of_range& ex) {
        return std::unexpected(Errors::Repository::AlreadyExists);
    } catch (const std::exception& ex) {
        log_error("failed append in storage. chat_id: {}, email.id: {}. Error: {}", chat_id, email.id, ex.what());
        return std::unexpected(Errors::Repository::Internal);
    }

    try {
        return refresh(chat_id);
    } catch (const std::exception& ex) {
        log_error("failed refresh chat in cache. chat_id: {}. Error: {}", chat_id, ex.what());
        return std::unexpected(Errors::Repository::Internal);
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
* @brief Обновление данных о электронной почте
* @param chat_id Идентификатор чата
* @param chat_id Данные чата
* @return Чат с обновленными данными или ошибку, если не удалось добавить
*/
IRepository::ChatResult Cache::update_email(int64_t chat_id, const Email& email) noexcept {

    if (!ImapDiscoveryTool::is_correct_email(email.address)) {
        return std::unexpected(Errors::Repository::InvalidEmail);
    }

    std::unique_lock lock(m_mutex);

    auto it = m_cache_data.find(chat_id);
    if (it == m_cache_data.end()) {
        it = append(chat_id);
    }
    if (it == m_cache_data.end()) {
        return std::unexpected(Errors::Repository::NotFound);
    }

    auto domain_opt = update_domain(email.address);
    if (domain_opt) {
        //: Что-то не то с доменом
        return std::unexpected(domain_opt.value());
    }

    auto& emails = it->second->emails;

    //: Проверка дублирования адресов
    for (const auto& [id, email_] : emails) {
        if (email_.address == email.address && email_.id != email.id) {
            return std::unexpected(Errors::Repository::AlreadyExists);
        }
    }

    try {
        auto email_cipher = convert_email_to_cipher(email);
        m_storage->update_email(chat_id, email_cipher);
    } catch (const std::out_of_range& ex) {
        return std::unexpected(Errors::Repository::NotFound);
    } catch (const std::exception& ex) {
        log_error("failed update email in storage. chat_id: {}, email.id: {}. Error: {}", chat_id, email.id, ex.what());
        return std::unexpected(Errors::Repository::Internal);
    }

    try {
        return refresh(chat_id);
    } catch (const std::exception& ex) {
        log_error("failed refresh chat in cache. chat_id: {}. Error: {}", chat_id, ex.what());
        return std::unexpected(Errors::Repository::Internal);
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
* @brief Обновление последнего UID письма электронной почты
* @param chat_id  Идентификатор чата
* @param email_id Идентификатор почты
* @param uid      Новый идентификатор письма
* @return Чат с обновленными данными или ошибку, если не удалось добавить
*/
IRepository::ChatResult Cache::update_email_uid(int64_t chat_id, int64_t email_id, int64_t uid) noexcept {

    std::unique_lock lock(m_mutex);

    auto it = m_cache_data.find(chat_id);
    if (it == m_cache_data.end()) {
        it = append(chat_id);
    }
    if (it == m_cache_data.end()) {
        return std::unexpected(Errors::Repository::NotFound);
    }

    auto& emails   = it->second->emails;
    auto  it_email = emails.find(email_id);
    if (it_email == emails.end()) {
        return std::unexpected(Errors::Repository::NotFound);
    }

    auto& email = it_email->second;
    if (uid <= email.last_uid) {
        return std::unexpected(Errors::Repository::InvalidUID);
    }

    if (email.last_uid == uid) {
        log_warn("request update mail uid on equal. chat_id={}, email={}, UID={}", chat_id, email.address, uid);
        return it->second;
    }

    email.last_uid = uid;

    try {
        auto email_cipher = convert_email_to_cipher(email);
        m_storage->update_email(chat_id, email_cipher);
    } catch (const std::out_of_range& ex) {
        return std::unexpected(Errors::Repository::NotFound);
    } catch (const std::exception& ex) {
        log_error("failed update email in storage. chat_id: {}, email.id: {}. Error: {}", chat_id, email.id, ex.what());
        return std::unexpected(Errors::Repository::Internal);
    }

    try {
        return refresh(chat_id);
    } catch (const std::exception& ex) {
        log_error("failed refresh chat in cache. chat_id: {}. Error: {}", chat_id, ex.what());
        return std::unexpected(Errors::Repository::Internal);
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Удаление информации об электронной почте
 * @param chat_id  Иденитификатор чата
 * @param email_id Идентификатор электронной почты
 * @return TRUE, если данные удалены. Иначе FALSE.
 */
bool Cache::delete_email(int64_t chat_id, int64_t email_id) noexcept {

    std::unique_lock lock(m_mutex);

    try {
        m_storage->delete_email(chat_id, email_id);
    } catch (const std::out_of_range& ex) {
        return false;
    } catch (const std::exception& ex) {
        log_error("failed delete email from storage. chat_id: {}, email_id: {}. Error: {}", chat_id, email_id, ex.what());
        return false;
    }

    auto it = m_cache_data.find(chat_id);
    if (it != m_cache_data.end()) {
        m_cache_data.erase(it);
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Обновление информации о доменах
 * @param email_addr Адрес почты
 * @return std::nullopt, если такой домен существует или успешно добален. Иначе ошибку.
 */
std::optional<Errors::Repository> Cache::update_domain(const std::string& email_addr) {

    auto domain_res = ImapDiscoveryTool::domain_from_email(email_addr);
    if (!domain_res.has_value()) {
        switch (domain_res.error()) {
            case Errors::ImapDiscover::EmailSyntax:
                return Errors::Repository::InvalidEmail;
            default:
                log_error("failed get domain from email. email={}, error: {}", email_addr, Errors::to_string(domain_res.error()));
                return Errors::Repository::Internal;
        }
    }

    if (m_mail_servers.find(domain_res.value()) != m_mail_servers.end()) {
        return std::nullopt; //: Такой домен известен
    }

    auto mail_server_res = ImapDiscoveryTool::detect_mail_server(email_addr);
    if (!mail_server_res.has_value()) {
        switch (mail_server_res.error()) {
            case Errors::ImapDiscover::DomainNotFound:
                return Errors::Repository::InvalidEmail;
            case Errors::ImapDiscover::EmailSyntax:
                return Errors::Repository::InvalidEmail;
            default:
                log_error("failed get domain info. email={}, error: {}", email_addr, Errors::to_string(mail_server_res.error()));
                return Errors::Repository::Internal;
        }
    }

    auto mail_server = std::move(mail_server_res.value());

    try {
        m_storage->append_mail_server(mail_server);
    } catch (const std::exception& ex) {
        log_error("exception add mail server in storage. email={}, ex: {}", email_addr, ex.what());
        return Errors::Repository::Internal;
    }

    auto domain = std::move(mail_server);

    m_mail_servers[domain.domain] = domain.url;
    return std::nullopt;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Перезагрузка данных из хранилища
 */
void Cache::reload_from_storage() {

    m_cache_data.clear();

    auto ids   = m_storage->chat_ids();
    auto chats = m_storage->find_chats(ids);
    for (ChatCipher& chat_cipher : chats) {
        auto chat = convert_chat_from_cipher(chat_cipher);
        m_cache_data.emplace(chat.id, std::make_shared<Chat>(std::move(chat)));
    }

    auto mail_servers = m_storage->mail_servers();
    for (const auto& mail_server : mail_servers) {
        m_mail_servers[mail_server.domain] = mail_server.url;
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Добавление чата в кэш
 * @param chat_id Идентификатор чата
 * @return Итератор, указывающий на добавленный чат или end(), если не удалось заргузить чат из хранилища.
 */
std::unordered_map<int64_t, std::shared_ptr<Chat>>::iterator Cache::append(int64_t chat_id) {

    auto chat_opt = m_storage->find_chat(chat_id);
    if (!chat_opt) {
        return m_cache_data.end();
    }

    auto chat     = convert_chat_from_cipher(chat_opt.value());
    auto chat_ptr = std::make_shared<Chat>(std::move(chat));
    return m_cache_data.emplace_hint(m_cache_data.end(), chat_id, std::move(chat_ptr));
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Добавление чатов в кэш
 * @param chat_ids Список идетнификаторов чатов
 * @return Список добавленных чатов
 */
std::vector<std::shared_ptr<Chat>> Cache::append(const std::vector<int64_t>& chat_ids) {

    std::vector<std::shared_ptr<Chat>> chats;
    chats.reserve(chat_ids.size());

    auto chats_storage = m_storage->find_chats(chat_ids);
    for (ChatCipher& chat_cipher : chats_storage) {

        auto chat     = convert_chat_from_cipher(chat_cipher);
        auto chat_ptr = std::make_shared<Chat>(std::move(chat));

        m_cache_data[chat_ptr->id] = chat_ptr;
        chats.push_back(chat_ptr);
    }

    return chats;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Перезагрузка чата в кэше
 * @param chat_id Идентификатор чата
 * @return Указатель на перезагруженный чат
 * 
 * @throw std::runtime_error Если в хранилище чат не найден
 */
std::shared_ptr<Chat> Cache::refresh(int64_t chat_id) {

    auto res_opt = m_storage->find_chat(chat_id);
    if (!res_opt) {
        throw std::runtime_error(std::format("not found chat in storage. chat_id = {}", chat_id));
    }

    auto chat     = convert_chat_from_cipher(res_opt.value());
    auto chat_ptr = std::make_shared<Chat>(std::move(chat));

    m_cache_data[chat_ptr->id] = chat_ptr;
    return chat_ptr;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Проверка корректности адреса электронной почты
 * @param addr Адрес электронной почты
 * @return TRUE, если адрес корректный. Иначе FALSE.
 */
bool Cache::is_correct_email_addr(const std::string_view& addr) const noexcept {
    const std::regex pattern(R"(^[\w\.-]+@[\w\.-]+\.\w{2,4}$)");
    if (!std::regex_match(static_cast<std::string>(addr), pattern)) {
        return false;
    }

    return true;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Преобразование обычного чата в зашифрованный
 * @param chat Обычный чат
 * @return Зашифрованный чат
 */
ChatCipher Cache::convert_chat_to_cipher(const Chat& chat) const {

    ChatCipher chat_cipher;
    chat_cipher.id         = chat.id;
    chat_cipher.username   = chat.username;
    chat_cipher.first_name = chat.first_name;
    chat_cipher.last_name  = chat.last_name;
    chat_cipher.extensions = chat.extensions;

    for (const auto& [email_id, email] : chat.emails) {
        chat_cipher.emails[email_id] = std::move(convert_email_to_cipher(email));
    }

    return chat_cipher;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Преобразование зашифрованного чата в обычный
 * @param chat_cipher Зашифрованный чат
 * @return Обычный чат
 */
Chat Cache::convert_chat_from_cipher(const ChatCipher& chat_cipher) const {

    Chat chat;
    chat.id         = chat_cipher.id;
    chat.username   = chat_cipher.username;
    chat.first_name = chat_cipher.first_name;
    chat.last_name  = chat_cipher.last_name;
    chat.extensions = chat_cipher.extensions;
    for (const auto& [email_id, email_cipher] : chat_cipher.emails) {
        chat.emails[email_id] = std::move(convert_email_from_cipher(email_cipher));
    }

    return chat;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Преобразование обычного Email в зашифрованный
 * @param email Обычный Email
 * @return Зашифрованный Email
 */
EmailCipher Cache::convert_email_to_cipher(const Email& email) const {

    EmailCipher email_cipher;
    email_cipher.id         = email.id;
    email_cipher.address    = email.address;
    email_cipher.last_uid   = email.last_uid;
    email_cipher.extensions = email.extensions;

    try {
        if (!email.password.empty()) {
            email_cipher.password = PasswordCryptor::encrypt(email.password);
        }
    } catch (const std::exception& ex) {
        log_error("failed encrypt email password. id={}, address={}, error: {}", email.id, email.password, ex.what());
    }

    return email_cipher;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief CПреобразование зашифрованного Email в обычный
 * @param email_cipher Зашифрованный Email
 * @return Обычный Email
 */
Email Cache::convert_email_from_cipher(const EmailCipher& email_cipher) const {

    Email email;
    email.id         = email_cipher.id;
    email.address    = email_cipher.address;
    email.last_uid   = email_cipher.last_uid;
    email.extensions = email_cipher.extensions;

    auto domain_res = ImapDiscoveryTool::domain_from_email(email.address);
    if (domain_res) {
        auto it = m_mail_servers.find(domain_res.value());
        if (it != m_mail_servers.end()) {
            email.url_imap = it->second;
        }
    }

    try {
        if (!email_cipher.password.data.empty()) {
            email.password = PasswordCryptor::decrypt(email_cipher.password);
        }
    } catch (const std::exception& ex) {
        log_error("failed decrypt email password. id={}, address={}, error: {}", email.id, email.password, ex.what());
    }

    return email;
}
//----------------------------------------------------------------------------------------------------------------------
