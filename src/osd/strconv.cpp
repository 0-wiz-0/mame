// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  strconv.cpp - Win32 string conversion
//
//============================================================
#if defined(SDLMAME_WIN32) || defined(OSD_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#undef min
#undef max
#include <algorithm>
#include <assert.h>
// MAMEOS headers
#include "strconv.h"

#if defined(SDLMAME_WIN32) || defined(OSD_WINDOWS)

namespace
{
	// abstract base class designed to provide inputs to WideCharToMultiByte() and MultiByteToWideChar()
	template<typename T>
	class string_source
	{
	public:
		virtual const T *string() const = 0;	// returns pointer to actual characters
		virtual int char_count() const = 0;		// returns the character count (including NUL terminater), or -1 if NUL terminated
	};

	// implementation of string_source for NUL-terminated strings
	template<typename T>
	class null_terminated_string_source : public string_source<T>
	{
	public:
		null_terminated_string_source(const T *str) : m_str(str)
		{
			assert(str != nullptr);
		}

		virtual const T *string() const override { return m_str; }
		virtual int char_count() const override { return -1; }

	private:
		const T *m_str;
	};

	// implementation of string_source for std::[w]string
	template<typename T>
	class basic_string_source : public string_source<T>
	{
	public:
		basic_string_source(const std::basic_string<T> &str) : m_str(str)
		{
		}

		virtual const T *string() const override { return m_str.c_str(); }
		virtual int char_count() const override { return (int) m_str.size() + 1; }

	private:
		const std::basic_string<T> &m_str;
	};
};

//============================================================
//  mbstring_from_wstring
//============================================================

static std::string &mbstring_from_wstring(std::string &dst, UINT code_page, const string_source<wchar_t> &src)
{	
	// convert UTF-16 to the specified code page
	int dst_char_count = WideCharToMultiByte(code_page, 0, src.string(), src.char_count(), nullptr, 0, nullptr, nullptr);
	dst.resize(dst_char_count - 1);
	WideCharToMultiByte(code_page, 0, src.string(), src.char_count(), &dst[0], dst_char_count, nullptr, nullptr);

	return dst;
}


//============================================================
//  wstring_from_mbstring
//============================================================

static std::wstring &wstring_from_mbstring(std::wstring &dst, const string_source<char> &src, UINT code_page)
{
	// convert multibyte string (in specified code page) to UTF-16
	int dst_char_count = MultiByteToWideChar(code_page, 0, src.string(), src.char_count(), nullptr, 0);
	dst.resize(dst_char_count - 1);
	MultiByteToWideChar(CP_UTF8, 0, src.string(), src.char_count(), &dst[0], dst_char_count - 1);

	return dst;
}


//============================================================
//  astring_from_utf8
//============================================================

std::string &astring_from_utf8(std::string &dst, const std::string &s)
{
	// convert MAME string (UTF-8) to UTF-16
	std::wstring wstring = wstring_from_utf8(s);

	// convert UTF-16 to "ANSI code page" string
	return mbstring_from_wstring(dst, CP_ACP, basic_string_source<wchar_t>(wstring));
}



//============================================================
//  astring_from_utf8
//============================================================

std::string &astring_from_utf8(std::string &dst, const char *s)
{
	// convert MAME string (UTF-8) to UTF-16
	std::wstring wstring = wstring_from_utf8(s);

	// convert UTF-16 to "ANSI code page" string
	return mbstring_from_wstring(dst, CP_ACP, basic_string_source<wchar_t>(wstring));
}


//============================================================
//  astring_from_utf8
//============================================================

std::string astring_from_utf8(const std::string &s)
{
	std::string result;
	astring_from_utf8(result, s);
	return result;
}


//============================================================
//  astring_from_utf8
//============================================================

std::string astring_from_utf8(const char *s)
{
	std::string result;
	astring_from_utf8(result, s);
	return result;
}


//============================================================
//  utf8_from_astring
//============================================================

std::string &utf8_from_astring(std::string &dst, const std::string &s)
{
	// convert "ANSI code page" string to UTF-16
	std::wstring wstring;
	wstring_from_mbstring(wstring, basic_string_source<char>(s), CP_ACP);

	// convert UTF-16 to MAME string (UTF-8)
	return utf8_from_wstring(dst, wstring);
}


//============================================================
//  utf8_from_astring
//============================================================

std::string &utf8_from_astring(std::string &dst, const CHAR *s)
{
	// convert "ANSI code page" string to UTF-16
	std::wstring wstring;
	wstring_from_mbstring(wstring, null_terminated_string_source<char>(s), CP_ACP);

	// convert UTF-16 to MAME string (UTF-8)
	return utf8_from_wstring(dst, wstring);
}


//============================================================
//  utf8_from_astring
//============================================================

std::string utf8_from_astring(const std::string &s)
{
	std::string result;
	utf8_from_astring(result, s);
	return result;
}


//============================================================
//  utf8_from_astring
//============================================================

std::string utf8_from_astring(const CHAR *s)
{
	std::string result;
	utf8_from_astring(result, s);
	return result;
}


//============================================================
//  wstring_from_utf8
//============================================================

std::wstring &wstring_from_utf8(std::wstring &dst, const std::string &s)
{
	// convert MAME string (UTF-8) to UTF-16
	return wstring_from_mbstring(dst, basic_string_source<char>(s), CP_UTF8);
}


//============================================================
//  wstring_from_utf8
//============================================================

std::wstring &wstring_from_utf8(std::wstring &dst, const char *s)
{
	// convert MAME string (UTF-8) to UTF-16
	return wstring_from_mbstring(dst, null_terminated_string_source<char>(s), CP_UTF8);
}


//============================================================
//  wstring_from_utf8
//============================================================

std::wstring wstring_from_utf8(const std::string &s)
{
	std::wstring result;
	wstring_from_utf8(result, s);
	return result;
}


//============================================================
//  wstring_from_utf8
//============================================================

std::wstring wstring_from_utf8(const char *s)
{
	std::wstring result;
	wstring_from_utf8(result, s);
	return result;
}


//============================================================
//  utf8_from_wstring
//============================================================

std::string &utf8_from_wstring(std::string &dst, const std::wstring &s)
{
	// convert UTF-16 to MAME string (UTF-8)
	return mbstring_from_wstring(dst, CP_UTF8, basic_string_source<wchar_t>(s));
}


//============================================================
//  utf8_from_wstring
//============================================================

std::string &utf8_from_wstring(std::string &dst, const WCHAR *s)
{
	// convert UTF-16 to MAME string (UTF-8)
	return mbstring_from_wstring(dst, CP_UTF8, null_terminated_string_source<wchar_t>(s));
}


//============================================================
//  utf8_from_wstring
//============================================================

std::string utf8_from_wstring(const std::wstring &s)
{
	std::string result;
	utf8_from_wstring(result, s);
	return result;
}


//============================================================
//  utf8_from_wstring
//============================================================

std::string utf8_from_wstring(const WCHAR *s)
{
	std::string result;
	utf8_from_wstring(result, s);
	return result;
}


//============================================================
//  osd_uchar_from_osdchar
//============================================================

int osd_uchar_from_osdchar(UINT32 *uchar, const char *osdchar, size_t count)
{
	WCHAR wch;
	CPINFO cp;

	if (!GetCPInfo(CP_ACP, &cp))
		goto error;

	// The multibyte char can't be bigger than the max character size
	count = std::min(count, size_t(cp.MaxCharSize));

	if (MultiByteToWideChar(CP_ACP, 0, osdchar, static_cast<DWORD>(count), &wch, 1) == 0)
		goto error;

	*uchar = wch;
	return static_cast<int>(count);

error:
	*uchar = 0;
	return static_cast<int>(count);
}

#else
#include "unicode.h"
//============================================================
//  osd_uchar_from_osdchar
//============================================================

int osd_uchar_from_osdchar(unicode_char *uchar, const char *osdchar, size_t count)
{
	wchar_t wch;

	count = mbstowcs(&wch, (char *)osdchar, 1);
	if (count != -1)
		*uchar = wch;
	else
		*uchar = 0;

	return count;
}

#endif
