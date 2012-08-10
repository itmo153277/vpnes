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
	inline uint8 ReadAddress(uint16 Address) { return 0x40; }
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {}
};

/* Базовый класс шины */
template <class _Clock,
          class _CPU,
          class _APU,
          class _PPU,
          class _ROM>
class CBus_Basic {
public:
	typedef _Clock ClockClass;
	typedef _CPU CPUClass;
	typedef _APU APUClass;
	typedef _PPU PPUClass;
	typedef _ROM ROMClass;
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
	/* PRG RW State */
	bool PRGRW;
public:
	inline explicit CBus_Basic(VPNES_CALLBACK Callback, CMemoryManager *Manager):
		_Callback(Callback), _Manager(Manager) {
		PRGRW = false;
	}
	inline ~CBus_Basic() {}

	/* Обращение к памяти CPU */
	inline uint8 ReadCPUMemory(uint16 Address) {
		PRGRW = true;
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

	/* Проверяем был ли цикл запись-чтение */
	inline bool WasPRGRWClocked() {
		bool Res = PRGRW;

		PRGRW = false;
		return Res;
	}

	/* Обращение к памяти PPU */
	inline uint8 ReadPPUMemory(uint16 Address) {
		if (Address < 0x2000) /* Mapper CHR data */
			return ROM->ReadPPUAddress(Address);
		else /* PPU attributes/nametables */
			return SolderPad.ReadPPUAddress(Address);
	}
	inline void WritePPUMemory(uint16 Address, uint8 Src) {
		if (Address < 0x2000) /* Mapper CHR data */
			ROM->WritePPUAddress(Address, Src);
		else /* PPU attributes/nametables */
			SolderPad.WritePPUAddress(Address, Src);
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
	inline typename ROMClass::SolderPad *GetSolderPad() { return &SolderPad; }
};

/* Стандартная шина */
template <template<class> class _Clock,
          template<class> class _CPU,
          template<class> class _APU,
          template<class> class _PPU,
          template<class> class _ROM>
class CBus: public CBus_Basic<_Clock<CBus<_Clock, _CPU, _APU, _PPU, _ROM> >,
                              _CPU<CBus<_Clock, _CPU, _APU, _PPU, _ROM> >,
                              _APU<CBus<_Clock, _CPU, _APU, _PPU, _ROM> >,
                              _PPU<CBus<_Clock, _CPU, _APU, _PPU, _ROM> >,
                              _ROM<CBus<_Clock, _CPU, _APU, _PPU, _ROM> > > {
public:
	inline explicit CBus(VPNES_CALLBACK Callback, CMemoryManager *Manager):
		CBus_Basic<_Clock<CBus>, _CPU<CBus>, _APU<CBus>, _PPU<CBus>,
			_ROM<CBus> >(Callback, Manager) {}
};

}

#endif
