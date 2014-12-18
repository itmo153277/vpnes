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

#include "manager.h"
#include "frontend.h"
#include "clock.h"
#include "bus.h"
#include "cpu.h"

namespace vpnes {

namespace PPUID {

typedef PPUGroup<1>::ID::NoBatteryID InternalDataID;
typedef PPUGroup<2>::ID::NoBatteryID StateID;
typedef PPUGroup<3>::ID::NoBatteryID RegistersID;
typedef PPUGroup<5>::ID::NoBatteryID EventDataID;
typedef PPUGroup<6>::ID::NoBatteryID SpriteRendererID;
typedef PPUGroup<7>::ID::NoBatteryID BackgroundRendererID;
typedef PPUGroup<8>::ID::NoBatteryID BusHandlerID;
typedef PPUGroup<9>::ID::NoBatteryID PalMemID;

}

/* PPU */
template <class _Bus, class _Settings>
class CPPU {
public:
	enum { ClockDivider = _Settings::PPU_Divider };
private:
	/* Шина */
	_Bus *Bus;

	/* Флаг готовности кадра */
	bool FrameReady;
	/* Время кадра */
	int FrameTime;
	/* Номинальное время отрисовки кадра */
	int NominalFrameTime;
	/* Номинальное время отрисовки для нечетных кадров */
	int NominalFrameTimeOdd;
	/* Chip Enable */
	bool CE;
	/* Палитра */
	uint8 *PalMem;

	/* Внутренние данные */
	struct SInternalData: public ManagerID<PPUID::InternalDataID> {
		int Scanline;
		int OddFrame;
		int IRQOffset;
		int InternalClock;
		int ScanlineClock;
		int FrameTime;
	} InternalData;

	/* События */
	enum {
		EVENT_PPU_FRAME = 0,
		MAX_EVENTS
	};
	/* Данные о событиях */
	SEventData LocalEvents[MAX_EVENTS];
	/* Указатели на события */
	SEvent *EventChain[MAX_EVENTS];

	/* Палитра */
	inline uint8 ReadPalette(uint16 Address) {
		if ((Address & 0x0003) == 0)
			return PalMem[Address & 0x000c];
		return PalMem[Address & 0x001f];
	}
	inline void WritePalette(uint16 Address, uint8 Src) {
		if ((Address & 0x0003) == 0)
			PalMem[Address & 0x000c] = Src & 0x3f;
		else
			PalMem[Address & 0x001f] = Src & 0x3f;
	}

	/* Статус PPU */
	struct SState: public ManagerID<PPUID::StateID> {
		uint8 State;
		/* Подавление VBlank */
		bool SupressVBlank;
		/* 0-object */
		inline void SetSprite0Hit() { State |= 0x40; }
		/* Set VBlank */
		inline void SetVBlank() { if (!SupressVBlank) State |= 0x80; }
		/* Get VBlank */
		inline bool GetVBlank() { return State & 0x80; }
		/* Claer VBlank */
		inline void ClearVBlank() { State &= 0x7f; SupressVBlank = true; }
		/* Overflow */
		inline void SetOverflow() { State |= 0x20; }
		/* Сброс состояния */
		inline void ClearState() { State = 0; }
		/* Получаем байт по маске */
		inline uint8 ReadState(uint8 Src) { return State | (Src & 0x1f); }
	} State;

	/* Внутренние регистры */
	struct SRegisters: public ManagerID<PPUID::RegistersID> {
		/* Текущий адрес на шине */
		uint16 CurAddrLine;
		/* Рассчитанный адрес */
		uint16 CalcAddr;
		/* Байт NT */
		uint8 NTByte;
		/* Регистр адреса (Loopy_V) */
		uint16 Addr_v;
		/* Буфер адреса (Loopy_T) */
		uint16 Addr_t;
		/* Указатель на палитру */
		int PalAddr;
		/* Страница фона */
		uint16 BGPage;
		/* Индекс для рендеринга */
		uint8 FHIndex;
		/* Флаг 0x2005/0x2006 */
		bool Trigger;
		/* Страница спрайтов */
		uint16 SpritePage;
		/* Тип инкремента 0x2007 */
		int IncrementVertical;
		/* Размер спрайтов */
		enum {
			Size8x8 = 8,
			Size8x16 = 16
		} SpriteSize;
		/* Генерировать NMI */
		bool GenerateNMI;
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
		/* Буфер IO */
		uint8 BufIO;

		/* Состояние рендерера */
		inline bool RenderingEnabled() {
			return ShowBackground || ShowSprites;
		}
		/* Запись 0x2000 */
		inline void Write2000(uint8 Src) {
			Addr_t = (Addr_t & 0x73ff) | ((Src & 0x03) << 10);
			IncrementVertical = Src & 0x04;
			SpritePage = (Src & 0x08) << 9;
			BGPage = (Src & 0x10) << 8;
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
		/* Запись 0x2005 */
		inline void Write2005(uint8 Src) {
			Trigger = !Trigger;
			if (Trigger) {
				FHIndex = 0x10 | ((~Src & 0x07) << 1);
				Addr_t = (Addr_t & 0x7fe0) | (Src >> 3);
			} else
				Addr_t = (Addr_t & 0x0c1f) | ((Src & 0x07) << 12) | ((Src & 0xf8) << 2);
		}
		/* Запись в 0x2006 */
		inline void Write2006(uint8 Src) {
			Trigger = !Trigger;
			if (Trigger)
				Addr_t = (Addr_t & 0x00ff) | ((Src & 0x3f) << 8);
			else
				Addr_t = (Addr_t & 0x7f00) | Src;
		}
		/* Чтение Nametable */
		inline void ReadNT() {
			CalcAddr = BGPage | (NTByte << 4) | (Addr_v >> 12);
		}
		/* Чтение атрибутов */
		inline uint8 ReadAT(uint8 Src) {
			return (Src >> (((Addr_v >> 4) & 0x0004) | (Addr_v & 0x0002))) & 0x03;
		}
		/* Установить адрес для NT */
		inline void SetNTAddr() {
			CurAddrLine = (Addr_v & 0x0fff) | 0x2000;
			NTByte = 0x00;
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
		/* Получить адрес для спрайта */
		inline uint16 GetSpriteAddress(int Sprite, CPPU &PPU) {
			uint8 *SpriteOffset = PPU.SpriteRenderer.OAM_sec + (Sprite << 2);
			switch (SpriteSize) {
				case Size8x8:
					if (*(SpriteOffset) > 240)
						return SpritePage | 0x0ff0;
					if (*(SpriteOffset + 2) & 0x80)
						return SpritePage | (*(SpriteOffset + 1) << 4) | (7 +
							*(SpriteOffset) - PPU.InternalData.Scanline);
					else
						return SpritePage | (*(SpriteOffset + 1) << 4) |
							(PPU.InternalData.Scanline - *(SpriteOffset));
					break;
				case Size8x16:
					if (*(SpriteOffset) > 240)
						return 0x1ff0;
					if (*(SpriteOffset + 2) & 0x80) {
						if ((PPU.InternalData.Scanline - *(SpriteOffset)) > 7)
							return ((*(SpriteOffset + 1) & 0x01) << 12) |
								((*(SpriteOffset + 1) & 0xfe) << 4) | (15 +
								*(SpriteOffset) - PPU.InternalData.Scanline);
						else
							return ((*(SpriteOffset + 1) & 0x01) << 12) |
								((*(SpriteOffset + 1) | 0x01) << 4) | (7 +
								*(SpriteOffset) - PPU.InternalData.Scanline);
					} else {
						if ((PPU.InternalData.Scanline - *(SpriteOffset)) > 7)
							return ((*(SpriteOffset + 1) & 0x01) << 12) |
								((*(SpriteOffset + 1) | 0x01) << 4) |
								(PPU.InternalData.Scanline - *(SpriteOffset) + 8);
						else
							return ((*(SpriteOffset + 1) & 0x01) << 12) |
								((*(SpriteOffset + 1) & 0xfe) << 4) |
								(PPU.InternalData.Scanline - *(SpriteOffset));
					}
					break;
			}
			return 0x0000;
		}
		/* Цвет при отсутствии рендеринга */
		inline void SetDropColour() {
			if (CurAddrLine < 0x3f00)
				PalAddr = 0x00;
			if (CurAddrLine & 0x0003)
				PalAddr = CurAddrLine & 0x001f;
			else
				PalAddr = CurAddrLine & 0x000c;
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
				Addr_v &= 0x0fff;
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
			Addr_v = (Addr_v & 0x7be0) | (Addr_t & 0x041f);
		}
	} Registers;

	/* Устройство обработки для PPU */
	struct SPPUUnit {
		int InternalClock; /* Внутренние часы */

		/*Сброс часов */
		inline void ResetClock(int Time) {
			InternalClock -= Time;
		}
	};

	/* Рендерер спрайтов */
	struct SSpriteRenderer: public SPPUUnit, public ManagerID<PPUID::SpriteRendererID> {
		using SPPUUnit::InternalClock;
		bool PrimNext; /* 0-object на следующем сканлайне */
		bool Prim; /* 0-object на текущем сканлайне */
		struct SObjectData {
			uint8 Colour;
			uint8 Attrib;
			bool Prim;
		} Prerendered[256]; /* Данные об объектах на сканлайне */
		uint8 OAM_sec[32]; /* Буфер для объектов следующего сканлайна */

		/* Обработка */
		inline void Process(CPPU &PPU) {
			ProcessClocks(PPU, PPU.InternalData.InternalClock);
		}
		/* Обработка ко времени */
		bool ProcessClocks(CPPU &PPU, int Time) {
			if (InternalClock >= Time)
				return true;
			InternalClock = Time;
			return true;
		}
		/* Новый сканлайн */
		void NewScanline(CPPU &PPU) {
			int i, j, k;
			uint16 ShiftReg;

			if (PPU.InternalData.Scanline >= _Settings::ActiveScanlines)
				return;
			memset(Prerendered, 0, sizeof(SObjectData) * 256);
			i = 0;
			k = 0;
			do {
				if (OAM_sec[i] == 0xff) {
					i += 4;
					k++;
					continue;
				}
				if (OAM_sec[(i + 2) & 0x1f] & 0x40)
					ShiftReg = ppu::ColourTable[ppu::FlipTable[\
						PPU.BusHandler.Fetches[SBusHandler::FetchSprite + k].TileA]] |
						(ppu::ColourTable[ppu::FlipTable[\
						PPU.BusHandler.Fetches[SBusHandler::FetchSprite + k].TileB]] << 8);
				else
					ShiftReg = ppu::ColourTable[\
						PPU.BusHandler.Fetches[SBusHandler::FetchSprite + k].TileA] |
						(ppu::ColourTable[\
						PPU.BusHandler.Fetches[SBusHandler::FetchSprite + k].TileB] << 8);
				for (j = OAM_sec[(i + 3) & 0x1f]; (j < 256) && (j < OAM_sec[(i + 3) & 0x1f]);
					j++) {
					if (Prerendered[j].Colour == 0)
						Prerendered[j].Colour = ShiftReg & 0x03;
					ShiftReg >>= 2;
				}
				i += 4;
			} while (i < 32);
		}
	} SpriteRenderer;


	/* Рендерер фона */
	struct SBackgroundRenderer: public SPPUUnit,
		public ManagerID<PPUID::BackgroundRendererID> {
		using SPPUUnit::InternalClock;
		int x; /* Текущая координата */
		uint32 BGReg; /* Сдвиговый регистр для фона*/
		uint32 ARReg; /* Сдвиговый регистр для атрибутов */

		/* Получить адрес для палитры */
		inline uint8 GetPALAddress(uint8 Colour, uint8 Attribute) {
			if ((Colour & 0x03) == 0)
				return 0x00;
			else
				return ((Attribute & 0x03) << 2) | Colour;
		}
		/* Вывод точки */
		void OutputPixel(CPPU &PPU) {
			if ((x >= _Settings::Left) && (PPU.InternalData.Scanline >= _Settings::Top) &&
				(x < _Settings::Right) && (PPU.InternalData.Scanline < _Settings::Bottom)) {
				uint8 Colour = PPU.PalMem[PPU.Registers.PalAddr];

				if (PPU.Registers.Grayscale)
					Colour &= 0x30;
				PPU.Bus->GetFrontend()->GetVideoFrontend()->PutPixel(Colour,
					PPU.Registers.Tint);
			}
		}
		/* Рисование точки */
		void DrawPixel(CPPU &PPU) {
				uint8 Colour;

				if (PPU.Registers.ShowBackground && (PPU.Registers.BackgroundClip ||
					(x > 7))) {
					Colour = (BGReg >> PPU.Registers.FHIndex) & 0x03;
					PPU.Registers.PalAddr = GetPALAddress(Colour, ARReg >>
						PPU.Registers.FHIndex);
				} else {
					Colour = 0;
					PPU.Registers.PalAddr = 0;
				}
				if (PPU.SpriteRenderer.Prerendered[x].Colour != 0 &&
					PPU.Registers.ShowSprites && (PPU.Registers.SpriteClip || (x > 7))) {
					if (PPU.SpriteRenderer.Prerendered[x].Prim &&
						(Colour != 0) && (x < 255)) {
						PPU.State.SetSprite0Hit();
					}
					if ((Colour == 0) || (~PPU.SpriteRenderer.Prerendered[x].Attrib & 0x20))
						PPU.Registers.PalAddr = GetPALAddress(\
							PPU.SpriteRenderer.Prerendered[x].Colour | 0x10,
							PPU.SpriteRenderer.Prerendered[x].Attrib);
				}
		}
		/* Внутренний обработчик */
		int Execute(CPPU &PPU, int Time) {
			int TimeLeft = Time;

			if (PPU.InternalData.Scanline >= _Settings::ActiveScanlines)
				return Time - Time % ClockDivider;
			while ((TimeLeft >= ClockDivider) && (x < 256)) {
				if (PPU.Registers.RenderingEnabled())
					DrawPixel(PPU);
				OutputPixel(PPU);
				TimeLeft -= ClockDivider;
				BGReg <<= 2;
				ARReg <<= 2;
				if ((x & 7) == 7) {
					BGReg |= ppu::ColourTable[PPU.BusHandler.Fetches[\
						(x >> 3)].TileA] | (ppu::ColourTable[PPU.BusHandler.Fetches[\
						(x >> 3)].TileB] << 1);
					ARReg |= ppu::AttributeTable[PPU.BusHandler.Fetches[\
						(x >> 3)].Attr];
				}
				x++;
			}
			if ((x >= 256) && (TimeLeft >= ClockDivider)) {
				int Count = TimeLeft / ClockDivider;

				x += Count;
				TimeLeft -= Count * ClockDivider;
			}
			return Time - TimeLeft;
		}
		/* Обработка */
		inline void Process(CPPU &PPU) {
			ProcessClocks(PPU, PPU.InternalData.InternalClock);
		}
		/* Обработка ко времени */
		bool ProcessClocks(CPPU &PPU, int Time) {
			if (InternalClock >= Time)
				return true;
			if (Time > PPU.BusHandler.InternalClock)
				PPU.BusHandler.ProcessClocks(PPU, Time);
			InternalClock += Execute(PPU, Time - InternalClock);
			return true;
		}
		/* Новый сканлайн */
		void NewScanline(CPPU &PPU) {
			if (PPU.InternalData.Scanline >= _Settings::ActiveScanlines)
				return;
			x = 0;
			BGReg = ((ppu::ColourTable[PPU.BusHandler.Fetches[\
				SBusHandler::FetchBG2].TileA] | (ppu::ColourTable[PPU.BusHandler.Fetches[\
				SBusHandler::FetchBG2].TileB] << 1)) << 16) |
				ppu::ColourTable[PPU.BusHandler.Fetches[\
				SBusHandler::FetchBG2 + 1].TileA] |	(ppu::ColourTable[\
				PPU.BusHandler.Fetches[SBusHandler::FetchBG2 + 1].TileB] << 1);
			ARReg = (ppu::AttributeTable[PPU.BusHandler.Fetches[\
				SBusHandler::FetchBG2].Attr] << 16) | ppu::AttributeTable[\
				PPU.BusHandler.Fetches[SBusHandler::FetchBG2 + 1].Attr];
		}
	} BackgroundRenderer;

	/* Обработчик шины */
	struct SBusHandler: public SPPUUnit, public ManagerID<PPUID::BusHandlerID> {
		using SPPUUnit::InternalClock;
		enum {
			FetchBG = 0,
			FetchSprite = 32,
			FetchScrollStart = 35,
			FetchScrollEnd = 38,
			FetchBG2 = 40,
			FetchMAX = 43
		}; /* Карта массива */
		struct SFetch {
			uint8 Nametable;
			uint8 Attr;
			uint8 TileA;
			uint8 TileB;
		} Fetches[FetchMAX]; /* Массив вытянутых данных */
		int FetchPos; /* Позиция в массиве */
		int FetchCycle; /* Текущий такт */
		bool FixScroll; /* Обновление скроллинга */

		/* Внутренний обработчик */
		int Execute(CPPU &PPU, int Time) {
			int TimeLeft = Time;

			/* Тут должна быть обработка 0x2006/0x2007 */
			if (PPU.InternalData.Scanline >= _Settings::ActiveScanlines ||
				!PPU.Registers.RenderingEnabled()) {
				int Cycles = Time / ClockDivider;

				if (PPU.InternalData.Scanline < _Settings::ActiveScanlines) {
					FetchPos += (FetchCycle + Cycles) >> 3;
					FetchCycle = (FetchCycle + Cycles) & 7;
					if (PPU.InternalData.Scanline == -1)
						FixScroll = (FetchPos > FetchScrollStart) &&
							(FetchPos < FetchScrollEnd);
				}
				if (Cycles > 0) {
					PPU.Registers.CurAddrLine = PPU.Registers.Get2007Address();
					PPU.Registers.SetDropColour();
				}
				return Cycles * ClockDivider;
			}
			while (TimeLeft >= ClockDivider) {
				switch (FetchCycle) {
					case 0:
						if (FetchPos == FetchSprite)
							PPU.Registers.UpdateScroll();
						PPU.Registers.SetNTAddr();
						break;
					case 1:
						Fetches[FetchPos].Nametable = PPU.Bus->ReadPPU(\
							PPU.Registers.CurAddrLine);
						break;
					case 2:
						if (FetchPos < FetchSprite || FetchPos >= FetchBG2)
							PPU.Registers.SetATAddr();
						else
							PPU.Registers.SetNTAddr();
						break;
					case 3:
						Fetches[FetchPos].Attr = PPU.Bus->ReadPPU(\
							PPU.Registers.CurAddrLine);
						break;
					case 4:
						if (FetchPos < FetchSprite || FetchPos >= FetchBG2)
							PPU.Registers.ReadNT();
						else
							PPU.Registers.CalcAddr = PPU.Registers.GetSpriteAddress(\
								FetchPos - FetchSprite, PPU);
						PPU.Registers.CurAddrLine = PPU.Registers.CalcAddr & 0x3ff7;
						break;
					case 5:
						Fetches[FetchPos].TileA = PPU.Bus->ReadPPU(\
							PPU.Registers.CurAddrLine);
						break;
					case 6:
						if (FetchPos < FetchSprite || FetchPos >= FetchBG2)
							PPU.Registers.ReadNT();
						else
							PPU.Registers.CalcAddr = PPU.Registers.GetSpriteAddress(\
								FetchPos - FetchSprite, PPU);
						PPU.Registers.CurAddrLine = PPU.Registers.CalcAddr | 0x0008;
						break;
					case 7:
						Fetches[FetchPos].TileB = PPU.Bus->ReadPPU(\
							PPU.Registers.CurAddrLine);
						break;
				};
				FetchCycle++;
				if (FetchCycle == 8) {
					FetchPos++;
					FetchCycle = 0;
					PPU.Registers.IncrementX();
					if (FetchPos == FetchSprite)
						PPU.Registers.IncrementY();
					if (PPU.InternalData.Scanline == -1) {
						if (FetchPos >= FetchScrollEnd)
							FixScroll = false;
						else if (FetchPos >= FetchScrollStart)
							FixScroll = true;
					}
				}
				if (FixScroll)
					PPU.Registers.Addr_v = PPU.Registers.Addr_t;
				TimeLeft -= ClockDivider;
			}
			return Time - TimeLeft;
		}

		/* Обработка */
		inline void Process(CPPU &PPU) {
			ProcessClocks(PPU, PPU.InternalData.InternalClock);
		}
		/* Обработка ко времени */
		bool ProcessClocks(CPPU &PPU, int Time) {
			if (InternalClock >= Time)
				return true;
			if ((FetchCycle <= FetchSprite) && ((FetchCycle + (Time - InternalClock) /
				ClockDivider) > FetchSprite))
				PPU.SpriteRenderer.ProcessClocks(PPU, Time);
			InternalClock += Execute(PPU, Time - InternalClock);
			return InternalClock == Time;
		}
		/* Новый сканлайн */
		void NewScanline(CPPU &PPU) {
			if (PPU.InternalData.Scanline >= _Settings::ActiveScanlines)
				return;
			FetchPos = 0;
			FetchCycle = 0;
			memset(Fetches, 0, FetchMAX * sizeof(SFetch));
		}
	} BusHandler;

	/* Вызов по событию */
	inline void EventCall(int CallTime) {
		Process(CallTime);
		InternalData.IRQOffset -= CallTime;
		InternalData.ScanlineClock -= CallTime;
		InternalData.InternalClock -= CallTime;
		SpriteRenderer.ResetClock(CallTime);
		BackgroundRenderer.ResetClock(CallTime);
		BusHandler.ResetClock(CallTime);
	}

	/* Вызов по шине CPU */
	inline void CPUCall(int Offset = 0) {
		Process(Bus->GetInternalClock() + InternalData.IRQOffset + Offset);
	}

	/* Вызов по шине CPU с учетом CE */
	inline void CPUCallCELow(int Offset = 0) {
		CPUCall(Offset - _Bus::CPUClass::GetM2Length());
	}
	inline void CPUCallCEHigh(int Offset = 0) {
		CE = true;
		CPUCall(Offset);
		CE = false;
	}
	inline void CPUCallCE(int Offset = 0) {
		CPUCallCELow(Offset);
		CPUCallCEHigh(Offset);
	}

	/* Обработчик */
	void Process(int Time);
public:
	CPPU(_Bus *pBus) {
		SEvent *NewEvent;

		Bus = pBus;
		memset(&InternalData, 0, sizeof(InternalData));
		Bus->GetManager()->template SetPointer<SInternalData>(&InternalData);
		memset(&State, 0, sizeof(State));
		Bus->GetManager()->template SetPointer<SState>(&State);
		memset(&Registers, 0, sizeof(Registers));
		Bus->GetManager()->template SetPointer<SRegisters>(&Registers);
		memset(LocalEvents, 0, sizeof(SEventData) * MAX_EVENTS);
		Bus->GetManager()->template SetPointer<ManagerID<PPUID::EventDataID> >(LocalEvents,
			sizeof(SEventData) * MAX_EVENTS);
		memset(&SpriteRenderer, 0, sizeof(SpriteRenderer));
		Bus->GetManager()->template SetPointer<SSpriteRenderer>(&SpriteRenderer);
		memset(&BackgroundRenderer, 0, sizeof(BackgroundRenderer));
		Bus->GetManager()->template SetPointer<SBackgroundRenderer>(&BackgroundRenderer);
		memset(&BusHandler, 0, sizeof(BusHandler));
		Bus->GetManager()->template SetPointer<SBusHandler>(&BusHandler);
		PalMem = static_cast<uint8 *>(
			Bus->GetManager()->template GetPointer<ManagerID<PPUID::PalMemID> >(\
				0x20 * sizeof(uint8)));
		memset(PalMem, 0xff, 0x20 * sizeof(uint8));
		NominalFrameTime = ClockDivider *
			((_Settings::ActiveScanlines + _Settings::PostRender +
			_Settings::VBlank + 1) * 341);
		NominalFrameTimeOdd = NominalFrameTime - ClockDivider * _Settings::OddSkip;
		FrameReady = false;
		CE = false;
		InternalData.FrameTime = NominalFrameTime;
		NewEvent = new SEvent;
		NewEvent->Data = &LocalEvents[EVENT_PPU_FRAME];
		NewEvent->Name = MSTR(EVENT_PPU_FRAME);
		NewEvent->Execute = [this] {
			EventCall(InternalData.FrameTime);
			BackgroundRenderer.Process(*this);
			//InternalData.OddFrame ^= 1;
			//if (Registers.RenderingEnabled() && InternalData.OddFrame)
			//	InternalData.FrameTime = NominalFrameTimeOdd;
			//else
			//	InternalData.FrameTime = NominalFrameTime;
			Bus->GetClock()->SetEventTime(EventChain[EVENT_PPU_FRAME],
				Bus->GetClock()->GetTime() + InternalData.FrameTime);
		};
		EventChain[EVENT_PPU_FRAME] = NewEvent;
		Bus->GetClock()->RegisterEvent(NewEvent);
		Bus->GetClock()->EnableEvent(EventChain[EVENT_PPU_FRAME]);
		Bus->GetClock()->SetEventTime(EventChain[EVENT_PPU_FRAME],
			Bus->GetClock()->GetTime() + InternalData.FrameTime);
	}
	inline ~CPPU() {
	}

	/* Сброс */
	inline void Reset() {
		InternalData.Scanline = -1;
		InternalData.ScanlineClock = InternalData.InternalClock + 341 * ClockDivider;
	}

	/* Сброс часов шины */
	inline void ResetInternalClock(int Time) {
		InternalData.IRQOffset += Time;
	}
	/* Чтение памяти */
	inline uint8 ReadByte(uint16 Address) {
		switch (Address & 0x0007) {
			case 2: /* 0x2002 */
				CPUCallCELow();
				BackgroundRenderer.Process(*this);
				State.SupressVBlank = true;
				Registers.BufIO = State.ReadState(Registers.BufIO);
				State.ClearVBlank();
				CPUCallCEHigh();
				BackgroundRenderer.Process(*this);
				State.SupressVBlank = false;
				break;
			// case 4: /* 0x2004 */
				// CPUCallCELow();
				// SpriteRenderer.Process(*this);
				// Registers.BufIO = SpriteRenderer.BufIO;
				// CPUCallCEHigh();
				// break;
			// case 7: /* 0x2007 */
			default:
				return 0x40;
		}
		return Registers.BufIO;
	}
	/* Запись памяти */
	inline void WriteByte(uint16 Address, uint8 Src) {
		bool OldNMI;

		switch (Address & 0x0007) {
			case 0: /* 0x2000 */
				CPUCallCELow();
				Registers.BufIO = Src;
				CPUCallCEHigh();
				OldNMI = Registers.GenerateNMI;
				Registers.Write2000(Src);
				/* UpdateAddressBus() */
				if (Registers.GenerateNMI && !OldNMI && State.GetVBlank())
					Bus->GetCPU()->GenerateNMI(InternalData.InternalClock -
						InternalData.IRQOffset - Bus->GetCPU()->GetIRQOffset() +
						_Bus::CPUClass::ClockDivider);
				break;
			case 1: /* 0x2001 */
				CPUCallCELow();
				Registers.BufIO = Src;
				CPUCallCEHigh();
				Registers.Write2000(Src);
				/* UpdateAddressBus() */
				break;
			case 3: /* 0x2003 */
				CPUCallCELow();
				Registers.BufIO = Src;
				CPUCallCEHigh();
				/* ... */
				break;
			case 4: /* 0x2004 */
				CPUCallCELow();
				Registers.BufIO = Src;
				CPUCallCEHigh();
				/* ... */
				break;
			case 5: /* 0x2005 */
				CPUCallCELow();
				Registers.BufIO = Src;
				Registers.Write2005(Src);
				CPUCallCEHigh();
				/* ... */
				break;
			case 6: /* 0x2006 */
				CPUCallCELow();
				Registers.BufIO = Src;
				Registers.Write2006(Src);
				/* ... */
				break;
			case 7: /* 0x2007 */
				/* ... */
				break;
		}
	}

	/* Обработать до текущего такта */
	inline void PreProcess() {
		CPUCall();
		BackgroundRenderer.Process(*this);
	}
	/* Флаг готовности кадра */
	inline bool IsFrameReady() { return FrameReady; }
	/* Время отрисовки кадра */
	inline int GetFrameTime() {
		FrameReady = false;
		return FrameTime;
	}
	/* Частота PPU */
	static inline const double GetFreq() { return _Settings::GetFreq(); }
	/* Текущее время на шине */
	inline int GetInternalTime() const { return BusHandler.InternalClock -
		InternalData.IRQOffset; }
};

/* Обработчик */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>::Process(int Time) {
	int LeftTime = Time - InternalData.InternalClock;
	int Diff;

	while ((InternalData.InternalClock + LeftTime) >= InternalData.ScanlineClock) {
		Diff = InternalData.ScanlineClock - InternalData.InternalClock;
		InternalData.InternalClock = InternalData.ScanlineClock;
		LeftTime -= Diff;
		BackgroundRenderer.Process(*this);
		InternalData.Scanline++;
		InternalData.ScanlineClock += ClockDivider * 341;
		if (InternalData.Scanline == (_Settings::ActiveScanlines + _Settings::PostRender +
			_Settings::VBlank)) {
			InternalData.Scanline = -1;
			FrameReady = true;
			FrameTime = InternalData.FrameTime;
			Bus->PushFrame();
		}
		SpriteRenderer.NewScanline(*this);
		BackgroundRenderer.NewScanline(*this);
		BusHandler.NewScanline(*this);
	}
	if (LeftTime > 0) {
		InternalData.InternalClock += LeftTime;
		BusHandler.Process(*this);
	}
}

}

#endif
