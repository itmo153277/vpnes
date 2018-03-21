/**
 * @file
 *
 * Basic macros and types
 */
/*
 NES Emulator
 Copyright (C) 2012-2018  Ivanov Viktor

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

#ifndef INCLUDE_VPNES_VPNES_HPP_
#define INCLUDE_VPNES_VPNES_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstddef>
#include <utility>
#include <type_traits>

#ifndef PACKAGE_STRING
#define PACKAGE_STRING "vpnes core"
#endif

#ifdef BUILDNUM
#define PACKAGE_BUILD \
	PACKAGE_STRING " Build " BUILDNUM " " __DATE__ " " __TIME__
#else
#define PACKAGE_BUILD PACKAGE_STRING " Build " __DATE__ " " __TIME__
#endif

namespace vpnes {

/**
 * Condition and
 */
template <typename... Conds>
struct cond_and : std::true_type {};

/**
 * Condition and recursive definition
 */
template <typename Cond, typename... Conds>
struct cond_and<Cond, Conds...>
    : std::conditional<Cond::value, cond_and<Conds...>, std::false_type>::type {
};

/**
 * Class pack helper
 */
template <typename... T>
struct class_pack {
	/**
	 * Actual type
	 */
	using type = class_pack;
	enum {
		size = sizeof...(T)  //!< Pack size
	};
};

/**
 * Merging class packs
 */
template <class ClassPack1, class ClassPack2>
struct merge_class_pack;

/**
 * Implementation for merging of class packs
 */
template <typename... T1, typename... T2>
struct merge_class_pack<class_pack<T1...>, class_pack<T2...>>
    : class_pack<T1..., T2...> {};

/**
 * Removes class from class pack
 */
template <typename T, class ClassPack>
struct remove_from_class_pack : ClassPack {};

/**
 * Implementation for removing class from class pack
 */
template <typename T, typename T1, typename... T2>
struct remove_from_class_pack<T, class_pack<T1, T2...>>
    : std::conditional<std::is_same<T, T1>::value, class_pack<T2...>,
          typename merge_class_pack<class_pack<T1>,
              typename remove_from_class_pack<T,
                  class_pack<T2...>>::type>::type>::type {};

/**
 * Creates distinct class pack from any class pack
 */
template <class ClassPack>
struct distinct_class_pack;

/**
 * Removing non-unique class from distinct class pack
 */
template <typename T1, typename... T2>
struct distinct_class_pack<class_pack<T1, T1, T2...>>
    : distinct_class_pack<class_pack<T1, T2...>>::type {};

/**
 * Removing unique class from distinct class pack
 */
template <typename T1, typename... T2>
struct distinct_class_pack<class_pack<T1, T2...>>
    : merge_class_pack<class_pack<T1>,
          typename distinct_class_pack<class_pack<T2...>>::type>::type {};

/**
 * Empty distinct class pack
 */
template <>
struct distinct_class_pack<class_pack<>> : class_pack<> {};

/**
 * Indices pack helpers
 */
template <std::size_t... I>
struct index_pack {
	/**
	 * Actual type
	 */
	using type = index_pack;
	enum {
		size = sizeof...(I)  //!< Pack size
	};
};

/**
 * Merging index pack
 */
template <class IndexPack1, class IndexPack2>
struct merge_index_pack;

/**
 * Implementation for merging index pack
 */
template <std::size_t... I1, std::size_t... I2>
struct merge_index_pack<index_pack<I1...>, index_pack<I2...>>
    : index_pack<I1..., (sizeof...(I1) + I2)...> {};

/**
 * Making index pack of given size
 */
template <std::size_t Size>
struct make_index_pack
    : merge_index_pack<typename make_index_pack<Size / 2>::type,
          typename make_index_pack<Size - Size / 2>::type> {};

/**
 * Empty index pack
 */
template <>
struct make_index_pack<0> : index_pack<> {};

/**
 * Index pack start
 */
template <>
struct make_index_pack<1> : index_pack<0> {};

}  // namespace vpnes

#endif  // INCLUDE_VPNES_VPNES_HPP_
