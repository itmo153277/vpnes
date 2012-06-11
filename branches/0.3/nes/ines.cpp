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

#include "ines.h"
#include <cstring>

using namespace vpnes;
using namespace ines;

int vpnes::ReadROM(std::istream &ROM, NES_ROM_Data *Data) {
	const char *NesHeader = "NES\032";
	iNES_Header Header;

	ROM.read((char *) &Header, sizeof(Header));
	if (strncmp(Header.Signature, NesHeader, 4))
		return -1;
	if ((Header.Flags_ex & 0x0c) == 0x08) /* iNES 2.0 */
		return -1;
	Data->Header.PRGSize = Header.PRGSize * 0x4000;
	Data->Header.CHRSize = Header.CHRSize * 0x2000;
	Data->Header.RAMSize = Header.RAMSize * 0x2000;
	Data->Header.Mirroring = (SolderPad) (Header.Flags & 0x09);
	Data->Header.HaveBattery = Header.Flags & 0x02;
	Data->Header.Mapper = Header.Flags >> 4;
	if (Header.BadROM)
		Data->Header.TVSystem = 0;
	else {
		Data->Header.Mapper |= Header.Flags_ex & 0xf0;
		Data->Header.TVSystem = Header.TV_system;
	}
	if (Header.Flags & 0x04) { /* Trainer */
		Data->Trainer = new uint8[0x0200];
		ROM.read((char *) Data->Trainer, 0x0200 * sizeof(uint8));
	} else
		Data->Trainer = NULL;
	/* PRG */
	Data->PRG = new uint8[Data->Header.PRGSize];
	ROM.read((char *) Data->PRG, Data->Header.PRGSize * sizeof(uint8));
	/* CHR */
	if (Header.CHRSize) {
		Data->CHR = new uint8[Data->Header.CHRSize];
		ROM.read((char *) Data->CHR, Data->Header.CHRSize * sizeof(uint8));
	} else { /* CHR RAM */
		Data->CHR = NULL;
		Data->Header.CHRSize = 0x2000;
	}
	if (ROM.fail()) {
		FreeROMData(Data);
		return -1;
	}
	return 0;
}
