// Formatting library for C++ - optional OS-specific functionality
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_OS_H_
#define FMT_OS_H_

#include <cerrno>
#include <clocale>  // locale_t
#include <cstddef>
#include <cstdio>
#include <cstdlib>       // strtod_l
#include <system_error>  // std::system_error

#include "format.h"

#include <fcntl.h> // for O_RDONLY

#ifndef FMT_POSIX
#    define FMT_POSIX(call) call
#endif

// Calls to system functions are wrapped in FMT_SYSTEM for testability.
#ifdef FMT_SYSTEM
#  define FMT_POSIX_CALL(call) FMT_SYSTEM(call)
#else
#  define FMT_SYSTEM(call) ::call
#    define FMT_POSIX_CALL(call) ::call
#endif

// Retries the expression while it evaluates to error_result and errno
// equals to EINTR.
#  define FMT_RETRY_VAL(result, expression, error_result) \
    do {                                                  \
      (result) = (expression);                            \
    } while ((result) == (error_result) && errno == EINTR)

#define FMT_RETRY(result, expression) FMT_RETRY_VAL(result, expression, -1)

FMT_BEGIN_NAMESPACE
FMT_MODULE_EXPORT_BEGIN

/**
  \rst
  A reference to a null-terminated string. It can be constructed from a C
  string or ``std::string``.

  You can use one of the following type aliases for common character types:

  +---------------+-----------------------------+
  | Type          | Definition                  |
  +===============+=============================+
  | cstring_view  | basic_cstring_view<char>    |
  +---------------+-----------------------------+
  | wcstring_view | basic_cstring_view<wchar_t> |
  +---------------+-----------------------------+

  This class is most useful as a parameter type to allow passing
  different types of strings to a function, for example::

    template <typename... Args>
    std::string format(cstring_view format_str, const Args & ... args);

    format("{}", 42);
    format(std::string("{}"), 42);
  \endrst
 */
template <typename Char> class basic_cstring_view {
 private:
  const Char* data_;

 public:
  /** Constructs a string reference object from a C string. */
  basic_cstring_view(const Char* s) : data_(s) {}

  /**
    \rst
    Constructs a string reference from an ``std::string`` object.
    \endrst
   */
  basic_cstring_view(const std::basic_string<Char>& s) : data_(s.c_str()) {}

  /** Returns the pointer to a C string. */
  const Char* c_str() const { return data_; }
};

using cstring_view = basic_cstring_view<char>;
using wcstring_view = basic_cstring_view<wchar_t>;

template <typename Char> struct formatter<std::error_code, Char> {
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext>
  FMT_CONSTEXPR auto format(const std::error_code& ec, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    auto out = ctx.out();
    out = detail::write_bytes(out, ec.category().name(),
                              basic_format_specs<Char>());
    out = detail::write<Char>(out, Char(':'));
    out = detail::write<Char>(out, ec.value());
    return out;
  }
};

inline const std::error_category& system_category() FMT_NOEXCEPT {
  return std::system_category();
}

// A buffered file.
class buffered_file {
 private:
  FILE* file_;

  friend class file;

  explicit buffered_file(FILE* f) : file_(f) {}

 public:
  buffered_file(const buffered_file&) = delete;
  void operator=(const buffered_file&) = delete;

  // Constructs a buffered_file object which doesn't represent any file.
  buffered_file() FMT_NOEXCEPT : file_(nullptr) {}

  // Destroys the object closing the file it represents if any.
  FMT_API ~buffered_file() FMT_NOEXCEPT;

 public:
  buffered_file(buffered_file&& other) FMT_NOEXCEPT : file_(other.file_) {
    other.file_ = nullptr;
  }

  buffered_file& operator=(buffered_file&& other) {
    close();
    file_ = other.file_;
    other.file_ = nullptr;
    return *this;
  }

  // Opens a file.
  FMT_API buffered_file(cstring_view filename, cstring_view mode);

  // Closes the file.
  FMT_API void close();

  // Returns the pointer to a FILE object representing this file.
  FILE* get() const FMT_NOEXCEPT { return file_; }

  // We place parentheses around fileno to workaround a bug in some versions
  // of MinGW that define fileno as a macro.
  FMT_API int(fileno)() const;

  void vprint(string_view format_str, format_args args) {
    fmt::vprint(file_, format_str, args);
  }

  template <typename... Args>
  inline void print(string_view format_str, const Args&... args) {
    vprint(format_str, fmt::make_format_args(args...));
  }
};

// A file. Closed file is represented by a file object with descriptor -1.
// Methods that are not declared with FMT_NOEXCEPT may throw
// fmt::system_error in case of failure. Note that some errors such as
// closing the file multiple times will cause a crash on Windows rather
// than an exception. You can get standard behavior by overriding the
// invalid parameter handler with _set_invalid_parameter_handler.
class file {
 private:
  int fd_;  // File descriptor.

  // Constructs a file object with a given descriptor.
  explicit file(int fd) : fd_(fd) {}

 public:
  // Possible values for the oflag argument to the constructor.
  enum {
    RDONLY = FMT_POSIX(O_RDONLY),  // Open for reading only.
    WRONLY = FMT_POSIX(O_WRONLY),  // Open for writing only.
    RDWR = FMT_POSIX(O_RDWR),      // Open for reading and writing.
    CREATE = FMT_POSIX(O_CREAT),   // Create if the file doesn't exist.
    APPEND = FMT_POSIX(O_APPEND),  // Open in append mode.
    TRUNC = FMT_POSIX(O_TRUNC)     // Truncate the content of the file.
  };

  // Constructs a file object which doesn't represent any file.
  file() FMT_NOEXCEPT : fd_(-1) {}

  // Opens a file and constructs a file object representing this file.
  FMT_API file(cstring_view path, int oflag);

 public:
  file(const file&) = delete;
  void operator=(const file&) = delete;

  file(file&& other) FMT_NOEXCEPT : fd_(other.fd_) { other.fd_ = -1; }

  // Move assignment is not noexcept because close may throw.
  file& operator=(file&& other) {
    close();
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }

  // Destroys the object closing the file it represents if any.
  FMT_API ~file() FMT_NOEXCEPT;

  // Returns the file descriptor.
  int descriptor() const FMT_NOEXCEPT { return fd_; }

  // Closes the file.
  FMT_API void close();

  // Returns the file size. The size has signed type for consistency with
  // stat::st_size.
  FMT_API long long size() const;

  // Attempts to read count bytes from the file into the specified buffer.
  FMT_API size_t read(void* buffer, size_t count);

  // Attempts to write count bytes from the specified buffer to the file.
  FMT_API size_t write(const void* buffer, size_t count);

  // Duplicates a file descriptor with the dup function and returns
  // the duplicate as a file object.
  FMT_API static file dup(int fd);

  // Makes fd be the copy of this file descriptor, closing fd first if
  // necessary.
  FMT_API void dup2(int fd);

  // Makes fd be the copy of this file descriptor, closing fd first if
  // necessary.
  FMT_API void dup2(int fd, std::error_code& ec) FMT_NOEXCEPT;

  // Creates a pipe setting up read_end and write_end file objects for reading
  // and writing respectively.
  FMT_API static void pipe(file& read_end, file& write_end);

  // Creates a buffered_file object associated with this file and detaches
  // this file object from the file.
  FMT_API buffered_file fdopen(const char* mode);
};

// Returns the memory page size.
long getpagesize();

FMT_BEGIN_DETAIL_NAMESPACE

struct buffer_size {
  buffer_size() = default;
  size_t value = 0;
  buffer_size operator=(size_t val) const {
    auto bs = buffer_size();
    bs.value = val;
    return bs;
  }
};

struct ostream_params {
  int oflag = file::WRONLY | file::CREATE | file::TRUNC;
  size_t buffer_size = BUFSIZ > 32768 ? BUFSIZ : 32768;

  ostream_params() {}

  template <typename... T>
  ostream_params(T... params, int new_oflag) : ostream_params(params...) {
    oflag = new_oflag;
  }

  template <typename... T>
  ostream_params(T... params, detail::buffer_size bs)
      : ostream_params(params...) {
    this->buffer_size = bs.value;
  }

// Intel has a bug that results in failure to deduce a constructor
// for empty parameter packs.
#  if defined(__INTEL_COMPILER) && __INTEL_COMPILER < 2000
  ostream_params(int new_oflag) : oflag(new_oflag) {}
  ostream_params(detail::buffer_size bs) : buffer_size(bs.value) {}
#  endif
};

FMT_END_DETAIL_NAMESPACE

// Added {} below to work around default constructor error known to
// occur in Xcode versions 7.2.1 and 8.2.1.
constexpr detail::buffer_size buffer_size{};

/** A fast output stream which is not thread-safe. */
class FMT_API ostream final : private detail::buffer<char> {
 private:
  file file_;

  void grow(size_t) override;

  ostream(cstring_view path, const detail::ostream_params& params)
      : file_(path, params.oflag) {
    set(new char[params.buffer_size], params.buffer_size);
  }

 public:
  ostream(ostream&& other)
      : detail::buffer<char>(other.data(), other.size(), other.capacity()),
        file_(std::move(other.file_)) {
    other.clear();
    other.set(nullptr, 0);
  }
  ~ostream() {
    flush();
    delete[] data();
  }

  void flush() {
    if (size() == 0) return;
    file_.write(data(), size());
    clear();
  }

  template <typename... T>
  friend ostream output_file(cstring_view path, T... params);

  void close() {
    flush();
    file_.close();
  }

  /**
    Formats ``args`` according to specifications in ``fmt`` and writes the
    output to the file.
   */
  template <typename... T> void print(format_string<T...> fmt, T&&... args) {
    vformat_to(detail::buffer_appender<char>(*this), fmt,
               fmt::make_format_args(args...));
  }
};

/**
  \rst
  Opens a file for writing. Supported parameters passed in *params*:

  * ``<integer>``: Flags passed to `open
    <https://pubs.opengroup.org/onlinepubs/007904875/functions/open.html>`_
    (``file::WRONLY | file::CREATE`` by default)
  * ``buffer_size=<integer>``: Output buffer size

  **Example**::

    auto out = fmt::output_file("guide.txt");
    out.print("Don't {}", "Panic");
  \endrst
 */
template <typename... T>
inline ostream output_file(cstring_view path, T... params) {
  return {path, detail::ostream_params(params...)};
}

FMT_MODULE_EXPORT_END
FMT_END_NAMESPACE

#endif  // FMT_OS_H_
