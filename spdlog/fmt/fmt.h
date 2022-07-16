//
// Copyright(c) 2016-2018 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

//
// Include a bundled header-only copy of fmtlib or an external one.
// By default spdlog include its own copy.
//

#if defined(SPDLOG_USE_STD_FORMAT) // SPDLOG_USE_STD_FORMAT is defined - use std::format
#    include <format>
#else
// enable the 'n' flag in for backward compatibility with fmt 6.x
#    include <spdlog/fmt/bundled/core.h>
#    include <spdlog/fmt/bundled/format.h>
#endif
