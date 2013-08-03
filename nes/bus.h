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

#ifndef __BUS_H_
#define __BUS_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include "frontend.h"
#include "manager.h"
#include "clock.h"

namespace vpnes {

/* Базовый класс шины */
template <class _CPU,
          class _APU,
          class _PPU,
          class _MMC>
class CBus_Basic {
public:
	typedef _CPU CPUClass;
	typedef _APU APUClass;
	typedef _PPU PPUClass;
	typedef _MMC MMCClass;
private:
	/* Интерфейс NES */
	CNESFrontend *Frontend;
	/* Менеджер памяти */
	CMemoryManager *Manager;
	/* Часы */
	CClock *Clock;
	/* CPU */
	CPUClass *CPU;
	/* APU */
	APUClass *APU;
	/* PPU */
	PPUClass *PPU;
	/* MMC */
	MMCClass *MMC;
public:
	inline CBus_Basic(CNESFrontend *pFrontend, CMemoryManager *pManager, CClock *pClock):
		Frontend(pFrontend), Manager(pManager), Clock(pClock) {
	}
	inline virtual ~CBus_Basic() {}

	/* Обращение к памяти CPU */
	inline uint8 ReadCPU(uint16 Address) {
		MMC->UpdateCPUBus(Address);
		if (Address < 0x2000) /* CPU internal RAM */
			return CPU->ReadByte(Address);
		if (Address < 0x4000) /* PPU registers */
			return PPU->ReadByte(Address);
		if (Address < 0x4018) /* APU registers */
			return APU->ReadByte(Address);
		else /* Mapper */
			return MMC->ReadByte(Address);
	}
	inline void WriteCPU(uint16 Address, uint8 Src) {
		MMC->UpdateCPUBus(Address, Src);
		if (Address < 0x2000) /* CPU internal RAM */
			CPU->WriteByte(Address, Src);
		else if (Address < 0x4000) /* PPU registers */
			PPU->WriteByte(Address, Src);
		else if (Address < 0x4018) /* APU registers */
			APU->WriteByte(Address, Src);
		/* MMC */
		MMC->WriteByte(Address, Src);
	}

	/* Интерфейс NES */
	inline CNESFrontend * const &GetFrontend() const { return Frontend; }
	/* Менеджер памяти */
	inline CMemoryManager * const &GetManager() const { return Manager; }
	/* Часы */
	inline CClock * const &GetClock() { return Clock; }
	/* CPU */
	inline CPUClass *&GetCPU() { return CPU; }
	/* APU */
	inline APUClass *&GetAPU() { return APU; }
	/* PPU */
	inline PPUClass *&GetPPU() { return PPU; }
	/* MMC */
	inline MMCClass *&GetMMC() { return MMC; }
};

}

#endif
