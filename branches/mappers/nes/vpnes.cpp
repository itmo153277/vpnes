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

#include "vpnes.h"
#include "ines.h"
#include "mapper.h"

using namespace vpnes;

/* Открыть картридж */
CBasicNES *vpnes::OpenROM(std::istream &ROM, CallbackFunc CallBack) {
	ines::NES_ROM_Data Data;

	if (ReadROM(ROM, &Data) < 0)
		return NULL;
	switch (Data.Header.Mapper) {
		case 0:
			CNES_NROM *NROM_NES;

			/* Возвращаем стандартный NES на 0-маппере */
			NROM_NES = new CNES_NROM(CallBack);
			NROM_NES->GetBus().GetDeviceList()[CNES_NROM::BusClass::ROM] =
				new CNROM<CNES_NROM::BusClass>(&NROM_NES->GetBus(), &Data);
			return NROM_NES;
		case 2:
			CNES_UxROM *UxROM_NES;

			/* Возвращаем стандартный NES на UxROM */
			UxROM_NES = new CNES_UxROM(CallBack);
			UxROM_NES->GetBus().GetDeviceList()[CNES_UxROM::BusClass::ROM] =
				new CUxROM<CNES_UxROM::BusClass>(&UxROM_NES->GetBus(), &Data);
			return UxROM_NES;
	}
	delete [] Data.PRG;
	delete [] Data.CHR;
	delete [] Data.Trainer;
	return NULL;
}
