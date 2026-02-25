//----------------------------------------------------------
#include <deque>
//----------------------------------------------------------
#include <gtest/gtest.h>
//----------------------------------------------------------
#include "nlohmann/json.hpp"
//----------------------------------------------------------
#include "../src/cache/Cache.h"
#include "../src/storage/memory/MemoryStorage.h"
#include "../src/telegram/TelegramController.h"
//----------------------------------------------------------
using json = nlohmann::json;
//----------------------------------------------------------
class MockMailRequest : public IMailRequest {

public:

    MockMailRequest()  = default;
    ~MockMailRequest() = default;

    [[nodiscard]] virtual UIDsOpt load_uids(const Email& email) const noexcept override {
        return {};
    }
    [[nodiscard]] virtual UIDOpt last_uid(const Email& email) const noexcept override {
        return 0;
    }
};
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Тестирование наличия исключений при создании объекта
 * 
 * Проверяется инициализация необходимых объектов
 * 
 * Ожидается исключение
 */
TEST(TelegramController, CheckExceptionToken) {

    ASSERT_ANY_THROW(TelegramController("QwEwEr", nullptr, std::make_unique<MockMailRequest>()));

    auto memory = std::make_unique<MemoryStorage>();
    auto cache  = std::make_shared<Cache>(std::move(memory));
    ASSERT_ANY_THROW(TelegramController("", cache, nullptr));
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Тестирование обработки невалидных ответов с пустым JSON
 * 
 * Ошижидается исключение
 */
TEST(TelegramController, CheckInvalidJSON) {

    std::vector<TelegramResponse> tests = {
        {""},
        {"123321"},
        {"[]"},
        {"{}"},
        {"{[]}"},
    };

    auto        memory      = std::make_unique<MemoryStorage>();
    auto        cache       = std::make_shared<Cache>(std::move(memory));
    std::string token       = "QwEwEr";
    int64_t     last_upd_id = 0;

    TelegramController controller(token, cache, std::make_unique<MockMailRequest>());

    for (auto& response : tests) {
        //ASSERT_ANY_THROW(controller.process(std::move(response), last_upd_id));
    }
}
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Тестирование обработки невалидных ответов с пустым JSON
 * 
 * Ошижидается исключение
 */
TEST(TelegramController, CheckIncompleteJSON) {

    // struct TestData
    // {
    //     TelegramResponse response;
    //     bool             wait_exception = {false};
    //     bool             wait_nullopt   = {false};
    // };
    // std::vector<TestData> tests;

    // { //: только result, без сообщений
    //     // clang-format off
    //     json js = {
    //         {
    //             {"ok", true},
    //             {"result", {
    //             }}
    //         }
    //     };
    //     // clang-format on

    //     TestData data;
    //     data.response     = TelegramResponse{js.dump()};
    //     data.wait_nullopt = true;

    //     tests.push_back(data);
    // }

    // { //: пустой message
    //     // clang-format off
    //     json js = {
    //         {
    //             {"ok", true},
    //             {"result", json::array({
    //                 {
    //                     {"message", {

    //                     }}
    //                 },
    //             })}
    //         }
    //     };
    //     // clang-format on

    //     TestData data;
    //     data.response       = TelegramResponse{js.dump()};
    //     data.wait_exception = true;

    //     tests.push_back(data);
    // }

    // { //: message без chat
    //     // clang-format off
    //     json js = {
    //         {
    //             {"ok", true},
    //             {"result", json::array({
    //                 {
    //                     {"message", {
    //                         {"from",
    //                             {
    //                                 {"first_name", "tester"  },
    //                                 {"id"        , "123321"  },
    //                                 {"last_name" , "testing" },
    //                                 {"type"      , "private" },
    //                                 {"username"  , "top_test"}
    //                             },
    //                         },
    //                     }}
    //                 },
    //             })}
    //         }
    //     };
    //     // clang-format on

    //     TestData data;
    //     data.response       = TelegramResponse{js.dump()};
    //     data.wait_exception = true;

    //     tests.push_back(data);
    // }

    // auto        memory      = std::make_unique<MemoryStorage>();
    // auto        cache       = std::make_shared<Cache>(std::move(memory));
    // std::string token       = "QwEwEr";
    // int64_t     last_upd_id = 0;

    // TelegramController controller(token, cache);

    // for (auto& test : tests) {

    //     if (test.wait_exception) {
    //         ASSERT_ANY_THROW(controller.process(std::move(test.response), last_upd_id));
    //     }

    //     if (test.wait_nullopt) {
    //         ASSERT_FALSE(controller.process(std::move(test.response), last_upd_id).has_value());
    //     }

    //     ASSERT_EQ(last_upd_id, 0);
    // }
}
//----------------------------------------------------------------------------------------------------------------------

TEST(TelegramController, CheckChainNewUser) {

    std::string token = "QwEwEr";

    struct ExpectedData
    {
        int64_t         last_upd  = {0};
        bool            has_value = {false};
        TelegramRequest request;
    };

    struct TestData
    {
        std::string json_str;
        ExpectedData expected;
    };
    std::deque<TestData> tests;

    { //: Пользователь отправляет /start
        TestData data;
        data.expected.last_upd  = 123123123;
        data.expected.has_value = true; //: Должно отправиться сообщение с описанием

        // clang-format off
        json js = {
            {"ok", true},
            {"result", json::array({
                {
                    {"message",
                        {
                            {"chat", {
                                        {"first_name", "Ivan"     },
                                        {"id"        , 12121211   },
                                        {"last_name" , "Ivanov"   },
                                        {"type"      , "private"  },
                                        {"username"  , "ivan"     },
                                     }
                            },
                            {"from", {
                                        {"first_name", "tester"  },
                                        {"id"        , "123321"  },
                                        {"last_name" , "testing" },
                                        {"type"      , "private" },
                                        {"username"  , "top_test"}
                                     },
                            },
                            {"date"      , 1771662609 },
                            {"message_id", 772        },
                            {"text"      , "/start"},
                        },
                    },
                    {"update_id", data.expected.last_upd},
                },
            })}
        };
        // clang-format on

        { //: expected
            constexpr std::string_view url = "/bot{}/sendMessage";

            TelegramRequest request;
            request.url          = std::format(url, token);
            request.body         = "";
            request.content_type = "application/json";

            data.expected.request = request;
        }

        data.json_str = js.dump();
        tests.push_back(data);
    }

    { //: В ответе на запрос нет новых сообщений

        // clang-format off
        json js = {
            {"ok"    , true},
            {"result", json::array()}
        };
        // clang-format on

        TestData data;
        data.json_str           = js.dump();
        data.expected.last_upd  = 0;
        data.expected.has_value = false;

        tests.push_back(data);
    }

    { //: Пользователь отправляет /email
        TestData data;
        data.expected.last_upd  = 123123123;
        data.expected.has_value = true; //: Должно отправиться сообщение для ответа на email

        // clang-format off
        json js = {
            {"ok", true},
            {"result", json::array({
                                    {
                                        {"message",
                                         {
                                             {"chat", {
                                                          {"first_name", "Ivan"     },
                                                          {"id"        , 12121211   },
                                                          {"last_name" , "Ivanov"   },
                                                          {"type"      , "private"  },
                                                          {"username"  , "ivan"     },
                                                          }
                                             },
                                             {"from", {
                                                          {"first_name", "tester"  },
                                                          {"id"        , "123321"  },
                                                          {"last_name" , "testing" },
                                                          {"type"      , "private" },
                                                          {"username"  , "top_test"}
                                                      },
                                              },
                                             {"date"      , 1771662609 },
                                             {"message_id", 772        },
                                             {"text"      , "/email"   },
                                             },
                                         },
                                        {"update_id", data.expected.last_upd},
                                        },
                                    })}
        };
        // clang-format on

        { //: expected
            constexpr std::string_view url = "/bot{}/sendMessage";

            TelegramRequest request;
            request.url          = std::format(url, token);
            request.body         = "";
            request.content_type = "application/json";

            data.expected.request = request;
        }

        data.json_str = js.dump();
        tests.push_back(data);
    }

    { //: Пользователь отправляет /password
        TestData data;
        data.expected.last_upd  = 123123123;
        data.expected.has_value = true; //: Должно отправиться сообщение для ответа на email

        // clang-format off
        json js = {
            {"ok", true},
            {"result", json::array({
                                    {
                                        {"message",
                                         {
                                             {"chat", {
                                                          {"first_name", "Ivan"     },
                                                          {"id"        , 12121211   },
                                                          {"last_name" , "Ivanov"   },
                                                          {"type"      , "private"  },
                                                          {"username"  , "ivan"     },
                                                          }
                                             },
                                             {"from", {
                                                          {"first_name", "tester"  },
                                                          {"id"        , "123321"  },
                                                          {"last_name" , "testing" },
                                                          {"type"      , "private" },
                                                          {"username"  , "top_test"}
                                                      },
                                              },
                                             {"date"      , 1771662609 },
                                             {"message_id", 772        },
                                             {"text"      , "/password"},
                                             },
                                         },
                                        {"update_id", data.expected.last_upd},
                                        },
                                    })}
        };
        // clang-format on

        { //: expected
            constexpr std::string_view url = "/bot{}/sendMessage";

            TelegramRequest request;
            request.url          = std::format(url, token);
            request.body         = "";
            request.content_type = "application/json";

            data.expected.request = request;
        }

        data.json_str = js.dump();
        tests.push_back(data);
    }

    { //: Пользователь отправляет /status
        TestData data;
        data.expected.last_upd  = 123123123;
        data.expected.has_value = true; //: Должно отправиться сообщение для ответа на email

        // clang-format off
        json js = {
            {"ok", true},
            {"result", json::array({
                                    {
                                        {"message",
                                         {
                                             {"chat", {
                                                          {"first_name", "Ivan"     },
                                                          {"id"        , 12121211   },
                                                          {"last_name" , "Ivanov"   },
                                                          {"type"      , "private"  },
                                                          {"username"  , "ivan"     },
                                                          }
                                             },
                                             {"from", {
                                                          {"first_name", "tester"  },
                                                          {"id"        , "123321"  },
                                                          {"last_name" , "testing" },
                                                          {"type"      , "private" },
                                                          {"username"  , "top_test"}
                                                      },
                                              },
                                             {"date"      , 1771662609 },
                                             {"message_id", 772        },
                                             {"text"      , "/status"  },
                                             },
                                         },
                                        {"update_id", data.expected.last_upd},
                                        },
                                    })}
        };
        // clang-format on

        { //: expected
            constexpr std::string_view url = "/bot{}/sendMessage";

            TelegramRequest request;
            request.url          = std::format(url, token);
            request.body         = "";
            request.content_type = "application/json";

            data.expected.request = request;
        }

        data.json_str = js.dump();
        tests.push_back(data);
    }

    while (!tests.empty()) {
        auto memory = std::make_unique<MemoryStorage>();
        auto cache  = std::make_shared<Cache>(std::move(memory));

        TelegramController controller(token, cache, std::make_unique<MockMailRequest>());

        TestData test = std::move(tests.front());
        tests.pop_front();

        int64_t    actual_last_upd_id = 0;
        RequestOpt actual_value;
        ASSERT_NO_THROW(actual_value = controller.process(TelegramResponse{test.json_str}, actual_last_upd_id));

        ASSERT_EQ(actual_last_upd_id, test.expected.last_upd);

        ASSERT_EQ(actual_value.has_value(), test.expected.has_value);
        if (!test.expected.has_value) {
            continue;
        }

        auto actual = actual_value.value();

        ASSERT_EQ(actual.url, test.expected.request.url);
        ASSERT_EQ(actual.content_type, test.expected.request.content_type);
    }
}
//----------------------------------------------------------------------------------------------------------------------
