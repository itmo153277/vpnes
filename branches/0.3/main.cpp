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

#include <SDL.h>
#include <iostream>
#include "gui/window.h"
#include "gui/gui.h"

#ifdef VPNES_INTERACTIVE
#include "gui/interactive.h"
#endif

#include "types.h"

/* Точка входа в программу */
int main(int argc, char *argv[]) {
#ifdef BUILDNUM
	std::clog << "VPNES " VERSION " Build " BUILDNUM
#ifdef SVNREV
		" (" SVNREV ")"
#endif
		<< std::endl;
	std::clog << "License: GPL v2" << std::endl;
#endif
#ifdef VPNES_INTERACTIVE
	if (argc != 2)
		DisableInteractive = 0;
	if (InitMainWindow(512, 448) < 0)
		return 0;
	if (DisableInteractive && CanLog)
		StartGUI(argv[1]);
	else
		InteractiveGUI();
#else
	if (argc != 2)
		return 0;
	if (InitMainWindow(512, 448) < 0)
		return 0;
	StartGUI(argv[1]);
#endif
	AppQuit();
	return 0;
}
