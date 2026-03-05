//----------------------------------------------------------
#include <ranges>
//----------------------------------------------------------
#include "MemoryStorage.h"
//----------------------------------------------------------

/**
 * @brief Создание нового чата
 * @param chat Данные чата
 * 
 * @throw std::logic_error Выбарсывается, если chat_id уже существует
 */
void MemoryStorage::create_chat(const ChatCipher& chat) {

    if (m_data.contains(chat.id)) {
        throw std::logic_error(std::format("chat already exists. chat_id: {}", chat.id));
    }

    m_data[chat.id] = chat;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Поиск чата по идентификатору
 * @param chat_id Идентификатор чата
 * @return Данные чата, если он анйден. Иначе std::nullopt.
 */
std::optional<ChatCipher> MemoryStorage::find_chat(int64_t chat_id) const noexcept {

    if (auto it = m_data.find(chat_id); it != m_data.end()) {
        return it->second;
    }

    return std::nullopt;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Поиск чатов по идентификаторам
 * @param chat_ids Идентификаторы чатов
 * @return Список найденных чатов
 * 
 * Возвращаются только найденные чаты.
 * Если какой-либо из чатов не найден, он будет проигнорирован.
 */
std::vector<ChatCipher> MemoryStorage::find_chats(const std::vector<int64_t>& chat_ids) const noexcept {

    std::vector<ChatCipher> chats;
    for (int64_t chat_id : chat_ids) {
        if (auto it = m_data.find(chat_id); it != m_data.end()) {
            chats.push_back(it->second);
        }
    }

    return chats;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Получение всех существующих идентификаторов чатов
 * @return Список существующих идентификаторов чатов
 */
std::vector<int64_t> MemoryStorage::chat_ids() const noexcept {

    std::vector<int64_t> ids;
    for (int64_t chat_id : m_data | std::views::keys) {
        ids.push_back(chat_id);
    }

    return ids;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Добавление электронной почты к чату
 * @param chat_id    Идентификатор чата
 * @param email      Данные электронной почты
 * 
 * @throw std::logic_error  Выбрасывается, если email.id != -1 или если такое email.address уже добавлен к этому чату
 * @throw std::out_of_range Выбрасывается, если chat_it не найден
 */
void MemoryStorage::append_email(int64_t chat_id, const EmailCipher& email) {

    if (email.id >= 0) {
        throw std::logic_error(std::format("failed create email: Email::id must been = -1"));
    }

    auto it = m_data.find(chat_id);
    if (it == m_data.end()) {
        throw std::out_of_range(std::format("not found chat. chat_id: {}", chat_id));
    }

    auto& emails = it->second.emails;

    int64_t id_autoincrement = emails.size();

    emails[id_autoincrement]    = email;
    emails[id_autoincrement].id = id_autoincrement;
}
//----------------------------------------------------------------------------------------------------------------------

/*!
 * @brief Обновление информации об электронной почте
 * @param chat_id    Идентификатор чата
 * @param email      Данные электронной почты
 * 
 * @throw std::out_of_range Выбрасывается, если не найден chat_id или email.id.
 */
void MemoryStorage::update_email(int64_t chat_id, const EmailCipher& email) {

    auto it = m_data.find(chat_id);
    if (it == m_data.end()) {
        throw std::out_of_range(std::format("chat not found. chat_id: {}", chat_id));
    }

    auto& emails = it->second.emails;

    auto it_email = emails.find(email.id);
    if (it_email == emails.end()) {
        throw std::out_of_range(std::format("email not found. email.id: {}", email.id));
    }

    it_email->second = email;
}
//----------------------------------------------------------------------------------------------------------------------

/**
* @brief Удаление данных об электронной почте
* @param chat_id  Идентификатор чата
* @param email_id Идентификатор электронной почты
* 
* @throw std::out_of_range Выбрасывается, если не найден chat_id или email.id.
*/
void MemoryStorage::delete_email(int64_t chat_id, int64_t id) {

    auto it = m_data.find(chat_id);
    if (it == m_data.end()) {
        throw std::out_of_range(std::format("chat not found. chat_id: {}", chat_id));
    }

    auto& emails = it->second.emails;
    if (!emails.contains(id)) {
        throw std::runtime_error(std::format("email not found. chat_id: {}, id: {}", chat_id, id));
    }

    emails.erase(id);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Добавление информации о почтовом сервере
 * @param server Данные почтового сервера
 */
void MemoryStorage::append_mail_server(const MailServer& server) {
    m_mail_servers[server.domain] = server.url;
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Получение информации о почтовых серверах
 * @return Список известных почтовых серверов
 */
std::vector<MailServer> MemoryStorage::mail_servers() {

    std::vector<MailServer> servers;
    servers.reserve(m_mail_servers.size());

    for (const auto& [domain, url_imap] : m_mail_servers) {
        MailServer server;
        server.domain = domain;
        server.url    = url_imap;
        servers.push_back(std::move(server));
    }

    return servers;
}
//----------------------------------------------------------------------------------------------------------------------
