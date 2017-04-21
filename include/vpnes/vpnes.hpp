/**
 * @file
 *
 * Basic macros and types
 */
/*
 NES Emulator
 Copyright (C) 2012-2017  Ivanov Viktor

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

 */

#ifndef VPNES_INCLUDE_VPNES_HPP_
#define VPNES_INCLUDE_VPNES_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <utility>

#ifndef PACKAGE_STRING
#define PACKAGE_STRING "vpnes core"
#endif

#ifdef BUILDNUM
#define PACKAGE_BUILD PACKAGE_STRING " Build " BUILDNUM " " __DATE__ " " __TIME__
#else
#define PACKAGE_BUILD PACKAGE_STRING " Build " __DATE__ " " __TIME__
#endif

namespace vpnes {

/**
 * Condition and
 */
template<typename ... Conds>
struct cond_and: std::true_type {
};

/**
 * Condition and recursive definition
 */
template<typename Cond, typename ... Conds>
struct cond_and<Cond, Conds...> : std::conditional<Cond::value,
		cond_and<Conds...>, std::false_type>::type {
};

}

#endif /* VPNES_INCLUDE_VPNES_H_ */
