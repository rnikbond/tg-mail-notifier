## Используется для парсинга конфигурационного файла и аргументов

include(FetchContent)

FetchContent_declare (
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.4.2
)

FetchContent_MakeAvailable(cli11)
