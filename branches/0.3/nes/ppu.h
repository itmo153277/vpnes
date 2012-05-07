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

#ifndef __PPU_H_
#define __PPU_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include <cstring>
#include "manager.h"
#include "bus.h"

namespace vpnes {

namespace PPUID {

typedef typename PPUGroup<1>::ID::NoBatteryID CycleDataID;

}

/* PPU */
template <class _Bus>
class CPPU: public CDevice {
private:
	/* Шина */
	_Bus *Bus;
	/* Обработка кадра завершена */
	bool FrameReady;
	/* Предобработано тактов */
	int PreprocessedCycles;
	/* Ушло тактов на фрейм */
	int FrameCycles;
	/* Данные о тактах */
	struct SCycleData {
		int CurCycle; /* Текущий такт */
		int DMACycle; /* Текущий такт DMA */
	} CycleData;

	/* Рендер */
	inline void Render(int Cycles) {
		CycleData.CurCycle += Cycles;
		if (CycleData.CurCycle > 341 * 262) {
			FrameReady = true;
			FrameCycles = 341 * 262;
			CycleData.CurCycle -= FrameCycles;
		}
	}
public:
	inline explicit CPPU(_Bus *pBus, VPNES_VBUF *Buf) {
		Bus = pBus;
		Bus->GetManager()->template SetPointer<PPUID::CycleDataID>(\
			&CycleData, sizeof(CycleData));
	}
	inline ~CPPU() {}

	/* Обработать такты */
	inline void Clock(int Cycles) {
		/* Обрабатываем необработанные такты */
		Render(Cycles - PreprocessedCycles);
		PreprocessedCycles = 0;
	}

	/* Предобработка */
	inline void PreRender() {
		/* Обрабатываем текущие такты */
		Render(Bus->GetClock()->GetPreCycles() - PreprocessedCycles);
		PreprocessedCycles = Bus->GetClock()->GetPreCycles();
	}

	/* Сброс */
	inline void Reset() {
		FrameReady = false;
		PreprocessedCycles = 0;
		memset(&CycleData, 0, sizeof(CycleData));
	}

	/* Обработка DMA */
	inline void ProccessDMA(int Cycles) {
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		return 0x00;
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
	}

	/* Флаг окончания рендеринга фрейма */
	inline const bool &IsFrameReady() const { return FrameReady; }
	/* Ушло татов на фрейм */
	inline int GetFrameCycles() {
		int Res = FrameCycles;

		/* Сбрасываем состояние */
		FrameCycles = 0;
		FrameReady = false;
		return Res;
	}
};

struct PPU_rebind {
	template <class _Bus>
	struct rebind {
		typedef CPPU<_Bus> rebinded;
	};
};

}

#endif
