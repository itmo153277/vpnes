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

#include "mapper.h"

#include <cstring>

using namespace vpnes;

/* Открыть картридж */
CBasicNES *OpenROM(std::istream &ROM, clock::CallbackFunc CallBack) {
	const char NesHead[] = "NES\032";
	char Buf[4];
	uint8 flags1, flags2, mapper;

	ROM.seekg(0, std::ios_base::beg);
	ROM.read(Buf, 4 * sizeof(char));
	if (strncmp(Buf, NesHead, 4))
		return NULL;
	ROM.seekg(6, std::ios_base::beg);
	ROM.read((char *) &flags1, sizeof(uint8));
	ROM.read((char *) &flags2, sizeof(uint8));
	mapper = (flags1 >> 4) | (flags2 & 0xf0);
	switch (mapper) {
		case 0:
			CNES_NROM *NROM_NES;

			/* Возвращаем стандартный NES на 0-маппере */
			NROM_NES = new CNES_NROM(CallBack);
			NROM_NES->GetBus().GetDeviceList()[CNES_NROM::BusClass::ROM] =
				new CNROM<CNES_NROM::BusClass>(&NROM_NES->GetBus(), ROM);
			return NROM_NES;
		case 2:
			CNES_UxROM *UxROM_NES;

			/* Возвращаем стандартный NES UxROM */
			UxROM_NES = new CNES_UxROM(CallBack);
			UxROM_NES->GetBus().GetDeviceList()[CNES_UxROM::BusClass::ROM] =
				new CUxROM<CNES_UxROM::BusClass>(&UxROM_NES->GetBus(), ROM);
			return UxROM_NES;
		case 7:
			CNES_AxROM *AxROM_NES;

			/* Возвращаем стандартный NES AxROM */
			AxROM_NES = new CNES_AxROM(CallBack);
			AxROM_NES->GetBus().GetDeviceList()[CNES_AxROM::BusClass::ROM] =
				new CAxROM<CNES_AxROM::BusClass>(&AxROM_NES->GetBus(), ROM);
			return AxROM_NES;
	}
	return NULL;
}
