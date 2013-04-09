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
#include "tables.h"
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
typedef PPUGroup<8>::ID::NoBatteryID RenderDataID;
typedef PPUGroup<9>::ID::NoBatteryID SpriteDataID;
typedef PPUGroup<10>::ID::NoBatteryID DMADataID;
typedef PPUGroup<11>::ID::NoBatteryID SpritesID;

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

	/* Данные для спрайтов */
	struct SSprite {
		int y;
		int x;
		int sx;
		int cx;
		int Top;
		uint16 ShiftReg;
		uint8 Attrib;
		uint8 Tile;
	} Sprites[8];

	/* Внутренние данные */
	struct SInternalData: public ManagerID<PPUID::InternalDataID> {
		/* Текущий адрес на шине */
		uint16 CurAddrLine;
		/* Рассчитанный адрес */
		uint16 CalcAddr;
		/* Байт для сохранения NT */
		uint8 NTByte;
		/* Адрес */
		uint16 Addr_v;
		/* Буфер адреса */
		uint16 Addr_t;
		/* Страницы фона */
		uint16 BGPage;
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
		uint8 State;
		bool SupressVBlank;
		/* Указатель OAM */
		uint8 OAMIndex;
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
		inline void SetVBlank() {  if (!SupressVBlank) State |= 0x80; }
		inline int GetVBlank() { return State & 0x80; }
		inline void ClearVBlank() { State &= 0x7f; SupressVBlank = true; }
		/* Переполнение */
		inline void SetOverflow() { State |= 0x20; }
		/* Получить статус */
		inline void ReadState() { Buf_Write = (Buf_Write & 0x1f) | State; }
		/* Состояние рендеринга */
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
		/* Запись в 0x2005/1 */
		inline void Write2005_1(uint8 Src) {
			FHIndex = 0x10 | ((~Src & 0x07) << 1);
			Addr_t = (Addr_t & 0x7fe0) | (Src >> 3);
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
		inline uint16 GetSpriteAddress(int Scanline, SSprite &Sprite) {
			switch (SpriteSize) {
				case Size8x8:
					if (Sprite.y > 240)
						return SpritePage | 0x0ff0;
					if (Sprite.Attrib & 0x80)
						return SpritePage | (Sprite.Tile << 4) | (7 +
							Sprite.y - Scanline);
					else	
						return SpritePage | (Sprite.Tile << 4) | (Scanline -
							Sprite.y);
					break;
				case Size8x16:
					if (Sprite.y > 240)
						return 0x1ff0;
					if (Sprite.Attrib & 0x80) {
						if ((Scanline - Sprite.y) > 7)
							return ((Sprite.Tile & 0x01) << 12) |
								((Sprite.Tile & 0xfe) << 4) | (15 +
								Sprite.y - Scanline);
						else
							return ((Sprite.Tile & 0x01) << 12) |
								((Sprite.Tile | 0x01) << 4) | (7 +
								Sprite.y - Scanline);
					} else {
						if ((Scanline - Sprite.y) > 7)
							return ((Sprite.Tile & 0x01) << 12) |
								((Sprite.Tile | 0x01) << 4) | (Scanline -
								Sprite.y + 8);
						else
							return ((Sprite.Tile & 0x01) << 12) |
								((Sprite.Tile & 0xfe) << 4) | (Scanline -
								Sprite.y);
					}
					break;
			}
			return 0x0000;
		}
		/* Цвет при отсутствии рендеринга */
		inline uint8 GetDropColour() {
			if (CurAddrLine < 0x3f00)
				return 0x00;
			if (CurAddrLine & 0x0003)
				return CurAddrLine & 0x001f;
			else
				return CurAddrLine & 0x000c;
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
		int sClock; /* Текуший такт для спрайтов */
		int hStart; /* Начало сканлайна */

		/* Вычесть такты */
		inline void SubstractCycles() {
			Cycles -= FrameCycles * ClockDivider;
			ProcessedCycles -= FrameCycles * ClockDivider;
			if (Cycle2006 >= CurCycle)
				Cycle2006 -= FrameCycles;
			if (Cycle2007 >= CurCycle)
				Cycle2007 -= FrameCycles;
			CurCycle -= FrameCycles;
			hStart -= FrameCycles;
		}
		/* Видимая часть */
		inline bool InRender() {
			return Scanline < _Settings::ActiveScanlines;
		}
	} CycleData;
	/* Данные для рендерера */
	struct SRenderData: public ManagerID<PPUID::RenderDataID> {
		int ShiftCount; /* Число сдвигов */
		uint32 BGReg; /* Регист для тайлов */
		uint8 TileA; /* Данные для подгрузки */
		uint8 TileB;
		uint32 ARReg; /* Регистр для атрибутов */
		uint8 AR; /* Даные для подгрузки */

		/* Синхроимульс */
		inline void Clock() {
			BGReg <<= 2;
			ARReg <<= 2;
			ShiftCount++;
			if (ShiftCount > 7) {
				ShiftCount = 0;
				BGReg |= ppu::ColourTable[TileA] | (ppu::ColourTable[TileB] << 1);
				ARReg |= ppu::AttributeTable[AR];
				TileA = 0x00;
				TileB = 0x00;
			}
		}
	} RenderData;
	
	/* Данные для выборки спрайтов */
	struct SSpriteData: public ManagerID<PPUID::SpriteDataID> {
		bool Stress; /* Стрессовая ситуация */
		int Count; /* Число спрайтов */
		int Index; /* Индекс при записи */
		int Mode; /* Режим поиска */
		int Delay; /* Пропустили тактов при заполнении OAM_sec */
		bool Overflow; /* Переход за пределы */
		bool Prim; /* Флаг 0-object на этом сканлайне */
		bool PrimNext; /* Флаг 0-object для следующего сканлайна */

		/* Сброс значений */
		inline void Reset() {
			Count = 0;
			Mode = 0;
			Index = 0;
			Overflow = false;
			PrimNext = false;
		}
	} SpriteData;

	/* Спрайт в зоне видимости */
	inline bool SpriteInRange(uint8 Sprite) {
		if (CycleData.Scanline == -1)
			return false;
		if (Sprite > 240)
			return false;
		if (Sprite > CycleData.Scanline)
			return false;
		if ((CycleData.Scanline - Sprite) >= InternalData.SpriteSize)
			return false;
		return true;
	}
	/* Регистры сдвига у спрайтов */
	inline void ClockSprites() {
		int Index;

		for (Index = 0; Index < 8; Index++) {
			if (Sprites[Index].cx < 0)
				continue;
			if ((Sprites[Index].cx < 8) && (Sprites[Index].cx > 0)) {
				Sprites[Index].cx--;
				Sprites[Index].x++;
				Sprites[Index].ShiftReg <<= 2;
			}
		}
	}

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

	/* Данные для обработки DMA */
	struct SDMAData: public ManagerID<PPUID::DMADataID> {
		uint8 Latch; /* Буфер */
		uint16 Address; /* Текущий адрес */
		bool UseLatch; /* Использовать буфер */
		int Left; /* Осталось */
	} DMAData;

	/* Получить адрес для палитры */
	inline uint8 GetPALAddress(uint8 Colour, uint8 Attribute) {
		if ((Colour & 0x03) == 0)
			return 0x00;
		else
			return ((Attribute & 0x03) << 2) | Colour;
	}
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
	inline void FetchCycle();
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
				0x20 * sizeof(uint8));
		memset(OAM_sec, 0xff, 0x20 * sizeof(uint8));
		Bus->GetManager()->template SetPointer<SInternalData>(&InternalData);
		Bus->GetManager()->template SetPointer<SCycleData>(&CycleData);
		Bus->GetManager()->template SetPointer<SRenderData>(&RenderData);
		Bus->GetManager()->template SetPointer<SSpriteData>(&SpriteData);
		Bus->GetManager()->template SetPointer<SDMAData>(&DMAData);
		Bus->GetManager()->template SetPointer<ManagerID<PPUID::SpritesID> >(\
			Sprites, sizeof(SSprite) * 8);
	}
	inline ~CPPU() {}

	/* Обработать такты */
	inline void Clock(int Cycles) {
		Render(Cycles);
		CycleData.Cycles += Cycles;
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
		Render(Bus->GetClock()->GetPreCycles(), false);
	}

	/* Текущий такт (относительно CPU) */
	inline int GetCycles() {
		return std::min(CycleData.CurCycle, (CycleData.hStart + CycleData.hClock)) *
			ClockDivider - CycleData.Cycles;
	}

	/* Сброс */
	inline void Reset() {
		FrameReady = false;
		memset(&CycleData, 0, sizeof(SCycleData));
		CycleData.Cycle2006 = -1;
		CycleData.Cycle2007 = -1;
		CycleData.Scanline = -1;
		memset(&InternalData, 0, sizeof(SInternalData));
		InternalData.FHIndex = 0x1e;
		memset(&RenderData, 0, sizeof(SRenderData));
		memset(&SpriteData, 0, sizeof(SSpriteData));
		memset(Sprites, 0, sizeof(SSprite) * 8);
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		switch (Address & 0x0007) {
			case 0x0002: /* Статус PPU */
				InternalData.Trigger = false;
				PreRenderBeforeCERise();
				InternalData.ReadState();
				InternalData.ClearVBlank();
				PreRenderBeforeCEFall();
				InternalData.SupressVBlank = false;
				break;
			case 0x0004: /* Чтение OAM */
				PreRenderBeforeCERise();
				RenderSprites();
				if (InternalData.RenderingEnabled() && CycleData.InRender())
					InternalData.Buf_Write = InternalData.OAMBuf;
				else
					InternalData.Buf_Write = OAM[InternalData.OAMIndex];
				break;
			case 0x0007: /* PPU RAM I/O */
				PreRenderBeforeCERise();
				if (InternalData.CurAddrLine >= 0x3f00)
					InternalData.Buf_Write = ReadPalette(InternalData.CurAddrLine);
				else
					InternalData.Buf_Write = InternalData.Buf_Read;
				PreRenderBeforeCEFall();
				InternalData.Read2007 = true;
				CycleData.Cycle2007 = CycleData.CurCycle + 1;
				break;
		}
		PreRenderBeforeCEFall();
		return InternalData.Buf_Write;
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		bool OldNMI;
		uint8 Buf;

		PreRenderBeforeCERise();
		InternalData.Buf_Write = Src;
		switch (Address & 0x0007) {
			case 0x0000: /* Управляющий регистр 1 */
				RenderSprites();
				OldNMI = !InternalData.GenerateNMI;
				PreRenderBeforeCEFall();
				InternalData.Write2000(Src);
				UpdateAddressBus();
				if (InternalData.GenerateNMI && OldNMI && InternalData.GetVBlank())
					Bus->GetCPU()->GenerateNMI(GetCycles());
				break;
			case 0x0001: /* Управляющий регистр 2 */
				RenderSprites();
				PreRenderBeforeCEFall();
				InternalData.Write2001(Src);
				UpdateAddressBus();
				break;
			case 0x0003: /* Установить указатель OAM */
				RenderSprites();
				InternalData.OAMIndex = Src;
				SpriteData.Stress = true;
				PreRenderBeforeCEFall();
				RenderSprites();
				SpriteData.Stress = false;
				break;
			case 0x0004: /* Записать байт OAM */
				PreRenderBeforeCEFall();
				RenderSprites();
				if (InternalData.RenderingEnabled() && CycleData.InRender())
					Buf = InternalData.OAMBuf;
				else {
					Buf = Src;
					if ((InternalData.OAMIndex & 0x03) == 0x02)
						Buf &= 0xe3;
				}
				OAM[InternalData.OAMIndex++] = Buf;
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
				if (!InternalData.Trigger)
					CycleData.Cycle2006 = CycleData.CurCycle + 1;
				break;
			case 0x0007: /* PPU RAM I/O */
				PreRenderBeforeCEFall();
				InternalData.Write2007 = true;
				CycleData.Cycle2007 = CycleData.CurCycle + 1;
				break;
		}
		PreRenderBeforeCEFall();
	}

	/* Включить DMA */
	inline void EnableDMA(uint8 Page) {
		PreRender();
		RenderSprites();
		DMAData.Address = Page << 8;
		DMAData.Left = 256;
	}
	
	inline void DoDMA() {
		int i;

		if (DMAData.Address < 0x2000) {
			DMAData.Address &= 0x07ff;
			if (InternalData.OAMIndex > 0) {
				memcpy(OAM + InternalData.OAMIndex, Bus->GetCPU()->GetRAM() +
					DMAData.Address, (0x0100 - InternalData.OAMIndex) *
					sizeof(uint8));
				memcpy(OAM, Bus->GetCPU()->GetRAM() + DMAData.Address + (0x0100 -
					InternalData.OAMIndex), InternalData.OAMIndex * sizeof(uint8));
			} else {
				memcpy(OAM, Bus->GetCPU()->GetRAM() + DMAData.Address,
					0x0100 * sizeof(uint8));
			}
			DMAData.Left = 0;
		} else
			do {
				OAM[InternalData.OAMIndex] =
					Bus->ReadCPUMemory(DMAData.Address);
				InternalData.OAMIndex++;
				DMAData.Address++;
				DMAData.Left--;
			} while (DMAData.Left > 0);
		for (i = 2; i < 256; i += 4)
			OAM[i] &= 0xe3;
	}

	/* Флаг окончания рендеринга фрейма */
	inline const bool &IsFrameReady() const { return FrameReady; }
	/* Длина фрейма */
	inline const int GetFrameCycles() {
		FrameReady = false;
		CycleData.SubstractCycles();
		return CycleData.FrameCycles * ClockDivider;
	}
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
				if ((CycleData.hClock < 256) ||
					((CycleData.hClock >= 320) && (CycleData.hClock < 336)))
					InternalData.SetATAddr();
				else
					InternalData.SetNTAddr();
				break;
			case 4:
				if ((CycleData.hClock < 256) ||
					((CycleData.hClock >= 320) && (CycleData.hClock < 336)))
					InternalData.ReadNT();
				else
					InternalData.CalcAddr =
						InternalData.GetSpriteAddress(CycleData.Scanline,
							Sprites[(CycleData.hClock & 0x38) >> 3]);
				InternalData.CurAddrLine = InternalData.CalcAddr & 0x3ff7;
				break;
			case 6:
				if ((CycleData.hClock < 256) ||
					((CycleData.hClock >= 320) && (CycleData.hClock < 336)))
					InternalData.ReadNT();
				else
					InternalData.CalcAddr =
						InternalData.GetSpriteAddress(CycleData.Scanline,
							Sprites[(CycleData.hClock & 0x38) >> 3]);
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
		WAIT_CYCLE(CycleData.Cycle2006) /* Обновление адреса после 0x2006/2 */
		WAIT_CYCLE(CycleData.Cycle2007) /* I/O после 0x2007 */
		RenderBG(NextCycle - CycleData.CurCycle);
		if (CycleData.CurCycle == CycleData.Cycle2007) {
			CycleData.Cycle2007++;
			NextCycle = CE ? (CycleData.Clock2007 + 2) : (CycleData.Clock2007 + 1);
			if ((CycleData.Clock2007 <= 2) && (NextCycle > 2)) {
				if (InternalData.Write2007) {
					InternalData.Write2007 = false;
					if (InternalData.CurAddrLine >= 0x3f00)
						WritePalette(InternalData.CurAddrLine, InternalData.Buf_Write);
					else if (!InternalData.RenderingEnabled() || !CycleData.InRender())
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
			if (NextCycle > 3) {
				IncrementAddress();
				UpdateAddressBus();
				CycleData.Clock2007 = 0;
				CycleData.Cycle2007 = -1;
			} else
				CycleData.Clock2007 = NextCycle;
		}
		if (CycleData.CurCycle == CycleData.Cycle2006) {
			InternalData.Addr_v = InternalData.Addr_t;
			UpdateAddressBus();
			CycleData.Cycle2006 = -1;
		}
	}

#undef WAIT_CYCLE

}


/* Рендеринг фона */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>::RenderBG(int Cycles) {

#define WAIT_CYCLE(_n) if (hCycles <= (_n)) break;\
	if (CycleData.hClock == (_n))
#define WAIT_CYCLE_RANGE(_a, _b) if (hCycles <= (_a)) break;\
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
					if (CycleData.Scanline != -1) {
						RenderPixel();
						RenderData.Clock();
					}
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
					RenderData.Clock();
					Count--;
					CycleData.hClock++;
				}
			}
			WAIT_CYCLE_RANGE(336, 340) {
				Count = std::min(340, hCycles) - CycleData.hClock;
				while (Count > 0) {
					if ((CycleData.Scanline == -1) && (CycleData.hClock == 338) &&
						CycleData.OddFrame)
						_Settings::SkipPPUClock(hCycles, CycleData.hStart);
					FetchCycle();
					Count--;
					CycleData.hClock++;
				}
			}
		} else {
			if (CycleData.hClock < 256) {
				if (CycleData.Scanline != -1) {
					uint8 Colour = PalMem[InternalData.GetDropColour()];

					if (InternalData.Grayscale)
						Colour &= 0x30;
					Count = std::min(256, hCycles) - CycleData.hClock;
					while (Count > 0) {
						RenderData.Clock();
						ClockSprites();
						OutputPixel(CycleData.hClock, CycleData.Scanline, Colour);
						Count--;
						CycleData.hClock++;
					}
				} else
					CycleData.hClock = std::min(256, hCycles);
			}
			WAIT_CYCLE_RANGE(256, 320)
				CycleData.hClock = std::min(320, hCycles);
			WAIT_CYCLE_RANGE(320, 336) {
				Count = std::min(336, hCycles) - CycleData.hClock;
				while (Count > 0) {
					RenderData.Clock();
					Count--;
					CycleData.hClock++;
				}
			}
			WAIT_CYCLE_RANGE(336, 340)
				CycleData.hClock = std::min(340, hCycles);
		}
		WAIT_CYCLE(340) {
			RenderSprites();
			CycleData.Scanline++;
			if (CycleData.InRender()) {
				CycleData.hClock = 0;
				CycleData.sClock = 0;
				SpriteData.Delay = 0;
				hCycles -= 341;
				CycleData.hStart += 341;
				continue;
			}
			CycleData.FrameCycles = CycleData.TotalCycles - (hCycles - 341);
			CycleData.TotalCycles = hCycles - 341;
			FrameReady = true;
			CycleData.OddFrame = !CycleData.OddFrame;
			CycleData.hClock = 341 + 341;
			hCycles -= (_Settings::PostRender - 1) * 341;
			CycleData.hStart += (_Settings::PostRender - 1) * 341;
		}
		WAIT_CYCLE(341) {
			InternalData.State = 0;
			CycleData.Scanline = -1;
			CycleData.hClock = 0;
			CycleData.sClock = 0;
			SpriteData.Delay = 0;
			CycleData.hStart += 341;
			hCycles -= 341;
			continue;
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
	int sCycles, Target, Index;

	if (CycleData.sClock >= 256)
		return;
	sCycles = CycleData.CurCycle - CycleData.hStart;
	if (InternalData.RenderingEnabled() && CycleData.InRender()) {
		if (CycleData.sClock < 64) {
			memset(OAM_sec + ((CycleData.sClock - SpriteData.Delay) >> 1), 0xff,
				((std::min(64, sCycles) - CycleData.sClock) >> 1) * sizeof(uint8));
			InternalData.OAMBuf = 0xff;
			CycleData.sClock = std::min(64, sCycles);
		}
		if (sCycles <= 64)
			return;
		if (CycleData.sClock == 64)
			SpriteData.Reset();
		if (CycleData.sClock < 256) {
			Target = std::min(256, sCycles);
			while (CycleData.sClock < Target) {
				/* В стрессовой ситуации OAMIndex &= 0x0f или OAMIndex &= 0xf0 */
				/* Не уверен, что это способно повлиять на чтение (адрес к тому */
				/* времени будет перезаписан) */
				InternalData.OAMBuf = OAM[InternalData.OAMIndex];
				if (CycleData.sClock & 1)
					switch (SpriteData.Mode) {
						case 0: /* Режим поиска видимых спрайтов */
							if (SpriteInRange(InternalData.OAMBuf)) {
								OAM_sec[SpriteData.Index] = InternalData.OAMBuf;
								if (CycleData.sClock == 65)
									SpriteData.PrimNext = true;
								if (!SpriteData.Stress)
									InternalData.OAMIndex++;
								SpriteData.Index++;
								SpriteData.Mode = 1;
							} else if (!SpriteData.Stress) {
								InternalData.OAMBuf = OAM_sec[SpriteData.Index];
								InternalData.OAMIndex =
									(InternalData.OAMIndex & 0xfc) + 4;
								if (InternalData.OAMIndex < 4)
									SpriteData.Mode = 4;
							}
							break;
						case 1: /* Копирование данных в буфер */
							OAM_sec[SpriteData.Index] = InternalData.OAMBuf;
							SpriteData.Index++;
							if (!SpriteData.Stress) {
								InternalData.OAMIndex++;
								if (InternalData.OAMIndex == 0)
									SpriteData.Overflow = true;
							}
							if (!(SpriteData.Index & 3)) {
								if (SpriteData.Overflow)
									SpriteData.Mode = 4;
								else if (SpriteData.Index < 32) {
									SpriteData.Mode = 0;
									if (!SpriteData.Stress)
										InternalData.OAMIndex &= 0xfc;
								} else
									SpriteData.Mode = 2;
							}
							break;
						case 2: /* Режим переполнения */
							InternalData.OAMBuf = OAM_sec[0x00];
							if (SpriteInRange(InternalData.OAMBuf)) {
								InternalData.SetOverflow();
								SpriteData.Mode = 3;
								SpriteData.Index = 0;
							} else {
								if (!SpriteData.Stress) {
									InternalData.OAMIndex++;
									if (InternalData.OAMIndex & 0x03)
										InternalData.OAMIndex += 4;
									if (InternalData.OAMIndex < 4)
										SpriteData.Mode = 4;
								}
							}
							break;
						case 3: /* Режим нерабочего копирования */
							InternalData.OAMBuf = OAM_sec[0x00];
							SpriteData.Index++;
							if (!SpriteData.Stress)
								InternalData.OAMIndex++;
							if (SpriteData.Index == 3)
								SpriteData.Mode = 4;
							break;
						case 4: /* Режим отсечения */
							InternalData.OAMBuf = OAM_sec[0x00];
							if (!SpriteData.Stress)
								InternalData.OAMIndex =
									(InternalData.OAMIndex & 0xfc) + 4;
							break;
					}
				CycleData.sClock++;
			}
			if (CycleData.sClock == 256) {
				InternalData.OAMIndex = 0;
				SpriteData.Prim = SpriteData.PrimNext;
				for (Index = 0; Index < 8; Index++) {
					Sprites[Index].cx = -1;
					Sprites[Index].x = Sprites[Index].sx;
					Sprites[Index].ShiftReg = 0;
				}
			}
		}
	} else {
		if (CycleData.sClock < 64) {
			SpriteData.Delay += sCycles - CycleData.sClock;
			CycleData.sClock = std::min(64, sCycles);
			if (CycleData.sClock == 64) {
				SpriteData.Reset();
				SpriteData.Mode = 4; /* Не можем определять спрайты, если пропустили такт */
			}
		}
		if (CycleData.sClock < 256) {
			CycleData.sClock = std::min(256, sCycles);
			if (CycleData.sClock == 256) {
				for (Index = 0; Index < 8; Index++) {
					Sprites[Index].cx = -1;
					Sprites[Index].x = Sprites[Index].sx;
					Sprites[Index].ShiftReg = 0;
				}
			}
		}
		CycleData.sClock = sCycles;
	}
}

/* Фетчинг */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>::FetchCycle() {
	InternalData.OAMBuf = OAM_sec[0x00];
	if (~CycleData.hClock & 1)
		UpdateAddressBus();
	else
		Bus->ReadPPUMemory(InternalData.CurAddrLine);
}

/* Фетчинг спрайтов */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>:: FetchSprite() {
	int Index;

	Index = (CycleData.hClock & 0x38) >> 3;
	switch (CycleData.hClock & 7) {
		case 0:
			InternalData.OAMBuf = OAM_sec[Index << 2];
			Sprites[Index].y = InternalData.OAMBuf;
			if (SpriteInRange(Sprites[Index].y))
				Sprites[Index].cx = 8;
			UpdateAddressBus();
			break;
		case 1:
			InternalData.OAMBuf = OAM_sec[(Index << 2) | 0x01];
			Sprites[Index].Tile = InternalData.OAMBuf;
			break;
		case 2:
			InternalData.OAMBuf = OAM_sec[(Index << 2) | 0x02];
			Sprites[Index].Attrib = InternalData.OAMBuf;
			Sprites[Index].Top = ~InternalData.OAMBuf & 0x20;
			UpdateAddressBus();
			break;
		case 3:
			InternalData.OAMBuf = OAM_sec[(Index << 2) | 0x03];
			Sprites[Index].sx = InternalData.OAMBuf;
			Sprites[Index].x = InternalData.OAMBuf;
			break;
		case 4:
			InternalData.OAMBuf = OAM_sec[(Index << 2) | 0x03];
			UpdateAddressBus();
			break;
		case 5:
			InternalData.OAMBuf = OAM_sec[(Index << 2) | 0x03];
			if (Sprites[Index].Attrib & 0x40)
				Sprites[Index].ShiftReg = ppu::ColourTable[\
					ppu::FlipTable[Bus->ReadPPUMemory(InternalData.CurAddrLine)]];
			else
				Sprites[Index].ShiftReg = ppu::ColourTable[\
					Bus->ReadPPUMemory(InternalData.CurAddrLine)];
			break;
		case 6:
			InternalData.OAMBuf = OAM_sec[(Index << 2) | 0x03];
			UpdateAddressBus();
			break;
		case 7:
			InternalData.OAMBuf = OAM_sec[(Index << 2) | 0x03];
			if (Sprites[Index].Attrib & 0x40)
				Sprites[Index].ShiftReg |= ppu::ColourTable[ppu::FlipTable[\
					Bus->ReadPPUMemory(InternalData.CurAddrLine)]] << 1;
			else
				Sprites[Index].ShiftReg |= ppu::ColourTable[\
					Bus->ReadPPUMemory(InternalData.CurAddrLine)] << 1;
			break;
	}
}

/* Фетчинг фона */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>::FetchBackground() {
	InternalData.OAMBuf = OAM_sec[0x00];
	if (~CycleData.hClock & 1)
		UpdateAddressBus();
	else switch (CycleData.hClock & 6) {
		case 0:
			InternalData.NTByte = Bus->ReadPPUMemory(InternalData.CurAddrLine);
			break;
		case 2:
			RenderData.AR =
				InternalData.ReadAT(Bus->ReadPPUMemory(InternalData.CurAddrLine));
			break;
		case 4:
			RenderData.TileA = Bus->ReadPPUMemory(InternalData.CurAddrLine);
			break;
		case 6:
			RenderData.TileB = Bus->ReadPPUMemory(InternalData.CurAddrLine);
			IncrementAddress(CycleData.hClock == 255);
			break;
	}
}

/* Рендеринг точки */
template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>::RenderPixel() {
	int i, SIndex;
	uint8 Colour;
	uint8 Sprite;
	uint16 Addr;

	if (InternalData.ShowBackground && (InternalData.BackgroundClip ||
		(CycleData.hClock > 7))) {
		Colour = (RenderData.BGReg >> InternalData.FHIndex) & 0x03;
		Addr = GetPALAddress(Colour, RenderData.ARReg >> InternalData.FHIndex);
	} else {
		Colour = 0;
		Addr = 0;
	}
	Sprite = 0x10;
	SIndex = 0;
	for (i = 0; i < 8; i++)
		if (Sprites[i].cx < 0)
			continue;
		else if (Sprites[i].x == CycleData.hClock) {
			if (Sprites[i].cx == 0)
				continue;
			Sprites[i].cx--;
			Sprites[i].x++;
			if (Sprite == 0x10) {
				Sprite |= (Sprites[i].ShiftReg >> 14) & 0x03;
				SIndex = i;
			}
			Sprites[i].ShiftReg <<= 2;
		}
	if ((Sprite != 0x10) && InternalData.ShowSprites &&
		(InternalData.SpriteClip || (CycleData.hClock > 7))) {
		if (!SIndex && SpriteData.Prim && (Colour != 0) && (CycleData.hClock < 255)) {
			InternalData.SetSprite0Hit();
		}
		if ((Colour == 0) || Sprites[SIndex].Top)
			Addr = GetPALAddress(Sprite, Sprites[SIndex].Attrib);
	}
	Colour = PalMem[Addr];
	if (InternalData.Grayscale)
		Colour &= 0x30;
	OutputPixel(CycleData.hClock, CycleData.Scanline, Colour);
}

}

#endif
