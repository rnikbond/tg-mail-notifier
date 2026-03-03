## Список ваших файлов
file(GLOB MIGRATION_SQL_FILES "*.sql")

## Путь к сгенерированному файлу
set(MIGRATION_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated_migrations.h")

## Функция для превращения файла в C-массив
function(generate_migrations_header OUTPUT_FILE)
    file(WRITE ${OUTPUT_FILE} "#pragma once\n#include <map>\n#include <string>\n\n")
    file(APPEND ${OUTPUT_FILE} "inline std::map<std::string, std::string> migration_files = {\n")
    foreach(FILE_PATH ${ARGN})
        file(READ ${FILE_PATH} CONTENT)
        get_filename_component(FILE_NAME ${FILE_PATH} NAME)
        file(APPEND ${OUTPUT_FILE} "    {\"${FILE_NAME}\", R\"raw(${CONTENT})raw\"},\n")
    endforeach()
    file(APPEND ${OUTPUT_FILE} "};\n")
endfunction()

## Запускаем генерацию
generate_migrations_header(${MIGRATION_HEADER_FILE} ${MIGRATION_SQL_FILES})

## Создание INTERFACE библиотеки для подключения сгенерированного файла миграций
add_library(migrations_interface INTERFACE
    ${MIGRATION_SQL_FILES}
    ${MIGRATION_HEADER_FILE}
)

## Добавляем сгенерированный файл, чтоб он был виден в коде в #include
target_include_directories(migrations_interface INTERFACE
    "${CMAKE_CURRENT_BINARY_DIR}"
)
