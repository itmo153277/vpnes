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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_SDL
#include <SDL.h>
#endif

#include <clocale>
#include <iostream>
#include <vpnes/vpnes.hpp>
#include <vpnes/gui/gui.hpp>

/**
 * Entry point
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Exit code
 */
int main(int argc, char **argv) {
	/*
	 * Setting up output
	 */
	std::setlocale(LC_ALL, "");
	std::clog << std::internal << std::hex << std::showbase;
	std::clog << PACKAGE_BUILD << std::endl << std::endl;
	/*
	 * Startup
	 */
	vpnes::gui::CGUI gui;
	return gui.startGUI(argc, argv);
}
