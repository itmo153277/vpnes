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
#include "clock.h"
#include "cpu.h"
#include "apu.h"
#include "ppu.h"
#include "mapper.h"


using namespace std;
using namespace vpnes;

/* Шаблон для конфигураций NES */
template <template<template<class> class,
                   template<class> class,
                   template<class> class,
                   template<class> class,
                   template<class> class> class _Bus,
          template<class> class _ROM, class _Settings>
struct Std_NES_Config {

	/* Стандартный генератор */
	template <class BusClass>
	class CClock: public CStdClock<BusClass, _Settings> {
	public:
		inline explicit CClock(BusClass *pBus): CStdClock<BusClass, _Settings>(pBus) {}
		inline ~CClock() {}
	};

	/* Стандартный ЦПУ */
	template <class BusClass>
	class CPU: public CCPU<BusClass, _Settings> {
	public:
		inline explicit CPU(BusClass *pBus): CCPU<BusClass, _Settings>(pBus) {}
		inline ~CPU() {}
	};

	/* Стандартный АПУ */
	template <class BusClass>
	class APU: public CAPU<BusClass, _Settings> {
	public:
		inline explicit APU(BusClass *pBus): CAPU<BusClass, _Settings>(pBus) {}
		inline ~APU() {}
	};

	/* Стандартный ГПУ */
	template <class BusClass>
	class PPU: public CPPU<BusClass, _Settings> {
	public:
		inline explicit PPU(BusClass *pBus): CPPU<BusClass, _Settings>(pBus) {}
		inline ~PPU() {}
	};

	/* Конфигурация */
	typedef CNESConfigTemplate<CNES<_Bus<CClock, CPU, APU, PPU, _ROM> >, _Settings> Config;
};

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

typedef Std_NES_Config<CBus, CNROM, NTSC_Settings >::Config NROM_NTSC_Config;
typedef Std_NES_Config<CBus, CNROM, PAL_Settings  >::Config NROM_PAL_Config;
typedef Std_NES_Config<CBus, CNROM, Dendy_Settings>::Config NROM_Dendy_Config;

typedef Std_NES_Config<CBus, CMMC1, NTSC_Settings >::Config MMC1_NTSC_Config;
typedef Std_NES_Config<CBus, CMMC1, PAL_Settings  >::Config MMC1_PAL_Config;
typedef Std_NES_Config<CBus, CMMC1, Dendy_Settings>::Config MMC1_Dendy_Config;

typedef Std_NES_Config<CBus, CUxROM, NTSC_Settings >::Config UxROM_NTSC_Config;
typedef Std_NES_Config<CBus, CUxROM, PAL_Settings  >::Config UxROM_PAL_Config;
typedef Std_NES_Config<CBus, CUxROM, Dendy_Settings>::Config UxROM_Dendy_Config;

typedef Std_NES_Config<CBus, CAxROM, NTSC_Settings >::Config AxROM_NTSC_Config;
typedef Std_NES_Config<CBus, CAxROM, PAL_Settings  >::Config AxROM_PAL_Config;
typedef Std_NES_Config<CBus, CAxROM, Dendy_Settings>::Config AxROM_Dendy_Config;

typedef Std_NES_Config<CBus, CMMC3, NTSC_Settings >::Config MMC3_NTSC_Config;
typedef Std_NES_Config<CBus, CMMC3, PAL_Settings  >::Config MMC3_PAL_Config;
typedef Std_NES_Config<CBus, CMMC3, Dendy_Settings>::Config MMC3_Dendy_Config;

/* Открыть картридж */
CNESConfig *vpnes::OpenROM(istream &ROM, ines::NES_ROM_Data *Data, ines::NES_Type Type) {
	ines::NES_Type Cfg = Type;

	if (ReadROM(ROM, Data) < 0)
		return NULL;
	if (Type == ines::NES_Auto) {
		if (Data->Header.TVSystem)
			Cfg = ines::NES_PAL;
		else
			Cfg = ines::NES_NTSC;
	}
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
			break;
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
			break;
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
			break;
		default:
			break;
	}
	FreeROMData(Data);
	return NULL;
}
