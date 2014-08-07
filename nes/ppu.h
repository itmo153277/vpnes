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
typedef PPUGroup<4>::ID::NoBatteryID SpritesID;
typedef PPUGroup<5>::ID::NoBatteryID EventDataID;

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
	/* Время отрисовки кадра */
	int FrameTime;
	/* Номинальное время отрисовки кадра */
	int NominalFrameTime;
	/* Номинальное время отрисовки для нечетных кадров */
	int NominalFrameTimeOdd;
	/* Chip Enable */
	bool CE;

	/* Внутренние данные */
	struct SInternalData: public ManagerID<PPUID::InternalDataID> {
		int Scanline;
		int OddFrame;
		int IRQOffset;
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
		inline void GetVBlank() { return State & 0x80; }
		/* Claer VBlank */
		inline void ClearVBlank() { State &= 0x7f; SupressVBlank = true; }
		/* Overflow */
		inline void SetOverflow() { State |= 0x20; }
		/* Сброс состояния */
		inline void ClearState() { State = 0; }
		/* Получаем байт по маске */
		inline void ReadState(uint8 Src) { return State | (Src & 0x1f); }
	} State;

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
	} Registers;

	/* Вызов по событию */
	inline void EventCall(int CallTime) {
		Process(CallTime);
		InternalData.IRQOffset -= CallTime;
	}

	/* Вызов по шине CPU */
	inline void CPUCall() {
		Process(Bus->GetInternalTime() + InternalData.IRQOffset);
	}

	void Process(int Time);
public:
	CPPU(_Bus *pBus) {
		SEvent *NewEvent;

		Bus = pBus;
		memset(&InternalData, 0, sizeof(SInternalData));
		Bus->GetManager()->template SetPointer<SInternalData>(&InternalData);
		memset(&State, 0, sizeof(SState));
		Bus->GetManager()->template SetPointer<SState>(&State);
		memset(&Registers, 0, sizeof(SRegisters));
		Bus->GetManager()->template SetPointer<SRegisters>(&Registers);
		memset(Sprites, 0, sizeof(SSprite) * 8);
		Bus->GetManager()->template SetPointer<ManagerID<PPUID::SpritesID> >(\
			Sprites, sizeof(SSprite) * 8);
		memset(LocalEvents, 0, sizeof(SEventData) * MAX_EVENTS);
		Bus->GetManager()->template SetPointer<ManagerID<PPUID::EventDataID> >(LocalEvents,
			sizeof(SEventData) * MAX_EVENTS);
		NominalFrameTime = ClockDivider *
			((_Settings::ActiveScanlines + _Settings::PostRender +
			_Settings::VBlank + 1) * 341);
		NominalFrameTimeOdd = NominalFrameTime - ClockDivider * _Settings::OddSkip;
		FrameReady = false;
		FrameTime = NominalFrameTime;
		NewEvent = new SEvent;
		NewEvent->Data = &LocalEvents[EVENT_PPU_FRAME];
		NewEvent->Name = MSTR(EVENT_PPU_FRAME);
		NewEvent->Execute = [this] {
			EventCall(FrameTime);
			/* TODO: Перенести в обработчик событий */
			FrameReady = true;
			Bus->GetClock()->SetEventTime(EventChain[EVENT_PPU_FRAME],
				Bus->GetClock()->GetTime() + FrameTime);
		};
		EventChain[EVENT_PPU_FRAME] = NewEvent;
		Bus->GetClock()->RegisterEvent(NewEvent);
		Bus->GetClock()->EnableEvent(EventChain[EVENT_PPU_FRAME]);
		Bus->GetClock()->SetEventTime(EventChain[EVENT_PPU_FRAME],
			Bus->GetClock()->GetTime() + FrameTime);
	}
	inline ~CPPU() {
	}

	/* Сброс */
	inline void Reset() {
	}

	/* Сброс часов шины */
	inline void ResetInternalClock(int Time) {
		InternalData.IRQOffset += Time;
	}
	/* Чтение памяти */
	inline uint8 ReadByte(uint16 Address) {
		return 0x40;
	}
	/* Запись памяти */
	inline void WriteByte(uint16 Address, uint8 Src) {
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
};

template <class _Bus, class _Settings>
void CPPU<_Bus, _Settings>::Process(int Time) {
}

}

#endif
