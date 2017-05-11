/**
 * @file
 *
 * Defines NROM class and factories
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
#include <vector>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/config.hpp>
#include <vpnes/core/factory.hpp>
#include <vpnes/core/mappers/helper.hpp>
#include <vpnes/core/mappers/nrom.hpp>

namespace vpnes {

namespace core {

namespace factory {

/**
 * NROM NES factory
 *
 * @param config NES config
 * @param frontEnd Front-end
 * @return NES
 */
CNES *factoryNROM(SNESConfig &config, CFrontEnd &frontEnd) {
	return factoryNES<CNROM>(config, frontEnd);
}

}  // namespace factory

/* CNROM */

/**
 * Constructs the object
 *
 * @param motherBoard Motherboard
 * @param config NES config
 */
CNROM::CNROM(CMotherBoard &motherBoard, const SNESConfig &config)
    : m_PRG(config.PRG)
    , m_CHR(config.CHR)
    , m_RAM(config.RAMSize)
    , m_Mirroring(config.Mirroring) {
	if (m_CHR.size() == 0) {
		m_CHR.assign(0x2000, 0);
		m_CHRBank = 1;
	} else {
		m_CHRBank = 0;
	}
	if ((m_PRG.size() != 0x4000 && m_PRG.size() != 0x8000) ||
	    (m_Mirroring != MirroringHorizontal &&
	        m_Mirroring != MirroringVertical) ||
	    m_CHR.size() != 0x2000 || m_RAM.size() > 0x2000 ||
	    (m_RAM.size() & 0x07ff)) {
		throw std::invalid_argument("Invalid ROM parameters");
	}
}

}  // namespace core

}  // namespace vpnes
