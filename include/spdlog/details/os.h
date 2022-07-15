// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/common.h>
#include <ctime> // std::time_t

namespace spdlog {
namespace details {
namespace os {

spdlog::log_clock::time_point now() noexcept;

std::tm localtime(const std::time_t &time_tt) noexcept;

std::tm localtime() noexcept;

std::tm gmtime(const std::time_t &time_tt) noexcept;

std::tm gmtime() noexcept;

// eol definition
#define SPDLOG_EOL "\n"

constexpr static const char *default_eol = SPDLOG_EOL;

// folder separator
#define SPDLOG_FOLDER_SEPS "/"

constexpr static const char folder_seps[] = SPDLOG_FOLDER_SEPS;
constexpr static const filename_t::value_type folder_seps_filename[] = SPDLOG_FILENAME_T(SPDLOG_FOLDER_SEPS);

// fopen_s on non windows for writing
bool fopen_s(FILE **fp, const filename_t &filename, const filename_t &mode);

// Remove filename. return 0 on success
int remove(const filename_t &filename) noexcept;

// Remove file if exists. return 0 on success
// Note: Non atomic (might return failure to delete if concurrently deleted by other process/thread)
int remove_if_exists(const filename_t &filename) noexcept;

int rename(const filename_t &filename1, const filename_t &filename2) noexcept;

// Return if file exists.
bool path_exists(const filename_t &filename) noexcept;

// Return file size according to open FILE* object
size_t filesize(FILE *f);

// Return utc offset in minutes or throw spdlog_ex on failure
int utc_minutes_offset(const std::tm &tm = details::os::localtime());

// Return current thread id as size_t
// It exists because the std::this_thread::get_id() is much slower(especially
// under VS 2013)
size_t _thread_id() noexcept;

// Return current thread id as size_t (from thread local storage)
size_t thread_id() noexcept;

// This is avoid msvc issue in sleep_for that happens if the clock changes.
// See https://github.com/gabime/spdlog/issues/609
void sleep_for_millis(unsigned int milliseconds) noexcept;

std::string filename_to_str(const filename_t &filename);

int pid() noexcept;

// Determine if the terminal supports colors
// Source: https://github.com/agauniyal/rang/
bool is_color_terminal() noexcept;

// Determine if the terminal attached
// Source: https://github.com/agauniyal/rang/
bool in_terminal(FILE *file) noexcept;

// Return directory name from given path or empty string
// "abc/file" => "abc"
// "abc/" => "abc"
// "abc" => ""
// "abc///" => "abc//"
filename_t dir_name(const filename_t &path);

// Create a dir from the given path.
// Return true if succeeded or if this dir already exists.
bool create_dir(const filename_t &path);

// non thread safe, cross platform getenv/getenv_s
// return empty string if field not found
std::string getenv(const char *field);

} // namespace os
} // namespace details
} // namespace spdlog

#include "os-inl.h"
