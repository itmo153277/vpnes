/**
 * @file
 *
 * Implements configuration parsing
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

#include <cassert>
#include <cstring>
#include <iostream>
#include <vpnes/gui/config.hpp>

namespace vpnes {

namespace gui {

/* SApplicationConfig */

/**
 * Sets default values
 */
SApplicationConfig::SApplicationConfig() : inputFile() {
}

/**
 * Parses command line options
 *
 * @param argc Amount of parameters
 * @param argv Array of parameters
 */
void SApplicationConfig::parseOptions(int argc, char **argv) {
	// TODO: Implement parsing
	if (argc >= 2) {
		setInputFile(argv[1]);
	}
}

/**
 * Opens input file and constructs ifstream object
 *
 * @return std::ifstream for reading file
 */
std::ifstream SApplicationConfig::getInputFile() {
	assert(inputFile.size() > 0);
	std::ifstream file;
	file.exceptions(file.exceptions() | std::fstream::failbit);
	file.open(inputFile, std::fstream::binary);
	return file;
}

}  // namespace gui

}  // namespace vpnes
