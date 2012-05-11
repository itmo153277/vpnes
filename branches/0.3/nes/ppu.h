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

#include <cstring>
#include "manager.h"
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
template <class _Bus>
class CPPU: public CDevice {
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
	struct SCycleData {
		int CurCycle; /* Текущий такт */
		int CyclesLeft; /* Необработанные такты */
		int Scanline; /* Текущий сканлайн */
		bool ShortFrame; /* Короткий фрейм */
	} CycleData;

	/* Данные для обработки DMA */
	struct SDMAData {
		uint8 Latch; /* Буфер */
		uint16 Address; /* Текущий адрес */
		bool UseLatch; /* Использовать буфер */
	} DMAData;

	/* Состояние PPU */
	struct SState {
		uint8 State;
		/* 0 объект */
		inline void SetSprite0Hit() { State |= 0x40; }
		/* VBlank set */
		inline void SetVBlank() { State |= 0x80; }
		/* VBlank clear */
		inline void ClearVBlank() { State &= 0x7f; }
		/* Overflow set */
		inline void SetOverflow() { State |= 0x20; }
		/* Garbadge */
		inline void SetGarbage(uint8 Src) { State = (State & 0xe0) | (Src & 0x1f); }
	} State;

	/* Внутренние регистры */
	struct SRegisters {
		uint16 BigReg1; /* Первая комбинация */
		uint16 RealReg1; /* Настоящий регистр */
		uint16 BigReg2; /* Вторая комбинация */
		uint16 TempAR;
		uint16 AR; /* Информация о аттрибутах */
		uint16 FHMask; /* Маска регистра */
		uint16 FH; /* Настоящий регистр */
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
		/* Получить адрес для чтения Palette */
		inline uint8 GetPALAddress(uint8 col, uint8 CAR) { if ((col & 0x03) == 0)
			return 0x00; else return ((CAR & 0x03) << 2) | col; }
		/* Получить адрес для чтения PatternTable спрайтов */
		inline uint16 GetSpriteAddress(uint8 Tile, uint8 v, uint16 CurPage) {
			return CurPage | (Tile << 4) | v; }
		/* Запись в 0x2000 */
		inline void Write2000(uint8 Src) { BigReg1 = (BigReg1 & 0xf3ff) |
			((Src & 0x03) << 10) ; BigReg2 = (BigReg2 & 0x0fff) | ((Src & 0x10) << 8);
			SpritePage = (Src & 0x08) << 9; }
		/* Запись в 0x2005/1 */
		inline void Write2005_1(uint8 Src) { FH = Src & 0x07; BigReg1 = (BigReg1 & 0xffe0) |
			(Src >> 3); }
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
		inline void Increment2007(bool VerticalIncrement) { if (VerticalIncrement)
			RealReg1 += 0x0020; else RealReg1++; }
		inline void IncrementX() { uint16 Src = (RealReg1 & 0x001f); Src++; RealReg1 = (RealReg1 & 0xffe0)
			| (Src & 0x001f); RealReg1 ^= (Src & 0x0020) << 5; }
		inline void IncrementY() { uint8 Src = (RealReg1 >> 12) | ((RealReg1 & 0x03e0) >> 2);
			if (Src == 239) { Src = 0; RealReg1 ^= 0x0800; } else Src++; RealReg1 = (RealReg1 & 0x8c1f) |
			((Src & 0x0007) << 12) | ((Src & 0x00f8) << 2); }
		/* Обновление скроллинга */
		inline void UpdateScroll() { RealReg1 = (RealReg1 & 0xfbe0) | (BigReg1 & 0x041f);
			FHMask = 0x8000 >> FH; }
	} Registers;

	/* Размер спрайта */
	enum SpriteSize {
		Size8x8 = 8,
		Size8x16 = 16
	};

	/* Управляющие регистры */
	struct SControlRegisters {
		bool VerticalIncrement; /* Инкремент по Y */
		SpriteSize Size; /* Размер спрайтов */
		bool GenerateNMI; /* Генерировать NMI */
		bool Grayscale; /* Черно-белый */
		bool BackgroundClip; /* Не показывать 8 пикселей слева */
		bool SpriteClip; /* Не показывать 8 пикселей слева */
		bool ShowBackground; /* Рендерить фон */
		bool ShowSprites; /* Рендерить спрайты */
		bool IntensifyRed; /* Только красный */
		bool IntensifyGreen; /* Только зеленый */
		bool IntensifyBlue; /* Только синий */
		inline void Controller(uint8 Src) { VerticalIncrement = Src & 0x04;
			Size = (Src & 0x20) ? Size8x16 : Size8x8; GenerateNMI = Src & 0x80; }
		inline void Mask(uint8 Src) { Grayscale = Src & 0x01;
			BackgroundClip = Src & 0x02; SpriteClip = Src & 0x04;
			ShowBackground = Src & 0x08; ShowSprites = Src & 0x10;
			IntensifyRed = Src & 0x20; IntensifyGreen = Src & 0x40;
			IntensifyBlue = Src & 0x80; }
		inline bool RenderingEnabled() { return ShowBackground || ShowSprites; }
	} ControlRegisters;

	/* Данные спрайтов */
	struct SSprites {
		int x; /* Координаты */
		int y;
		int cx; /* Текущая x */
		bool prim;
		uint8 ShiftRegA; /* Регистр сдвига A */
		uint8 ShiftRegB; /* Регистр сдвига B */
		uint8 Attrib; /* Аттрибуты */
		uint8 Tile; /* Чар */
	} Sprites[8];


	/* Палитра */
	uint8 *PalMem;
	/* Спрайты */
	uint8 *OAM;
	/* Данные видеобуфера */
	VPNES_VBUF *vbuf;

	struct SInternalData {
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
		/* Регистр сдвига A */
		uint16 ShiftRegA;
		/* Регистр сдвига B */
		uint16 ShiftRegB;
		/* Подавить VBlank */
		bool SupressVBlank;
	} InternalData;

	/* Вывод точки */
	inline void DrawPixel(int x, int y, uint32 color) {
		if ((x >= 0) && (y >= 0) && (x < 256) && (y < 224))
			vbuf->Buf[x + y * 256] = color;
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

	/* Выборка спрайтов */
	inline void EvaluateSprites();
	/* Загрузить спрайт */
	inline void FetchSprite();
	/* Загрузить фон */
	inline void FetchBackground();
	/* Загрузить фон следующего сканлайна */
	inline void FetchNextBackground();
	/* Рисуем сканалайн */
	inline void DrawScanline();
	/* Рендер */
	inline void Render(int Cycles);
public:
	inline explicit CPPU(_Bus *pBus, VPNES_VBUF *Buf) {
		Bus = pBus;
		Bus->GetManager()->template SetPointer<PPUID::CycleDataID>(\
			&CycleData, sizeof(CycleData));
		Bus->GetManager()->template SetPointer<PPUID::DMADataID>(\
			&DMAData, sizeof(DMAData));
		Bus->GetSolderPad()->Screen1 = (uint8 *)
			Bus->GetManager()->template GetPointer<PPUID::RAM1ID>(\
				0x0800 * sizeof(uint8));
		memset(Bus->GetSolderPad()->Screen1, 0x00, 0x0800 * sizeof(uint8));
		Bus->GetSolderPad()->Screen2 = (uint8 *)
			Bus->GetManager()->template GetPointer<PPUID::RAM2ID>(\
				0x0800 * sizeof(uint8));
		memset(Bus->GetSolderPad()->Screen2, 0x00, 0x0800 * sizeof(uint8));
		Bus->GetManager()->template SetPointer<PPUID::StateID>(\
			&State, sizeof(State));
		memset(&State, 0x00, sizeof(State));
		Bus->GetManager()->template SetPointer<PPUID::RegistersID>(\
			&Registers, sizeof(Registers));
		Bus->GetManager()->template SetPointer<PPUID::ControlRegistersID>(\
			&ControlRegisters, sizeof(ControlRegisters));
		Bus->GetManager()->template SetPointer<PPUID::SpritesID>(\
			Sprites, sizeof(SSprites) * 8);
		PalMem = (uint8 *)
			Bus->GetManager()->template GetPointer<PPUID::PalMemID>(\
				0x20 * sizeof(uint8));
		memset(PalMem, 0x00, 0x20 * sizeof(uint8));
		OAM = (uint8 *)
			Bus->GetManager()->template GetPointer<PPUID::OAMID>(\
				0x0100 * sizeof(uint8));
		vbuf = Buf;
		Bus->GetManager()->template SetPointer<PPUID::InternalDataID>(\
			&InternalData, sizeof(InternalData));
	}
	inline ~CPPU() {}

	/* Обработать такты */
	inline void Clock(int Cycles) {
		/* Обрабатываем необработанные такты */
		Render(Cycles - PreprocessedCycles);
		PreprocessedCycles = 0;
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
		PreprocessedCycles = 0;
		memset(&CycleData, 0, sizeof(CycleData));
		memset(&DMAData, 0, sizeof(DMAData));
		memset(&ControlRegisters, 0, sizeof(ControlRegisters));
		memset(&Registers, 0, sizeof(Registers));
		memset(OAM, 0xff, 0x0100 * sizeof(uint8));
		memset(&InternalData, 0, sizeof(InternalData));
		CycleData.Scanline = 261; /* Начинаем с пре-рендер сканлайна */
		InternalData.ShortScanline = true;
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		uint8 Res;
		uint16 t;

		switch (Address & 0x2007) {
			case 0x2002: /* Получить информаию о состоянии */
				InternalData.Trigger = false;
				Res = State.State;
				InternalData.SupressVBlank = true;
				return Res;
			case 0x2004: /* Взять байт из памяти SPR */
				if (ControlRegisters.ShowSprites && ((CycleData.Scanline < 240) ||
					(CycleData.Scanline == 261)) && (CycleData.CurCycle < 256))
					return 0xff;
				return OAM[InternalData.OAM_addr];
			case 0x2007: /* Взять байт из памяти PPU */
				PreRender();
				t = Registers.Get2007Address();
				Res = InternalData.Buf_B;
				InternalData.Buf_B = Bus->ReadPPUMemory(t);
				Registers.Increment2007(ControlRegisters.VerticalIncrement);
				if (t >= 0x3f00)
					Res = ReadPalette(t);
				return Res;
		}
		return 0x00;
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		uint16 t;

		switch (Address & 0x2007) {
			case 0x2000: /* Установить режим работы */
				PreRender();
				ControlRegisters.Controller(Src);
				Registers.Write2000(Src);
				break;
			case 0x2001: /* Настроить маску */
				PreRender();
				ControlRegisters.Mask(Src);
				break;
			case 0x2002: /* Мусор */
				State.SetGarbage(Src);
				break;
			case 0x2003: /* Установить указатель памяти SPR */
				PreRender();
				InternalData.OAM_addr = Src;
				break;
			case 0x2004: /* Записать в память SPR */
				PreRender();
				OAM[InternalData.OAM_addr] = Src;
				InternalData.OAM_addr++;
				break;
			case 0x2005: /* Скроллинг */
				PreRender();
				InternalData.Trigger = !InternalData.Trigger;
				if (InternalData.Trigger)
					Registers.Write2005_1(Src);
				else
					Registers.Write2005_2(Src);
				break;
			case 0x2006: /* Установить VRAM указатель */
				PreRender();
				InternalData.Trigger = !InternalData.Trigger;
				if (InternalData.Trigger)
					Registers.Write2006_1(Src);
				else
					Registers.Write2006_2(Src);
				break;
			case 0x2007: /* Запись в VRAM память */
				PreRender();
				t = Registers.Get2007Address();
				if (t < 0x3f00)
					Bus->WritePPUMemory(t, Src);
				else
					WritePalette(t, Src);
				Registers.Increment2007(ControlRegisters.VerticalIncrement);
				break;
		}
	}

	/* Обработка DMA */
	inline void ProccessDMA(int Cycles) {
	}

	/* Флаг окончания рендеринга фрейма */
	inline const bool &IsFrameReady() const { return FrameReady; }
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
template <class _Bus>
inline void CPPU<_Bus>::EvaluateSprites() {
	int i = 0; /* Всего найдено */
	int n = 0; /* Текущий спрайт в буфере */
	uint8 *pOAM = OAM; /* Указатель на SPR */

	Sprites[0].x = -1;
	for (;;) {
		if ((((uint8) (InternalData.y - *pOAM)) <
			ControlRegisters.Size) && (*pOAM != 0xff)) {
			/* Спрайт попал в диапазон */
			/* Заполняем данные */
			Sprites[i].y = *(pOAM++);
			Sprites[i].Tile = *(pOAM++);
			Sprites[i].Attrib = *(pOAM++);
			Sprites[i].x = *(pOAM++);
			Sprites[i].cx = 0;
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
			return;
	}
}

/* Загрузка битовых образов спрайтов */
template <class _Bus>
inline void CPPU<_Bus>::FetchSprite() {
	int i = (CycleData.CurCycle >> 3) & 0x07;

	if ((InternalData.SpriteAddr == 0xffff) && !CycleData.CurCycle)
		return;
	switch (CycleData.CurCycle & 0x07) {
		case 4:
			if (Sprites[i].x < 0) {
				InternalData.SpriteAddr = 0xffff;
				return;
			}
			switch (ControlRegisters.Size) {
				case Size8x8:
					if (Sprites[i].Attrib & 0x80) /* Flip-V */
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
					if (Sprites[i].Attrib & 0x80) { /* Flip-V */
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
			Sprites[i].ShiftRegA = Bus->ReadPPUMemory(InternalData.SpriteAddr);
			break;
		case 6:
			Sprites[i].ShiftRegB = Bus->ReadPPUMemory(InternalData.SpriteAddr | 0x08);
			break;
	}
}

/* Загрузка битовых образов фона */
template <class _Bus>
inline void CPPU<_Bus>::FetchBackground() {
	switch (CycleData.CurCycle & 0x07) {
		case 0: /* Символ чара */
			Registers.ReadNT(Bus->ReadPPUMemory(Registers.GetNTAddress()));
			break;
		case 2: /* Атрибуты */
			Registers.ReadAT(Bus->ReadPPUMemory(Registers.GetATAddress()));
			break;
		case 4: /* Образ A */
			InternalData.TileA = Bus->ReadPPUMemory(Registers.GetPTAddress());
			break;
		case 6: /* Образ B */
			InternalData.TileB = Bus->ReadPPUMemory(Registers.GetPTAddress() | 0x08);
			Registers.IncrementX();
			break;
	}
}

/* Подгружаем образы */
template <class _Bus>
inline void CPPU<_Bus>::FetchNextBackground() {
	FetchBackground();
	if ((CycleData.CurCycle & 0x07) == 6) {
		InternalData.ShiftRegA = (InternalData.ShiftRegA << 8) | InternalData.TileA;
		InternalData.ShiftRegB = (InternalData.ShiftRegB << 8) | InternalData.TileB;
		Registers.AR = (Registers.AR >> 2) | Registers.TempAR;
	}
}

/* Рисуем пиксель */
template <class _Bus>
inline void CPPU<_Bus>::DrawScanline() {
	int i, rspr, col, scol;
	uint16 t;

	do {
		col = 0;
		if (ControlRegisters.ShowBackground &&
			(ControlRegisters.BackgroundClip || (InternalData.x > 7))) {
			if (InternalData.ShiftRegA & Registers.FHMask)
				col |= 1;
			if (InternalData.ShiftRegB & Registers.FHMask)
				col |= 2;
			t = Registers.GetPALAddress(col, Registers.AR);
		} else
			t = 0;
		/* Рисуем спрайты */
		scol = 0x10;
		if (InternalData.x < 255) {
			for (i = 0; i < 8; i++)
				if (Sprites[i].x < 0)
					break;
				else if (Sprites[i].x == InternalData.x) {
					/* Нашли спрайт на этом пикселе */
					Sprites[i].cx++;
					if (Sprites[i].cx == 9) /* Спрайт уже закончился */
						continue;
					Sprites[i].x++;
					if (scol != 0x10) {
						if (Sprites[i].Attrib & 0x40) { /* H-Flip */
							Sprites[i].ShiftRegA >>= 1;
							Sprites[i].ShiftRegB >>= 1;
						} else {
							Sprites[i].ShiftRegA <<= 1;
							Sprites[i].ShiftRegB <<= 1;
						}
						continue;
					}
					if (Sprites[i].Attrib & 0x40) { /* H-Flip */
						if (Sprites[i].ShiftRegA & 0x01)
							scol |= 1;
						if (Sprites[i].ShiftRegB & 0x01)
							scol |= 2;
						Sprites[i].ShiftRegA >>= 1;
						Sprites[i].ShiftRegB >>= 1;
					} else {
						if (Sprites[i].ShiftRegA & 0x80)
							scol |= 1;
						if (Sprites[i].ShiftRegB & 0x80)
							scol |= 2;
						Sprites[i].ShiftRegA <<= 1;
						Sprites[i].ShiftRegB <<= 1;
					}
					rspr = i;
				}
		}
		if ((scol != 0x10) && ControlRegisters.ShowSprites &&
			(ControlRegisters.SpriteClip || (InternalData.x > 7))) {
			if (Sprites[rspr].prim && (col != 0)) {
				State.SetSprite0Hit();
			}
			/* Мы не прозрачны, фон пустой или у нас приоритет */
			if ((col == 0) || (~Sprites[rspr].Attrib & 0x20))
				/* Перекрываем пиксель фона */
				t = Registers.GetPALAddress(scol, Sprites[rspr].Attrib);
		}
		DrawPixel(InternalData.x, InternalData.y - 8, vbuf->Pal[PalMem[t] & 0x3f]);
		InternalData.ShiftRegA <<= 1;
		InternalData.ShiftRegB <<= 1;
		if ((InternalData.x & 0x07) == (Registers.FH ^ 0x07))
			Registers.AR >>= 2;
		InternalData.x++;
		if (!(InternalData.x & 0x07)) { /* Подгружаем тайлы */
			InternalData.ShiftRegA |= InternalData.TileA;
			InternalData.ShiftRegB |= InternalData.TileB;
			Registers.AR |= Registers.TempAR;
		}
	} while (InternalData.x & 0x01);
}

/* Рендер */
template <class _Bus>
inline void CPPU<_Bus>::Render(int Cycles) {
	CycleData.CyclesLeft += Cycles;
	for (;;) {
		if ((CycleData.CyclesLeft > 0) && (CycleData.CurCycle == 340)) {
			CycleData.CyclesLeft--;
			CycleData.CurCycle = 0;
			CycleData.Scanline++;
			if (CycleData.Scanline == 262) {
				CycleData.Scanline = 0;
			}
			if (CycleData.Scanline == 261) { /* sc 261 сброс VBlank и т.п. */
				State.State = 0;
			}
			if (CycleData.Scanline == 241) { /* Начало sc 241 ставим VBlank и выполняем NMI */
				if (!InternalData.SupressVBlank) {
					State.SetVBlank();
					//Generate NMI
				}
				FrameReady = true;
				FrameCycles = 341 * 262;
				InternalData.y = -1;
			}
		}
		if (InternalData.SupressVBlank) { /* Подавляем VBlank (а как же NMI? T_T )*/
			State.ClearVBlank();
			InternalData.SupressVBlank = false;
		}
		if (CycleData.CyclesLeft < 2)
			break;
		if (CycleData.Scanline == 261) { /* sc 261 пре-рендер сканлайн */
			if (ControlRegisters.ShowSprites) {
				if (CycleData.CurCycle == 256) /* cc 0-255 выборка спрайтов */
					EvaluateSprites();
				/* cc 256-319 загружаем данные спрайтов */
				if ((CycleData.CurCycle >= 256) && (CycleData.CurCycle <= 319))
					FetchSprite();
			}
			if (ControlRegisters.RenderingEnabled()) {
				if (CycleData.CurCycle == 304) { /* cc 304 обновляем скроллинг */
					Registers.RealReg1 = Registers.BigReg1;
					Registers.FHMask = 0x8000 >> Registers.FH;
				}
				/* cc 340-335 загружаем первые 16 пикселей */
				if ((CycleData.CurCycle >= 320) && (CycleData.CurCycle <= 335))
					FetchNextBackground();
			}
			if (CycleData.CurCycle == 320) {
				InternalData.y++;
				InternalData.x = 0;
			}
			if (CycleData.CurCycle == 338) { /* Короткий сканлайн */
				if (ControlRegisters.ShowBackground) {
					if (InternalData.ShortScanline) {
						CycleData.CyclesLeft++;
					}
					InternalData.ShortScanline = !InternalData.ShortScanline;
				} else
					InternalData.ShortScanline = false;
			}
		}
		if (CycleData.Scanline <= 239) { /* sc 0-239 рендеринг видимой области */
			if (ControlRegisters.ShowSprites) {
				if (CycleData.CurCycle == 256) /* cc 0-255 выборка спрайтов */
					EvaluateSprites();
				/* cc 256-319 загружаем данные спрайтов */
				if ((CycleData.CurCycle >= 256) && (CycleData.CurCycle <= 319))
					FetchSprite();
			}
			if (ControlRegisters.RenderingEnabled()) {
				if (CycleData.CurCycle <= 255) /* cc 0-255 подгружаем данные */
					FetchBackground(); /* Данные для _следующих_ 16 пикселей */
				/* cc 320-335 загружаем 16 пикселей */
				if ((CycleData.CurCycle >= 320) && (CycleData.CurCycle <= 335))
					FetchNextBackground();
				if (CycleData.CurCycle == 250) /* cc 250 инкремент y */
					Registers.IncrementY();
				if (CycleData.CurCycle == 256) { /* cc 257 обновляем скроллинг */
					Registers.UpdateScroll();
				}
			}
			if (CycleData.CurCycle <= 255) /* cc 0-255 рисуем сканлайн */
				DrawScanline();
			if (CycleData.CurCycle == 320) {
				InternalData.y++;
				InternalData.x = 0;
			}
		}
		CycleData.CurCycle += 2;
		CycleData.CyclesLeft -= 2;
	}
}

}

#endif
