//----------------------------------------------------------
#ifndef LOGGER_H
#define LOGGER_H
//----------------------------------------------------------
#include <spdlog/spdlog.h>
//----------------------------------------------------------

namespace logger {};
//----------------------------------------------------------

#define log_info(...) SPDLOG_INFO(__VA_ARGS__)
#define log_warn(...) SPDLOG_WARN(__VA_ARGS__)
#define log_error(...) SPDLOG_ERROR(__VA_ARGS__)
#define log_debug(...) SPDLOG_DEBUG(__VA_ARGS__)
//----------------------------------------------------------

#endif // LOGGER_H
