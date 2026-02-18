//----------------------------------------------------------
#ifndef IREPOSITORY_H
#define IREPOSITORY_H
//----------------------------------------------------------
#include <memory>
#include <unordered_map>
//----------------------------------------------------------
#include "chat/Chat.h"
//----------------------------------------------------------
using ChatPtr = std::optional<std::shared_ptr<const Chat>>;
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
    [[nodiscard]] virtual ChatPtr create(const Chat& chat) noexcept = 0;

    /**
     * @brief Обновление информации о чате
     * @param chat Обновленные данные чата
     * @return TRUE, если данные обновлены. Иначе FALSE.
     */
    [[nodiscard]] virtual bool update(const Chat& chat) noexcept = 0;

    /**
     * @brief Поиск чата
     * @param chat_id Идентификатор чата
     * @return Указатель на чат или nullopt, если он не найден
     */
    [[nodiscard]] virtual ChatPtr find(int64_t chat_id) noexcept = 0;

    /**
     * @brief Получение карты чатов и email
     * @return арта: <chat_id> = Email
     */
    [[nodiscard]] virtual std::unordered_map<int64_t, Email> chats() noexcept = 0;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // IREPOSITORY_H
