//----------------------------------------------------------
#ifndef ISTORAGE_H
#define ISTORAGE_H
//----------------------------------------------------------
#include "chat/Chat.h"
//----------------------------------------------------------

/**
 * @brief Абстрактный базовый класс для реализации кэша чатов
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
    virtual void create(const Chat& chat) = 0;

    /**
     * @brief Обновление данных о чате
     * @param chat Данные чата
     */
    virtual void update(const Chat& chat) = 0;

    /**
     * @brief Поиск чата
     * @param chat_ids Список идентификаторов чатов
     * @return Данные чата, если он найден
     */
    [[nodiscard]] virtual std::optional<std::vector<Chat>> find(const std::vector<int64_t>& chat_ids) const noexcept = 0;

    /**
     * @brief Получение идетнификторов всех чатов
     * @return Список идентификаторов чатов
     */
    [[nodiscard]] virtual std::vector<int64_t> chat_ids() const noexcept = 0;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // ISTORAGE_H
