/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2014  Ivanov Viktor

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

#include <cstring>
#include "ines.h"
#include "mappers/list.h"

using namespace vpnes;
using namespace ines;
using namespace mappers;

int vpnes::ReadROM(std::istream &ROM, NES_ROM_Data *Data, NES_Type PerfferedType) {
	const char *NesHeader = "NES\032";
	iNES_Header Header;

	ROM.read((char *) &Header, sizeof(Header));
	if (strncmp(Header.Signature, NesHeader, 4))
		return -1;
	if ((Header.Flags_ex & 0x0c) == 0x08) /* iNES 2.0 */
		return -1;
	Data->Header.PRGSize = Header.PRGSize * 0x4000;
	Data->Header.CHRSize = Header.CHRSize * 0x2000;
	Data->Header.Mirroring = (SolderPad) (Header.Flags & 0x09);
	if (Header.Flags & 0x02)
		Data->Header.BatterySize = 0x2000;
	Data->Header.Mapper = Header.Flags >> 4;
	if (Header.BadROM) {
		Data->Header.TVSystem = 0;
		Data->Header.RAMSize = 0x2000;
	} else {
		Data->Header.Mapper |= Header.Flags_ex & 0xf0;
		Data->Header.TVSystem = Header.TV_system;
		if (Header.RAMSize == 0)
			Data->Header.RAMSize = 0x2000;
		else
			Data->Header.RAMSize = Header.RAMSize * 0x2000;
		if ((Header.RAMSize == 0) && (Header.Flags_unofficial & 0x10))
			Data->Header.RAMSize = 0;
		/* Overrule */
		if ((~Header.Flags_unofficial & 0x01) && (Header.Flags_unofficial & 0x02))
			Data->Header.TVSystem = (Header.Flags_unofficial & 0x02);
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
	Data->Parameters.Mapper = MapperIDList[Data->Header.Mapper];
	//Db::FixRomData(Data);
	if (PerfferedType == NES_Auto) {
		if (Data->Header.TVSystem)
			Data->Parameters.Type = NES_PAL;
		else
			Data->Parameters.Type = NES_NTSC;
	} else
		Data->Parameters.Type = PerfferedType;
	return 0;
}

/* Освободить память */
void vpnes::FreeROMData(ines::NES_ROM_Data *Data) {
	delete [] Data->PRG;
	Data->PRG = NULL;
	delete [] Data->CHR;
	Data->CHR = NULL;
	delete [] Data->Trainer;
	Data->Trainer = NULL;
}
