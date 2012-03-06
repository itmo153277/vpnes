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

namespace vpnes {

/* PPU */
template <class _Bus>
class CPPU: public CClockedDevice<_Bus> {
	using CClockedDevice<_Bus>::Clocks;
	using CDevice<_Bus>::Bus;
private:

	/* Состояние PPU */
	struct SState {
		uint8 Garbage; /* Мусор */
		bool Overflow; /* Превышение лимита на спрайты */
		bool Sprite0Hit; /* Первый спрайт */
		bool VBlank; /* VBlank */
		/* Упаковать все */
		inline uint8 PackState() { return (Garbage & 0x1f) | (Overflow << 5) |
			(Sprite0Hit << 6) | (VBlank << 7); }
	} State;

	/* Внутренние регистры */
	struct SRegisters {
		uint16 BigReg1; /* Первая комбинация */
		uint16 RealReg1; /* Настоящий регистр */
		uint16 BigReg2; /* Вторая комбинация */
		uint16 AR; /* Информация о аттрибутах */
		uint16 FH; /* Точное смещение по горизонтали */
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
		inline void ReadAT(uint8 Src) { AR = Src >> (((RealReg1 >> 4) & 0x004) | (RealReg1 & 0x0002)); }
		/* Инкременты */
		inline void Increment2007(bool VerticalIncrement) { if (VerticalIncrement)
			RealReg1 += 0x0020; else RealReg1++; }
		inline void IncrementX() { uint16 Src = (RealReg1 & 0x001f); Src++; RealReg1 = (RealReg1 & 0xffe0)
			| (Src & 0x001f); RealReg1 ^= (Src & 0x0020) << 5; }
		inline void IncrementY() { uint8 Src = (RealReg1 >> 12) | ((RealReg1 & 0x03e0) >> 2);
			if (Src == 239) { Src = 0; RealReg1 ^= 0x800; } else Src++; RealReg1 = (RealReg1 & 0x8c1f) |
			((Src & 0x0007) << 12) | ((Src & 0x00f8) << 2); RealReg1 ^= 0x0400; }
		inline void BeginScanline() { RealReg1 = (RealReg1 & 0xfbe0) | (BigReg1 & 0x041f); }
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
	/* Буфер для спрайтов */
	uint8 Sec_OAM[0x20];
	/* Указатель для пикселей */
	uint32 *pixels;
	/* Указатель на палитру */
	uint32 *palette;
	/* Готовность кадра для вывода */
	bool FrameReady;
	/* Текущий scanline */
	int scanline;
	/* Внутренний флаг */
	bool Trigger;
	/* Внутренний буфер */
	uint8 Buf_B;
	/* Текущий адрес OAM */
	uint8 OAM_addr;
	/* Битовый образ чара */
	uint8 TileA;
	/* Битовый образ чара */
	uint8 TileB;
	/* Регистр сдвига */
	uint8 ShiftRegA;
	/* Регистр сдвига */
	uint8 ShiftRegB;
	/* DMA */
	uint16 OAM_DMA;
	/* DMA pos */
	int DMA_pos;
	/* DMA flag */
	bool DMA_use;
	int cur_frame;

	/* Вывод точки */
	inline void DrawPixel(int x, int y, uint32 color) {
		if ((x >= 0) && (y >= 0) && (x < 256) && (y < 224))
			pixels[x + y * 256] = color;
	}
	/* Фетчинг */
	inline void FetchData() {
		uint16 t;

		if (ControlRegisters.ShowBackground) {
			Registers.ReadNT(Bus->ReadPPUMemory(Registers.GetNTAddress()));
			Registers.ReadAT(Bus->ReadPPUMemory(Registers.GetATAddress()));
			t = Registers.GetPTAddress();
			TileA = Bus->ReadPPUMemory(t);
			TileB = Bus->ReadPPUMemory(t | 0x08);
			ShiftRegA = TileA << Registers.FH;
			ShiftRegB = TileB << Registers.FH;
		} else {
			ShiftRegA = 0;
			ShiftRegB = 0;
		}
	}
	/* Определение спрайтов */
	inline void EvaluateSprites(int y) {
		uint8 *d = Sec_OAM;
		uint8 *s = OAM;
		int n = 0;
		int m = 0;
		int i = 0;

		/* Волшебный код, который я нифига не понимаю */
		memset(Sec_OAM, 0xff, 0x20 * sizeof(uint8));
		while (true) {
			n++;
			if (n == 64)
				break;
			if ((y >= *s) && (y < (*s + ControlRegisters.Size))) {
				*d = *s;
				*(d + 1) = *(s + 1);
				*(d + 2) = *(s + 2);
				*(d + 3) = *(s + 3);
				d += 4;
				i++;
				if (i == 8) {
					s += 4;
					while (true) {
						if ((y >= *s) && (y < (*s + ControlRegisters.Size))) {
							State.Overflow = true;
							n++;
							s += 4;
						} else {
							n++;
							m++;
							if (m == 4) {
								m = 0;
								s += 1;
							} else
								s += 5;
						}
						if (n == 64)
							break;
					}
					break;
				}
			}
			s += 4;
		}
	}

	/* Отработать команду */
	inline int PerformOperation();
	/* Отработать команду */
	inline void RenderScanline();

public:
	inline explicit CPPU(_Bus *pBus): pixels(NULL), FrameReady(false), scanline(0),
		Trigger(false), OAM_addr(0) {
		Bus = pBus;
		memset(&Registers, 0, sizeof(SRegisters));
		memset(RAM, 0x00, 0x1000 * sizeof(uint8));
		memset(PalMem, 0x00, 0x20 * sizeof(uint8));
	}
	inline ~CPPU() {}

	/* Выполнить DMA */
	inline void ProcessDMA() {
		int num = DMA_pos / 6;

		DMA_pos -= num * 6;
		for (; num > 0; num--) {
			OAM[OAM_addr] = Bus->ReadCPUMemory(OAM_DMA);
			OAM_DMA++;
			OAM_addr++;
			if (OAM_addr == 0) {
				OAM_addr = 0;
				DMA_use = false;
				break;
			}
		}
	}

	/* Установить DMA */
	inline void SetDMA(sint16 DMA) {
		OAM_DMA = DMA << 8;
		OAM_addr = 0;
		DMA_pos = 0;
		DMA_use = true;
	}

	/* Выполнить действие */
	inline void Clock(int DoClocks) {
		if (DMA_use)
			DMA_pos += DoClocks;
		if ((Clocks -= DoClocks) == 0) {
			Clocks = PerformOperation();
		}
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		uint8 Res;

		switch (Address & 0x2007) {
			case 0x2002:
				Trigger = false;
				Res = State.PackState();
				State.VBlank = false;
				State.Overflow = false;
				State.Sprite0Hit = false;
				return Res;
			case 0x2004:
				return OAM[OAM_addr];
			case 0x2007:
				Res = Buf_B;
				Buf_B = Bus->ReadPPUMemory(Registers.Get2007Address());
				Registers.Increment2007(ControlRegisters.VerticalIncrement);
				return Res;
		}
		return 0x00;
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		switch (Address & 0x2007) {
			case 0x2000:
				ControlRegisters.Controller(Src);
				Registers.Write2000(Src);
				break;
			case 0x2001:
				ControlRegisters.Mask(Src);
				break;
			case 0x2002:
				State.Garbage = Src;
				break;
			case 0x2003:
				OAM_addr = Src;
				break;
			case 0x2004:
				OAM[OAM_addr] = Src;
				OAM_addr++;
				break;
			case 0x2005:
				Trigger = !Trigger;
				if (Trigger)
					Registers.Write2005_1(Src);
				else
					Registers.Write2005_2(Src);
				break;
			case 0x2006:
				Trigger = !Trigger;
				if (Trigger)
					Registers.Write2006_1(Src);
				else
					Registers.Write2006_2(Src);
				break;
			case 0x2007:
				if (Registers.Get2007Address() == 0x2883) {
					Buf_B = Src;
				}
				Buf_B = Src;
				Bus->WritePPUMemory(Registers.Get2007Address(), Src);
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
		ControlRegisters.ShowBackground = true;
		memset(&State, 0, sizeof(SState));
		memset(OAM, 0xff, 0x0100 * sizeof(uint8));
		Registers.BigReg1 = 0;
		Registers.BigReg2 = 0;
		Buf_B = 0;
		scanline = 21;
		DMA_use = false;
		cur_frame = 0;
	}

	/* Флаг вывода */
	inline bool &IsFrameReady() { return FrameReady; }
	/* Буфер для вывода */
	inline uint32 *&GetBuf() { return pixels; }
	/* Палитра */
	inline uint32 *&GetPalette() { return palette; }
	/* Палитра */
	inline uint8 *GetPalMem() { return PalMem; }
};

/* Махинации с классом */
struct PPU_rebind {
	template <class _Bus>
	struct rebind {
		typedef CPPU<_Bus> rebinded;
	};
};

/* Рендер */
/* Код очень приблизительный, надо переписать полностью */
template <class _Bus>
inline void CPPU<_Bus>::RenderScanline() {
	int x, y, i;
	int col, scol;
	uint8 *s = Sec_OAM;
	uint16 t;

	if ((scanline >= 21) && (scanline <= 261)) {
		y = scanline - 21;
		x = 0;
		if (ControlRegisters.RenderingEnabled()) {
			Registers.BeginScanline();
			if (ControlRegisters.ShowSprites)
				EvaluateSprites(y);
			while (true) {
				if ((scanline > 28) && (scanline < 253)) {
					while (Registers.FH < 8) {
						col = 0;
						if (ShiftRegA & 0x80)
							col |= 1;
						if (ShiftRegB & 0x80)
							col |= 2;
						if (Registers.AR == 0x55)
							Registers.AR = 0x55;
						t = Registers.GetPALAddress(col, Registers.AR);
						if (ControlRegisters.ShowSprites)
							for (i = 0; i < 8; i++)
								if (Sprites[i].x == x) {
									Sprites[i].cx++;
									if (Sprites[i].cx == 9) {
										Sprites[i].x = -1;
										continue;
									}
									scol = 0x10;
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
									Sprites[i].x++;
									if ((scol != 0x10) && ((col == 0) ||
										(~Sprites[i].Attrib & 0x20))) {
										t = Registers.GetPALAddress(scol, Sprites[i].Attrib);
										if (col != 0)
											State.Sprite0Hit = true;
										break;
									}
								}
						DrawPixel(x, y - 8, palette[PalMem[t] & 0x3f]);
						x++;
						ShiftRegA <<= 1;
						ShiftRegB <<= 1;
						Registers.FH++;
						if (x == 256)
							break;
					}
					if (Registers.FH != 8)
						break;
					Registers.FH = 0;
				} else
					x += 8;
				Registers.IncrementX();
				if (x == 256)
					break;
				FetchData();
			}
			if (ControlRegisters.ShowSprites) {
				i = 0;
				while (*s != 0xff) {
					Sprites[i].y = *(s++);
					Sprites[i].Tile = *(s++);
					Sprites[i].Attrib = *(s++);
					Sprites[i].x = *(s++);
					Sprites[i].cx = 0;
					switch (ControlRegisters.Size) {
					case Size8x8:
						if (Sprites[i].Attrib & 0x80)
							t = Registers.GetSpriteAddress(Sprites[i].Tile, 7 + Sprites[i].y - y, Registers.SpritePage);
						else
							t = Registers.GetSpriteAddress(Sprites[i].Tile, y - Sprites[i].y, Registers.SpritePage);
						break;
					case Size8x16:
						t = (Sprites[i].Tile & 0x01) << 12;
//						if (Sprites[i].Attrib & 0x80)
							if ((y - Sprites[i].y) >= 8)
								t = Registers.GetSpriteAddress(Sprites[i].Tile | 0x01, (y - Sprites[i].y) & 0x07, t);
							else
								t = Registers.GetSpriteAddress(Sprites[i].Tile & 0xfe, y - Sprites[i].y, t);
							break;
					}
					Sprites[i].ShiftRegA = Bus->ReadPPUMemory(t);
					Sprites[i].ShiftRegB = Bus->ReadPPUMemory(t | 0x08);
					i++;
					if (i == 8)
						break;
					Sprites[i].x = -1;
				}
			}
			Registers.IncrementY();
			FetchData();
		} else
			DrawPixel(x, y - 8, palette[0x0d]);
	}
}

/* Отработать такт */
template <class _Bus>
inline int CPPU<_Bus>::PerformOperation() {
	scanline++;
	if (scanline == 1) {
		State.VBlank = true;
		if (ControlRegisters.GenerateNMI)
			static_cast<typename _Bus::CPUClass *>(Bus->GetDeviceList()[_Bus::CPU])->GetNMIPin() = true;
	} else if (scanline == 21) {
		State.VBlank = false;
		if (ControlRegisters.RenderingEnabled())
			Registers.RealReg1 = Registers.BigReg1;
	} else if (scanline == 262) {
		FrameReady = true;
		scanline = 0;
		cur_frame++;
	}
	RenderScanline();
	return 341;
}

}

#endif
