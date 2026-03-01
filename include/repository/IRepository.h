//----------------------------------------------------------
#ifndef IREPOSITORY_H
#define IREPOSITORY_H
//----------------------------------------------------------
#include <expected>
#include <memory>
//----------------------------------------------------------
#include "chat/Chat.h"
#include "errs/Errors.h"
//----------------------------------------------------------

/**
 * @brief Абстрактный базовый класс для реализации репозитория
 * 
 * Предоставляет интерфейс для создания/обновления и получения информации о чатах.
 */
class IRepository {

public:

    IRepository()          = default;
    virtual ~IRepository() = default;

public:

    using ChatResult  = std::expected<std::shared_ptr<const Chat>, Errors::Repository>;
    using ChatsResult = std::optional<std::vector<std::shared_ptr<const Chat>>>;
    using Chats       = std::vector<std::shared_ptr<const Chat>>;

public:

    /**
     * @brief Создание нового чата
     * @param chat Данные нового чата
     * @return Указатель на созданный чат или ошибку, если не удалось создать
     */
    virtual ChatResult create_chat(const Chat& chat) noexcept = 0;

    /**
     * @brief Поиск чата
     * @param chat_id Идентификатор чата
     * @return Данные чата, если он найден или ошибку
     */
    [[nodiscard]] virtual ChatResult find_chat(int64_t chat_id) noexcept = 0;

    /**
     * @brief Поиск чатов
     * @param chat_ids Список идентификаторов чатов
     * @return Найденные чатов или ошибку
     * 
     * Если какие-либо чаты не найдены, вернётся список только найденных
     */
    [[nodiscard]] virtual ChatsResult find_chats(const std::vector<int64_t>& chat_ids) noexcept = 0;

    /**
     * @brief Получение всех чатов
     * @return Список всех чатов
     */
    [[nodiscard]] virtual Chats chats() noexcept = 0;

    /**
     * @brief Добавление новой почты
     * @param chat_id Идентификатор чата
     * @param email   Данные электронной почты
     * @return Чат с обновленными данными или ошибку, если не удалось добавить
     */
    virtual ChatResult append_email(int64_t chat_id, const Email& email) noexcept = 0;

    /**
     * @brief Обновление данных о электронной почте
     * @param chat_id Идентификатор чата
     * @param email   Данные электронной почты
     * @return Чат с обновленными данными или ошибку, если не удалось добавить
     */
    virtual ChatResult update_email(int64_t chat_id, const Email& email) noexcept = 0;

    /**
     * @brief Обновление последнего UID письма электронной почты
     * @param chat_id  Идентификатор чата
     * @param email_id Идентификатор почты
     * @param uid      Новый идентификатор письма
     * @return Чат с обновленными данными или ошибку, если не удалось добавить
     */
    virtual ChatResult update_email_uid(int64_t chat_id, int64_t email_id, int64_t uid) noexcept = 0;

    /**
     * @brief Удаление информации об электронной почте
     * @param chat_id  Иденитификатор чата
     * @param email_id Идентификатор электронной почты
     * @return TRUE, если данные удалены. Иначе FALSE.
     */
    virtual bool delete_email(int64_t chat_id, int64_t email_id) noexcept = 0;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // IREPOSITORY_H
