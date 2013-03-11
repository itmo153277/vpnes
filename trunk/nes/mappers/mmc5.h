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

#ifndef __MMC5_H_
#define __MMC5_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../../types.h"

#include "../ines.h"
#include "../mapper.h"
#include "nrom.h"

namespace vpnes {

namespace MMC5ID {

typedef MapperGroup<'E'>::Name<1>::ID::NoBatteryID InternalDataID;

}

/* Реализация маппера 5 */
template <class _Bus>
class CMMC5: public CNROM<_Bus> {
	using CNROM<_Bus>::Bus;
	using CNROM<_Bus>::CHR;
	using CNROM<_Bus>::ROM;
public:
	typedef DynamicSolderPad<_Bus> SolderPad;
private:
	/* Внутренник данные */
	struct SInternalData: public ManagerID<MMC5ID::InternalDataID> {
		/* Защита памяти */
		uint8 EnableWrite;
		/* Режим переключения банков PRG */
		enum {
			PRGSwitch_One, /* One 32k bank */
			PRGSwitch_Two, /* Two 16k banks */
			PRGSwitch_Three, /* One 16k bank 0x8000-0xbfff and two 8k banks */
			PRGSwitch_Four /* Four 8k banks */
		} PRGSwitch;
		/* Режим переключения банков CHR */
		enum {
			CHRSwitch_8k,
			CHRSwitch_4k,
			CHRSwitch_2k,
			CHRSwitch_1k
		} CHRSwitch;
		/* Следим за спрайтами */
		int SpriteWatch;
		/* Следим за состоянием рендера */
		int RenderWatch;
		/* Флаг фрема */
		bool InFrame;
		/* Счетчик сканлайнов */
		int Scanline;
		/* Фаза рендеринга */
		int RenderPhase;
		/* Следим за фетчами PPU */
		enum {
			PPUFetch_BG,
			PPUFetch_SP
		} PPUFetch;
		/* IRQ */
		bool IRQEnabled;
		bool IRQPending;
		int IRQScanline;
	} InternalData;
public:
	inline explicit CMMC5(_Bus *pBus, const ines::NES_ROM_Data *Data):
		CNROM<_Bus>(pBus, Data) {
		/* Обратная ссылка */
		Bus->GetSolderPad()->Bus = pBus;
		Bus->GetManager()->template SetPointer<SInternalData>(&InternalData);
		memset(&InternalData, 0, sizeof(InternalData));
		InternalData.Scanline = -1;
	}

	/* Управление памятью PPU */
	inline uint8 NTRead(uint16 Address) {
		return static_cast<StaticSolderPad *>(Bus->GetSolderPad())->ReadPPUAddress(Address);
	}
	inline void NTWrite(uint16 Address, uint8 Src) {
		static_cast<StaticSolderPad *>(Bus->GetSolderPad())->WritePPUAddress(Address, Src);
	}

	/* Обновление адреса на шине CPU — следим за записью */
	inline void UpdateCPUBus(uint16 Address) {
	}
	inline void UpdateCPUBus(uint16 Address, uint8 Src) {
		switch (Address & 0xf007) { /* Следим за PPU */
			case 0x2000:
				InternalData.SpriteWatch = Src & 0x20;
				break;
			case 0x2001:
				InternalData.RenderWatch = Src & 0x18;
				if (!InternalData.RenderWatch) { /* Сбрасываем флаги мгновенно */
					InternalData.InFrame = false;
					InternalData.Scanline = -1;
					InternalData.RenderPhase = 0;
				}
				break;
		}
	}
	/* Обновление адреса на шине PPU — следим за чтением */
	inline void UpdatePPUBus(uint16 Address) {
		if (!InternalData.RenderWatch)
			return;
		InternalData.InFrame = true; /* Попали на считывание — начало фрейма */
		if ((InternalData.RenderPhase == 0) &&
			(InternalData.Scanline == InternalData.IRQScanline)) {
			InternalData.IRQPending = true;
			if (InternalData.IRQEnabled)
				Bus->GetCPU()->GenerateIRQ(Bus->GetPPU()->GetCycles() *
					_Bus::PPUClass::ClockDivider);
		}
		InternalData.RenderPhase++;
		if (InternalData.RenderPhase <= 128) {
			InternalData.PPUFetch = SInternalData::PPUFetch_BG;
			return;
		}
		if (InternalData.RenderPhase <= (128 + 16)) {
			InternalData.PPUFetch = SInternalData::PPUFetch_SP;
			return;
		} else
			InternalData.PPUFetch = SInternalData::PPUFetch_BG;
		if (InternalData.RenderPhase == (128 + 16 + 8)) {
			InternalData.RenderPhase = 0; /* Конец сканлайна */
			InternalData.Scanline++;
			if (InternalData.Scanline == 240) { /* Начало VBlank */
				InternalData.InFrame = false;
				InternalData.Scanline = -1;
				return;
			}
		}
	}
	inline void UpdatePPUBus(uint16 Address, uint8 Src) {
	}
};

}

#endif
