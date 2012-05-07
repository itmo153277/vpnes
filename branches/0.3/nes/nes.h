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

#ifndef __NES_H_
#define __NES_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include <iostream>
#include "ines.h"
#include "manager.h"

namespace vpnes {

/* Интерфейс для NES */
class CBasicNES {
protected:
	/* Мендеджер памяти */
	CMemoryManager Manager;
public:
	inline explicit CBasicNES() {}
	inline virtual ~CBasicNES() {
		/* Удаляем всю динамическую память */
		Manager.FreeMemory<DynamicGroupID>();
	}

	/* Запустить цикл эмуляции */
	virtual int PowerOn() = 0;
	/* Программный сброс */
	virtual int Reset() = 0;

	/* Выключить приставку (сохранение памяти) */
	inline int PowerOff(std::ostream &RamFile) {
		Manager.SaveMemory<BatteryGroupID>(RamFile);
		return 0;
	}

	/* Сохранить текущее состоянии приставки (сохранение */
	/* всей динамической памяти) */
	inline int SaveState(std::ostream &RamFile) {
		Manager.SaveMemory<DynamicGroupID>(RamFile);
		return 0;
	}

	/* Загрузить текущее состояние (загрузка всей динамической */
	/* памяти в образе, т.е. можно использовть для загрузки */
	/* PRG RAM питающейся от батареи */
	inline int LoadState(std::istream &RamFile) {
		Manager.LoadMemory<DynamicGroupID>(RamFile);
		return 0;
	}
};

/* Интерфейс для параметров NES */
class CNESConfig {
protected:
	/* Длина */
	int Width;
	/* Высота */
	int Height;
public:
	inline explicit CNESConfig() {}
	inline virtual ~CNESConfig() {}

	/* Длина экрана */
	inline const int &GetWidth() const { return Width; }
	/* Высота экрана */
	inline const int &GetHeight() const { return Height; }
	/* Получить приставку по нашим параметрам */
	virtual CBasicNES *GetNES(VPNES_CALLBACK Callback, VPNES_VBUF *Buf) = 0;
};

/* Стандартный шаблон для параметров NES */
template <class _Nes, int _Width, int _Height>
class CNESConfigTemplate: public CNESConfig {
private:
	const ines::NES_ROM_Data *Data;
public:
	inline explicit CNESConfigTemplate(const ines::NES_ROM_Data *ROM) {
		Width = _Width;
		Height = _Height;
		Data = ROM;
	}
	inline ~CNESConfigTemplate() {}

	/* Получить новенький NES */
	CBasicNES *GetNES(VPNES_CALLBACK Callback, VPNES_VBUF *Buf) {
		_Nes *NES;

		NES = new _Nes(Callback, Buf);
		NES->GetBus().GetROM() = new typename _Nes::BusClass::ROMClass(&NES->GetBus(),
			Data);
		return NES;
	}
};

/* Стандартный NES */
template <class _Bus>
class CNES: public CBasicNES {
public:
	typedef _Bus BusClass;
private:
	/* Шина */
	BusClass Bus;
	/* Генератор */
	typename BusClass::ClockClass Clock;
	/* CPU */
	typename BusClass::CPUClass CPU;
	/* APU */
	typename BusClass::APUClass APU;
	/* PPU */
	typename BusClass::PPUClass PPU;
public:
	inline explicit CNES(VPNES_CALLBACK Callback, VPNES_VBUF *Buf):
		Bus(Callback, &Manager), Clock(&Bus), CPU(&Bus), APU(&Bus), PPU(&Bus, Buf) {
		Bus.GetClock() = &Clock;
		Bus.GetCPU() = &CPU;
		Bus.GetAPU() = &APU;
		Bus.GetPPU() = &PPU;
	}
	inline ~CNES() {
		/* ROM добавляется маппером */
		delete Bus.GetROM();
	}

	/* Запустить цикл эмуляции */
	int PowerOn() {
		VPNES_FRAME data;

		for (;;) {
			data = Clock.ProccessFrame();
			if (Bus.GetCallback()(VPNES_CALLBACK_FRAME, (void *) &data) < 0)
				break;
		}
		return 0;
	}

	/* Программный сброс */
	int Reset() {
		CPU.Reset();
		APU.Reset();
		PPU.Reset();
		return 0;
	}

	/* Доступ к шине */
	inline BusClass &GetBus() { return Bus; }
};

/* Параметры стандартного NES */
template <class _Bus>
struct _NES_Config {
	typedef CNESConfigTemplate<CNES<_Bus>, 256, 224> Config;
};

}

#endif
