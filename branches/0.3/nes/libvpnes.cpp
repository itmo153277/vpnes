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

#include "libvpnes.h"
#include "clock.h"
#include "cpu.h"
#include "apu.h"
#include "ppu.h"
#include "mapper.h"


using namespace std;
using namespace vpnes;

typedef _NES_Config<CBus<CClock, CCPU, CAPU, CPPU, CNROM> >::Config NROM_NES_Config;

/* Открыть картридж */
CNESConfig *vpnes::OpenROM(istream &ROM, ines::NES_ROM_Data *Data) {
	if (ReadROM(ROM, Data) < 0)
		return NULL;
	return new NROM_NES_Config(Data);
}
