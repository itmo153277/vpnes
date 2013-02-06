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

typedef PPUGroup<1>::ID::NoBatteryID CycleDataID;
typedef PPUGroup<2>::ID::NoBatteryID DMADataID;
typedef PPUGroup<3>::ID::NoBatteryID RAM1ID;
typedef PPUGroup<4>::ID::NoBatteryID RAM2ID;
typedef PPUGroup<5>::ID::NoBatteryID StateID;
typedef PPUGroup<6>::ID::NoBatteryID RegistersID;
typedef PPUGroup<7>::ID::NoBatteryID ControlRegistersID;
typedef PPUGroup<8>::ID::NoBatteryID SpritesID;
typedef PPUGroup<9>::ID::NoBatteryID PalMemID;
typedef PPUGroup<10>::ID::NoBatteryID OAMID;
typedef PPUGroup<11>::ID::NoBatteryID InternalDataID;

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
	/* Обработка кадра завершена */
	bool FrameReady;
	/* Предобработано тактов */
	int PreprocessedCycles;
	/* Ушло тактов на фрейм */
	int FrameCycles;

	/* Данные о тактах */
	struct SCycleData: public ManagerID<PPUID::CycleDataID> {
		int CurCycle; /* Текущий такт */
		int CyclesLeft; /* Необработанные такты */
		int PrecCycles; /* Точные такты */
		int LastCycle; /* Последний такт */
		int CyclesTotal; /* Всего тактов */
		int Scanline; /* Текущий сканлайн */
		bool ShortFrame; /* Короткий фрейм */
		int DebugTimer;
	} CycleData;

	/* Данные для обработки DMA */
	struct SDMAData: public ManagerID<PPUID::DMADataID> {
		uint8 Latch; /* Буфер */
		uint16 Address; /* Текущий адрес */
		bool UseLatch; /* Использовать буфер */
		int Left; /* Осталось */
	} DMAData;

	/* Состояние PPU */
	struct SState: public ManagerID<PPUID::StateID> {
		uint8 State;
		bool VBlank;
		/* 0 объект */
		inline void SetSprite0Hit() { State |= 0x40; }
		/* VBlank set */
		inline void SetVBlank1() { VBlank = true; }
		/* VBlank set */
		inline void SetVBlank2() { if (VBlank) State |= 0x80; VBlank = false; };
		/* VBlank get */
		inline int GetVBlank() { return State & 0x80; }
		/* VBlank clear */
		inline void ClearVBlank() { State &= 0x7f; VBlank = false; }
		/* Overflow set */
		inline void SetOverflow() { State |= 0x20; }
		/* Garbadge */
		inline void SetGarbage(uint8 Src) { State = (State & 0xe0) | (Src & 0x1f); }
	} State;

	/* Внутренние регистры */
	struct SRegisters: public ManagerID<PPUID::RegistersID> {
		uint16 BigReg1; /* Первая комбинация */
		uint16 RealReg1; /* Настоящий регистр */
		uint16 BigReg2; /* Вторая комбинация */
		uint16 TempAR;
		uint16 AR; /* Информация о аттрибутах */
		uint16 FH; /* Точный сдвиг по горизонатли */
		uint16 SpritePage; /* Страница для спрайтов */
		/* Получить адрес для 0x2007 */
		inline uint16 Get2007Address() { return RealReg1 & 0x3fff; }
		/* Получить адрес для чтения NameTable */
		inline uint16 GetNTAddress() { return (RealReg1 & 0x0fff) | 0x2000; }
		/* Получить адрес для чтения AttributeTable */
		inline uint16 GetATAddress() { return ((RealReg1 >> 2) & 0x0007) |
			((RealReg1 >> 4) & 0x0038) | (RealReg1 & 0x0c00) | 0x23c0; }
		/* Получить адрес для чтения PatternTable */
		inline uint16 GetPTAddress() { return BigReg2 | (RealReg1 >> 12); }
		/* Получить страницу PatternTable */
		inline uint16 GetPTPage() { return BigReg2 & 0x1000; }
		/* Получить адрес для чтения Palette */
		inline uint8 GetPALAddress(uint8 col, uint16 CAR) { if ((col & 0x03) == 0)
			return 0x00; else return ((CAR & 0x03) << 2) | col; }
		/* Получить адрес для чтения PatternTable спрайтов */
		inline uint16 GetSpriteAddress(uint8 Tile, uint8 v, uint16 CurPage) {
			return CurPage | (Tile << 4) | v; }
		/* Запись в 0x2000 */
		inline void Write2000(uint8 Src) { BigReg1 = (BigReg1 & 0xf3ff) |
			((Src & 0x03) << 10) ; BigReg2 = (BigReg2 & 0x0fff) | ((Src & 0x10) << 8);
			SpritePage = (Src & 0x08) << 9; }
		/* Запись в 0x2005/1 */
		inline void Write2005_1(uint8 Src) { FH = ~Src & 0x07;
			BigReg1 = (BigReg1 & 0xffe0) | (Src >> 3); }
		/* Запись в 0x2005/2 */
		inline void Write2005_2(uint8 Src) { BigReg1 = (BigReg1 & 0x8c1f) | ((Src & 0x07) << 12) |
			((Src & 0xf8) << 2); }
		/* Запись в 0x2006/1 */
		inline void Write2006_1(uint8 Src) { BigReg1 = (BigReg1 & 0x00ff) | ((Src & 0x3f) << 8); }
		/* Запись в 0x2006/2 */
		inline void Write2006_2(uint8 Src) { BigReg1 = (BigReg1 & 0xff00) | Src; RealReg1 = BigReg1; }
		/* Прочитали NameTable */
		inline void ReadNT(uint8 Src) { BigReg2 = (BigReg2 & 0x1000) | (Src << 4); }
		/* Прочитали AttributeTable */
		inline void ReadAT(uint8 Src) { TempAR = ((Src >> (((RealReg1 >> 4) & 0x004) |
			(RealReg1 & 0x0002))) & 0x03) << 2; }
		/* Инкременты */
		inline void Increment2007(int VerticalIncrement) { if (VerticalIncrement)
			RealReg1 += 0x0020; else RealReg1++; }
		inline void IncrementX() { uint16 Src = (RealReg1 & 0x001f); Src++; RealReg1 = (RealReg1 & 0xffe0)
			| (Src & 0x001f); RealReg1 ^= (Src & 0x0020) << 5; }
		inline void IncrementY() { uint8 Src = (RealReg1 >> 12) | ((RealReg1 & 0x03e0) >> 2);
			if (Src == 239) { Src = 0; RealReg1 ^= 0x0800; } else Src++; RealReg1 = (RealReg1 & 0x8c1f) |
			((Src & 0x0007) << 12) | ((Src & 0x00f8) << 2); }
		/* Обновление скроллинга */
		inline void UpdateScroll() { RealReg1 = (RealReg1 & 0xfbe0) | (BigReg1 & 0x041f); }
	} Registers;

	/* Размер спрайта */
	enum SpriteSize {
		Size8x8 = 8,
		Size8x16 = 16
	};

	/* Управляющие регистры */
	struct SControlRegisters: public ManagerID<PPUID::ControlRegistersID> {
		int VerticalIncrement; /* Инкремент по Y */
		SpriteSize Size; /* Размер спрайтов */
		int GenerateNMI; /* Генерировать NMI */
		int Grayscale; /* Черно-белый */
		int BackgroundClip; /* Не показывать 8 пикселей слева */
		int SpriteClip; /* Не показывать 8 пикселей слева */
		int ShowBackground; /* Рендерить фон */
		int ShowSprites; /* Рендерить спрайты */
		int Tint; /* Флаг затемнения */
		inline void Controller(uint8 Src) { VerticalIncrement = Src & 0x04;
			Size = (Src & 0x20) ? Size8x16 : Size8x8; GenerateNMI = Src & 0x80; }
		inline void Mask(uint8 Src) { Grayscale = Src & 0x01;
			BackgroundClip = Src & 0x02; SpriteClip = Src & 0x04;
			ShowBackground = Src & 0x08; ShowSprites = Src & 0x10;
			Tint = Src >> 5; }
		inline bool RenderingEnabled() { return ShowBackground || ShowSprites; }
	} ControlRegisters;

	/* Данные спрайтов */
	struct SSprites: public ManagerID<PPUID::SpritesID> {
		int x; /* Координаты */
		int y;
		int cx; /* Текущая x */
		bool prim;
		uint16 ShiftReg; /* Регистр сдвига */
		uint8 Attrib; /* Аттрибуты */
		uint8 Tile; /* Чар */
		int HFlip; /* Горизонтальное отражение */
		int VFlip; /* Вертикальное отражение */
		int Top; /* Поверх фона */
	} Sprites[8];

	/* Палитра */
	uint8 *PalMem;
	/* Спрайты */
	uint8 *OAM;

	struct SInternalData: public ManagerID<PPUID::InternalDataID> {
		/* Внутренний флаг */
		bool Trigger;
		/* Внутренний буфер */
		uint8 Buf_B;
		/* Текущий адрес OAM */
		uint8 OAM_addr;
		/* Координата x */
		int x;
		/* Координата y */
		int y;
		/* Короткий сканлайн */
		bool ShortScanline;
		/* Адрес образа спрайта */
		uint16 SpriteAddr;
		/* Образ фона A */
		uint8 TileA;
		/* Образ фона B */
		uint8 TileB;
		/* Регистр сдвига */
		uint32 ShiftReg;
	} InternalData;

	/* Таблица с номерами цветов */
	static const uint16 ColourTable[256];

	/* Вывод точки */
	inline void OutputPixel(int x, int y, int Colour, int Tint) {
		if ((x >= Screen::Left) && (y >= Screen::Top) &&
			(x < Screen::Right) && (y < Screen::Bottom)) {
			Bus->GetFrontend()->GetVideoFrontend()->PutPixel(x - Screen::Left,
				y - Screen::Top, Colour, Tint);
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

	/* Чтеник с проверкой на A12 */
	inline uint8 ReadWithA12(uint16 Address) {
		Bus->GetROM()->UpdatePPUAddress(Address);
		return Bus->ReadPPUMemory(Address);
	}

	/* Выборка спрайтов */
	inline void EvaluateSprites();
	/* Загрузить спрайт */
	inline void FetchSprite();
	/* Загрузить фон */
	inline void FetchBackground();
	/* Рисуем сканалайн */
	inline void DrawPixel();
	/* Точный рендер */
	inline void FineRender(int Cycles);
	/* Рендер 8 пикселей сразу */
	inline void Render_8(int Cycles);
	/* Рендер */
	inline void Render(int Cycles);
public:
	inline explicit CPPU(_Bus *pBus) {
		Bus = pBus;
		Bus->GetManager()->template SetPointer<SCycleData>(&CycleData);
		Bus->GetManager()->template SetPointer<SDMAData>(&DMAData);
		Bus->GetSolderPad()->Screen1 = (uint8 *)
			Bus->GetManager()->template GetPointer<ManagerID<PPUID::RAM1ID> >(\
				0x0400 * sizeof(uint8));
		memset(Bus->GetSolderPad()->Screen1, 0x00, 0x0400 * sizeof(uint8));
		Bus->GetSolderPad()->Screen2 = (uint8 *)
			Bus->GetManager()->template GetPointer<ManagerID<PPUID::RAM2ID> >(\
				0x0400 * sizeof(uint8));
		memset(Bus->GetSolderPad()->Screen2, 0x00, 0x0400 * sizeof(uint8));
		Bus->GetManager()->template SetPointer<SState>(&State);
		memset(&State, 0x00, sizeof(State));
		Bus->GetManager()->template SetPointer<SRegisters>(&Registers);
		Bus->GetManager()->template SetPointer<SControlRegisters>(&ControlRegisters);
		Bus->GetManager()->template SetPointer<SSprites>(\
			Sprites, sizeof(SSprites) * 8);
		PalMem = (uint8 *)
			Bus->GetManager()->template GetPointer<ManagerID<PPUID::PalMemID> >(\
				0x20 * sizeof(uint8));
		memset(PalMem, 0x00, 0x20 * sizeof(uint8));
		OAM = (uint8 *)
			Bus->GetManager()->template GetPointer<ManagerID<PPUID::OAMID> >(\
				0x0100 * sizeof(uint8));
		Bus->GetManager()->template SetPointer<SInternalData>(&InternalData);
	}
	inline ~CPPU() {}

	/* Обработать такты */
	inline void Clock(int Cycles) {
		/* Обрабатываем необработанные такты */
		Render(Cycles - PreprocessedCycles);
		PreprocessedCycles = 0;
		CycleData.LastCycle = CycleData.CyclesLeft;
	}

	/* Предобработка */
	inline void PreRender() {
		/* Обрабатываем текущие такты */
		Render(Bus->GetClock()->GetPreCycles() - PreprocessedCycles);
		PreprocessedCycles = Bus->GetClock()->GetPreCycles();
	}

	/* Сброс */
	inline void Reset() {
		FrameReady = false;
		FrameCycles = 0;
		PreprocessedCycles = 0;
		memset(&CycleData, 0, sizeof(CycleData));
		memset(&DMAData, 0, sizeof(DMAData));
		memset(&ControlRegisters, 0, sizeof(ControlRegisters));
		memset(&Registers, 0, sizeof(Registers));
		memset(OAM, 0xff, 0x0100 * sizeof(uint8));
		memset(&InternalData, 0, sizeof(InternalData));
		CycleData.Scanline = -1; /* Начинаем с пре-рендер сканлайна */
		CycleData.CurCycle = 341;
		CycleData.CyclesLeft = 341;
		CycleData.LastCycle = 341;
		InternalData.ShortScanline = true;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		uint8 Res;
		uint16 t;

		PreRender();
		switch (Address & 0x2007) {
			case 0x2002: /* Получить информаию о состоянии */
				InternalData.Trigger = false;
				Res = State.State;
				State.ClearVBlank();
				return Res;
			case 0x2004: /* Взять байт из памяти SPR */
				if (ControlRegisters.ShowSprites && (CycleData.Scanline < 240)
					&& (CycleData.CurCycle < 256))
					return 0xff;
				return OAM[InternalData.OAM_addr];
			case 0x2007: /* Взять байт из памяти PPU */
				PreRender();
				t = Registers.Get2007Address();
				Res = InternalData.Buf_B;
				InternalData.Buf_B = Bus->ReadPPUMemory(t);
				Registers.Increment2007(ControlRegisters.VerticalIncrement);
				Bus->GetROM()->UpdatePPUAddress(Registers.Get2007Address());
				if (t >= 0x3f00)
					Res = ReadPalette(t);
				return Res;
		}
		return 0x40;
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		uint16 t;
		bool old;

		PreRender();
		switch (Address & 0x2007) {
			case 0x2000: /* Установить режим работы */
				old = ControlRegisters.GenerateNMI;
				ControlRegisters.Controller(Src);
				Registers.Write2000(Src);
				if (ControlRegisters.GenerateNMI && !old && State.GetVBlank() &&
					(CycleData.CyclesLeft < 341))
					Bus->GetCPU()->GenerateNMI(GetCycles() * ClockDivider +
					_Bus::CPUClass::ClockDivider);
				break;
			case 0x2001: /* Настроить маску */
				old = ControlRegisters.RenderingEnabled();
				ControlRegisters.Mask(Src);
				if (old && !ControlRegisters.RenderingEnabled())
					Bus->GetROM()->UpdatePPUAddress(Registers.Get2007Address());
				break;
			case 0x2002: /* Мусор */
				State.SetGarbage(Src);
				break;
			case 0x2003: /* Установить указатель памяти SPR */
				InternalData.OAM_addr = Src;
				break;
			case 0x2004: /* Записать в память SPR */
				OAM[InternalData.OAM_addr] = Src;
				InternalData.OAM_addr++;
				break;
			case 0x2005: /* Скроллинг */
				InternalData.Trigger = !InternalData.Trigger;
				if (InternalData.Trigger)
					Registers.Write2005_1(Src);
				else
					Registers.Write2005_2(Src);
				break;
			case 0x2006: /* Установить VRAM указатель */
				InternalData.Trigger = !InternalData.Trigger;
				if (InternalData.Trigger)
					Registers.Write2006_1(Src);
				else {
					Registers.Write2006_2(Src);
					Bus->GetROM()->UpdatePPUAddress(Registers.Get2007Address());
				}
				break;
			case 0x2007: /* Запись в VRAM память */
				t = Registers.Get2007Address();
				if (t < 0x3f00)
					Bus->WritePPUMemory(t, Src);
				else
					WritePalette(t, Src);
				Registers.Increment2007(ControlRegisters.VerticalIncrement);
				Bus->GetROM()->UpdatePPUAddress(Registers.Get2007Address());
				break;
		}
	}

	/* Включить DMA */
	inline void EnableDMA(uint8 Page) {
		PreRender();
		DMAData.Address = Page << 8;
		DMAData.Left = 256;
	}
	
	inline void DoDMA() {
		if (DMAData.Address < 0x2000) {
			DMAData.Address &= 0x07ff;
			if (InternalData.OAM_addr > 0) {
				memcpy(OAM + InternalData.OAM_addr, Bus->GetCPU()->GetRAM() +
					DMAData.Address, (0x0100 - InternalData.OAM_addr) *
					sizeof(uint8));
				memcpy(OAM, Bus->GetCPU()->GetRAM() + DMAData.Address + (0x0100 -
					InternalData.OAM_addr), InternalData.OAM_addr * sizeof(uint8));
			} else {
				memcpy(OAM, Bus->GetCPU()->GetRAM() + DMAData.Address,
					0x0100 * sizeof(uint8));
			}
			DMAData.Left = 0;
		} else
			do {
				OAM[InternalData.OAM_addr] =
					Bus->ReadCPUMemory(DMAData.Address);
				InternalData.OAM_addr++;
				DMAData.Address++;
				DMAData.Left--;
			} while (DMAData.Left > 0);
	}

	/* Флаг окончания рендеринга фрейма */
	inline const bool &IsFrameReady() const { return FrameReady; }
	/* Текущий такт */
	inline int GetCycles() {
		return std::min(CycleData.CyclesLeft, CycleData.CurCycle) -
			CycleData.LastCycle + 1;
	}
	/* Ушло татов на фрейм */
	inline int GetFrameCycles() {
		int Res = FrameCycles;

		/* Сбрасываем состояние */
		FrameCycles = 0;
		FrameReady = false;
		return Res;
	}
};

/* Выборка спрайтов */
template <class _Bus, class _Settings>
inline void CPPU<_Bus, _Settings>::EvaluateSprites() {
	int i = 0; /* Всего найдено */
	int n = 0; /* Текущий спрайт в буфере */
	uint8 *pOAM = OAM; /* Указатель на SPR */

	InternalData.SpriteAddr = 0;
	Sprites[0].x = -1;
	for (;;) {
		if ((((uint8) (InternalData.y - *pOAM)) <
			ControlRegisters.Size) && (*pOAM  < 0xf0)) {
			/* Спрайт попал в диапазон */
			/* Заполняем данные */
			Sprites[i].y = *(pOAM++);
			Sprites[i].Tile = *(pOAM++);
			Sprites[i].Attrib = *(pOAM++);
			Sprites[i].HFlip = Sprites[i].Attrib & 0x40;
			Sprites[i].VFlip = Sprites[i].Attrib & 0x80;
			Sprites[i].Top = ~Sprites[i].Attrib & 0x20;
			Sprites[i].x = *(pOAM++);
			Sprites[i].cx = 8;
			Sprites[i].prim = n == 0;
			i++;
			if (i == 8) /* Нашли 8 спрайтов — память закончилась =( */ {
				State.SetOverflow();
				break;
			}
			Sprites[i].x = -1; /* Конец списка */
		} else
			pOAM += 4;
		 /* Следующий спрайт */
		n++;
		if (n == 64) /* Просмотрели все спрайты */
			break;
	}
}

/* Загрузка битовых образов спрайтов */
template <class _Bus, class _Settings>
inline void CPPU<_Bus, _Settings>::FetchSprite() {
	int i = (CycleData.CurCycle >> 3) & 7;

	switch (CycleData.CurCycle & 7) {
		case 0:
			if (InternalData.SpriteAddr == 0xffff)
				break;
			if (Sprites[i].x < 0) {
				InternalData.SpriteAddr = 0xffff;
				break;
			}
			switch (ControlRegisters.Size) {
				case Size8x8:
					if (Sprites[i].VFlip) /* Flip-V */
						InternalData.SpriteAddr = Registers.GetSpriteAddress(\
							Sprites[i].Tile, 7 + Sprites[i].y - InternalData.y,
							Registers.SpritePage);
					else
						InternalData.SpriteAddr = Registers.GetSpriteAddress(\
							Sprites[i].Tile, InternalData.y - Sprites[i].y,
							Registers.SpritePage);
					break;
				case Size8x16:
					/* Страница спрайтов */
					InternalData.SpriteAddr = (Sprites[i].Tile & 0x01) << 12;
					if (Sprites[i].VFlip) { /* Flip-V */
						if ((InternalData.y - Sprites[i].y) > 7)
							InternalData.SpriteAddr = Registers.GetSpriteAddress(\
								Sprites[i].Tile & 0xfe, 15 + Sprites[i].y -
								InternalData.y, InternalData.SpriteAddr);
						else
							InternalData.SpriteAddr = Registers.GetSpriteAddress(\
								Sprites[i].Tile | 0x01, 7 + Sprites[i].y -
								InternalData.y, InternalData.SpriteAddr);
					} else
						if ((InternalData.y - Sprites[i].y) > 7)
							InternalData.SpriteAddr = Registers.GetSpriteAddress(\
								Sprites[i].Tile | 0x01, 8 + InternalData.y -
								Sprites[i].y, InternalData.SpriteAddr);
						else
							InternalData.SpriteAddr = Registers.GetSpriteAddress(\
								Sprites[i].Tile & 0xfe, InternalData.y -
								Sprites[i].y, InternalData.SpriteAddr);
					break;
				}
			break;
		case 4:
			if (InternalData.SpriteAddr == 0xffff)
				Bus->GetROM()->UpdatePPUAddress((ControlRegisters.Size == Size8x8) ?
					Registers.SpritePage | 0x0ff0 : 0x1ff0);
			else
				Sprites[i].ShiftReg = ColourTable[ReadWithA12(InternalData.SpriteAddr)];
			break;
		case 6:
			if (InternalData.SpriteAddr == 0xffff)
				Bus->GetROM()->UpdatePPUAddress((ControlRegisters.Size == Size8x8) ?
					Registers.SpritePage | 0x0ff8 : 0x1ff8);
			else
				Sprites[i].ShiftReg |=
					ColourTable[ReadWithA12(InternalData.SpriteAddr | 0x08)] << 1;
			break;
	}
}

/* Загрузка битовых образов фона */
template <class _Bus, class _Settings>
inline void CPPU<_Bus, _Settings>::FetchBackground() {
	switch (CycleData.CurCycle & 7) {
		case 0: /* Символ чара */
			Registers.ReadNT(Bus->ReadPPUMemory(Registers.GetNTAddress()));
			break;
		case 2: /* Атрибуты */
			Registers.ReadAT(Bus->ReadPPUMemory(Registers.GetATAddress()));
			break;
		case 4: /* Образ A */
			InternalData.TileA = ReadWithA12(Registers.GetPTAddress());
			break;
		case 6: /* Образ B */
			InternalData.TileB = ReadWithA12(Registers.GetPTAddress() | 0x08);
			break;
	}
}

/* Рисуем пиксель */
template <class _Bus, class _Settings>
inline void CPPU<_Bus, _Settings>::DrawPixel() {
	uint8 i, rspr, col, scol;
	uint16 t;

	col = 0;
	if (ControlRegisters.ShowBackground &&
		(ControlRegisters.BackgroundClip || (InternalData.x > 7))) {
		col = (InternalData.ShiftReg >> (0x10 | (Registers.FH << 1))) & 0x03;
		t = Registers.GetPALAddress(col, Registers.AR);
	} else
		t = 0;
	/* Рисуем спрайты */
	scol = 0x10;
	for (i = 0; i < 8; i++)
		if (Sprites[i].x < 0)
			break;
		else if (Sprites[i].x == InternalData.x) {
			/* Нашли спрайт на этом пикселе */
			if (Sprites[i].cx == 0) /* Спрайт уже закончился */
				continue;
			Sprites[i].cx--;
			Sprites[i].x++;
			if (scol != 0x10) {
				if (Sprites[i].HFlip) { /* H-Flip */
					Sprites[i].ShiftReg >>= 2;
				} else {
					Sprites[i].ShiftReg <<= 2;
				}
				continue;
			}
			if (Sprites[i].HFlip) { /* H-Flip */
				scol |= Sprites[i].ShiftReg & 0x03;
				Sprites[i].ShiftReg >>= 2;
			} else {
				scol |= (Sprites[i].ShiftReg >> 14) & 0x03;
				Sprites[i].ShiftReg <<= 2;
			}
			rspr = i;
		}
	if ((scol != 0x10) && ControlRegisters.ShowSprites &&
		(ControlRegisters.SpriteClip || (InternalData.x > 7))) {
		if (Sprites[rspr].prim && (col != 0) && (InternalData.x < 255)) {
			State.SetSprite0Hit();
		}
		/* Мы не прозрачны, фон пустой или у нас приоритет */
		if ((col == 0) || Sprites[rspr].Top)
			/* Перекрываем пиксель фона */
			t = Registers.GetPALAddress(scol, Sprites[rspr].Attrib);
	}
	col = PalMem[t];
	if (ControlRegisters.Grayscale)
		col &= 0x30;
	else
		col &= 0x3f;
	OutputPixel(InternalData.x, InternalData.y, col, ControlRegisters.Tint);
	InternalData.ShiftReg <<= 2;
	if ((InternalData.x & 0x07) == Registers.FH)
		Registers.AR >>= 2;
	InternalData.x++;
	if (!(InternalData.x & 0x07)) { /* Подгружаем тайлы */
		InternalData.ShiftReg |= ColourTable[InternalData.TileA] |
			(ColourTable[InternalData.TileB] << 1);
		Registers.AR |= Registers.TempAR;
	}
}

/* Точный рендер */
template <class _Bus, class _Settings>
inline void CPPU<_Bus, _Settings>::FineRender(int Cycles) {
	for (; Cycles > 0; CycleData.CurCycle++, Cycles--) {
		if (ControlRegisters.RenderingEnabled()) {
			if (~CycleData.CurCycle & 1) {
				//EvaluateSprites();
				FetchBackground();
			}
			if (CycleData.CurCycle == 251)
				Registers.IncrementY();
			if ((CycleData.CurCycle & 7) == 3)
				Registers.IncrementX();
		}
		DrawPixel();
	}
}

/* Рендер 8 пикселей сразу */
template <class _Bus, class _Settings>
inline void CPPU<_Bus, _Settings>::Render_8(int Cycles) {
	while (Cycles > 0) {
		//CycleData.CurCycle += 8;
		FineRender(8);
		Cycles--;
	}
}

/* Рендер */
template <class _Bus, class _Settings>
inline void CPPU<_Bus, _Settings>::Render(int Cycles) {

#define WAIT_CYCLE(_n) if (CycleData.CyclesLeft <= (_n)) break;\
	if (CycleData.CurCycle == (_n))
#define WAIT_CYCLE_RANGE(_a, _b) if (CycleData.CyclesLeft <= (_a)) break;\
	if (CycleData.CurCycle < (_b))

	int Count, Prec, ActCycles;

	CycleData.PrecCycles += Cycles;
	ActCycles = CycleData.PrecCycles / ClockDivider;
	CycleData.PrecCycles %= ClockDivider;
	CycleData.CyclesLeft += ActCycles;
	CycleData.CyclesTotal += ActCycles;
	CycleData.DebugTimer += ActCycles;
	while (CycleData.CyclesLeft > CycleData.CurCycle) {
		if (CycleData.CurCycle == 0) {
			/* TODO: Если тут DMAData.Left > 0, то большая печалька */
			InternalData.x = 0;
			InternalData.y++;
		}
		/* cc 0 - 255 — видимые пиксели, выборка спрайтов и фетчинг BG и SP */
		if (CycleData.CurCycle < 256) {
			/* Всего нужно отрисовать */
			Count = std::min(256, CycleData.CyclesLeft) - CycleData.CurCycle;
			/* Точная обработка */
			Prec = std::min(Count, 8 - (CycleData.CurCycle & 7));
			/* Обрабатываем */
			FineRender(Prec);
			if (Count > Prec) {
				Render_8((Count - Prec) / 8);
				FineRender((Count - Prec) & 7);
			}
		}
		WAIT_CYCLE(256) {
			EvaluateSprites();
		}
		/* cc 256 - 319 — фетчинг спрайтов */
		//WAIT_CYCLE_RANGE(256, 320) {
		if (CycleData.CurCycle < 320) {
			if (ControlRegisters.RenderingEnabled()) {
				if (CycleData.CurCycle == 256)
					Registers.UpdateScroll();
				else if ((CycleData.CurCycle == 316) &&
					(CycleData.Scanline == (_Settings::ActiveScanlines - 2)))
					InternalData.OAM_addr = 0;
				while (CycleData.CurCycle < std::min(320, CycleData.CyclesLeft)) {
					FetchSprite();
					CycleData.CurCycle += 2;
				}
			} else
				CycleData.CurCycle = std::min(320, CycleData.CyclesLeft +
					(CycleData.CyclesLeft & 1));
		}
		/* cc 320 - 335 — фетчинг фона следующего сканлайна */
		WAIT_CYCLE_RANGE(320, 336) {
			if (ControlRegisters.RenderingEnabled())
				while (CycleData.CurCycle < std::min(336, CycleData.CyclesLeft)) {
					FetchBackground();
					CycleData.CurCycle += 2;
					if ((CycleData.CurCycle & 7) == 0) {
						InternalData.ShiftReg = (InternalData.ShiftReg << 16) |
							ColourTable[InternalData.TileA] |
							(ColourTable[InternalData.TileB] << 1);
						Registers.AR = (Registers.AR >> 2) | Registers.TempAR;
						Registers.IncrementX();
					}
				}
			else
				CycleData.CurCycle = std::min(336, CycleData.CyclesLeft +
					(CycleData.CyclesLeft & 1));
		}
		/* cc 336 — пустое чтение */
		WAIT_CYCLE(336) {
			CycleData.CurCycle++;
		}
		/* cc 337 — пропускаем такт */
		WAIT_CYCLE(337) {
			if (CycleData.Scanline == -1) {
				if (ControlRegisters.RenderingEnabled() && InternalData.ShortScanline) {
					_Settings::SkipPPUClock(CycleData.CyclesLeft);
				}
				InternalData.ShortScanline = !InternalData.ShortScanline;
			}
			CycleData.CurCycle++;
		}
		/* cc 338 - 341 Конец сканлайна, пропускаем 2 лишних чтения и IDLE такт */
		WAIT_CYCLE(338) {
			CycleData.Scanline++;
			if (CycleData.Scanline != _Settings::ActiveScanlines) {
				CycleData.CurCycle = 0;
				CycleData.CyclesLeft -= 341;
				CycleData.LastCycle -= 341;
				continue;
			} else { /* 240 сканлайн — начинаем VBlank в конце сканлайна */
				CycleData.CyclesTotal -= CycleData.CyclesLeft - 338;
				if (!FrameReady) { /* Завершили рендер кадра */
					FrameReady = true;
					FrameCycles = CycleData.CyclesTotal * ClockDivider;
				}
				CycleData.CyclesTotal = CycleData.CyclesLeft - 338;
				/* Ждем 340 такт */
				CycleData.CurCycle = 341 + 340;
				CycleData.CyclesLeft -= (_Settings::PostRender - 1) * 341;
			}
		}
		WAIT_CYCLE(341) { /* Сюда можно попасть только на -1 сканлайне */
			State.State = 0;
			CycleData.Scanline = -1;
			InternalData.y = -1;
		}
		/* На пре-рендер сканлайне делаем пустые запросы */
		if (CycleData.CurCycle < 341 + 256) {
			if (ControlRegisters.RenderingEnabled()) {
				while (CycleData.CurCycle < std::min(341 + 256, CycleData.CyclesLeft)) {
					switch (CycleData.CurCycle & 7) {
						case 5: /* Name Table */
							break;
						case 7: /* Attribute Table */
							break;
						case 1: /* Pattern Table */
						case 3: /* Pattern Table */
							Bus->GetROM()->UpdatePPUAddress(Registers.GetPTPage());
							break;
					}
					CycleData.CurCycle += 2;
				}
			} else
				CycleData.CurCycle = std::min(341 + 256, CycleData.CyclesLeft +
					(~CycleData.CyclesLeft & 1));
		}
		WAIT_CYCLE(341 + 256) {
			EvaluateSprites();
		}
		/* cc 256 - 319 — фетчинг спрайтов + обновление скроллинга */
		if (CycleData.CurCycle < 341 + 320) {
			if (ControlRegisters.RenderingEnabled()) {
				CycleData.CurCycle -= 341;
				CycleData.LastCycle -= 341;
				while (CycleData.CurCycle < std::min(320, CycleData.CyclesLeft - 341)) {
					FetchSprite();
					if (CycleData.CurCycle == 304)
						Registers.RealReg1 = Registers.BigReg1;
					CycleData.CurCycle += 2;
				}
				CycleData.CurCycle += 341;
				CycleData.LastCycle += 341;
			} else
				CycleData.CurCycle = std::min(341 + 320, CycleData.CyclesLeft +
					(~CycleData.CyclesLeft & 1));
		}
		/* с cc 320 возвращаемся на обычный рендеринг */
		WAIT_CYCLE(341 + 320) {
			CycleData.CurCycle = 320;
			CycleData.CyclesLeft -= 341;
			CycleData.LastCycle -= 341;
			continue;
		}
		WAIT_CYCLE(341 + 340) { /* Устанавливаем внутренний флаг */
			State.SetVBlank1();
			CycleData.CurCycle = 341 + 341;
		}
		WAIT_CYCLE(341 + 341) { /* Устанвливаем флаг состояния */
			State.SetVBlank2();
			CycleData.CurCycle = 341 + 343;
		}
		WAIT_CYCLE(341 + 343) { /* Генерируем NMI */
			if (ControlRegisters.GenerateNMI && State.GetVBlank())
				Bus->GetCPU()->GenerateNMI(GetCycles() * ClockDivider);
			/* Период VBlank — 20/70 сканлайнов */
			CycleData.CyclesLeft -= (_Settings::VBlank + 1) * 341;
			CycleData.LastCycle -= (_Settings::VBlank + 1) * 341;
			/* Ждем -1 сканлайн */
			CycleData.CurCycle = 341;
			continue;
		}
		break;
	}

#undef WAIT_CYCLE
#undef WAIT_CYCLE_RANGE

}

template<class _Bus, class _Settings>
const uint16 CPPU<_Bus, _Settings>::ColourTable[256] = {
	0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015, 0x0040, 0x0041,
	0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055, 0x0100, 0x0101, 0x0104, 0x0105,
	0x0110, 0x0111, 0x0114, 0x0115, 0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151,
	0x0154, 0x0155, 0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415,
	0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455, 0x0500, 0x0501,
	0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515, 0x0540, 0x0541, 0x0544, 0x0545,
	0x0550, 0x0551, 0x0554, 0x0555, 0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011,
	0x1014, 0x1015, 0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055,
	0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115, 0x1140, 0x1141,
	0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155, 0x1400, 0x1401, 0x1404, 0x1405,
	0x1410, 0x1411, 0x1414, 0x1415, 0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451,
	0x1454, 0x1455, 0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515,
	0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555, 0x4000, 0x4001,
	0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015, 0x4040, 0x4041, 0x4044, 0x4045,
	0x4050, 0x4051, 0x4054, 0x4055, 0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111,
	0x4114, 0x4115, 0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155,
	0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415, 0x4440, 0x4441,
	0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455, 0x4500, 0x4501, 0x4504, 0x4505,
	0x4510, 0x4511, 0x4514, 0x4515, 0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551,
	0x4554, 0x4555, 0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015,
	0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055, 0x5100, 0x5101,
	0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115, 0x5140, 0x5141, 0x5144, 0x5145,
	0x5150, 0x5151, 0x5154, 0x5155, 0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411,
	0x5414, 0x5415, 0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455,
	0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515, 0x5540, 0x5541,
	0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
};

/* Стандартный ГПУ */
template <class _Settings>
struct StdPPU {
	template <class _Bus>
	class PPU: public CPPU<_Bus, _Settings> {
	public:
		inline explicit PPU(_Bus *pBus): CPPU<_Bus, _Settings>(pBus) {}
		inline ~PPU() {}
	};
};

}

#endif
