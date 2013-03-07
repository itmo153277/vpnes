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

#ifndef __NBUILD_H_
#define __NBUILD_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ines.h"
#include "nes.h"
#include "clock.h"
#include "cpu.h"
#include "apu.h"
#include "aputables.h"
#include "ppu.h"

namespace vpnes {

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

/* Собрать стандартный NES */
template <template<class> class _ROM>
inline CNESConfig *BuildStdNES(const ines::NES_ROM_Data *Data, ines::NES_Type Type) {
	switch (Type) {
		case ines::NES_Auto:
		case ines::NES_NTSC:
			return new typename Std_NES_Config<CBus, _ROM, NTSC_Settings >::Config(Data);
		case ines::NES_PAL:
			return new typename Std_NES_Config<CBus, _ROM, PAL_Settings  >::Config(Data);
		case ines::NES_Dendy:
			return new typename Std_NES_Config<CBus, _ROM, Dendy_Settings>::Config(Data);
	}
	return NULL;
}

/* Сборщики для мапперов */
CNESConfig *NROM_Config (const ines::NES_ROM_Data *Data, ines::NES_Type Type); /* 0 */
CNESConfig *MMC1_Config (const ines::NES_ROM_Data *Data, ines::NES_Type Type); /* 1 */
CNESConfig *UxROM_Config(const ines::NES_ROM_Data *Data, ines::NES_Type Type); /* 2 */
CNESConfig *CNROM_Config(const ines::NES_ROM_Data *Data, ines::NES_Type Type); /* 3 */
CNESConfig *MMC3_Config (const ines::NES_ROM_Data *Data, ines::NES_Type Type); /* 4 */
CNESConfig *MMC5_Config (const ines::NES_ROM_Data *Data, ines::NES_Type Type); /* 5 */
CNESConfig *AxROM_Config(const ines::NES_ROM_Data *Data, ines::NES_Type Type); /* 7 */

}

#endif
