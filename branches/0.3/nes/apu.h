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

#ifndef __APU_H_
#define __APU_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include <cstring>
#include "manager.h"
#include "bus.h"

namespace vpnes {

namespace APUID {

typedef APUGroup<1>::ID::NoBatteryID InternalDataID;

}

/* APU */
template <class _Bus>
class CAPU: public CDevice {
private:
	/* Шина */
	_Bus *Bus;

	/* Внутренние данные */
	struct SInternalData {
		int WasteCycles;
	} InternalData;

	/* Буфер */
	VPNES_IBUF ibuf;
public:
	inline explicit CAPU(_Bus *pBus) {
		Bus = pBus;
		Bus->GetManager()->template SetPointer<APUID::InternalDataID>(\
			&InternalData, sizeof(InternalData));
	}
	inline ~CAPU() {}

	/* Обработать такты */
	inline void Clock(int Cycles) {
	}

	/* Сброс */
	inline void Reset() {
		memset(&InternalData, 0, sizeof(InternalData));
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		switch (Address) {
			case 0x4015: /* Состояние APU */
			case 0x4016: /* Данные контроллера 1 */
			case 0x4017: /* Данные контроллера 2 */
				break;
		}
		return 0x40;
	}

	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		switch (Address) {
			/* Прямоугольный канал 1 */
			case 0x4000:
			case 0x4001:
			case 0x4002:
			case 0x4003:
			/* Прямоугольный канал 2 */
			case 0x4004:
			case 0x4005:
			case 0x4006:
			case 0x4007:
			/* Пилообразный канал */
			case 0x4008:
			case 0x400a:
			case 0x400b:
			/* Шум */
			case 0x400c:
			case 0x400e:
			case 0x400f:
			/* ДМ-канал */
			case 0x4010:
			case 0x4011:
			case 0x4012:
			case 0x4013:
			/* Другое */
			case 0x4014: /* DMA */
			case 0x4015: /* Упраление каналами */
			case 0x4016: /* Стробирование контроллеров */
			case 0x4017: /* Счетчик кадров */
				break;
		}
	}

	/* CPU IDLE */
	inline int WasteCycles() {
		int Res = InternalData.WasteCycles;

		InternalData.WasteCycles = 0;
		return Res;
	}
	/* Буфер */
	inline VPNES_IBUF &GetBuf() { return ibuf; }
};

}

#endif
