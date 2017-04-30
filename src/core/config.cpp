/**
 * @file
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

#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>
#include <fstream>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/config.hpp>
#include <vpnes/core/ines.hpp>
#include <vpnes/core/factory.hpp>

using namespace ::std;
using namespace ::vpnes;
using namespace ::vpnes::gui;
using namespace ::vpnes::core;

/**
 * Factory list
 */
const NESFactory factoryList[MMCAmount] = {
    &factory::factoryNROM,  // NROM-128
    &factory::factoryNROM   // NROM-256
};

/* SNESConfig */

/**
 * Default constructor
 */
SNESConfig::SNESConfig()
    : CHR()
    , PRG()
    , Trainer()
    , PRGSize()
    , CHRSize()
    , RAMSize()
    , MMCType()
    , Mirroring()
    , NESType() {
}

/**
 * Configures the class
 *
 * @param appConfig Application configuration
 * @param inputFile Input file stream
 */
void SNESConfig::configure(
    const SApplicationConfig &appConfig, ifstream &inputFile) {
	ines::SNESData nesData(inputFile);
	PRGSize = nesData.PRGSize;
	CHRSize = nesData.CHRSize;
	RAMSize = nesData.RAMSize;
	Mirroring = nesData.Mirroring;
	MMCType = nesData.MMCType;
	NESType = nesData.NESType;
	PRG = std::move(nesData.PRG);
	CHR = std::move(nesData.CHR);
	Trainer = std::move(nesData.Trainer);
}

/**
 * Creates an instance of NES
 *
 * @param frontEnd Front-end
 * @return Instance of NES
 */
CNES *SNESConfig::createInstance(CFrontEnd &frontEnd) {
	assert(MMCType < MMCAmount);
	return factoryList[MMCType](*this, frontEnd);
}
