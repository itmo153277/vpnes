/**
 * @file
 *
 * Implements iNES format routines
 */
/*
 NES Emulator
 Copyright (C) 2012-2018  Ivanov Viktor

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

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <fstream>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/config.hpp>
#include <vpnes/core/ines.hpp>

namespace vpnes {

namespace core {

namespace ines {

/* SNESData */

/**
 * Constructs the structure from the data
 *
 * @param ROM
 */
SNESData::SNESData(std::ifstream *ROM) : PRG(), CHR(), Trainer() {
	const char *iNESSignature = "NES\32";
	uint8_t mapper;
	iNES_Header header;

	ROM->read(reinterpret_cast<char *>(&header), sizeof(header));
	if (strncmp(header.Signature, iNESSignature, 4)) {
		throw std::invalid_argument("Unknown file format");
	}
	if ((header.Flags_ex & 0x0c) == 0x08) {
		throw std::invalid_argument("Wrong version");
	}
	PRGSize = header.PRGSize * 0x4000;
	CHRSize = header.CHRSize * 0x2000;
	switch (header.Flags & 0x09) {
	case 0x01:
		Mirroring = MirroringVertical;
		break;
	case 0x08:
		Mirroring = MirroringFourScreens;
		break;
	default:
		Mirroring = MirroringHorizontal;
	}
	BatterySize = (header.Flags & 0x02) ? 0x2000 : 0;
	mapper = header.Flags >> 4;
	if (header.BadROM) {
		NESType = NESTypeNTSC;
		RAMSize = 0;
	} else {
		mapper |= header.Flags_ex & 0xf0;
		switch (header.TV_system) {
		case 0x02:
			NESType = NESTypePAL;
			break;
		default:
			NESType = NESTypeNTSC;
		}
		RAMSize = (header.RAMSize == 0 && !(header.Flags_unofficial & 0x10))
		              ? 0x2000
		              : header.RAMSize * 0x2000;
	}
	if (header.Flags & 0x04) {
		Trainer.resize(0x0200);
		ROM->read(
		    reinterpret_cast<char *>(Trainer.data()), 0x0200 * sizeof(uint8_t));
	}
	PRG.resize(PRGSize);
	ROM->read(reinterpret_cast<char *>(PRG.data()), PRGSize * sizeof(uint8_t));
	if (CHRSize) {
		CHR.resize(CHRSize);
		ROM->read(
		    reinterpret_cast<char *>(CHR.data()), CHRSize * sizeof(uint8_t));
	}
	switch (mapper) {
	case 0:
		if (PRGSize > 0x4000) {
			MMCType = MMCNROM256;
		} else {
			MMCType = MMCNROM128;
		}
		break;
	default:
		throw std::invalid_argument("Unsupported mapper");
	}
}

}  // namespace ines

}  // namespace core

}  // namespace vpnes
