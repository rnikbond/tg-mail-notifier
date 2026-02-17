## sudo apt install nlohmann-json3-dev

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG        v3.11.2
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(nlohmann_json)
