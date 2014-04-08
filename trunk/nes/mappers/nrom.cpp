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

#include "nrom.h"
#include <cstring>
#include "../bus.h"
#include "../cpu.h"
#include "../apu.h"
#include "../tables.h"

using namespace vpnes;

struct NROMBanks {
	struct PagedPRGData {
		int Addr[2];
		uint16 Mask[2];
		int Shift;
	};
	typedef void *PagedPRGDataPar;

	inline static void InitPagedPRG(PagedPRGData &PagedData, ines::NES_ROM_Data *Data) {
		PagedData.Addr[0] = 0x0000;
		PagedData.Addr[1] = 0x0000;
		if (Data->Header.PRGSize > 0x4000) {
			PagedData.Shift = 16;
			PagedData.Mask[0] = 0x7fff;
			PagedData.Mask[1] = 0x7fff;
		} else {
			PagedData.Shift = 14;
			PagedData.Mask[0] = 0x3fff;
			PagedData.Mask[1] = 0x3fff;
		}
	}

	inline static int PagedPRGIndex(PagedPRGData &PagedData, uint16 Address) {
		return (Address >> PagedData.Shift) & 1;
	}
};

/* Картридж с прямой CHR */
template <class _Bus, class _Parent>
class PlainCHR: public _Parent {
protected:
	using _Parent::Bus;
	using _Parent::Data;
	using _Parent::CHR;
public:
	PlainCHR(_Bus *pBus, ines::NES_ROM_Data *pData): _Parent(pBus, pData) {
	}
	inline ~PlainCHR() {
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		return CHR[Address & 0x1fff];
	}
	/* Запись памяти PPU */
	inline uint8 WritePPUAddress(uint16 Address, uint8 Src) {
		if (Data->CHR != NULL)
			return;
		CHR[Address & 0x1fff] = Src;
	}
};

template <class _Bus>
struct NROM_Chain {
	typedef
		PlainCHR<_Bus,
		mapper::StandardPRG<_Bus,
		mapper::PagedPRG<_Bus, NROMBanks,
		mapper::BasicMMC<_Bus> > > >
		Chain;
};

struct NTSCSettings {
	enum {
		CPU_Divider = 12,
		PPU_Divider = 4,
		Top = 8,
		Left = 0,
		Right = 256,
		Bottom = 232,
		ActiveScanlines = 240,
		PostRender = 1,
		VBlank = 20,
		OddSkip = 1
	};
	static inline const double GetFreq() { return 44.0 / 945000.0; }
	typedef apu::NTSC_Tables APUTables;
};

template <class _Bus, class _Settings>
struct EmptyPPU {
private:
	_Bus *Bus;
	enum {MAX_EVENTS = 1};
	SEventData LocalEvents[MAX_EVENTS];
	SEvent *EventChain[MAX_EVENTS];
	int Period;
public:
	EmptyPPU(_Bus *pBus) {
		SEvent *NewEvent;

		Bus = pBus;
		Period = _Settings::PPU_Divider *
			((_Settings::ActiveScanlines + _Settings::PostRender +
			_Settings::VBlank + 1) * 341);
		memset(LocalEvents, 0, sizeof(SEventData) * MAX_EVENTS);
		NewEvent = new SEvent;
		NewEvent->Data = &LocalEvents[0];
		NewEvent->Execute = [this]{
			Bus->GetClock()->SetEventTime(EventChain[0],
				Bus->GetClock()->GetTime() + Period);
		};
		EventChain[0] = NewEvent;
		Bus->GetClock()->RegisterEvent(NewEvent);
		Bus->GetClock()->EnableEvent(EventChain[0]);
		Bus->GetClock()->SetEventTime(EventChain[0], Period);
	}
	~EmptyPPU() {}

	inline bool IsFrameReady() { return true; }
	inline double GetFrameTime() { return Period; }
	inline void Reset() {}
	inline uint8 ReadByte(uint16 Address) { return 0x40; }
	inline void WriteByte(uint16 Address, uint8 Src) {}
	inline void ResetInternalClock(int Time) {}
	static inline const double GetFreq() { return _Settings::GetFreq(); }
};

template <class _Nes, class _Settings>
class CNESConfigTemplate: public CNESConfig {
private:
	ines::NES_ROM_Data *Data;
public:
	inline explicit CNESConfigTemplate(ines::NES_ROM_Data *pData) {
		Name = typeid(this).name();
		Width = _Settings::Right - _Settings::Left;
		Height = _Settings::Bottom - _Settings::Top;
		Frame = _Settings::GetFreq() * _Settings::PPU_Divider *
			((_Settings::ActiveScanlines + _Settings::PostRender +
			_Settings::VBlank + 1) * 341 - _Settings::OddSkip * 0.5);
		Data = pData;
	}
	inline ~CNESConfigTemplate() {}

	CBasicNES *GetNES(CNESFrontend *Frontend) {
		_Nes *NES;

		NES = new _Nes(Frontend);
		NES->GetBus().SetMMC(new typename _Nes::BusClass::MMCClass(&NES->GetBus(),
			Data));
		return NES;
	}
};

template <template<template<class> class,
                   template<class> class,
                   template<class> class,
                   template<class> class> class _Bus,
          template<class> class _ROM, class _Settings>
struct Std_NES_Config {
	template <class BusClass>
	class CPU: public CCPU<BusClass, _Settings> {
	public:
		inline explicit CPU(BusClass *pBus): CCPU<BusClass, _Settings>(pBus) {}
		inline ~CPU() {}
	};

	template <class BusClass>
	class APU: public CAPU<BusClass, _Settings> {
	public:
		inline explicit APU(BusClass *pBus): CAPU<BusClass, _Settings>(pBus) {}
		inline ~APU() {}
	};

	template <class BusClass>
	class PPU: public EmptyPPU<BusClass, _Settings> {
	public:
		inline explicit PPU(BusClass *pBus): EmptyPPU<BusClass, _Settings>(pBus) {}
		inline ~PPU() {}
	};

	typedef CNESConfigTemplate<CNES<_Bus<CPU, APU, PPU, _ROM> >, _Settings> Config;
};

/* Обработчик */
CNESConfig *vpnes::NROMHandler(ines::NES_ROM_Data *Data) {
	return new Std_NES_Config<CBus_Basic, mapper::MMCChain_Adapter<NROM_Chain>::MMCChain,
		NTSCSettings>::Config(Data);
}
