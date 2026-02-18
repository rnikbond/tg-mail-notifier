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
include(deps/spdlog/CMakeLists.txt)
include(deps/cli11/CMakeLists.txt)
include(deps/httplib/CMakeLists.txt)
include(deps/nlohmann/CMakeLists.txt)
include(deps/curl/CMakeLists.txt)
include(deps/gmime/CMakeLists.txt)
include(deps/sqlite/CMakeLists.txt)

## Перечисление исходников
set(SOURCES
    src/main.cpp
)

## Добавление исходников к проекту
add_executable(tg-mail-notifier
    ${SOURCES}
    src/core/Config.h src/core/Config.cpp)

## Добавление доп. библиотек
target_link_libraries(tg-mail-notifier spdlog::spdlog)
target_link_libraries(tg-mail-notifier CLI11::CLI11)
target_link_libraries(tg-mail-notifier httplib)
target_link_libraries(tg-mail-notifier nlohmann_json::nlohmann_json)
target_link_libraries(tg-mail-notifier CURL::libcurl)
target_link_libraries(tg-mail-notifier ${GMIME_LIBRARIES})
target_link_libraries(tg-mail-notifier SQLite::SQLite3)
