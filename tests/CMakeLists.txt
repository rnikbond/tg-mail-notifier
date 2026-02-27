cmake_minimum_required(VERSION 3.14)

## Подключение googletests
include(../third_party/googletest/CMakeLists.txt)
include(../third_party/spdlog/CMakeLists.txt)

## Перечисление путей для include
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${googletest_SOURCE_DIR}/include
)

## Сбор всех исходников с тестами
file(GLOB TEST_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

## Перечисление заголовочных файлов
set(INCLUDE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/storage/IStorage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/repository/IRepository.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/chat/Chat.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/telegram/TelegramAPI.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mail/IMailRequest.h
)

## Перечисление исходников
set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cache/Cache.h                  ${CMAKE_CURRENT_SOURCE_DIR}/../src/cache/Cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/storage/memory/MemoryStorage.h ${CMAKE_CURRENT_SOURCE_DIR}/../src/storage/memory/MemoryStorage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/mail/MailRequest.h             ${CMAKE_CURRENT_SOURCE_DIR}/../src/mail/MailRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/mail/MailFactory.h             ${CMAKE_CURRENT_SOURCE_DIR}/../src/mail/MailFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/telegram/TelegramController.h  ${CMAKE_CURRENT_SOURCE_DIR}/../src/telegram/TelegramController.cpp
)

## Добавление исходников к проекту
add_executable(unit_tests
    ${INCLUDE_FILES}
    ${SRC_FILES}
    ${TEST_SRC_FILES}
)

## Добавление доп. библиотек
target_link_libraries(unit_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    CURL::libcurl
)

enable_testing()
add_test(NAME UnitTests COMMAND unit_tests)
