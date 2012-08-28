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

/* Параметры стандартного NES */
struct NTSC_Oscillator {
	static inline const double GetFreq() { return 44.0 / 945000.0; }
};

template <template<template<class> class,
                   template<class> class,
                   template<class> class,
                   template<class> class,
                   template<class> class> class _Bus,
          template<class> class _ROM>
struct NTSC_NES_Config {
	typedef CNESConfigTemplate<CNES<_Bus<StdClock<NTSC_Oscillator>::CClock,
		StdCPU<12>::CPU, StdAPU<apu::NTSC_Tables>::APU, StdPPU<4>::PPU, _ROM> >,
		256, 224> Config;
};

typedef NTSC_NES_Config<CBus, CNROM>::Config NROM_NES_Config;
typedef NTSC_NES_Config<CBus, CMMC1>::Config MMC1_NES_Config;
typedef NTSC_NES_Config<CBus, CUxROM>::Config UxROM_NES_Config;
typedef NTSC_NES_Config<CBus, CMMC3>::Config MMC3_NES_Config;
typedef NTSC_NES_Config<CBus, CAxROM>::Config AxROM_NES_Config;

/* Открыть картридж */
CNESConfig *vpnes::OpenROM(istream &ROM, ines::NES_ROM_Data *Data) {
	if (ReadROM(ROM, Data) < 0)
		return NULL;
	switch (Data->Header.Mapper) {
		case 0:
			return new NROM_NES_Config(Data);
		case 1:
			return new MMC1_NES_Config(Data);
		case 2:
			return new UxROM_NES_Config(Data);
		case 4:
			return new MMC3_NES_Config(Data);
		case 7:
			return new AxROM_NES_Config(Data);
	}
	FreeROMData(Data);
	return NULL;
}
