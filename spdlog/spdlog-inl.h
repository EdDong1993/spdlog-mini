// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/common.h>
#include <spdlog/pattern_formatter.h>

namespace spdlog {

inline void initialize_logger(std::shared_ptr<logger> logger)
{
    details::registry::instance().initialize_logger(std::move(logger));
}

inline std::shared_ptr<logger> get(const std::string &name)
{
    return details::registry::instance().get(name);
}

inline void set_formatter(std::unique_ptr<spdlog::formatter> formatter)
{
    details::registry::instance().set_formatter(std::move(formatter));
}

inline void set_pattern(std::string pattern, pattern_time_type time_type)
{
    set_formatter(std::unique_ptr<spdlog::formatter>(new pattern_formatter(std::move(pattern), time_type)));
}

inline void enable_backtrace(size_t n_messages)
{
    details::registry::instance().enable_backtrace(n_messages);
}

inline void disable_backtrace()
{
    details::registry::instance().disable_backtrace();
}

inline void dump_backtrace()
{
    default_logger_raw()->dump_backtrace();
}

inline level::level_enum get_level()
{
    return default_logger_raw()->level();
}

inline bool should_log(level::level_enum log_level)
{
    return default_logger_raw()->should_log(log_level);
}

inline void set_level(level::level_enum log_level)
{
    details::registry::instance().set_level(log_level);
}

inline void flush_on(level::level_enum log_level)
{
    details::registry::instance().flush_on(log_level);
}

inline void flush_every(std::chrono::seconds interval)
{
    details::registry::instance().flush_every(interval);
}

inline void set_error_handler(void (*handler)(const std::string &msg))
{
    details::registry::instance().set_error_handler(handler);
}

inline void register_logger(std::shared_ptr<logger> logger)
{
    details::registry::instance().register_logger(std::move(logger));
}

inline void apply_all(const std::function<void(std::shared_ptr<logger>)> &fun)
{
    details::registry::instance().apply_all(fun);
}

inline void drop(const std::string &name)
{
    details::registry::instance().drop(name);
}

inline void drop_all()
{
    details::registry::instance().drop_all();
}

inline void shutdown()
{
    details::registry::instance().shutdown();
}

inline void set_automatic_registration(bool automatic_registration)
{
    details::registry::instance().set_automatic_registration(automatic_registration);
}

inline std::shared_ptr<spdlog::logger> default_logger()
{
    return details::registry::instance().default_logger();
}

inline spdlog::logger *default_logger_raw()
{
    return details::registry::instance().get_default_raw();
}

inline void set_default_logger(std::shared_ptr<spdlog::logger> default_logger)
{
    details::registry::instance().set_default_logger(std::move(default_logger));
}

} // namespace spdlog
