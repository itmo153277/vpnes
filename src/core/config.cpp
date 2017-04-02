/*
 * config.cpp
 *
 * Implements NES configure
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

#include <fstream>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/config.hpp>

using namespace ::std;
using namespace ::vpnes;
using namespace ::vpnes::gui;
using namespace ::vpnes::core;

/* SNESConfig */

/**
 * Configures the class
 *
 * @param appConfig Application configuration
 * @param inputFile Input file stream
 */
void SNESConfig::configure(const SApplicationConfig &appConfig,
		ifstream &inputFile) {
}

/**
 * Creates an instance of NES
 *
 * @return Instance of NES
 */
CNES SNESConfig::createInstance() {
	return CNES();
}
