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
#include "gui/gui.h"

#include "types.h"

/* Точка входа в программу */
int main(int argc, char *argv[]) {
#ifdef BUILDNUM
	std::cerr << "VPNES " VERSION " Build " << BUILDNUM <<
#ifdef SVNREV
		" (" SVNREV ")" <<
#endif
		std::endl;
	std::cerr << "License: GPL v2" << std::endl;
#endif
	if (argc != 2)
		return 0;
	StartGUI(argv[1]);
	return 0;
}
