cmake_minimum_required(VERSION 3.16)

## Название проекта
project(tg-mail-notifier LANGUAGES CXX)

## Указание версии С++
set(CMAKE_CXX_STANDARD 23)
## Указание, что версия С++ должна быть обязательно 23
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## Инициализация переменной, куда должен быть помещен исполняемый файл
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

## Добавление зависимостей
include(third_party/spdlog/CMakeLists.txt)
include(third_party/cli11/CMakeLists.txt)
include(third_party/httplib/CMakeLists.txt)
include(third_party/nlohmann/CMakeLists.txt)
include(third_party/curl/CMakeLists.txt)
include(third_party/gmime/CMakeLists.txt)
include(third_party/sqlite/CMakeLists.txt)

## Перечисление путей для include
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/logger
)

## Перечисление заголовочных файлов
set(INCLUDES
    include/storage/IStorage.h
    include/repository/IRepository.h
    include/chat/Chat.h
    include/errs/Errors.h
    include/mail/IMailRequest.h
    include/telegram/TelegramAPI.h
    include/telegram/ITelegramSender.h
    include/logger/logger.h
)

## Перечисление исходников
set(SOURCES
    src/main.cpp
    src/core/Config.h                    src/core/Config.cpp
    src/cache/Cache.h                    src/cache/Cache.cpp
    src/storage/memory/MemoryStorage.h   src/storage/memory/MemoryStorage.cpp
    src/telegram/TelegramManager.h       src/telegram/TelegramManager.cpp
    src/mail/MailManager.h               src/mail/MailManager.cpp
    src/mail/MailRequest.h               src/mail/MailRequest.cpp
    src/mail/MailFactory.h               src/mail/MailFactory.cpp
    src/telegram/TelegramController.h    src/telegram/TelegramController.cpp
    src/telegram/TelegramSender.h        src/telegram/TelegramSender.cpp
    src/telegram/TelegramSenderFactory.h src/telegram/TelegramSenderFactory.cpp
)

## Добавление исходников к проекту
add_executable(tg-mail-notifier
    ${INCLUDES}
    ${SOURCES}
)

## Добавление доп. библиотек
target_link_libraries(tg-mail-notifier spdlog::spdlog)
target_link_libraries(tg-mail-notifier CLI11::CLI11)
target_link_libraries(tg-mail-notifier httplib)
target_link_libraries(tg-mail-notifier nlohmann_json::nlohmann_json)
target_link_libraries(tg-mail-notifier CURL::libcurl)
target_link_libraries(tg-mail-notifier ${GMIME_LIBRARIES})
target_link_libraries(tg-mail-notifier SQLite::SQLite3)

add_subdirectory(tests)
