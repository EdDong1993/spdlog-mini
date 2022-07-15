// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/common.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>
#include <array>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h> //Use gettid() syscall under linux to get thread id

#ifndef __has_feature          // Clang - feature checking macros.
#    define __has_feature(x) 0 // Compatibility with non-clang compilers.
#endif

namespace spdlog {
namespace details {
namespace os {

inline spdlog::log_clock::time_point now() noexcept
{

#if defined __linux__ && defined SPDLOG_CLOCK_COARSE
    timespec ts;
    ::clock_gettime(CLOCK_REALTIME_COARSE, &ts);
    return std::chrono::time_point<log_clock, typename log_clock::duration>(
        std::chrono::duration_cast<typename log_clock::duration>(std::chrono::seconds(ts.tv_sec) + std::chrono::nanoseconds(ts.tv_nsec)));

#else
    return log_clock::now();
#endif
}
inline std::tm localtime(const std::time_t &time_tt) noexcept
{
    std::tm tm;
    ::localtime_r(&time_tt, &tm);
    return tm;
}

inline std::tm localtime() noexcept
{
    std::time_t now_t = ::time(nullptr);
    return localtime(now_t);
}

inline std::tm gmtime(const std::time_t &time_tt) noexcept
{
    std::tm tm;
    ::gmtime_r(&time_tt, &tm);
    return tm;
}

inline std::tm gmtime() noexcept
{
    std::time_t now_t = ::time(nullptr);
    return gmtime(now_t);
}

// fopen_s on non windows for writing
inline bool fopen_s(FILE **fp, const filename_t &filename, const filename_t &mode)
{
#    if defined(SPDLOG_PREVENT_CHILD_FD)
    const int mode_flag = mode == SPDLOG_FILENAME_T("ab") ? O_APPEND : O_TRUNC;
    const int fd = ::open((filename.c_str()), O_CREAT | O_WRONLY | O_CLOEXEC | mode_flag, mode_t(0644));
    if (fd == -1)
    {
        return true;
    }
    *fp = ::fdopen(fd, mode.c_str());
    if (*fp == nullptr)
    {
        ::close(fd);
    }
#    else
    *fp = ::fopen((filename.c_str()), mode.c_str());
#    endif

    return *fp == nullptr;
}

inline int remove(const filename_t &filename) noexcept
{
    return std::remove(filename.c_str());
}

inline int remove_if_exists(const filename_t &filename) noexcept
{
    return path_exists(filename) ? remove(filename) : 0;
}

inline int rename(const filename_t &filename1, const filename_t &filename2) noexcept
{
    return std::rename(filename1.c_str(), filename2.c_str());
}

// Return true if path exists (file or directory)
inline bool path_exists(const filename_t &filename) noexcept
{
    struct stat buffer;
    return (::stat(filename.c_str(), &buffer) == 0);
}

// Return file size according to open FILE* object
inline size_t filesize(FILE *f)
{
    if (f == nullptr)
    {
        throw_spdlog_ex("Failed getting file size. fd is null");
    }

    int fd = ::fileno(f);

    struct stat64 st;
    if (::fstat64(fd, &st) == 0)
    {
        return static_cast<size_t>(st.st_size);
    }
    throw_spdlog_ex("Failed getting file size from fd", errno);
    return 0; // will not be reached.
}

// Return utc offset in minutes or throw spdlog_ex on failure
inline int utc_minutes_offset(const std::tm &tm)
{
    return static_cast<int>(tm.tm_gmtoff / 60);
}

// Return current thread id as size_t
// It exists because the std::this_thread::get_id() is much slower(especially
// under VS 2013)
inline size_t _thread_id() noexcept
{
    return static_cast<size_t>(::syscall(SYS_gettid));
}

// Return current thread id as size_t (from thread local storage)
inline size_t thread_id() noexcept
{
#if defined(SPDLOG_NO_TLS)
    return _thread_id();
#else // cache thread id in tls
    static thread_local const size_t tid = _thread_id();
    return tid;
#endif
}

// This is avoid msvc issue in sleep_for that happens if the clock changes.
// See https://github.com/gabime/spdlog/issues/609
inline void sleep_for_millis(unsigned int milliseconds) noexcept
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

inline std::string filename_to_str(const filename_t &filename)
{
    return filename;
}

inline int pid() noexcept
{
    return conditional_static_cast<int>(::getpid());
}

// Determine if the terminal supports colors
// Based on: https://github.com/agauniyal/rang/
inline bool is_color_terminal() noexcept
{
    static const bool result = []() {
        const char *env_colorterm_p = std::getenv("COLORTERM");
        if (env_colorterm_p != nullptr)
        {
            return true;
        }

        static constexpr std::array<const char *, 16> terms = {{"ansi", "color", "console", "cygwin", "gnome", "konsole", "kterm", "linux",
            "msys", "putty", "rxvt", "screen", "vt100", "xterm", "alacritty", "vt102"}};

        const char *env_term_p = std::getenv("TERM");
        if (env_term_p == nullptr)
        {
            return false;
        }

        return std::any_of(terms.begin(), terms.end(), [&](const char *term) { return std::strstr(env_term_p, term) != nullptr; });
    }();

    return result;
}

// Determine if the terminal attached
// Source: https://github.com/agauniyal/rang/
inline bool in_terminal(FILE *file) noexcept
{
    return ::isatty(fileno(file)) != 0;
}

// return true on success
static inline bool mkdir_(const filename_t &path)
{
    return ::mkdir(path.c_str(), mode_t(0755)) == 0;
}

// create the given directory - and all directories leading to it
// return true on success or if the directory already exists
inline bool create_dir(const filename_t &path)
{
    if (path_exists(path))
    {
        return true;
    }

    if (path.empty())
    {
        return false;
    }

    size_t search_offset = 0;
    do
    {
        auto token_pos = path.find_first_of(folder_seps_filename, search_offset);
        // treat the entire path as a folder if no folder separator not found
        if (token_pos == filename_t::npos)
        {
            token_pos = path.size();
        }

        auto subdir = path.substr(0, token_pos);

        if (!subdir.empty() && !path_exists(subdir) && !mkdir_(subdir))
        {
            return false; // return error if failed creating dir
        }
        search_offset = token_pos + 1;
    } while (search_offset < path.size());

    return true;
}

// Return directory name from given path or empty string
// "abc/file" => "abc"
// "abc/" => "abc"
// "abc" => ""
// "abc///" => "abc//"
inline filename_t dir_name(const filename_t &path)
{
    auto pos = path.find_last_of(folder_seps_filename);
    return pos != filename_t::npos ? path.substr(0, pos) : filename_t{};
}

std::string inline getenv(const char *field)
{
    char *buf = ::getenv(field);
    return buf ? buf : std::string{};
}

} // namespace os
} // namespace details
} // namespace spdlog
