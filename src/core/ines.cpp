/**
 * @file
 *
 * Implements iNES format routines
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

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <fstream>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/config.hpp>
#include <vpnes/core/ines.hpp>

using namespace ::std;
using namespace ::vpnes;
using namespace ::vpnes::core;
using namespace ::vpnes::core::ines;

/* SNESData */

/**
 * Constructs the structure from the data
 * @param ROM
 */
SNESData::SNESData(ifstream &ROM) : PRG(), CHR(), Trainer() {
	const char *iNESSignature = "NES\32";
	uint8_t mapper;
	iNES_Header header;

	ROM.read((char *) &header, sizeof(header));
	if (strncmp(header.Signature, iNESSignature, 4)) {
		throw invalid_argument("Unknown file format");
	}
	if ((header.Flags_ex & 0x0c) == 0x08) {
		throw invalid_argument("Wrong version");
	}
	PRGSize = header.PRGSize * 0x4000;
	CHRSize = header.CHRSize * 0x2000;
	switch (header.Flags) {
	case 0x01:
		Mirroring = MirorringVertical;
		break;
	case 0x08:
		Mirroring = MirorringSingleScreen1;
		break;
	case 0x09:
		Mirroring = MirorringSingleScreen2;
		break;
	case 0x18:
		Mirroring = MirorringFourScreens;
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
		ROM.read((char *) Trainer.data(), 0x0200 * sizeof(uint8_t));
	}
	PRG.resize(PRGSize);
	ROM.read((char *) PRG.data(), PRGSize * sizeof(uint8_t));
	if (CHRSize) {
		CHR.resize(CHRSize);
		ROM.read((char *) CHR.data(), CHRSize * sizeof(uint8_t));
	}
	switch (mapper) {
	case 0:
		MMCType = MMCNROM;
		break;
	default:
		throw invalid_argument("Unsupported mapper");
	}
}
