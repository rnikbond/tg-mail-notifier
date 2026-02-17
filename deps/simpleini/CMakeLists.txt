include(FetchContent)

FetchContent_declare (
    simpleini
    GIT_REPOSITORY https://github.com/brofield/simpleini
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(simpleini)
