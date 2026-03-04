//----------------------------------------------------------
#ifndef ISTORAGE_H
#define ISTORAGE_H
//----------------------------------------------------------
#include "chat/Chat.h"
//----------------------------------------------------------

/**
 * @brief Абстрактный базовый класс для реализации хранилища
 */
class IStorage {

public:

    IStorage()          = default;
    virtual ~IStorage() = default;

public:

    /**
     * @brief Создание нового чата
     * @param chat Данные чата
     */
    virtual void create_chat(const ChatCipher& chat) = 0;

    /**
     * @brief Поиск чата
     * @param chat_id Идентификатор чата
     * @return Данные чата, если он найден
     */
    [[nodiscard]] virtual std::optional<ChatCipher> find_chat(int64_t chat_id) const noexcept = 0;

    /**
     * @brief Поиск чатов
     * @param chat_ids Список идентификаторов чатов
     * @return Данные найденных чатов
     * 
     * Если какие-либо чаты не найдены, вернётся список только найденных
     */
    [[nodiscard]] virtual std::vector<ChatCipher> find_chats(const std::vector<int64_t>& chat_ids) const noexcept = 0;

    /**
     * @brief Получение идетнификторов всех чатов
     * @return Список идентификаторов чатов
     */
    [[nodiscard]] virtual std::vector<int64_t> chat_ids() const noexcept = 0;

    /**
     * @brief Добавление новой почты
     * @param chat_id    Идентификатор чата
     * @param email      Данные электронной почты
     */
    virtual void append_email(int64_t chat_id, const EmailCipher& email) = 0;

    /**
     * @brief Обновление данных об электронной почте
     * @param chat_id    Идентификатор чата
     * @param email      Данные электронной почты
     */
    virtual void update_email(int64_t chat_id, const EmailCipher& email) = 0;

    /**
     * @brief Удаление данных об электронной почте
     * @param chat_id  Идентификатор чата
     * @param email_id Идентификатор электронной почты
     */
    virtual void delete_email(int64_t chat_id, int64_t email_id) = 0;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // ISTORAGE_H
