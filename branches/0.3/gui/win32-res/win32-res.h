/****************************************************************************\

	NES Emulator
	Copyright (C) 2012  Ivanov Viktor

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

#ifndef __WIN32_RES_H_
#define __WIN32_RES_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <windows.h>

#define IDI_MAINICON         101
#if defined(VPNES_INTERACTIVE)
#define IDR_MAINMENU         101
#define IDD_ABOUTDIALOG      101
#define ID_FILE_OPEN         40001
#define ID_FILE_CLOSE        40002
#define ID_FILE_SETTINGS     40003
#define ID_FILE_EXIT         40004
#define ID_CPU_SOFTWARERESET 40005
#define ID_CPU_HARDWARERESET 40006
#define ID_CPU_SAVESTATE     40007
#define ID_CPU_LOADSTATE     40008
#define ID_CPU_ABOUT         40009
#define IDC_STATIC           -1
#define IDC_STATICINFO       101

#ifndef VPNES_VERSION
#ifdef VERSION
#define VPNES_VERSION VERSION
#else
#define VPNES_VERSION "0.3"
#endif
#endif

#ifndef VPNES_BUILD
#ifdef BUILDNUM
#ifdef SVNREV
#define VPNES_BUILD "\nBuild " BUILDNUM " (" SVNREV ")"
#else
#define VPNES_BUILD "\nBuild " BUILDNUM
#endif
#else
#define VPNES_BUILD ""
#endif
#endif

#endif

#endif
