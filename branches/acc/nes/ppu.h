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
#include "device.h"
#include "clock.h"

#include <iostream>

namespace vpnes {

/* PPU */
template <class _Bus>
class CPPU: public CDevice<_Bus> {
	using CDevice<_Bus>::Bus;
private:

	/* Состояние PPU */
	struct SState {
		uint8 State;
		inline void SetSprite0Hit() { State |= 0x40; }
		inline void SetVBlank() { State |= 0x80; }
		inline void ClearVBlank() { State &= 0x7f; }
		inline void SetOverflow() { State |= 0x20; }
		inline void SetGarbage(uint8 Src) { State = (State & 0xe0) | (Src & 0x1f); }
	} State;

	/* Внутренние регистры */
	struct SRegisters {
		uint16 BigReg1; /* Первая комбинация */
		uint16 RealReg1; /* Настоящий регистр */
		uint16 BigReg2; /* Вторая комбинация */
		uint16 TempAR;
		uint16 AR; /* Информация о аттрибутах */
		uint16 TempFH; /* Точное смещение по горизонтали */
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
		inline void Write2005_1(uint8 Src) { TempFH = Src & 0x07; BigReg1 = (BigReg1 & 0xffe0) |
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
		inline void UpdateScroll() { RealReg1 = (RealReg1 & 0xfbe0) | (BigReg1 & 0x041f);
			FH = TempFH; }
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

	/* Память */
	uint8 RAM[0x1000];
	/* Палитра */
	uint8 PalMem[0x20];
	/* Спрайты */
	uint8 OAM[0x0100];
	/* Указатель для пикселей */
	uint32 *pixels;
	/* Указатель на палитру */
	uint32 *palette;
	/* Готовность кадра для вывода */
	bool FrameReady;
	/* Текущий scanline */
	int scanline;
	/* Текущий такт */
	int CurClock;
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
	/* Осталось циклов */
	int ClocksLeft;
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
	/* DMA */
	uint16 OAM_DMA;
	/* DMA pos */
	int DMA_pos;
	/* DMA осталось */
	int DMA_left;
	int totalclocks;
	int precclocks;
	bool SupressVBlank;

	/* Вывод точки */
	inline void DrawPixel(int x, int y, uint32 color) {
		if ((x >= 0) && (y >= 0) && (x < 256) && (y < 224))
			pixels[x + y * 256] = color;
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
	inline void EvaluateSprites(int sy);
	inline void FetchSprite();
	inline void FetchBackground();
	inline void FetchNextBackground();
	inline void DrawScanline();
public:
	inline explicit CPPU(_Bus *pBus): pixels(NULL), FrameReady(false), scanline(0), CurClock(0),
		Trigger(false), OAM_addr(0) {
		Bus = pBus;
		memset(RAM, 0x00, 0x1000 * sizeof(uint8));
		memset(PalMem, 0x00, 0x20 * sizeof(uint8));
		memset(&State, 0x00, sizeof(SState));
	}
	inline ~CPPU() {}

	/* Выполнить действие */
	inline void Clock(int DoClocks);

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		uint8 Res;
		uint16 t;
	//	uint16 t1;

		switch (Address & 0x2007) {
			case 0x2002: /* Получить информаию о состоянии */
				Trigger = false;
				Res = State.State;
				SupressVBlank = true;
	//			State.ClearVBlank();
				/* Supression NMI */
	//			static_cast<typename _Bus::CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->GetNMIPin() = false;
	//			t1 = static_cast<typename _Bus::CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->Registers.pc;
	//			if ((t1 == 58119) || (t1 == 58190) || (t1 == 58193))
	//				std::cout << "$2002 read\tClock " << precclocks << std::endl;
				return Res;
			case 0x2004: /* Взять байт из памяти SPR */
				if (ControlRegisters.ShowSprites && ((scanline < 240) || (scanline == 261))
					&& (CurClock < 256))
					return 0xff;
				return OAM[OAM_addr];
			case 0x2007: /* Взять байт из памяти PPU */
				t = Registers.Get2007Address();
				Res = Buf_B;
				Buf_B = Bus->ReadPPUMemory(t);
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
				ControlRegisters.Controller(Src);
				Registers.Write2000(Src);
				break;
			case 0x2001: /* Настроить маску */
				ControlRegisters.Mask(Src);
				break;
			case 0x2002: /* Мусор */
				State.SetGarbage(Src);
				break;
			case 0x2003: /* Установить указатель памяти SPR */
				OAM_addr = Src;
				break;
			case 0x2004: /* Записать в память SPR */
				OAM[OAM_addr] = Src;
				OAM_addr++;
				break;
			case 0x2005: /* Скроллинг */
				Trigger = !Trigger;
				if (Trigger)
					Registers.Write2005_1(Src);
				else
					Registers.Write2005_2(Src);
				break;
			case 0x2006: /* Установить VRAM указатель */
				Trigger = !Trigger;
				if (Trigger)
					Registers.Write2006_1(Src);
				else
					Registers.Write2006_2(Src);
				break;
			case 0x2007: /* Запись в VRAM память */
				t = Registers.Get2007Address();
				if (t < 0x3f00)
					Bus->WritePPUMemory(t, Src);
				else
					WritePalette(t, Src);
				Registers.Increment2007(ControlRegisters.VerticalIncrement);
				break;
		}
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) {
		return RAM[Address & 0x0fff];
	}
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {
		RAM[Address & 0x0fff] = Src;
	}

	/* Сброс */
	inline void Reset() {
		memset(&ControlRegisters, 0, sizeof(SControlRegisters));
		memset(&Registers, 0, sizeof(SRegisters));
		memset(OAM, 0xff, 0x0100 * sizeof(uint8));
		Buf_B = 0;
		scanline = 261; /* Начинаем с пре-рендер сканлайна */
		ClocksLeft = 0;
		CurClock = 0;
		ShortScanline = true;
		TileA = 0;
		TileB = 0;
		ShiftRegA = 0;
		ShiftRegB = 0;
		DMA_left = 0;
		totalclocks = 0;
		precclocks = 0;
		SupressVBlank = false;
	}
	/* Выполнить DMA */
	inline void ProcessDMA(int Passed) {
		int num = (DMA_pos += Passed) / 6;

		if (num <= 0)
			return;
		DMA_pos -= num * 6;
		if (num > DMA_left)
			num = DMA_left;
		DMA_left -= num;
		if (OAM_DMA >= 0x2000) { /* Вышли за границы памяти CPU — обращаемся к шине */
			for (; num > 0; num--) {
				OAM[OAM_addr] = Bus->ReadCPUMemory(OAM_DMA);
				OAM_DMA++;
				OAM_addr++;
			}
		} else { /* Читаем данные напрямую из памяти */
			if ((num + OAM_addr) > 256) {
				memcpy(OAM + OAM_addr,
					static_cast<typename _Bus::CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->DirectAccess() +
					OAM_DMA, (256 - OAM_addr) * sizeof(uint8));
				memcpy(OAM,
					static_cast<typename _Bus::CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->DirectAccess() +
					OAM_DMA + 256 - OAM_addr, (num + OAM_addr - 256) * sizeof(uint8));
			} else
				memcpy(OAM + OAM_addr,
					static_cast<typename _Bus::CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->DirectAccess() +
					OAM_DMA, num * sizeof(uint8));
			OAM_addr += num;
			OAM_DMA += num;
		}
	}

	/* Установить DMA */
	inline void SetDMA(sint16 DMA) {
		OAM_DMA = DMA << 8; /* Адрес в памяти CPU */
		DMA_pos = 0;        /* Число в циклах */
		DMA_left = 256;
	}
	/* Флаг вывода */
	inline bool &IsFrameReady() { return FrameReady; }
	/* Буфер для вывода */
	inline uint32 *&GetBuf() { return pixels; }
	/* Палитра */
	inline uint32 *&GetPalette() { return palette; }
};

/* Махинации с классом */
struct PPU_rebind {
	template <class _Bus>
	struct rebind {
		typedef CPPU<_Bus> rebinded;
	};
};

/* Выборка спрайтов */
template <class _Bus>
inline void CPPU<_Bus>::EvaluateSprites(int sy) {
	int i = 0; /* Всего найдено */
	int n = 0; /* Текущий спрайт в буфере */
	uint8 *pOAM = OAM; /* Указатель на SPR */

	Sprites[0].x = -1;
	for (;;) {
		if (((uint8) (sy - *pOAM)) < ControlRegisters.Size) { /* Спрайт попал в диапазон */
			/* Заполняем данные */
			Sprites[i].y = *(pOAM++);
			Sprites[i].Tile =  *(pOAM++);
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
	int i = (CurClock >> 3) & 0x07;

	if ((SpriteAddr == 0xffff) && !CurClock)
		return;
	switch (CurClock & 0x07) {
		case 4:
			if (Sprites[i].x < 0) {
				SpriteAddr = 0xffff;
				return;
			}
			switch (ControlRegisters.Size) {
					case Size8x8:
						if (Sprites[i].Attrib & 0x80) /* Flip-V */
							SpriteAddr = Registers.GetSpriteAddress(Sprites[i].Tile, 7 + Sprites[i].y - y,
								Registers.SpritePage);
						else
							SpriteAddr = Registers.GetSpriteAddress(Sprites[i].Tile, y - Sprites[i].y,
								Registers.SpritePage);
						break;
					case Size8x16:
						SpriteAddr = (Sprites[i].Tile & 0x01) << 12; /* Страница спрайтов */
						if (Sprites[i].Attrib & 0x80) { /* Flip-V */
							if ((y - Sprites[i].y) > 7)
								SpriteAddr = Registers.GetSpriteAddress(Sprites[i].Tile & 0xfe,
									15 + Sprites[i].y - y, SpriteAddr);
							else
								SpriteAddr = Registers.GetSpriteAddress(Sprites[i].Tile | 0x01,
									7 + Sprites[i].y - y, SpriteAddr);
						} else
							if ((y - Sprites[i].y) > 7)
								SpriteAddr = Registers.GetSpriteAddress(Sprites[i].Tile | 0x01,
									8 + y - Sprites[i].y, SpriteAddr);
							else
								SpriteAddr = Registers.GetSpriteAddress(Sprites[i].Tile & 0xfe,
									y - Sprites[i].y, SpriteAddr);
						break;
				}
			Sprites[i].ShiftRegA = Bus->ReadPPUMemory(SpriteAddr);
			break;
		case 6:
			Sprites[i].ShiftRegB = Bus->ReadPPUMemory(SpriteAddr | 0x08);
			break;
	}
}

/* Загрузка битовых образов фона */
template <class _Bus>
inline void CPPU<_Bus>::FetchBackground() {
	switch (CurClock & 0x07) {
		case 0: /* Символ чара */
			Registers.ReadNT(Bus->ReadPPUMemory(Registers.GetNTAddress()));
			break;
		case 2: /* Атрибуты */
			Registers.ReadAT(Bus->ReadPPUMemory(Registers.GetATAddress()));
			break;
		case 4: /* Образ A */
			TileA = Bus->ReadPPUMemory(Registers.GetPTAddress());
			break;
		case 6: /* Образ B */
			TileB = Bus->ReadPPUMemory(Registers.GetPTAddress() | 0x08);
			Registers.IncrementX();
			break;
	}
}

/* Подгружаем образы */
template <class _Bus>
inline void CPPU<_Bus>::FetchNextBackground() {
	FetchBackground();
	if ((CurClock & 0x07) == 6) {
		ShiftRegA = (ShiftRegA << 8) | TileA;
		ShiftRegB = (ShiftRegB << 8) | TileB;
		Registers.AR >>= 2;
		Registers.AR |= Registers.TempAR;
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
			(ControlRegisters.BackgroundClip || (x > 7))) {
			if (ShiftRegA & (0x8000 >> Registers.FH))
				col |= 1;
			if (ShiftRegB & (0x8000 >> Registers.FH))
				col |= 2;
			t = Registers.GetPALAddress(col, Registers.AR);
		} else
			t = 0;
		/* Рисуем спрайты */
		scol = 0x10;
		for (i = 0; i < 8; i++)
			if (Sprites[i].x < 0)
				break;
			else if (Sprites[i].x == x) { /* Нашли спрайт на этом пикселе */
				Sprites[i].cx++;
				if (Sprites[i].cx == 9) /* Спрайт уже закончился */
					continue;
				Sprites[i].x++;
				if (scol != 0x10) {
					if (Sprites[i].Attrib & 0x40) {
						Sprites[i].ShiftRegA >>= 1;
						Sprites[i].ShiftRegB >>= 1;
					} else {
						Sprites[i].ShiftRegA <<= 1;
						Sprites[i].ShiftRegB <<= 1;
					}
					continue;
				}
				switch (Sprites[i].Attrib & 0x40) {
					case 0x00:
						if (Sprites[i].ShiftRegA & 0x80)
							scol |= 1;
						if (Sprites[i].ShiftRegB & 0x80)
							scol |= 2;
						Sprites[i].ShiftRegA <<= 1;
						Sprites[i].ShiftRegB <<= 1;
						break;
					case 0x40:
						if (Sprites[i].ShiftRegA & 0x01)
							scol |= 1;
						if (Sprites[i].ShiftRegB & 0x01)
							scol |= 2;
						Sprites[i].ShiftRegA >>= 1;
						Sprites[i].ShiftRegB >>= 1;
						break;
				}
				rspr = i;
			}
		if ((scol != 0x10) && ControlRegisters.ShowSprites &&
			(ControlRegisters.SpriteClip || (x > 7)) && (x != 255) && (Sprites[rspr].y != 255)) {
			if (Sprites[rspr].prim && (col != 0))
				State.SetSprite0Hit();
			/* Мы не прозрачны, фон пустой или у нас приоритет */
			if ((col == 0) || (~Sprites[rspr].Attrib & 0x20))
				/* Перекрываем пиксель фона */
				t = Registers.GetPALAddress(scol, Sprites[rspr].Attrib);
		}
		DrawPixel(x, y - 8, palette[PalMem[t] & 0x3f]);
		ShiftRegA <<= 1;
		ShiftRegB <<= 1;
		if ((x & 0x07) == (Registers.FH ^ 0x07))
			Registers.AR >>= 2;
		x++;
		if (!(x & 0x07)) {
			ShiftRegA |= TileA;
			ShiftRegB |= TileB;
			Registers.AR |= Registers.TempAR;
		}
	} while (x & 0x01);
}

/* Выполнить действие */
template <class _Bus>
inline void CPPU<_Bus>::Clock(int DoClocks) {
	if (DMA_left > 0)
		ProcessDMA(DoClocks);
	ClocksLeft += DoClocks;
	precclocks += DoClocks;
	for (;;) {
		if ((ClocksLeft > 0) && (CurClock == 340)) {
			ClocksLeft -= 1;
			totalclocks++;
			CurClock = 0;
			scanline++;
			if (scanline == 262) {
				scanline = 0;
				FrameReady = true;
			}
			if (scanline == 261) { /* sc 261 сброс VBlank и т.п. */
				State.State = 0;
	//			std::cout << "VBlank clear\tClock " << totalclocks << std::endl;
			}
			if (scanline == 241) { /* Начало sc 241 ставим VBlank и выполняем NMI */
	//			std::cout << "VBlank set\tClock " << totalclocks << std::endl;
	//			if (!SupressVBlank) {// || (ClocksLeft & 0x01)) {
					State.SetVBlank();
					if (ControlRegisters.GenerateNMI)
						static_cast<typename _Bus::CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->GetNMIPin() = true;
	//			}
			}
		}
		if (SupressVBlank) {
			State.ClearVBlank();
			SupressVBlank = false;
		}
		if (ClocksLeft < 2)
			break;
		if (scanline == 261) { /* sc 261 пре-рендер сканлайн */
			if (ControlRegisters.ShowSprites) {
				if (CurClock == 256) /* cc 0-255 выборка спрайтов */
					EvaluateSprites(-1);
				if ((CurClock >= 256) && (CurClock <= 319)) /* cc 256-319 загружаем данные спрайтов */
					FetchSprite();
			}
			if (ControlRegisters.ShowSprites || ControlRegisters.ShowBackground) {
				if (CurClock == 304) { /* cc 304 обновляем скроллинг */
					Registers.RealReg1 = Registers.BigReg1;
					Registers.FH = Registers.TempFH;
				}
				if ((CurClock >= 320) && (CurClock <= 335)) /* cc 340-335 загружаем первые 16 пикселей */
					FetchNextBackground();
			}
			if (CurClock == 320) {
				y = 0;
				x = 0;
			}
			if (CurClock == 338) { /* cc 340 пропускается через кадр при включенном рендерере */
				/* 340 такт проскакивает, поэтому делаем проверку за шаг до этого */
				if (ControlRegisters.ShowBackground) {
					ShortScanline = !ShortScanline;
					ClocksLeft++;
					totalclocks--;
				} else
					ShortScanline = false;
			}
		}
		if (scanline <= 239) { /* sc 0-239 рендеринг видимой области */
			if (ControlRegisters.ShowSprites) {
				if (CurClock == 256) /* cc 0-255 выборка спрайтов */
					EvaluateSprites(y);
				if ((CurClock >= 256) && (CurClock <= 319)) /* cc 256-319 загружаем данные спрайтов */
					FetchSprite();
			}
			if (ControlRegisters.ShowSprites || ControlRegisters.ShowBackground) {
				if (CurClock <= 255) /* cc 0-255 подгружаем данные */
					FetchBackground(); /* Данные для _следующих_ 16 пикселей */
				if ((CurClock >= 320) && (CurClock <= 335)) /* cc 320-335 загружаем 16 пикселей */
					FetchNextBackground();
				if (CurClock == 256) { /* cc 256 обновляем скроллинг */
					Registers.IncrementY();
					Registers.UpdateScroll();
				}
			}
			if (CurClock <= 255) /* cc 0-255 рисуем сканлайн */
				DrawScanline();
			if (CurClock == 320) {
				y++;
				x = 0;
			}
		}
		CurClock += 2;
		ClocksLeft -= 2;
		totalclocks += 2;
	}
}

}

#endif
