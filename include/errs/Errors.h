//----------------------------------------------------------
#ifndef ERRORS_H
#define ERRORS_H
//----------------------------------------------------------
#include <string_view>
//----------------------------------------------------------

namespace Errors {
/// @brief Ошибки при работе с почтовым сервисом
enum class Mail {
    OK       = 0,   ///< Ошибок нет
    Auth     = 1,   ///< Ошибка айтентификации
    Internal = 100, ///< Внутренняя ошибка
};
//----------------------------------------------------------

/// @brief Ошибки при работе с репозиторием
enum class Repository {
    OK,            ///<
    NotFound,      ///<
    AlreadyExists, ///<
    InvalidEmail,  ///<
    InvalidChat,   ///<
    InvalidUID,    ///<
    Internal,      ///<
};
//----------------------------------------------------------

template<typename ErrT>
[[nodiscard]] constexpr std::string_view to_string(ErrT err) noexcept {

    if constexpr (std::same_as<ErrT, Mail>) {
        switch (err) {
            case Mail::OK:
                return "OK";
            case Mail::Auth:
                return "invalid email or password";
            case Mail::Internal:
                return "internal mail error";
            default:
                return "unknown mail error";
        }
    } else if constexpr (std::same_as<ErrT, Repository>) {
        switch (err) {
            case Repository::OK:
                return "OK";
            case Repository::NotFound:
                return "not found";
            case Repository::AlreadyExists:
                return "already exists";
            case Repository::InvalidEmail:
                return "invalid email data";
            case Repository::InvalidChat:
                return "invalid chat data";
            case Repository::InvalidUID:
                return "invalid mail UID";
            case Repository::Internal:
                return "internal retository error";
            default:
                return "unknown repository error";
        }
    }

    return "unknown error";
}
//----------------------------------------------------------

}; // namespace Errors
//----------------------------------------------------------

#endif // ERRORS_H
