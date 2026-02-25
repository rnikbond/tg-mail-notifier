//----------------------------------------------------------
#ifndef IREPOSITORY_H
#define IREPOSITORY_H
//----------------------------------------------------------
#include <memory>
#include <unordered_map>
//----------------------------------------------------------
#include "chat/Chat.h"
//----------------------------------------------------------
using ChatOpt  = std::optional<std::shared_ptr<const Chat>>;
using ChatsOpt = std::optional<std::vector<std::shared_ptr<const Chat>>>;
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

    /**
     * @brief Создание нового чата
     * @param chat Данные нового чата
     * @return Указатель на созданный чат или nullopt, если пошло что-то не так
     */
    [[nodiscard]] virtual ChatOpt create(const Chat& chat) noexcept = 0;

    /**
     * @brief Обновление информации о чате
     * @param chat Обновленные данные чата
     * @return TRUE, если данные обновлены. Иначе FALSE.
     */
    [[nodiscard]] virtual bool update(const Chat& chat) noexcept = 0;

    /**
     * @brief Обновление информации о Email
     * @param chat_id Иденитификатор чата
     * @param email   Обновленные данные email
     * @return TRUE, если данные обновлены. Иначе FALSE.
     */
    [[nodiscard]] virtual bool update_email(int64_t chat_id, const Email& email) noexcept = 0;

    /**
     * @brief Поиск чата
     * @param chat_ids Идентификаторы чатов
     * @return Указатель на чат или nullopt, если он не найден
     */
    [[nodiscard]] virtual ChatsOpt find(const std::vector<int64_t>& chat_ids) noexcept = 0;

    /**
     * @brief Получение карты чатов и email
     * @return карта: <chat_id> = Email
     */
    [[nodiscard]] virtual std::unordered_map<int64_t, Email> chats() noexcept = 0;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // IREPOSITORY_H
