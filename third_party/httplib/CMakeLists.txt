## sudo apt install libssl-dev openssl

include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
    GET_TAG        v0.14.1
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(httplib)

## Поддержка OpenSSL
find_package(OpenSSL REQUIRED)
if(OpenSSL)
    target_compile_defenitions(httplib
        PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT
    )

    target_link_libraries(httplib
        PRIVATE OpenSSL::SSL OpenSSL::Crypto
    )
endif()
