//----------------------------------------------------------
#include <gtest/gtest.h>
//----------------------------------------------------------
#include "../src/cache/Cache.h"
#include "../src/storage/memory/MemoryStorage.h"
//----------------------------------------------------------

/**
 * @brief Тестирование успешных сценариев при вызове Cache::create
 * 
 * Проверяется, когда заполнены все поля и когда заполнены тольок обязательные
 * 
 * Ошибок не ожидается
 */
TEST(Test_Cache, CheckCreate_Positive) {

    std::vector<Chat> chats;

    { //: Все данные
        Chat chat;
        chat.chat_id                  = 123;
        chat.username                 = "user";
        chat.first_name               = "Qwerty";
        chat.last_name                = "123";
        chat.silent_interval          = TimeImterval{3600, 7200};
        chat.email.address            = "user@mail.ru";
        chat.email.password           = "123321";
        chat.email.addr_filter_rules  = {"1", "2", "3"};
        chat.email.title_filter_rules = {"4", "5"};
        chat.email.body_filter_rules  = {"a", "b", "c"};

        chats.push_back(chat);
    }

    { //: Только обязательные данные
        Chat chat;
        chat.chat_id  = 123;
        chat.username = "user";

        chats.push_back(chat);
    }

    for (const Chat& chat : chats) {

        auto  memory = std::make_unique<MemoryStorage>();
        Cache cache(std::move(memory));

        auto res = cache.create(chat);
        ASSERT_TRUE(res.has_value());

        auto out_chat = res.value();
        EXPECT_EQ(chat.chat_id, out_chat->chat_id);
        EXPECT_EQ(chat.username, out_chat->username);
        EXPECT_EQ(chat.first_name, out_chat->first_name);
        EXPECT_EQ(chat.last_name, out_chat->last_name);
        EXPECT_EQ(chat.silent_interval.start_secs, out_chat->silent_interval.start_secs);
        EXPECT_EQ(chat.silent_interval.end_secs, out_chat->silent_interval.end_secs);
        EXPECT_EQ(chat.email.address, out_chat->email.address);
        EXPECT_EQ(chat.email.password, out_chat->email.password);
        EXPECT_EQ(chat.email.addr_filter_rules, out_chat->email.addr_filter_rules);
        EXPECT_EQ(chat.email.title_filter_rules, out_chat->email.title_filter_rules);
        EXPECT_EQ(chat.email.body_filter_rules, out_chat->email.body_filter_rules);
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Тестирование неуспешных сценариев при вызове Cache::create в части отсуствия обязательных данных
 * 
 * Проверяются ситуации, когда не заполнены обязательные поля
 * 
 * Ожидается ошибка
 */
TEST(Test_Cache, CheckCreate_Negative) {

    std::vector<Chat> chats;

    { //: Не указаны никакие данные
        chats.push_back(Chat());
    }

    { //: Указан только chat_id
        Chat chat;
        chat.chat_id = 123;

        chats.push_back(chat);
    }

    { //: Указан только username
        Chat chat;
        chat.username = "user";

        chats.push_back(chat);
    }

    for (const Chat& chat : chats) {
        auto  memory = std::make_unique<MemoryStorage>();
        Cache cache(std::move(memory));

        auto res    = cache.create(chat);
        bool actual = res.has_value();
        EXPECT_FALSE(actual);
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Тестирование неуспешных сценариев при вызове Cache::create при повторном добавлении
 * 
 * Проверяется повторное создание пользователя
 * 
 * Ожидается ошибка
 */
TEST(Test_Cache, CheckCreate_AlreadyExists_Negative) {

    Chat chat;
    chat.chat_id  = 123;
    chat.username = "user";

    auto  memory = std::make_unique<MemoryStorage>();
    Cache cache(std::move(memory));

    auto res = cache.create(chat);
    ASSERT_TRUE(res.has_value());

    res = cache.create(chat);
    EXPECT_FALSE(res.has_value());
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Тестирование успешных сценариев при вызове Cache::update
 * 
 * Ошибки не ожидается
 */
TEST(Test_Cache, CheckUpdate_Positive) {

    Chat chat;
    chat.chat_id  = 123;
    chat.username = "user";

    auto  memory = std::make_unique<MemoryStorage>();
    Cache cache(std::move(memory));

    auto res = cache.create(chat);
    ASSERT_TRUE(res.has_value());

    bool actual = cache.update(chat);
    EXPECT_TRUE(actual);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Тестирование неуспешных сценариев при вызове Cache::update
 * 
 * Проверяется обновдление чата, который не был создан
 * 
 * Ожидается ошибка
 */
TEST(Test_Cache, CheckUpdate_Negative) {

    Chat chat;
    chat.chat_id  = 123;
    chat.username = "user";

    auto  memory = std::make_unique<MemoryStorage>();
    Cache cache(std::move(memory));

    bool actual = cache.update(chat);
    EXPECT_FALSE(actual);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Тестирование успешных сценариев при вызове Cache::find
 * 
 * Ошибки не ожидается
 */
TEST(Test_Cache, CheckFind_Positive) {

    Chat chat;
    chat.chat_id  = 123;
    chat.username = "user";

    auto  memory = std::make_unique<MemoryStorage>();
    Cache cache(std::move(memory));

    auto res = cache.create(chat);
    ASSERT_TRUE(res.has_value());

    auto res_find = cache.find({chat.chat_id});
    EXPECT_TRUE(res_find.has_value());
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Тестирование успешных сценариев при вызове Cache::find
 * 
 * Проверяется, что нет ошибки при поиске чата, которого нет
 * 
 * Ошибки не ожидается
 */
TEST(Test_Cache, CheckFind_OneChatID_Positive) {

    Chat chat;
    chat.chat_id  = 123;
    chat.username = "user";

    auto  memory = std::make_unique<MemoryStorage>();
    Cache cache(std::move(memory));

    auto res_find = cache.find({chat.chat_id});
    ASSERT_TRUE(res_find.has_value());
    EXPECT_EQ(res_find.value().size(), 0);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Тестирование успешных сценариев при вызове Cache::find
 * 
 * Проверяется, что при запросе нескольких чатов, только часть которых есть,
 * ошибки не будет и найденную вернутся
 * 
 * Ошибки не ожидается
 */
TEST(Test_Cache, CheckFind_ChatID_Positive) {

    Chat chat;
    chat.chat_id  = 123;
    chat.username = "user";

    auto  memory = std::make_unique<MemoryStorage>();
    Cache cache(std::move(memory));

    auto res = cache.create(chat);
    ASSERT_TRUE(res.has_value());

    auto res_find = cache.find({chat.chat_id, 333});
    ASSERT_TRUE(res_find.has_value());

    auto value = res_find.value();
    ASSERT_EQ(value.size(), 1);
    EXPECT_EQ((*value.begin())->chat_id, chat.chat_id);
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Тестирование успешных сценариев при вызове Cache::find
 * 
 * Проверяется, что 2 пользователя, которые были созданы, успешно нашлись.
 * 
 * Ошибки не ожидается
 */
TEST(Test_Cache, CheckFind_MultiChatID_Positive) {

    std::vector<Chat> chats;

    {
        Chat chat;
        chat.chat_id  = 123;
        chat.username = "user1";
        chats.push_back(chat);
    }

    {
        Chat chat;
        chat.chat_id  = 321;
        chat.username = "user2";
        chats.push_back(chat);
    }

    auto  memory = std::make_unique<MemoryStorage>();
    Cache cache(std::move(memory));

    std::vector<int64_t> ids;
    for (const Chat& chat : chats) {
        auto res = cache.create(chat);
        ASSERT_TRUE(res.has_value());
        ids.push_back(chat.chat_id);
    }

    auto res_find = cache.find(ids);
    ASSERT_TRUE(res_find.has_value());

    auto chats_find = res_find.value();
    ASSERT_EQ(chats_find.size(), chats.size());

    for (auto it = chats_find.begin(); it != chats_find.end(); ++it) {
        auto it_v = std::find(ids.begin(), ids.end(), (*it)->chat_id);
        if (it_v != ids.end()) {
            ids.erase(it_v);
        }
    }

    EXPECT_EQ(ids.size(), 0);
}
//----------------------------------------------------------------------------------------------------------------------
