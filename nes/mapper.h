/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2014  Ivanov Viktor

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

#ifndef __MAPPER_H_
#define __MAPPER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include "ines.h"
#include "manager.h"
#include "cpu.h"
#include "bus.h"

namespace vpnes {

namespace MiscID {

typedef MiscGroup<1>::ID::StaticID PRGID;
typedef MiscGroup<2>::ID::StaticID CHRID;
typedef MiscGroup<3>::ID::NoBatteryID CHRRAMID;
typedef MiscGroup<4>::ID::NoBatteryID RAMID;
typedef MiscGroup<5>::ID::BatteryID BatteryID;
typedef MiscGroup<6>::ID::NoBatteryID PagedPRGID;
typedef MiscGroup<7>::ID::NoBatteryID PagedCHRID;

}

namespace mapper {

/* Адаптер для цепочки */
template <template<class> class _ChainContainer>
struct MMCChain_Adapter {
	template <class _Bus>
	struct MMCChain: public _ChainContainer<_Bus>::Chain {
	public:
		MMCChain(_Bus *pBus, ines::NES_ROM_Data *pData):
			_ChainContainer<_Bus>::Chain(pBus, pData) {}
		~MMCChain() {}
	};
};

/* Элементы цепочки шаблонов */

/* Пустой картридж */
template <class _Bus>
class BasicMMC {
protected:
	/* Шина */
	_Bus *Bus;
	/* ROM */
	ines::NES_ROM_Data *Data;
	/* CHR */
	uint8 *CHR;
	/* RAM */
	uint8 *RAM;
public:
	BasicMMC(_Bus *pBus, ines::NES_ROM_Data *pData) {
		Bus = pBus;
		Data = pData;
		Bus->GetManager()->template SetPointer<ManagerID<MiscID::PRGID> >(\
			Data->PRG, Data->Header.PRGSize * sizeof(uint8));
		if (Data->CHR != NULL) {
			Bus->GetManager()->template SetPointer<ManagerID<MiscID::CHRID> >(\
				Data->CHR, Data->Header.CHRSize * sizeof(uint8));
			CHR = Data->CHR;
		} else
			CHR = static_cast<uint8 *>(Bus->GetManager()->\
				template GetPointer<ManagerID<MiscID::CHRRAMID> >(\
				Data->Header.CHRSize * sizeof(uint8)));
		if (Data->Header.RAMSize == 0)
			RAM = NULL;
		else {
			RAM = new uint8[Data->Header.RAMSize];
			memset(RAM, 0x00, Data->Header.RAMSize * sizeof(uint8));
			if (Data->Trainer != NULL)
				memcpy(RAM + 0x1000, Data->Trainer, 0x0200 * sizeof(uint8));
			if (Data->Header.BatterySize > 0) {
				Bus->GetManager()->template SetPointer<ManagerID<MiscID::BatteryID> >(\
					RAM + (Data->Header.RAMSize - Data->Header.BatterySize),
					Data->Header.BatterySize * sizeof(uint8));
				if (Data->Header.RAMSize > Data->Header.BatterySize)
					Bus->GetManager()->template SetPointer<ManagerID<MiscID::RAMID> >(\
						RAM, (Data->Header.RAMSize - Data->Header.BatterySize) *
						sizeof(uint8));
			} else
				Bus->GetManager()->template SetPointer<ManagerID<MiscID::RAMID> >(\
					RAM, Data->Header.RAMSize * sizeof(uint8));
		}
	}
	inline virtual ~BasicMMC() {
		delete [] RAM;
	}

	/* Обновление адреса на шине CPU */
	inline void UpdateCPUBus(uint16 Address) {
	}
	inline void UpdateCPUBus(uint16 Address, uint8 Src) {
	}
	/* Чтение памяти */
	inline uint8 ReadByte(uint16 Address) {
		return 0x40;
	}
	/* Запись памяти */
	inline void WriteByte(uint16 Address, uint8 Src) {
	}
	/* Выполнить зависимости CPU */
	inline void CPUDependencies() {
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		return 0x00;
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
	}

	/* Обновление семплов аудио */
	inline void UpdateSound(double &DACOut) {
	}
	/* Такты аудио */
	inline bool Do_Timer(int Cycles) {
		return false;
	}
	/* Обновить такты APU */
	inline void UpdateAPUCycles(int &Cycle) {
	}

	/* Сброс */
	inline void Reset() {
	}
};

/* Картридж со стандартной картой RAM */
template <class _Bus, class _Parent>
class StandardPRG: public _Parent {
protected:
	using _Parent::Bus;
	using _Parent::Data;
	using _Parent::RAM;
public:
	StandardPRG(_Bus *pBus, ines::NES_ROM_Data *pData): _Parent(pBus, pData) {
	}
	inline ~StandardPRG() {
	}

	/* Чтение памяти */
	inline uint8 ReadByte(uint16 Address) {
		if (Address < 0x6000) /* Регистры */
			return 0x40;
		if (Address < 0x8000) { /* SRAM */
			if (RAM != NULL)
				return RAM[Address & 0x1fff];
			else if ((Data->Trainer != NULL) && (Address >= 0x7000)
				&& (Address < 0x7200))
				return Data->Trainer[Address & 0x01ff];
			return 0x40;
		}
		return _Parent::ReadByte(Address);
	}
	/* Запись памяти */
	inline void WriteByte(uint16 Address, uint8 Src) {
		if (Address < 0x6000)
			return;
		if ((RAM == NULL) || (Address >= 0x8000))
			_Parent::WriteByte(Address, Src);
		else
			RAM[Address & 0x1fff] = Src;
	}
};

/* Картридж со страничной PRG */
template <class _Bus, class _Settings, class _Parent>
class PagedPRG: public _Parent {
protected:
	using _Parent::Bus;
	using _Parent::Data;
private:
	typedef typename _Settings::PagedPRGData SPagedData;
	typedef typename _Settings::PagedPRGDataPar SPagedDataPar;

	SPagedData PagedData;
public:
	PagedPRG(_Bus *pBus, ines::NES_ROM_Data *pData): _Parent(pBus, pData) {
		Bus->GetManager()->template SetPointer<ManagerID<MiscID::PagedPRGID> >(\
			&PagedData, sizeof(SPagedData));
		_Settings::InitPagedPRG(PagedData, Data);
	}
	inline ~PagedPRG() {
	}

	/* Чтение памяти */
	inline uint8 ReadByte(uint16 Address) {
		int i = _Settings::PagedPRGIndex(PagedData, Address);
		return Data->PRG[PagedData.Addr[i] | (Address & PagedData.Mask[i])];
	}

	/* Обновление данных страниц */
	inline void UpdatePagedPRGData(SPagedDataPar &PagedDataPar) {
		_Settings::UpdatePagedPRG(PagedData, PagedDataPar);
	}
};

/* Картридж со страничной CHR */
template <class _Bus, class _Settings, class _Parent>
class PagedCHR: public _Parent {
protected:
	using _Parent::Bus;
	using _Parent::Data;
	using _Parent::CHR;
private:
	typedef typename _Settings::PagedCHRData SPagedData;
	typedef typename _Settings::PagedCHRDataPar SPagedDataPar;

	SPagedData PagedData;
public:
	PagedCHR(_Bus *pBus, ines::NES_ROM_Data *pData): _Parent(pBus, pData) {
		Bus->GetManager()->template SetPointer<ManagerID<MiscID::PagedCHRID> >(\
			&PagedData, sizeof(SPagedData));
		_Settings::InitPagedCHR(PagedData, Data);
	}
	inline ~PagedCHR() {
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		int i = _Settings::PagedCHRIndex(PagedData, Address);
		return CHR[PagedData.Addr[i] | (Address & PagedData.Mask[i])];
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		if (Data->CHR != NULL)
			return;

		int i = _Settings::PagedCHRIndex(PagedData, Address);

		CHR[PagedData.Addr[i] | (Address & PagedData.Mask[i])] = Src;
	}

	/* Обновление данных страниц */
	inline void UpdatePagedCHRData(SPagedDataPar &PagedDataPar) {
		_Settings::UpdatePagedCHR(PagedData, PagedDataPar);
	}
};

/* Конфликт шины */
template <class _Bus, class _Parent>
class BusConflict: public _Parent {
protected:
	using _Parent::Bus;
public:
	BusConflict(_Bus *pBus, ines::NES_ROM_Data *pData): _Parent(pBus, pData) {
	}
	inline ~BusConflict() {}

	/* Запись памяти */
	inline void WriteByte(uint16 Address, uint8 Src) {
		uint8 BusR = _Parent::ReadByte(Address) & Src;

		_Parent::WriteByte(Address, BusR);
		if ((_Parent::ReadByte(Address) & Src) != BusR)
			/* Невозможно определить результат действия */
			Bus->GetCPU()->Panic();
	}
};

}

}

#endif
