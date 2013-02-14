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

#ifdef __cplusplus

#include <exception>

/* Стандартный класс для исключений */
class CGenericException: std::exception {
private:
	const char *str;
public:
	CGenericException(const char *Msg) throw(): str(Msg) {}
	CGenericException(const CGenericException& Copy) throw(): str(Copy.str) {}
	const char * what() const throw() { return str; }
};

extern "C" {
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
