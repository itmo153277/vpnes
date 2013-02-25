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

#ifndef __NES_H_
#define __NES_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include <typeinfo>
#include <iostream>
#include "ines.h"
#include "manager.h"
#include "frontend.h"

namespace vpnes {

namespace MiscID {

typedef MiscGroup<1>::ID::NoBatteryID Controller1InternalDataID;
typedef MiscGroup<2>::ID::NoBatteryID Controller2InternalDataID;

}

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
	/* Время одного фрейма */
	double Frame;
	/* Имя конфигурации */
	const char *Name;
public:
	inline explicit CNESConfig() {}
	inline virtual ~CNESConfig() {}

	/* Длина экрана */
	inline const int &GetWidth() const { return Width; }
	/* Высота экрана */
	inline const int &GetHeight() const { return Height; }
	/* Время фрейма */
	inline const double &GetFrameLength() const { return Frame; }
	/* Имя конфигурации */
	inline const char * const &GetName() const { return Name; }
	/* Получить приставку по нашим параметрам */
	virtual CBasicNES *GetNES(CNESFrontend *Frontend) = 0;
};

/* Стандартный шаблон для параметров NES */
template <class _Nes, class _Settings>
class CNESConfigTemplate: public CNESConfig {
private:
	const ines::NES_ROM_Data *Data;
public:
	inline explicit CNESConfigTemplate(const ines::NES_ROM_Data *ROM) {
		Name = typeid(this).name();
		Width = _Settings::Right - _Settings::Left;
		Height = _Settings::Bottom - _Settings::Top;
		Frame = _Settings::GetFreq() * _Settings::PPU_Divider *
			((_Settings::ActiveScanlines + _Settings::PostRender +
			_Settings::VBlank + 1) * 341 - _Settings::OddSkip * 0.5);
		Data = ROM;
	}
	inline ~CNESConfigTemplate() {}

	/* Получить новенький NES */
	CBasicNES *GetNES(CNESFrontend *Frontend) {
		_Nes *NES;
		const CInputFrontend::SInternalMemory *ControllerMemory;

		NES = new _Nes(Frontend);
		NES->GetBus().GetROM() = new typename _Nes::BusClass::ROMClass(&NES->GetBus(),
			Data);
		/* Регистрируем память контроллеров */
		ControllerMemory = Frontend->GetInput1Frontend()->GetInternalMemory();
		NES->GetBus().GetManager()->template SetPointer
			<ManagerID<MiscID::Controller1InternalDataID> >(ControllerMemory->RAM,
			ControllerMemory->Size);
		ControllerMemory = Frontend->GetInput2Frontend()->GetInternalMemory();
		NES->GetBus().GetManager()->template SetPointer
			<ManagerID<MiscID::Controller2InternalDataID> >(ControllerMemory->RAM,
			ControllerMemory->Size);
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
	inline explicit CNES(CNESFrontend *Frontend):
		Bus(Frontend, &Manager), Clock(&Bus), CPU(&Bus), APU(&Bus), PPU(&Bus) {
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
		double FrameTime;

		Bus.GetFrontend()->GetAudioFrontend()->ResumeAudio();
		do {
			FrameTime = Clock.ProccessFrame();
			APU.FlushBuffer();
		} while (Bus.GetFrontend()->GetVideoFrontend()->UpdateFrame(FrameTime));
		Bus.GetFrontend()->GetAudioFrontend()->StopAudio();
		return 0;
	}

	/* Программный сброс */
	int Reset() {
		CPU.Reset();
		APU.Reset();
		PPU.Reset();
		APU.Clock(CPU.GetCycles());
		PPU.Clock(CPU.GetCycles());
		return 0;
	}

	/* Доступ к шине */
	inline BusClass &GetBus() { return Bus; }
};

}

#endif
