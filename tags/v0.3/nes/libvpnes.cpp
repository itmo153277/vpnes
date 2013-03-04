/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2013  Ivanov Viktor

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

#include "libvpnes.h"
#include "nbuild.h"

using namespace std;
using namespace vpnes;

/* Открыть картридж */
CNESConfig *vpnes::OpenROM(istream &ROM, ines::NES_ROM_Data *Data, ines::NES_Type Type) {
	ines::NES_Type Cfg = Type;
	CNESConfig *Res = NULL;

	if (ReadROM(ROM, Data) < 0)
		return NULL;
	if (Type == ines::NES_Auto) {
		if (Data->Header.TVSystem)
			Cfg = ines::NES_PAL;
		else
			Cfg = ines::NES_NTSC;
	}
	switch (Data->Header.Mapper) {
		case 0:
			Res = NROM_Config(Data, Cfg);
			break;
		case 1:
			Res = MMC1_Config(Data, Cfg);
			break;
		case 2:
			Res = UxROM_Config(Data, Cfg);
			break;
		case 4:
			Res = MMC3_Config(Data, Cfg);
			break;
		case 7:
			Res = AxROM_Config(Data, Cfg);
			break;
	}
	if (Res == NULL)
		FreeROMData(Data);
	return Res;
}
