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

#ifndef __BUS_H_
#define __BUS_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include "manager.h"

namespace vpnes {

/* Стандартное устройство */
class CDevice {
public:
	inline explicit CDevice() {}
	inline virtual ~CDevice() {}
	inline explicit CDevice(CDevice const &) {}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) { return 0x00; }
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {}
};

/* Базовый класс шины */
template <class _Clock_rebind, class _CPU_rebind, class _APU_rebind, class _PPU_rebind,
	class _ROM_rebind, class _BusClass>
class CBus_Basic {
public:
	typedef typename _Clock_rebind::template rebind<_BusClass>::rebinded ClockClass;
	typedef typename _CPU_rebind::template rebind<_BusClass>::rebinded CPUClass;
	typedef typename _APU_rebind::template rebind<_BusClass>::rebinded APUClass;
	typedef typename _PPU_rebind::template rebind<_BusClass>::rebinded PPUClass;
	typedef typename _ROM_rebind::template rebind<_BusClass>::rebinded ROMClass;
private:
	/* Обработчик событий */
	VPNES_CALLBACK _Callback;
	/* Менеджер памяти */
	CMemoryManager *_Manager;
	/* Clock */
	ClockClass *Clock;
	/* CPU */
	CPUClass *CPU;
	/* APU */
	APUClass *APU;
	/* PPU */
	PPUClass *PPU;
	/* ROM */
	ROMClass *ROM;
	/* SolderPad */
	typename ROMClass::SolderPad SolderPad;
public:
	inline explicit CBus_Basic(VPNES_CALLBACK Callback, CMemoryManager *Manager):
		_Callback(Callback), _Manager(Manager) {}
	inline ~CBus_Basic() {}

	/* Обращение к памяти CPU */
	inline uint8 ReadCPUMemory(uint16 Address) {
		if (Address < 0x2000) /* CPU internal RAM */
			return CPU->ReadAddress(Address);
		if (Address < 0x4000) /* PPU registers */
			return PPU->ReadAddress(Address);
		if (Address < 0x4018) /* APU registers */
			return APU->ReadAddress(Address);
		else /* Mapper */
			return ROM->ReadAddress(Address);
	}
	inline void WriteCPUMemory(uint16 Address, uint8 Src) {
		if (Address < 0x2000) /* CPU internal RAM */
			CPU->WriteAddress(Address, Src);
		else if (Address < 0x4000) /* PPU registers */
			PPU->WriteAddress(Address, Src);
		else if (Address < 0x4018) /* APU registers */
			APU->WriteAddress(Address, Src);
		else /* Mapper */
			ROM->WriteAddress(Address, Src);
	}

	/* Обращение к памяти PPU */
	inline uint8 ReadPPUMemory(uint16 Address) {
		if (Address < 0x2000) /* Mapper CHR data */
			return ROM->ReadPPUAddress(Address);
		else /* PPU attributes/nametables */
			return PPU->ReadPPUAddress(SolderPad.GetAddress(Address));
	}
	inline void WritePPUMemory(uint16 Address, uint8 Src) {
		if (Address < 0x2000) /* Mapper CHR data */
			ROM->WritePPUAddress(Address, Src);
		else /* PPU attributes/nametables */
			PPU->WritePPUAddress(SolderPad.GetAddress(Address), Src);
	}

	/* Обработчик событий */
	inline const VPNES_CALLBACK &GetCallback() const { return _Callback; }
	/* Менеджер памяти */
	inline CMemoryManager * const &GetManager() const { return _Manager; }
	/* Clock */
	inline ClockClass *&GetClock() { return Clock; }
	/* CPU */
	inline CPUClass *&GetCPU() { return CPU; }
	/* APU */
	inline APUClass *&GetAPU() { return APU; }
	/* PPU */
	inline PPUClass *&GetPPU() { return PPU; }
	/* ROM */
	inline ROMClass *&GetROM() { return ROM; }
	/* SolderPad */
	inline typename ROMClass::SolderPad *&GetSolderPad() { return SolderPad; }
};

/* Стандартная шина */
template <class _Clock_rebind, class _CPU_rebind, class _APU_rebind, class _PPU_rebind,
	class _ROM_rebind>
class CBus: public CBus_Basic<_Clock_rebind, _CPU_rebind, _APU_rebind, _PPU_rebind,
	_ROM_rebind, CBus<_Clock_rebind, _CPU_rebind, _APU_rebind, _PPU_rebind, _ROM_rebind> > {
public:
	inline explicit CBus(VPNES_CALLBACK Callback, CMemoryManager *Manager):
		CBus_Basic<_Clock_rebind, _CPU_rebind, _APU_rebind, _PPU_rebind,
		_ROM_rebind, CBus<_Clock_rebind, _CPU_rebind, _APU_rebind, _PPU_rebind,
		_ROM_rebind> >(Callback, Manager) {}
};

}

#endif
