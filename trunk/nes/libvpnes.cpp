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

/* NTSC NES */
struct NTSC_Settings {
	/* Таблицы для APU */
	typedef apu::NTSC_Tables Tables;
	/* Частота */
	static inline const double GetFreq() { return 44.0 / 945000.0; }
	/* Делители частоты */
	enum {
		CPU_Divider = 12,
		PPU_Divider = 4
	};
	/* Параметры экрана */
	enum {
		Top = 8,
		Left = 0,
		Right = 256,
		Bottom = 232
	};
	/* Параметры сканлайнов */
	enum {
		ActiveScanlines = 240, /* Продолжительность картинки */
		PostRender = 1, /* Продолжительность между картинкой и VBlank */
		VBlank = 20, /* Продолжительность VBlank */
		OddSkip = 1 /* Число пропускаемых тактов на нечетных фреймах */
	};
	/* Обработка 337 такта на пре-сканлайне */
	static inline void SkipPPUClock(int &Clocks) { Clocks++; }
};

template <template<template<class> class,
                   template<class> class,
                   template<class> class,
                   template<class> class,
                   template<class> class> class _Bus,
          template<class> class _ROM>
struct NTSC_NES_Config {
	typedef CNESConfigTemplate<CNES<_Bus<StdClock<NTSC_Settings>::CClock,
		StdCPU<NTSC_Settings>::CPU, StdAPU<NTSC_Settings>::APU,
		StdPPU<NTSC_Settings>::PPU, _ROM> >, NTSC_Settings> Config;
};

/* PAL NES */
struct PAL_Settings {
	/* Таблицы для APU */
	typedef apu::PAL_Tables Tables;
	/* Частота */
	static inline const double GetFreq() { return 1.0 / 26601.7125; }
	/* Делители частоты */
	enum {
		CPU_Divider = 16,
		PPU_Divider = 5
	};
	/* Параметры экрана */
	enum {
		Top = 0,
		Left = 0,
		Right = 256,
		Bottom = 240
	};
	/* Параметры сканлайнов */
	enum {
		ActiveScanlines = 240, /* Продолжительность картинки */
		PostRender = 1, /* Продолжительность между картинкой и VBlank */
		VBlank = 70, /* Продолжительность VBlank */
		OddSkip = 0 /* Число пропускаемых тактов на нечетных фреймах */
	};
	/* Обработка 337 такта на пре-сканлайне */
	static inline void SkipPPUClock(int &Clocks) { }
};

template <template<template<class> class,
                   template<class> class,
                   template<class> class,
                   template<class> class,
                   template<class> class> class _Bus,
          template<class> class _ROM>
struct PAL_NES_Config {
	typedef CNESConfigTemplate<CNES<_Bus<StdClock<PAL_Settings>::CClock,
		StdCPU<PAL_Settings>::CPU, StdAPU<PAL_Settings>::APU,
		StdPPU<PAL_Settings>::PPU, _ROM> >, PAL_Settings> Config;
};

/* Dendy */
struct Dendy_Settings {
	/* Таблицы для APU */
	typedef apu::NTSC_Tables Tables;
	/* Частота */
	static inline const double GetFreq() { return 1.0 / 26601.7125; }
	/* Делители частоты */
	enum {
		CPU_Divider = 15,
		PPU_Divider = 5
	};
	/* Параметры экрана */
	enum {
		Top = 0,
		Left = 0,
		Right = 256,
		Bottom = 240
	};
	/* Параметры сканлайнов */
	enum {
		ActiveScanlines = 240, /* Продолжительность картинки */
		PostRender = 51, /* Продолжительность между картинкой и VBlank */
		VBlank = 20, /* Продолжительность VBlank */
		OddSkip = 0 /* Число пропускаемых тактов на нечетных фреймах */
	};
	/* Обработка 337 такта на пре-сканлайне */
	static inline void SkipPPUClock(int &Clocks) { }
};

template <template<template<class> class,
                   template<class> class,
                   template<class> class,
                   template<class> class,
                   template<class> class> class _Bus,
          template<class> class _ROM>
struct Dendy_NES_Config {
	typedef CNESConfigTemplate<CNES<_Bus<StdClock<Dendy_Settings>::CClock,
		StdCPU<Dendy_Settings>::CPU, StdAPU<Dendy_Settings>::APU,
		StdPPU<Dendy_Settings>::PPU, _ROM> >, Dendy_Settings> Config;
};

typedef NTSC_NES_Config <CBus, CNROM>::Config NROM_NTSC_Config;
typedef PAL_NES_Config  <CBus, CNROM>::Config NROM_PAL_Config;
typedef Dendy_NES_Config<CBus, CNROM>::Config NROM_Dendy_Config;

typedef NTSC_NES_Config <CBus, CMMC1>::Config MMC1_NTSC_Config;
typedef PAL_NES_Config  <CBus, CMMC1>::Config MMC1_PAL_Config;
typedef Dendy_NES_Config<CBus, CMMC1>::Config MMC1_Dendy_Config;

typedef NTSC_NES_Config <CBus, CUxROM>::Config UxROM_NTSC_Config;
typedef PAL_NES_Config  <CBus, CUxROM>::Config UxROM_PAL_Config;
typedef Dendy_NES_Config<CBus, CUxROM>::Config UxROM_Dendy_Config;

typedef NTSC_NES_Config <CBus, CAxROM>::Config AxROM_NTSC_Config;
typedef PAL_NES_Config  <CBus, CAxROM>::Config AxROM_PAL_Config;
typedef Dendy_NES_Config<CBus, CAxROM>::Config AxROM_Dendy_Config;

typedef NTSC_NES_Config <CBus, CMMC3>::Config MMC3_NTSC_Config;
typedef PAL_NES_Config  <CBus, CMMC3>::Config MMC3_PAL_Config;
typedef Dendy_NES_Config<CBus, CMMC3>::Config MMC3_Dendy_Config;

/* Открыть картридж */
CNESConfig *vpnes::OpenROM(istream &ROM, ines::NES_ROM_Data *Data, ines::NES_Type Type) {
	ines::NES_Type Cfg = Type;

	if (ReadROM(ROM, Data) < 0)
		return NULL;
	if (Type == ines::NES_Auto)
		if (Data->Header.TVSystem)
			Cfg = ines::NES_PAL;
		else
			Cfg = ines::NES_NTSC;
	switch (Cfg) {
		case ines::NES_NTSC:
			switch (Data->Header.Mapper) {
				case 0:
					return new NROM_NTSC_Config(Data);
				case 1:
					return new MMC1_NTSC_Config(Data);
				case 2:
					return new UxROM_NTSC_Config(Data);
				case 4:
					return new MMC3_NTSC_Config(Data);
				case 7:
					return new AxROM_NTSC_Config(Data);
			}
		case ines::NES_PAL:
			switch (Data->Header.Mapper) {
				case 0:
					return new NROM_PAL_Config(Data);
				case 1:
					return new MMC1_PAL_Config(Data);
				case 2:
					return new UxROM_PAL_Config(Data);
				case 4:
					return new MMC3_PAL_Config(Data);
				case 7:
					return new AxROM_PAL_Config(Data);
			}
		case ines::NES_Dendy:
			switch (Data->Header.Mapper) {
				case 0:
					return new NROM_Dendy_Config(Data);
				case 1:
					return new MMC1_Dendy_Config(Data);
				case 2:
					return new UxROM_Dendy_Config(Data);
				case 4:
					return new MMC3_Dendy_Config(Data);
				case 7:
					return new AxROM_Dendy_Config(Data);
			}
		default:
			break;
	}
	FreeROMData(Data);
	return NULL;
}
