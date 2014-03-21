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

#include <iostream>
#include "manager.h"
#include "clock.h"
#include "frontend.h"

namespace vpnes {

namespace MiscID {

}

/* Интерфейс для NES */
class CBasicNES {
protected:
	/* Мендеджер памяти */
	CMemoryManager Manager;
	/* Часы */
	CClock Clock;
public:
	inline CBasicNES() {}
	inline virtual ~CBasicNES() {
		Manager.FreeMemory<DynamicGroupID>();
	}

	/* Запустить цикл эмуляции */
	virtual int PowerOn() = 0;
	/* Программный сброс */
	virtual int Reset() = 0;

	/* Сохранить энергонезависимую память */
	inline int SaveButteryBackedMemory(std::ostream &RamFile) {
		return Manager.SaveMemory<BatteryGroupID>(RamFile);
	}
	/* Загрузить энергонезависимую память */
	inline int LoadButteryBackedMemory(std::istream &RamFile) {
		int RetVal = Manager.LoadMemory<BatteryGroupID>(RamFile);

		/* Обновление списка исходя из новых данных*/
		Clock.UpdateList();
		return RetVal;
	}
	/* Сохранить текущее состояние */
	inline int SaveState(std::ostream &RamFile) {
		return Manager.SaveMemory<DynamicGroupID>(RamFile);
	}
	/* Загрузить текущее состояние */
	inline int LoadState(std::istream &RamFile) {
		int RetVal = Manager.LoadMemory<DynamicGroupID>(RamFile);

		/* Обновление списка исходя из новых данных*/
		Clock.UpdateList();
		return RetVal;
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
	inline CNESConfig() {}
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

/* Стандартный NES */
template <class _Bus>
class CNES: public CBasicNES {
public:
	typedef _Bus BusClass;
private:
	/* Шина */
	BusClass Bus;
public:
	inline explicit CNES(CNESFrontend *Frontend):
		Bus(Frontend, &Clock, &Manager) {
	}
	inline ~CNES() {
	}

	/* Запустить цикл эмуляции */
	int PowerOn() {
		bool Quit;

		Bus.GetFrontend()->GetAudioFrontend()->ResumeAudio();
		Clock.Start([&, this, Quit](int Clocks) {
			if (this->Bus.GetPPU()->IsFrameReady()) {
				this->Bus.GetAPU()->FlushBuffer();
				Quit = !this->Bus.GetFrontend()->GetVideoFrontend()->UpdateFrame(this->Bus.GetPPU()->GetFrameTime());
				this->Clock.Reset();
				if (Quit) {
					this->Clock.Terminate();
					return;
				}
			}
			this->Bus.GetCPU()->Execute(Clocks);
		});
		Bus.GetFrontend()->GetAudioFrontend()->StopAudio();
		return 0;
	}

	/* Программный сброс */
	int Reset() {
		Bus.Reset();
		return 0;
	}

	/* Доступ к шине */
	inline BusClass * const GetBus() const { return &Bus; }
};

}

#endif
