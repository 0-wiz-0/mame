// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic
/***************************************************************************

    asio.cpp

    ASIO library implementation loader

***************************************************************************/

// Clang warnings with -Weverything
#ifdef __clang__
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif 

#define ASIO_STANDALONE
#define ASIO_SEPARATE_COMPILATION
#define ASIO_NOEXCEPT noexcept(true)
#define ASIO_NOEXCEPT_OR_NOTHROW noexcept(true)
#define ASIO_ERROR_CATEGORY_NOEXCEPT noexcept(true)

#include "asio/impl/src.hpp"
