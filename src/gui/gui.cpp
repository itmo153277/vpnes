/**
 * Implements cross-platform console ui
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vpnes/vpnes.hpp>
#include <vpnes/gui/config.hpp>
#include <vpnes/gui/gui.hpp>
#include <vpnes/core/config.hpp>
#include <vpnes/core/nes.hpp>

using namespace ::std;
using namespace ::vpnes;
using namespace ::vpnes::gui;
using namespace ::vpnes::core;

/* CGUI */

/**
 * Starts GUI
 *
 * @param argc Amount of parameters
 * @param argv Array of parameters
 * @return Exit code
 */
int vpnes::gui::CGUI::startGUI(int argc, char **argv) {
	config.parseOptions(argc, argv);
	if (!config.hasInputFile()) {
		cerr << "Usage:" << endl;
		cerr << argv[0] << " path_to_rom.nes" << endl;
		return 0;
	}
	try {
		// expected transformation into move
		ifstream inputFile = config.getInputFile();
		SNESConfig nesConfig;
		nesConfig.configure(config, inputFile);
		CNES nes = nesConfig.createInstance();
		nes.powerUp();
	} catch (fstream::failure &e) {
		cerr << e.what() << endl;
	} catch (invalid_argument &e) {
		cerr << e.what() << endl;
	}
	return 0;
}
