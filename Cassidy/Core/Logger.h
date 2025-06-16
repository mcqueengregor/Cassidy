#pragma once
#include <Vendor/spdlog/include/spdlog/spdlog.h>

#ifndef CS_LOG_INFO
#define CS_LOG_INFO(...) spdlog::info(__VA_ARGS__)
#endif

#ifndef CS_LOG_ERROR
#define CS_LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#endif

#ifndef CS_LOG_WARN
#define CS_LOG_WARN(...) spdlog::warn(__VA_ARGS__)
#endif

#ifndef CS_LOG_CRITICAL
#define CS_LOG_CRITICAL(...) spdlog::critical(__VA_ARGS__)
#endif