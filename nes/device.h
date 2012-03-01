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

#ifndef __DEVICE_H_
#define __DEVICE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

namespace vpnes {

/* Стандартное устройство */
template <class _Bus>
class CDevice {
protected:
	/* Указатель на шину */
	_Bus *Bus;
public:
	inline explicit CDevice() {}
	inline ~CDevice() {}
	inline explicit CDevice(CDevice const &) {}

	/* Чтение памяти */
	inline virtual uint8 ReadAddress(uint16 Address) { return 0x00; }
	/* Запись памяти */
	inline virtual void WriteAddress(uint16 Address, uint8 Src) {}
};

/* Стандартное устройство PPU */
class CPPUDevice {
public:
	inline explicit CPPUDevice() {}
	inline ~CPPUDevice() {}
	inline explicit CPPUDevice(CPPUDevice const &) {}

	/* Чтение памяти */
	inline virtual uint8 ReadPPUAddress(uint16 Address) { return 0x00; }
	/* Запись памяти */
	inline virtual void WritePPUAddress(uint16 Address, uint8 Src) {}
};


/* Стандартная реализация шины */
template <class _CPU_rebind, class _PPU_rebind, class _ROM_rebind>
class CBus {
public:
	/* Стандартные устройства */
	enum StandardDevices {
		CPU = 0,
		APU,
		PPU,
		ROM,
		Input,
		StandardDevicesNum
	};

	/* Жесткие махинации для получения классов устройств */
	typedef class _CPU_rebind::template rebind<CBus>::rebinded CPUClass;
//	typedef class _APU_rebind::template rebind<CBus>::rebinded APUClass;
	typedef class _PPU_rebind::template rebind<CBus>::rebinded PPUClass;
	typedef class _ROM_rebind::template rebind<CBus>::rebinded ROMClass;

	/* *NOTE* */
	/* Суть махинаций — и шина, и устройства теперь будут знать друг друга, */
	/* а значит компилятор сможет использовать inline-методы */

	/* Вообще по идее код компилироваться не должен... */

private:
	/* Маска для PPU */
	uint16 MirrorMask;

protected:
	/* Список стандартных устройств */
	CDevice<CBus> *DeviceList[StandardDevicesNum];

public:
	inline explicit CBus() {}
	inline ~CBus() {}

	/* Обращение к памяти CPU */
	inline uint8 ReadCPUMemory(uint16 Address) {
		if (Address < 0x2000) /* CPU internal RAM */
			return static_cast<CPUClass *>(DeviceList[CPU])->ReadMemory(Address);
		if (Address < 0x4000) /* PPU registers */
			return static_cast<PPUClass *>(DeviceList[PPU])->ReadMemory(Address);
//		if (Address < 0x4018) /* APU registers */
//			return static_cast<APUClass *>(DeviceList[APU])->ReadMemory(Address);
		/* Mapper */
		return static_cast<ROMClass *>(DeviceList[ROM])->ReadMemory(Address);
	}
	inline void WriteCPUMemory(uint16 Address) {
		if (Address < 0x2000) /* CPU internal RAM */
			static_cast<CPUClass *>(DeviceList[CPU])->WriteMemory(Address);
		else if (Address < 0x4000) /* PPU registers */
			static_cast<PPUClass *>(DeviceList[PPU])->WriteMemory(Address);
//		else if (Address < 0x4018) /* APU registers */
//			return static_cast<APUClass *>(DeviceList[APU])->ReadMemory(Address);
		else /* Mapper */
			static_cast<ROMClass *>(DeviceList[ROM])->WriteMemory(Address);
	}

	/* Обращение к памяти PPU */
	inline uint8 ReadPPUMemory(uint16 Address) {
		if (Address < 0x2000) /* Mapper CHR data */
			return static_cast<ROMClass *>(DeviceList[ROM])->ReadPPUMemory(Address);
		if (Address < 0x3f00) /* PPU attributes/nametables */
			return static_cast<PPUClass *>(DeviceList[PPU])->ReadPPUMemory(Address & MirrorMask);
		/* PPU */
		return static_cast<PPUClass *>(DeviceList[PPU])->ReadPPUMemory(Address);
	}
	inline void WritePPUMemory(uint16 Address) {
		if (Address < 0x2000) /* Mapper CHR data, forbidden */
			static_cast<ROMClass *>(DeviceList[ROM])->WritePPUMemory(Address);
		else if (Address < 0x3f00) /* PPU attributes/nametables */
			static_cast<PPUClass *>(DeviceList[PPU])->WritePPUMemory(Address & MirrorMask);
		else /* PPU */
			static_cast<PPUClass *>(DeviceList[PPU])->WritePPUMemory(Address);
	}

	/* Список стандартных устройств */
	inline CDevice<CBus> **GetDeviceList() { return DeviceList; }
	/* Доступ к маске адресов PPU */
	inline uint16 &GetMirrorMask() { return MirrorMask; }
};

}

#endif
