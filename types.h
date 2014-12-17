/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2013  Ivanov Viktor

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

\****************************************************************************/

#ifndef __TYPES_H_
#define __TYPES_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Настройки для файлов */

#ifdef _WIN32
#ifndef _WIN32_IE
#define _WIN32_IE 0x0500
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#endif

#ifndef VPNES_MAX_PATH
#ifdef MAX_PATH
#define VPNES_MAX_PATH MAX_PATH
#else
#define VPNES_MAX_PATH 256
#endif
#endif

#ifndef VPNES_PATH
#ifdef VPNES_PATH_COPY
#undef VPNES_PATH_COPY
#endif
#ifdef VPNES_PATH_CONVERT
#undef VPNES_PATH_CONVERT
#endif
#ifdef VPNES_PATH_FREE
#undef VPNES_PATH_FREE
#endif
#ifdef VPNES_PATH_CONVERTB
#undef VPNES_PATH_CONVERTB
#endif
#ifdef VPNES_PATH_FREEB
#undef VPNES_PATH_FREEB
#endif
#ifdef VPNES_PATH_ISTREAM
#undef VPNES_PATH_ISTREAN
#endif
#ifdef VPNES_PATH_OSTREAM
#undef VPNES_PATH_OSTREAN
#endif
#ifdef VPNES_PATH_IOSTREAM
#undef VPNES_PATH_IOSTREAN
#endif
#ifdef _WIN32
#define VPNES_PATH WCHAR
#define VPNES_PATH_COPY wcsncpy
#define VPNES_PATH_CONVERT(arg) ((WCHAR *) arg)
#define VPNES_PATH_FREE(arg) do {} while (0)
#define VPNES_PATH_CONVERTB(arg) ((wchar_t *) arg)
#define VPNES_PATH_FREEB(arg) do {} while (0)
#define VPNES_PATH_ISTREAM ::vpnes::ifstream_win32
#define VPNES_PATH_OSTREAM ::vpnes::ofstream_win32
#define VPNES_PATH_IOSTREAM ::vpnes::fstream_win32
#else
#define VPNES_PATH char
#define VPNES_PATH_COPY strncpy
#define VPNES_PATH_CONVERT(arg) ::vpnes::ConvertToWChar(arg)
#define VPNES_PATH_FREE(arg) delete [] (arg)
#define VPNES_PATH_CONVERTB(arg) ::vpnes::ConvertFromWChar(arg)
#define VPNES_PATH_FREEB(arg) delete [] (arg)
#define VPNES_PATH_ISTREAM ::std::ifstream
#define VPNES_PATH_OSTREAM ::std::ofstream
#define VPNES_PATH_IOSTREAM ::std::fstream
#endif
#endif

#ifdef __cplusplus

#include <exception>

/* Стандартный класс для исключений */
class CGenericException: public std::exception {
private:
	const char *str;
public:
	CGenericException(const char *Msg) throw(): str(Msg) {}
	CGenericException(const CGenericException& Copy) throw(): str(Copy.str) {}
	const char * what() const throw() { return str; }
};

#include <cstdlib>
#include <cwchar>

#ifdef _WIN32
#include <iostream>
#endif

namespace vpnes {

/* Операции с wchar_t */

wchar_t *ConvertToWChar(const char *Text);
char *ConvertFromWChar(const wchar_t *Text);

/* Классы потоков для Windows */

#ifdef _WIN32

/* Простейшая реализаций win32_filebuf */
class win32_filebuf: public std::streambuf {
private:
	HANDLE fd;
	char *buf;
	std::streamsize maxlen;
	bool opened;
protected:
	int_type overflow(int_type c = EOF) {
		DWORD n = pptr() - pbase(), w;

		if ((n > 0) && (!::WriteFile(fd, buf, n, &w, NULL) || w < n))
			return EOF;
		pbump(-static_cast<int>(n));
		if (c != EOF) {
			*(pptr()) = c;
			pbump(1);
		}
		return 0;
	}
	int_type pbackfail(int_type c = EOF) {
		if (gptr() == egptr())
			return EOF;
		if (c != EOF)
			*(gptr()) = c;
		gbump(-1);
		return 0;
	}
	int_type sync() {
		ssize_t n = pptr() - pbase();

		return (n > 0) ? overflow() : 0;
	}
	int_type underflow() {
		DWORD n;

		if (!::ReadFile(fd, buf, maxlen, &n, NULL))
			return EOF;
		if (n < 1)
			return EOF;
		setg(buf, buf, buf + n);
		return traits_type::to_int_type(*buf);
	}
	std::streamsize xsgetn(char *s, std::streamsize n) {
		std::streamsize l, k;
		char *p = s;

		for (l = n; l > 0; l -= k, p += k, gbump(k)) {
			if ((gptr() == egptr()) && (underflow() == EOF))
				return n - l;
			k = egptr() - gptr();
			if (l < k)
				k = l;
			memcpy(p, gptr(), k);
		}
		return n;
	}
	std::streamsize xsputn(const char *s, std::streamsize n) {
		std::streamsize l, k;
		const char *p = s;
		for (l = n; l > 0; l -= k, p += k, pbump(k)) {
			if ((pptr() == epptr()) && (overflow() == EOF))
				return n - l;
			k = epptr() - pptr();
			if (l < k)
				k = l;
			memcpy(pptr(), p, k);
		}
		return n;
	}
	std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way,
		std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) {
		LONG high = off >> 32;
		DWORD low = static_cast<DWORD>(off), flag;

		switch (way) {
			case std::ios_base::beg:
				flag = FILE_BEGIN;
				break;
			case std::ios_base::cur:
				flag = FILE_CURRENT;
				break;
			case std::ios_base::end:
				flag = FILE_END;
				break;
			default:
				flag = 0;
		}
		sync();
		low = ::SetFilePointer(fd, low, &high, flag);
		if (low == INVALID_SET_FILE_POINTER)
			return -1;
		return low || (static_cast<std::streampos>(high) << 32);
	}
	std::streampos seekpos(std::streampos sp,
		std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) {
		LONG high = sp >> 32;
		DWORD low = static_cast<DWORD>(sp);

		sync();
		low = ::SetFilePointer(fd, low, &high, FILE_BEGIN);
		if (low == INVALID_SET_FILE_POINTER)
			return -1;
		return low || (static_cast<std::streampos>(high) << 32);
	}
public:
	win32_filebuf(std::streamsize bufsize = 4096) {
		maxlen = bufsize;
		fd = INVALID_HANDLE_VALUE;
		buf = new char[maxlen];
		opened = false;
	}
	~win32_filebuf() {
		if (opened) {
			sync();
			::CloseHandle(fd);
		}
		delete [] buf;
	}
	int open(wchar_t *name, std::ios_base::openmode mode) {
		DWORD access = 0;
		DWORD share = 0;
		DWORD create = OPEN_EXISTING;

		if (opened)
			return -1;
		if (mode & std::ios_base::in)
			access |= GENERIC_READ;
		if (mode & std::ios_base::out)
			access |= GENERIC_WRITE;
		if (mode & std::ios_base::trunc)
			create = CREATE_ALWAYS;
		if (mode & std::ios_base::in && !(mode & std::ios_base::out))
			share = FILE_SHARE_READ;
		fd = CreateFileW(name, access, share, NULL, create, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fd == INVALID_HANDLE_VALUE)
			return -1;
		opened = true;
		setg(buf, buf, buf);
		setp(buf, buf + maxlen);
		return 0;
	}
	void close() {
		if (!opened)
			return;
		sync();
		CloseHandle(fd);
		opened = false;
	}
	bool is_open() const { return opened; }
};

class ifstream_win32: public std::istream, public win32_filebuf {
public:
	ifstream_win32(): std::istream(this) {}
	void open(wchar_t *name, std::ios_base::openmode mode) {
		if (win32_filebuf::open(name, mode) >= 0)
			clear();
		else
			setstate(std::ios_base::failbit);
	}
};

class ofstream_win32: public std::ostream, public win32_filebuf {
public:
	ofstream_win32(): std::ostream(this) {}
	void open(wchar_t *name, std::ios_base::openmode mode) {
		if (win32_filebuf::open(name, mode) >= 0)
			clear();
		else
			setstate(std::ios_base::failbit);
	}
};

class fstream_win32: public std::iostream, public win32_filebuf {
public:
	fstream_win32(): std::iostream(this) {}
	void open(wchar_t *name, std::ios_base::openmode mode) {
		if (win32_filebuf::open(name, mode) >= 0)
			clear();
		else
			setstate(std::ios_base::failbit);
	}
};

#endif

}

extern "C" {
#endif

/* Макрос строки */
#define MSTR(str) #str

/* swnprintf */

#ifndef HAVE_SNWPRINTF
#ifdef NORMAL_SWPRINTF
#define snwprintf(str, num, format, param) swprintf(str, num, format, param)
#else
#ifdef HAVE_SWPRINTF_S
#define snwprintf(str, num, format, param) swprintf_s(str, num, format, param)
#else
#define snwprintf(str, num, format, param) swprintf(str, format, param)
#endif
#endif
#endif

/* Основные типы данных */

#if !defined(HAVE_STDINT_H) && !defined(HAVE_INTTYPES_H)

typedef unsigned char uint8;
typedef signed char sint8;
typedef unsigned short int uint16;
typedef signed short int sint16;
typedef unsigned int uint32;
typedef signed int sint32;
typedef unsigned long long uint64;
typedef signed long long sint64;

#else

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

typedef uint8_t uint8;
typedef int8_t sint8;
typedef uint16_t uint16;
typedef int16_t sint16;
typedef uint32_t uint32;
typedef int32_t sint32;
typedef uint64_t uint64;
typedef int64_t sint64;

#endif

#ifdef __cplusplus
}
#endif

#endif
