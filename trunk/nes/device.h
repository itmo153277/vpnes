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
	virtual uint8 ReadAddress(uint16 Address) { return 0x00; }
	/* Запись памяти */
	virtual void WriteAddress(uint16 Address, uint8 Src) {}
};

/* Стандартное устройство PPU */
class CPPUDevice {
public:
	inline explicit CPPUDevice() {}
	inline ~CPPUDevice() {}
	inline explicit CPPUDevice(CPPUDevice const &) {}

	/* Чтение памяти */
	virtual uint8 ReadPPUAddress(uint16 Address) { return 0x00; }
	/* Запись памяти */
	virtual void WritePPUAddress(uint16 Address, uint8 Src) {}
};


/* Стандартная реализация шины */
class CBus {
public:
	/* Тип обращения к памяти */
	enum MemoryAccessType {
		CPUAccess,
		PPUAccess
	};

	/* Стандартные устройства */
	enum StandardDevices {
		CPU = 0,
		APU,
		PPU,
		ROM,
		Input,
		StandardDevicesNum
	};

protected:
	/* Список стандартных устройств */
	CDevice<CBus> *DevicesList[StandardDevicesNum];

public:
	inline explicit CBus() {}
	inline ~CBus() {}
	inline explicit CBus(CBus const &) {}

	/* Обращение к памяти CPU */
	inline uint8 ReadCPUMemory(uint16 Address) { return 0x00; }
	inline void WriteCPUMemory(uint16 Address) {}

	/* Обращение к памяти PPU */
	inline uint8 ReadPPUMemory(uint16 Address) { return 0x00; }
	inline void WritePPUMemory(uint16 Address) {}

	/* Список стандартных устройств */
	inline CDevice<CBus> **GetDeviceList() { return DevicesList; }
};

}

#endif
