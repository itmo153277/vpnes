/*
 * vpnes.h
 *
 * Basic macros and types
 */

#ifndef VPNES_INCLUDE_VPNES_HPP_
#define VPNES_INCLUDE_VPNES_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef PACKAGE_STRING
#define PACKAGE_STRING "vpnes core"
#endif

#ifdef BUILDNUM
#define PACKAGE_BUILD PACKAGE_STRING " Build " BUILDNUM " " __DATE__ " " __TIME__
#else
#define PACKAGE_BUILD PACKAGE_STRING " Build " __DATE__ " " __TIME__
#endif

#endif /* VPNES_INCLUDE_VPNES_H_ */
