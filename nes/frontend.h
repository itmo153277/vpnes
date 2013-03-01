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

#ifndef __FRONTEND_H_
#define __FRONTEND_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include <cmath>
#include <cstring>

namespace vpnes {

/* Интерфейс для звука */
class CAudioFrontend {
private:
	/* Различее времени */
	double TimeDiff;
	/* Время */
	double Time;
	/* Интегральное значение DAC */
	double Sum;
	/* Какие-то левые переменные */
	double a[4];
	double b[2];
	double x[2];
protected:
	/* Аудио буфер */
	sint16 *Buf;
	/* Длина в семплах */
	int Size;
	/* Текущая позиция */
	int Pos;
	/* Громкость */
	double Volume;
	/* Частота */
	double Frequency;
	/* Обновить буфер */
	virtual void UpdateBuffer() = 0;

	/* Сброс параметров DAC */
	inline void ResetDAC() {
		TimeDiff = 0.0;
		Time = 0.0;
		Sum = 0.0;
		x[0] = 0.0;
		x[1] = 0.0;
	}
	/* Обновить параметры */
	inline void RecalculateDecay() {
		/* Конечно, представлять это обчным колебательным звеном (а это */
		/* колебательное звено) — неправильно */
		/* И непонятно, чем это лучше обычного апериодического */
		/* Порядок должен быть не меньше 3 с 2 мнимыми корнями и одним действительным */
		double RA = exp(-0.343 / Frequency);
		double RC = cos(0.34993 / Frequency);
		double RS = sin(0.34993 / Frequency);

		a[0] = RA * (RC + 0.980196 * RS);
		a[1] = RA * (-0.686137) * RS;
		a[2] = RA * 2.85771 * RS;
		a[3] = RA * (RC - 0.980196 * RS);
		b[0] = 0.686 + RA * (-0.686 * RC + 0.0137227 * RS);
		b[1] = 1 - RA * (RC + 0.980196 * RS);
	}
public:
	inline explicit CAudioFrontend() {
		Volume = 2;
		ResetDAC();
	}
	inline virtual ~CAudioFrontend() {}

	/* Остановить воспроизведение */
	virtual void StopAudio() = 0;
	/* Продолжить воспроизведение */
	virtual void ResumeAudio() = 0;
	/* Поместить семпл в буфер */
	inline void PushSample(sint16 Sample) {
		Buf[Pos++] = Sample;
		if (Pos >= Size) {
			Pos = 0;
			UpdateBuffer();
		}
	}
	inline void PushSample(double Sample) {
		double Res, tx;

		Res = (Sample - x[1]) * Volume;
		if (Res < -1.0)
			Res = -1.0;
		else if (Res > 1.0)
			Res = 1.0;
		tx = x[0];
		x[0] = a[0] * tx + a[1] * x[1] + b[0] * Sample;
		x[1] = a[2] * tx + a[3] * x[1] + b[1] * Sample;
		PushSample((sint16) (Res * 37267.0));
	}
	inline void PushSample(double DACOutput, double Length) {
		int i, num;

		Time += Length * Frequency;
		num = (int) Time;
		if (num > 0) {
			Time -= num;
			Sum += DACOutput * (1.0 - TimeDiff);
			PushSample(Sum);
			for (i = 1; i < num; i++)
				PushSample(DACOutput);
			Sum = DACOutput * Time;
		} else
			Sum += DACOutput * (Time - TimeDiff);
		TimeDiff = Time;
	}
};

/* Интерфейс для видео */
class CVideoFrontend {
protected:
	/* Видео буфер */
	uint32 *Buf;
	/* Палитра */
	uint32 *Pal;
public:
	inline explicit CVideoFrontend() {}
	inline virtual ~CVideoFrontend() {}

	/* Вывести картинку на экран */
	virtual bool UpdateFrame(double FrameTime) = 0;
	/* Поместить пиксель в буфер */
	inline void PutPixel(int x, int y, int Colour, int Tint) {
		Buf[x + (y << 8)] = Pal[Colour + (Tint << 6)];
	}
};

/* Интерфейс для ввода */
class CInputFrontend {
public:
	/* Параметры внутренней памяти */
	struct SInternalMemory {
		void *RAM;
		size_t Size;
	};
protected:
	/* Битовая маска */
	uint8 OutputMask;
	/* Буфер ввода */
	uint8 *Buf;
	/* Длина буфера */
	int Length;
	/* Текущая позиция */
	int *Pos;
	/* Внутренняя память */
	SInternalMemory InternalMemory;
public:
	inline explicit CInputFrontend() {}
	inline virtual ~CInputFrontend() {}

	/* Стробирующий сигнал */
	virtual void InputSignal(uint8 Signal) = 0;
	/* Чтение устройства */
	inline uint8 ReadState() {
		if (*Pos >= Length)
			return 0x40 | OutputMask; /* Открытая шина */
		return (0x40 & ~OutputMask) | (Buf[(*Pos)++] & OutputMask);
	}
	/* Параметры внутреннней памяти */
	inline const SInternalMemory * const GetInternalMemory() const { return &InternalMemory; }
};

/* Стандартный контроллер NES */
class CStdController: public CInputFrontend {
public:
	/* Имена кнопок */
	enum ButtonName {
		ButtonA = 0,
		ButtonB = 1,
		ButtonSelect = 2,
		ButtonStart = 3,
		ButtonUp = 4,
		ButtonDown = 5,
		ButtonLeft = 6,
		ButtonRight = 7
	};
private:
	struct SInternalData {
		int Pos;
		uint8 Strobe;
	} InternalData;
public:
	inline explicit CStdController() {
		InternalMemory.RAM = &InternalData;
		InternalMemory.Size = sizeof(SInternalData);
		Buf = new uint8[8];
		memset(Buf, 0x00, 8 * sizeof(uint8));
		Pos = &InternalData.Pos;
		InternalData.Pos = 0;
		Length = 8;
		OutputMask = 0x01;
		InternalData.Strobe = 0x00;
	}
	inline ~CStdController() {
		delete [] Buf;
	}

	/* Стробирующий сигнал */
	void InputSignal(uint8 Signal) {
		if (!(Signal & 0x01) && (InternalData.Strobe & 0x01))
			InternalData.Pos = 0;
		InternalData.Strobe = Signal;
	}

	/* Управление кнопками */
	inline void PushButton(ButtonName Button) {
		Buf[Button] |= 0x01;
		if (Button >= ButtonUp) {
			/* Запрещаем нажимать на весь крестик */
			if (Buf[ButtonUp] && Buf[ButtonDown]) {
				Buf[ButtonUp] = 0x02;
				Buf[ButtonDown] = 0x02;
			}
			if (Buf[ButtonLeft] && Buf[ButtonRight]) {
				Buf[ButtonLeft] = 0x02;
				Buf[ButtonRight] = 0x02;
			}
		}
	}
	inline void PopButton(ButtonName Button) {
		Buf[Button] &= 0xfc;
		switch (Button) {
			case ButtonUp:
				Buf[ButtonDown] >>= 1;
				break;
			case ButtonDown:
				Buf[ButtonUp] >>= 1;
				break;
			case ButtonLeft:
				Buf[ButtonRight] >>= 1;
				break;
			case ButtonRight:
				Buf[ButtonLeft] >>= 1;
				break;
			default:
				break;
		}
	}
};

/* Интерфейс NES */
class CNESFrontend {
protected:
	/* Интерфейс аудио */
	CAudioFrontend *AudioFrontend;
	/* Интерфейс видео */
	CVideoFrontend *VideoFrontend;
	/* Интерфейс ввода 1 */
	CInputFrontend *Input1Frontend;
	/* Интерфейс ввода 2 */
	CInputFrontend *Input2Frontend;
public:
	inline explicit CNESFrontend() {}
	inline virtual ~CNESFrontend() {}

	/* Интерфейс аудио */
	inline CAudioFrontend * const &GetAudioFrontend() const { return AudioFrontend; }
	/* Интерфейс видео */
	inline CVideoFrontend * const &GetVideoFrontend() const { return VideoFrontend; }
	/* Интерфейс ввода 1 */
	inline CInputFrontend * const &GetInput1Frontend() const { return Input1Frontend; }
	/* Интерфейс ввода 2 */
	inline CInputFrontend * const &GetInput2Frontend() const { return Input2Frontend; }
	/* Отладочная информация ЦПУ */
	virtual void CPUDebug(uint16 PC, uint8 A, uint8 X, uint8 Y, uint8 S, uint8 P) = 0;
	/* Отладочная информация ГПУ */
	virtual void PPUDebug(int Frame, int Scanline, int Cycle, uint16 Reg1, uint16 Reg2,
		uint16 Reg1Buf) = 0;
	/* Критическая ошибка */
	virtual void Panic() = 0;
};

}

#endif
