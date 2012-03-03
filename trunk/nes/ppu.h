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
		uint16 BigReg2; /* Вторая комбинация */
		uint16 AR; /* Информация о аттрибутах */
		uint16 FH; /* Точное смещение по горизонтали */
		/* Получить адрес для 0x2007 */
		inline uint16 Get2007Address() { return BigReg1 & 0x3fff; }
		/* Получить адрес для чтения NameTable */
		inline uint16 GetNTAddress() { return (BigReg1 & 0x0fff) | 0x2000; }
		/* Получить адрес для чтения AttributeTable */
		inline uint16 GetATAddress() { return ((BigReg1 >> 2) & 0x0007) |
			((BigReg1 >> 7) & 0x0007) | (BigReg1 & 0x0c00) | 0x23c0; }
		/* Получить адрес для чтения PatternTable */
		inline uint16 GetPTAddress() { return BigReg2 | (BigReg1 >> 12); }
		/* Запись в 0x2000 */
		inline void Write2000(uint8 Src) { BigReg1 = (BigReg1 & 0xf3ff) |
			((Src & 0x03) << 10) ; BigReg2 = (BigReg2 & 0x0fff) | ((Src & 0x10) << 8); }
		/* Запись в 0x2005/1 */
		inline void Write2005_1(uint8 Src) { FH = Src & 0x07; BigReg1 = (BigReg1 & 0xffe0) |
			(Src >> 3); }
		/* Запись в 0x2005/2 */
		inline void Write2005_2(uint8 Src) { BigReg1 = (BigReg1 & 0x8c1f) | ((Src & 0x07) << 12) |
			((Src & 0xf8) << 2); }
		/* Запись в 0x2006/1 */
		inline void Write2006_1(uint8 Src) { BigReg1 = (BigReg1 & 0x80ff) | ((Src & 0x3f) << 8); }
		/* Запись в 0x2006/2 */
		inline void Write2006_2(uint8 Src) { BigReg1 = (BigReg1 & 0xff00) | Src; }
		/* Прочитали NameTable */
		inline void ReadNT(uint8 Src) { BigReg2 = (BigReg2 & 0x1000) | (Src << 4); }
		/* Прочитали AttributeTable */
		inline void ReadAT(uint8 Src) { AR = Src; }
		/* Инкременты */
		inline void Increment2007(bool VerticalIncrement) { if (VerticalIncrement)
			BigReg1 += 0x0020; else BigReg1++; }
		inline void IncrementX() { uint16 Src = (BigReg1 & 0x001f); Src++; BigReg1 = (BigReg1 & 0xffe0)
			| (Src & 0x001f); BigReg1 ^= (Src & 0x0020) << 5; }
		inline void IncrementY() { uint16 Src = (BigReg1 >> 12) | ((BigReg1 & 0x03e0) >> 2); Src++;
			Write2005_2((uint8) Src); BigReg1 ^= (Src & 0x0100) << 3; }
	} Registers;

	/* Размер спрайта */
	enum SpriteSize {
		Size8x8,
		Size8x16
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

	/* Память */
	uint8 RAM[0x1000];
	/* Готовность кадра для вывода */
	bool FrameReady;
	/* Текущий scanline */
	int scanline;
	/* Внутренний флаг */
	bool Trigger;

public:
	inline explicit CPPU(_Bus *pBus): FrameReady(false), scanline(0),
		Trigger(false) { Bus = pBus; }
	inline ~CPPU() {}

	/* Выполнить действие */
	inline void Clock(int DoClocks) {
		if ((Clocks -= DoClocks) == 0)
			Clocks = PerformOperation();
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		uint8 Res;

		switch (Address & 0x2007) {
			case 0x2002:
				Trigger = false;
				Res = State.PackState();
				State.VBlank = false;
				return Res;
		}
		return 0x00;
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		switch (Address & 0x2007) {
			case 0x2000:
				ControlRegisters.Controller(Src);
				break;
			case 0x2001:
				ControlRegisters.Mask(Src);
				break;
			case 0x2002:
				State.Garbage = Src;
				break;
			case 0x2005:
				Trigger = !Trigger;
				break;
			case 0x2006:
				Trigger = !Trigger;
				break;
			case 0x2007:
				;
		}
	}

	/* Чтение памяти PPU */
	inline uint8 ReadPPUAddress(uint16 Address) { return 0x00; }
	/* Запись памяти PPU */
	inline void WritePPUAddress(uint16 Address, uint8 Src) {}

	/* Отработать команду */
	int PerformOperation();

	/* Флаг вывода */
	inline bool &IsFrameReady() { return FrameReady; }
};

/* Махинации с классом */
struct PPU_rebind {
	template <class _Bus>
	struct rebind {
		typedef CPPU<_Bus> rebinded;
	};
};

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
	} else if (scanline == 263) {
		FrameReady = true;
		scanline = 0;
	}
	return 341;
}

}

#endif
