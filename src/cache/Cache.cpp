//----------------------------------------------------------
#include <ranges>
#include <regex>
//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "Cache.h"
//----------------------------------------------------------

/**
 * @brief Конструктор кэша
 * @param storage Указатель на объект, реализующий интерфейс хранилища
 */
Cache::Cache(std::unique_ptr<IStorage> storage)
    : m_storage(std::move(storage)) {
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
        m_storage->create_chat(chat);
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

    auto chat = std::make_shared<Chat>(std::move(chat_opt.value()));

    m_cache_data[chat_id] = chat;
    return chat;
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

    std::shared_lock lock(m_mutex);

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

    if (!is_correct_email_addr(email.address)) {
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

    auto& emails = it->second->emails;

    //: Проверка дублирования адресов
    for (const auto& [id, email_] : emails) {
        if (email_.address == email.address && email_.id != email.id) {
            return std::unexpected(Errors::Repository::AlreadyExists);
        }
    }

    try {
        m_storage->append_email(chat_id, email);
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

    if (!is_correct_email_addr(email.address)) {
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

    auto& emails = it->second->emails;

    //: Проверка дублирования адресов
    for (const auto& [id, email_] : emails) {
        if (email_.address == email.address && email_.id != email.id) {
            return std::unexpected(Errors::Repository::AlreadyExists);
        }
    }

    try {
        m_storage->update_email(chat_id, email);
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

    it_email->second.last_uid = uid;

    try {
        m_storage->update_email(chat_id, email);
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
 * @brief Добавление чата в кэш
 * @param chat_id Идентификатор чата
 * @return Итератор, указывающий на добавленный чат или end(), если не удалось заргузить чат из хранилища.
 */
std::unordered_map<int64_t, std::shared_ptr<Chat>>::iterator Cache::append(int64_t chat_id) {

    auto chat_opt = m_storage->find_chat(chat_id);
    if (!chat_opt) {
        return m_cache_data.end();
    }
    auto chat_ptr = std::make_shared<Chat>(std::move(chat_opt.value()));
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
    for (Chat& chat : chats_storage) {

        auto chat_ptr = std::make_shared<Chat>(std::move(chat));

        m_cache_data[chat.id] = chat_ptr;
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

    auto res_final = m_storage->find_chat(chat_id);
    if (!res_final) {
        throw std::runtime_error(std::format("not found chat in storage. chat_id = {}", chat_id));
    }

    auto chat = std::make_shared<Chat>(std::move(res_final.value()));

    m_cache_data[chat->id] = chat;
    return chat;
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
