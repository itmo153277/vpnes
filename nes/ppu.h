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

#ifndef __PPU_H_
#define __PPU_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include <algorithm>
#include <cstring>
#include "manager.h"
#include "frontend.h"
#include "bus.h"

namespace vpnes {

namespace PPUID {

typedef PPUGroup<1>::ID::NoBatteryID InternalDataID;
typedef PPUGroup<2>::ID::NoBatteryID CycleDataID;
typedef PPUGroup<3>::ID::NoBatteryID RAM1ID;
typedef PPUGroup<4>::ID::NoBatteryID RAM2ID;
typedef PPUGroup<5>::ID::NoBatteryID PalMemID;
typedef PPUGroup<6>::ID::NoBatteryID OAMID;
typedef PPUGroup<7>::ID::NoBatteryID OAMSecID;

}

/* PPU */
template <class _Bus, class _Settings>
class CPPU: public CDevice {
public:
	/* Делитель частоты */
	enum { ClockDivider = _Settings::PPU_Divider };
	/* Параметры экрана */
	struct Screen {
		enum {
			Top = _Settings::Top,
			Left = _Settings::Left,
			Right = _Settings::Right,
			Bottom = _Settings::Bottom
		};
	};
private:
	/* Шина */
	_Bus *Bus;
	/* Палитра */
	uint8 *PalMem;
	/* Спрайты */
	uint8 *OAM;
	/* Видимые спрайты */
	uint8 *OAM_sec;
	/* Внутренние данные */
	struct SInternalData: public ManagerID<PPUID::InternalDataID> {
		/* Текущий адрес на шине */
		uint16 CurAddrLine;
		/* Рассчитанный адрес */
		uint16 CalcAddr;
		/* Адрес */
		uint16 Addr_v;
		/* Буфер адреса */
		uint16 Addr_t;
		/* Буфер для адреса PT */
		uint16 Reg_PT;
		/* Буфер для атрибутов */
		uint8 Reg_AT;
		/* Индекс для рендеринга */
		uint8 FHIndex;
		/* Флаг для 0x2005/0x2006 */
		bool Trigger;
		/* Флаги I/O через 0x2007 */
		bool Read2007;
		bool Write2007;
		/* Буфер для записи */
		uint8 Buf_Write;
		/* Буфер для чтения */
		uint8 Buf_Read;
		/* Статус PPU */
		bool VBlank;
		uint8 State;
		/* Указатель OAM */
		uint8 OAMIndex;
		/* Блокировка OAM */
		bool OAMLock;
		/* Буфер для OAM */
		uint8 OAMBuf;
		/* Страница для спрайтов */
		uint16 SpritePage;
		/* Инкремент 0x2007 */
		int IncrementVertical;
		/* Размер спрайтов */
		enum {
			Size8x8 = 8,
			Size8x16 = 16
		} SpriteSize;
		/* Генерировать NMI */
		int GenerateNMI;
		/* Игнорировать фазу */
		int Grayscale;
		/* Клиппинг */
		int BackgroundClip;
		int SpriteClip;
		/* Управление рендерером */
		int ShowBackground;
		int ShowSprites;
		/* Затемнение */
		int Tint;

		/* Бит для 0-object hit */
		inline void SetSprite0Hit() { State |= 0x40; }
		/* VBlank */
		inline void SetVBlank() { if (VBlank) State |= 0x80; VBlank = false; }
		inline int GetVBlank() { return State & 0x80; }
		inline void ClearVBlank() { State &= 0x7f; VBlank = false; }
		/* Переполнение */
		inline void SetOverflow() { State |= 0x20; }
		/* Получить статус */
		inline void ReadState() { Buf_Write = (Buf_Write & 0x1f) | State; }

		/* Запись 0x2000 */
		inline void Write2000(uint8 Src) {
			Addr_t = (Addr_t & 0x33ff) | ((Src & 0x03) << 10);
			IncrementVertical = Src & 0x04;
			SpritePage = (Src & 0x08) << 9;
			Reg_PT = (Reg_PT & 0x0fff) | ((Src & 0x10) << 8);
			SpriteSize = (Src & 0x20) ? Size8x16 : Size8x8;
			GenerateNMI = Src & 0x80;
		}
		/* Запись 0x2001 */
		inline void Write2001(uint8 Src) {
			Grayscale = Src & 0x01;
			BackgroundClip = Src & 0x02;
			SpriteClip = Src & 0x04;
			ShowBackground = Src & 0x08;
			ShowSprites = Src & 0x10;
			Tint = Src >> 5;
		}
		/* Запись в 0x2005/1 */
		inline void Write2005_1(uint8 Src) {
			FHIndex = ~Src & 0x07;
			Addr_t = (Addr_t & 0x3fe0) | (Src >> 3);
		}
		/* Запись в 0x2005/2 */
		inline void Write2005_2(uint8 Src) {
			Addr_t = (Addr_t & 0x0c1f) | ((Src & 0x07) << 12) | ((Src & 0xf8) << 2);
		}
		/* Запись в 0x2006/1 */
		inline void Write2006_1(uint8 Src) {
			Addr_t = (Addr_t & 0x00ff) | ((Src & 0x3f) << 8);
		}
		/* Запись в 0x2006/2 */
		inline void Write2006_2(uint8 Src) {
			Addr_t = (Addr_t & 0x3f00) | Src;
		}
		/* Состояние рендеринга */
		inline bool RenderingEnabled() {
			return ShowBackground || ShowSprites;
		}
		/* Установить адрес для NT */
		inline void SetNTAddr() {
			CurAddrLine = (Addr_v & 0x0fff) | 0x2000;
		}
		/* Установить адрес для AT */
		inline void SetATAddr() {
			CurAddrLine = ((Addr_v >> 2) & 0x0007) | ((Addr_v >> 4) & 0x0038) |
				(Addr_v & 0x0c00) | 0x23c0;
		}
		/* Получить адрес для 0x2007 */
		inline uint16 Get2007Address() {
			return Addr_v & 0x3fff;
		}
		/* Инкременты */
		inline void IncrementX() {
			if ((Addr_v & 0x001f) == 0x001f) /* HT Overflow */
				Addr_v = (Addr_v & 0x7fe0) ^ 0x0400;
			else
				Addr_v++;
		}
		inline void IncrementY() {
			if ((Addr_v & 0x73e0) == 0x73a0) { /* 240 VT overflow */
				Addr_v = (Addr_v & 0x0c1f) ^ 0x0800;
			} else if ((Addr_v & 0x73e0) == 0x73e0) { /* 256 VT overflow */
				Addr_v &= 0x0c1f;
			} else if ((Addr_v & 0x7000) == 0x7000) { /* FV overflow */
				Addr_v &= 0x0fe0;
				Addr_v += 0x0020;
			} else
				Addr_v += 0x1000;
		}
		inline void Increment2007() {
			if (IncrementVertical)
				Addr_v += 0x0020;
			else
				Addr_v++;
			Addr_v &= 0x7fff;
		}
		inline void UpdateScroll() {
			Addr_v = (Addr_v & 0xfbe0) | (Addr_t & 0x041f);
		}
	} InternalData;

	/* Данные о тактах */
	struct SCycleData: public ManagerID<PPUID::CycleDataID> {
		bool OddFrame; /* Нечетность кадра */
		int Cycles; /* Всего тактов в этом кадре */
		int ProcessedCycles; /* Обработано тактов для кадра */
		int FrameCycles; /* Полный кадр */
		int Cycle2006; /* Такт обработки 0x2006 */
		int Cycle2007; /* Такт обработки 0x2007 */
		int Clock2007; /* Текущая фаза обработки 0x2007 */
		int CurCycle; /* Текущий такт */
		int TotalCycles; /* Всего тактов */
		int Scanline; /* Текущий сканлайн */
		int hClock; /* Текушая точка */
		int hStart; /* Начало сканлайна */

		/* Вычесть такты */
		inline void SubstractCycles() {
			Cycles -= FrameCycles * ClockDivider;
			ProcessedCycles -= FrameCycles * ClockDivider;
			CurCycle -= FrameCycles;
			hStart -= FrameCycles;
			if (Cycle2006 > 0)
				Cycle2006 -= FrameCycles;
			if (Cycle2007 > 0)
				Cycle2007 -= FrameCycles;
		}
		/* Видимая часть */
		inline bool InRender() {
			return Scanline < _Settings::ActiveScanlines;
		}
	} CycleData;

	/* Обработка кадра завершена */
	bool FrameReady;

	/* Вывод точки */
	inline void OutputPixel(int x, int y, int Colour) {
		if ((x >= Screen::Left) && (y >= Screen::Top) &&
			(x < Screen::Right) && (y < Screen::Bottom)) {
			Bus->GetFrontend()->GetVideoFrontend()->PutPixel(x - Screen::Left,
				y - Screen::Top, Colour, InternalData.Tint);
		}
	}

	/* Палитра */
	inline uint8 ReadPalette(uint16 Address) {
		if ((Address & 0x0003) == 0)
			return PalMem[Address & 0x000c];
		return PalMem[Address & 0x001f];
	}
	inline void WritePalette(uint16 Address, uint8 Src) {
		if ((Address & 0x0003) == 0)
			PalMem[Address & 0x000c] = Src;
		else
			PalMem[Address & 0x001f] = Src;
	}

	/* Рендеринг */
	inline void Render(int Cycles = 0, bool CE = false);
	/* Рендеринг фона */
	inline void RenderBG(int Cycles);
	/* Рендеринг спрайтов */
	inline void RenderSprites();

	/* Рендеринг точки */
	inline void RenderPixel();
	/* Инкремент адреса */
	inline void IncrementAddress(bool Flag = true);
	/* Обновление шины адреса */
	inline void UpdateAddressBus();
	/* Фетчинг */
	inline uint8 FetchCycle();
	/* Фетчинг спрайтов */
	inline void FetchSprite();
	/* Фетчинг фона */
	inline void FetchBackground();
public:
	inline explicit CPPU(_Bus *pBus) {
		Bus = pBus;
		Bus->GetSolderPad()->Screen1 = (uint8 *)
			Bus->GetManager()->template GetPointer<ManagerID<PPUID::RAM1ID> >(\
				0x0400 * sizeof(uint8));
		memset(Bus->GetSolderPad()->Screen1, 0x00, 0x0400 * sizeof(uint8));
		Bus->GetSolderPad()->Screen2 = (uint8 *)
			Bus->GetManager()->template GetPointer<ManagerID<PPUID::RAM2ID> >(\
				0x0400 * sizeof(uint8));
		memset(Bus->GetSolderPad()->Screen2, 0x00, 0x0400 * sizeof(uint8));
		PalMem = (uint8 *)
			Bus->GetManager()->template GetPointer<ManagerID<PPUID::PalMemID> >(\
				0x20 * sizeof(uint8));
		memset(PalMem, 0xff, 0x20 * sizeof(uint8));
		OAM = (uint8 *)
			Bus->GetManager()->template GetPointer<ManagerID<PPUID::OAMID> >(\
				0x0100 * sizeof(uint8));
		memset(OAM, 0xff, 0x0100 * sizeof(uint8));
		OAM_sec = (uint8 *)
			Bus->GetManager()->template GetPointer<ManagerID<PPUID::OAMSecID> >(\
				0x40 * sizeof(uint8));
		memset(OAM_sec, 0xff, 0x40 * sizeof(uint8));
		Bus->GetManager()->template SetPointer<SInternalData>(&InternalData);
		Bus->GetManager()->template SetPointer<SCycleData>(&CycleData);
	}
	inline ~CPPU() {}

	/* Обработать такты */
	inline void Clock(int Cycles) {
		FrameReady = false;
		Render(Cycles);
		CycleData.Cycles += Cycles;
		if (FrameReady)
			CycleData.SubstractCycles();
	}

	/* Предобработка до текущего CE */
	inline void PreRenderBeforeCERise() {
		Render(Bus->GetCPU()->GetCyclesBeforeCERise(), false);
	}
	/* Предобработка до конца CE */
	inline void PreRenderBeforeCEFall() {
		Render(Bus->GetCPU()->GetCyclesBeforeCEFall(), true);
	}
	/* Предобработка полного цикла записи CPU */
	inline void PreRender() {
		PreRenderBeforeCERise();
		PreRenderBeforeCEFall();
	}

	/* Текущий такт (относительно CPU) */
	inline int GetCycles() {
		return std::min(CycleData.ProcessedCycles, (CycleData.hStart + CycleData.hClock) *
			ClockDivider) - CycleData.Cycles;
	}

	/* Сброс */
	inline void Reset() {
		memset(&CycleData, 0, sizeof(CycleData));
		CycleData.Cycle2006 = -2;
		CycleData.Cycle2007 = -2;
		memset(&InternalData, 0, sizeof(InternalData));
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		uint16 Addr;

		switch (Address & 0x0007) {
			case 0x0002: /* Статус PPU */
				InternalData.Trigger = false;
				PreRenderBeforeCERise();
				InternalData.ReadState();
				InternalData.ClearVBlank();
				break;
			case 0x0004: /* Чтение OAM */
				PreRenderBeforeCERise();
				RenderSprites();
				if (InternalData.OAMLock)
					InternalData.Buf_Write = InternalData.OAMBuf;
				else
					InternalData.Buf_Write = OAM[InternalData.OAMIndex];
				break;
			case 0x0007: /* PPU RAM I/O */
				PreRenderBeforeCERise();
				Addr = InternalData.Get2007Address();
				if (Addr >= 0x3f00)
					InternalData.Buf_Write = ReadPalette(Addr);
				else
					InternalData.Buf_Write = InternalData.Buf_Read;
				PreRenderBeforeCEFall();
				InternalData.Read2007 = true;
				CycleData.Cycle2007 = CycleData.CurCycle;
				break;
		}
		PreRenderBeforeCEFall();
		return InternalData.Buf_Write;
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		bool OldNMI;
		uint8 Buf;
		uint16 Addr;

		PreRenderBeforeCERise();
		InternalData.Buf_Write = Src;
		switch (Address & 0x0007) {
			case 0x0000: /* Управляющий регистр 1 */
				OldNMI = !InternalData.GenerateNMI;
				InternalData.Write2000(Src);
				PreRenderBeforeCEFall();
				if (InternalData.GenerateNMI && OldNMI && InternalData.GetVBlank())
					Bus->GetCPU()->GenerateNMI(GetCycles());
				break;
			case 0x0001: /* Управляющий регистр 2 */
				InternalData.Write2001(Src);
				PreRenderBeforeCEFall();
				UpdateAddressBus();
				break;
			case 0x0003: /* Установить указатель OAM */
				PreRenderBeforeCEFall();
				RenderSprites();
				InternalData.OAMIndex = Src;
				break;
			case 0x0004: /* Записать байт OAM */
				PreRenderBeforeCEFall();
				RenderSprites();
				if (!InternalData.OAMLock)
					Buf = InternalData.OAMBuf;
				else if ((InternalData.OAMIndex & 0x03) == 0x02)
					Buf = Src & 0x3e;
				else
					Buf = Src;
				OAM[InternalData.OAMIndex] = Buf;
				break;
			case 0x0005: /* Скорллинг */
				InternalData.Trigger = !InternalData.Trigger;
				if (InternalData.Trigger)
					InternalData.Write2005_1(Src);
				else
					InternalData.Write2005_2(Src);
				break;
			case 0x0006: /* Установить адрес PPU RAM */
				InternalData.Trigger = !InternalData.Trigger;
				if (InternalData.Trigger)
					InternalData.Write2006_1(Src);
				else
					InternalData.Write2006_2(Src);
				PreRenderBeforeCEFall();
				CycleData.Cycle2006 = CycleData.CurCycle;
				break;
			case 0x0007: /* PPU RAM I/O */
				Addr = InternalData.Get2007Address();
				if (Addr >= 0x3f00)
					WritePalette(Addr, Src);
				PreRenderBeforeCEFall();
				if (Addr < 0x3f00)
					InternalData.Write2007 = true;
				CycleData.Cycle2007 = CycleData.CurCycle;
				break;
		}
		PreRenderBeforeCEFall();
	}

	/* Флаг окончания рендеринга фрейма */
	inline const bool &IsFrameReady() const { return FrameReady; }
	/* Длина фрейма */
	inline const int GetFrameCycles() const { return CycleData.FrameCycles * ClockDivider; }
};


/* Инкремент адреса */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>::IncrementAddress(bool Flag) {
	if (InternalData.RenderingEnabled() && CycleData.InRender()) {
		InternalData.IncrementX();
		if (Flag)
			InternalData.IncrementY();
	} else
		InternalData.Increment2007();
}

/* Обновление шины адреса */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>::UpdateAddressBus() {
	if (InternalData.RenderingEnabled() && CycleData.InRender()) {
		switch (CycleData.hClock & 6) {
			case 0:
				InternalData.SetNTAddr();
				break;
			case 2:
				if (((CycleData.hClock < 256) && (CycleData.Scanline != -1)) ||
					((CycleData.hClock >= 320) && (CycleData.hClock < 336)))
					InternalData.SetATAddr();
				else
					InternalData.SetNTAddr();
				break;
			case 4:
				InternalData.CurAddrLine = InternalData.CalcAddr & 0x3ff7;
				break;
			case 6:
				InternalData.CurAddrLine = InternalData.CalcAddr | 0x0008;
				break;
		}
	} else
		InternalData.CurAddrLine = InternalData.Get2007Address();
	Bus->GetROM()->UpdatePPUAddress(InternalData.CurAddrLine);
}

/* Рендеринг */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>::Render(int Cycles, bool CE) {

#define WAIT_CYCLE(Cycle) \
	if ((CycleData.CurCycle < Cycle) && (NextCycle > Cycle)) \
		NextCycle = Cycle;

	int NextCycle, LastCycle;

	LastCycle = (Cycles + CycleData.Cycles) / ClockDivider;
	while (CycleData.CurCycle < LastCycle) {
		NextCycle = LastCycle;
		WAIT_CYCLE(CycleData.Cycle2006 + 1) /* Обновление адреса после 0x2006/2 */
		WAIT_CYCLE(CycleData.Cycle2007 + 1) /* I/O после 0x2007 */
		RenderBG(NextCycle - CycleData.CurCycle);
		if (CycleData.CurCycle == (CycleData.Cycle2007 + 1)) {
			CycleData.Cycle2007++;
			NextCycle = CE ? (CycleData.Clock2007 + 2) : (CycleData.Clock2007 + 1);
			if ((CycleData.Clock2007 <= 2) && (NextCycle > 2)) {
				if (InternalData.Write2007) {
					InternalData.Write2007 = false;
					if (!InternalData.RenderingEnabled() || !CycleData.InRender())
						Bus->WritePPUMemory(InternalData.CurAddrLine,
							InternalData.Buf_Write);
				}
				if (InternalData.Read2007) {
					InternalData.Read2007 = false;
					if (!InternalData.RenderingEnabled() || !CycleData.InRender())
						InternalData.Buf_Read =
							Bus->ReadPPUMemory(InternalData.CurAddrLine);
				}
			}
			if ((CycleData.Clock2007 <= 3) && (NextCycle > 3) &&
				!(InternalData.RenderingEnabled() && (CycleData.hClock == 256) &&
				(CycleData.InRender())))
				IncrementAddress();
			if (NextCycle > 4) {
				UpdateAddressBus();
				CycleData.Clock2007 = 0;
				CycleData.Cycle2007 = -2;
			} else
				CycleData.Clock2007 = NextCycle;
		}
		if (CycleData.CurCycle == (CycleData.Cycle2006 + 1)) {
			InternalData.Addr_v = InternalData.Addr_t;
			UpdateAddressBus();
			CycleData.Cycle2006 = -2;
		}
	}

#undef WAIT_CYCLE

}


/* Рендеринг фона */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>::RenderBG(int Cycles) {

#define WAIT_CYCLE(_n) if (hCycles <= (_n)) continue;\
	if (CycleData.hClock == (_n))
#define WAIT_CYCLE_RANGE(_a, _b) if (hCycles <= (_a)) continue;\
	if (CycleData.hClock < (_b))

	int hCycles, Count;

	CycleData.CurCycle += Cycles;
	CycleData.TotalCycles += Cycles;
	CycleData.ProcessedCycles = CycleData.CurCycle * ClockDivider;
	hCycles = CycleData.CurCycle - CycleData.hStart;
	while (hCycles > CycleData.hClock) {
		if (InternalData.RenderingEnabled()) {
			/* Видимые пиксели */
			if (CycleData.hClock < 256) {
				Count = std::min(256, hCycles) - CycleData.hClock;
				while (Count > 0) {
					FetchBackground();
					//DrawPixel();
					Count--;
					CycleData.hClock++;
				}
			}
			WAIT_CYCLE(256) {
				RenderSprites();
				InternalData.UpdateScroll();
			}
			/* Фетчинг спрайтов */
			if (CycleData.hClock < 320) {
				if ((CycleData.Scanline == -1) && (CycleData.hClock < 304)
					&& (hCycles > 279))
					InternalData.Addr_v = InternalData.Addr_t;
				Count = std::min(320, hCycles) - CycleData.hClock;
				while (Count > 0) {
					FetchSprite();
					Count--;
					CycleData.hClock++;
				}
			}
			/* Фетчинг фона следующего сканлайна */
			WAIT_CYCLE_RANGE(320, 336) {
				Count = std::min(336, hCycles) - CycleData.hClock;
				while (Count > 0) {
					FetchBackground();
					Count--;
					CycleData.hClock++;
				}
			}
			WAIT_CYCLE_RANGE(336, 340) {
				Count = std::min(340, hCycles) - CycleData.hClock;
				while (Count > 0) {
					if ((CycleData.Scanline == -1) && (CycleData.hClock == 337) &&
						CycleData.OddFrame)
						_Settings::SkipPPUClock(hCycles, CycleData.hStart);
					FetchCycle();
					Count--;
					CycleData.hClock++;
				}
			}
		} else if (CycleData.hClock < 340)
			CycleData.hClock = std::min(340, hCycles);
		WAIT_CYCLE(340) {
			CycleData.Scanline++;
			if (CycleData.InRender()) {
				CycleData.hClock = 0;
				hCycles -= 341;
				CycleData.hStart += 341;
				continue;
			}
			CycleData.FrameCycles = CycleData.TotalCycles - (hCycles - 341);
			CycleData.TotalCycles = hCycles - 341;
			FrameReady = true;
			CycleData.OddFrame = !CycleData.OddFrame;
			CycleData.hClock = 341 + 340;
			hCycles -= (_Settings::PostRender - 1) * 341;
			CycleData.hStart += (_Settings::PostRender - 1) * 341;
		}
		WAIT_CYCLE(341) {
			InternalData.State = 0;
			CycleData.Scanline = -1;
			CycleData.hClock = 0;
			CycleData.hStart += 341;
			hCycles -= 341;
			continue;
		}
		WAIT_CYCLE(341 + 340) {
			InternalData.VBlank = true;
			CycleData.hClock = 341 + 341;
		}
		WAIT_CYCLE(341 + 341) {
			InternalData.SetVBlank();
			CycleData.hClock = 341 + 343;
		}
		WAIT_CYCLE(341 + 343) {
			if (InternalData.GenerateNMI && InternalData.GetVBlank())
				Bus->GetCPU()->GenerateNMI(GetCycles());
			CycleData.hClock = 341;
			CycleData.hStart += (_Settings::VBlank + 1) * 341;
			hCycles -= (_Settings::VBlank + 1) * 341;
		}
	}
	
#undef WAIT_CYCLE
#undef WAIT_CYCLE_RANGE

}

/* Рендеринг спрайтов */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>::RenderSprites() {
}

/* Фетчинг */
template <class _Bus, class _Settings>
uint8 CPPU<_Bus, _Settings>::FetchCycle() {
	return 0x00;
}

/* Фетчинг спрайтов */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>:: FetchSprite() {
}

/* Фетчинг фона */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>::FetchBackground() {
}

}

#endif
