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

#ifndef __CPU_H_
#define __CPU_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include "device.h"
#include "clock.h"

namespace vpnes {

/* CPU */
template <class _Bus>
class CCPU: public CClockedDevice<_Bus> {
	using CClockedDevice<_Bus>::Clocks;
	using CDevice<_Bus>::Bus;
public:
#if 0
	/* Типы адресации */
	enum AddressingMode {
		Accumulator, /* Аккумулятор */
		Implied, /* Без параметра */
		Immediate, /* В параметре значение */
		Absolute, /* В параметре адрес */
		ZeroPage, /* ZP */
		Relative, /* PC + знаечние */
		AbsoluteX, /* Адрес + X */
		AbsoluteY, /* Адрес + Y */
		ZeroPageX, /* ZP + X */
		ZeroPageY, /* ZP + Y */
		AbsoluteInd, /* Адрес + X + indirect */
		ZeroPageInd, /* ZP + X + indirect */
		ZeroPageIndY, /* ZP + indirect + Y */
		Indirect /* Адрес + indirect */
	};
#endif

	/* Обработчик инструкции */
	typedef int (CCPU::*OpHandler)();

	/* Описание опкода */
	struct SOpcode {
		int Clocks; /* Число тактов */
		int Length; /* Длина команды */
		bool Bound; /* +1 такт при переходе страницы */
		OpHandler Handler; /* Обработчик */
	};

	/* Таблица опкодов */
	static const SOpcode Opcodes[256];

	/* Регистр состояния */
	struct SState {
		uint8 State;
		inline bool Negative() { return State & 0x80; }
		inline bool Overflow() { return State & 0x40; }
		inline bool Break() { return State & 0x10; }
		inline bool Decimal() { return State & 0x08; }
		inline bool Interrupt() { return State & 0x04; }
		inline bool Zero() { return State & 0x02; }
		inline bool Carry() { return State & 0x01; }
		inline void SetState(uint8 NewState) { State = NewState | 0x20; }
		inline void NFlag(uint8 Src) { State = (State & 0x7f) | (Src & 0x80); }
		inline void VFlag(bool Flag) { if (Flag) State |= 0x40; else State &= 0xbf; }
		inline void ZFlag(uint8 Src) { if (Src == 0) State |= 0x02; else State &= 0xfd; }
		inline void CFlag(bool Flag) { if (Flag) State |= 0x01; else State &= 0xfe; }
	} State;

	/* Регистры */
	struct SRegisters {
		uint8 a; /* Аккумулятор */
		uint8 x; /* X */
		uint8 y; /* Y */
		uint8 s; /* S */
		uint16 pc; /* PC */
	} Registers;

	/* Состояние прерывания */
	bool CurBreak;
	/* RAM */
	uint8 *RAM;
	/* NMI pin */
	bool NMI;
	/* IRQ pin */
	bool IRQ;
	/* Зависание */
	bool Halt;

	/* Типы адресации */

	/* Аккумулятор */
	struct Accumulator {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Registers.a;
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Registers.a = Src;
		}
	};

	/* Без параметра */
	struct Implied {
	};

	/* В параметре значение */
	struct Immediate {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadMemory(CPU.Registers.pc - 1);
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteMemory(CPU.Registers.pc - 1);
		}
	};

	/* В параметре адрес */
	struct Absolute {
		inline static uint16 GetAddr(CCPU &CPU) {
			return CPU.Bus->ReadMemory(CPU.Registers.pc - 2) |
				(CPU.Bus->ReadMemory(CPU.Registers.pc - 1) << 8);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteMemory(GetAddr(CPU));
		}
	};

	/* ZP */
	struct ZeroPage {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.RAM[CPU.Bus->ReadMemory(CPU.Registers.pc - 1)];
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.RAM[CPU.Bus->ReadMemory(CPU.Registers.pc - 1)] = Src;
		}
	};

	/* PC + значение */
	struct Relative {
		inline static uint16 ReadWord(CCPU &CPU) {
			return CPU.Registers.pc + (sint8) CPU.Bus->ReadMemory(CPU.Registers.pc - 1);
		}
	};

	/* Адрес + X */
	struct AbsoluteX {
		inline static uint16 GetAddr(CCPU &CPU) {
			return CPU.Bus->ReadMemory(CPU.Registers.pc - 2) |
				(CPU.Bus->ReadMemory(CPU.Registers.pc - 1) << 8) + CPU.Registers.x;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteMemory(GetAddr(CPU));
		}
	};

	/* Адрес + Y */
	struct AbsoluteY {
		inline static uint16 GetAddr(CCPU &CPU) {
			return CPU.Bus->ReadMemory(CPU.Registers.pc - 2) |
				(CPU.Bus->ReadMemory(CPU.Registers.pc - 1) << 8) + CPU.Registers.y;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteMemory(GetAddr(CPU));
		}
	};

	/* ZP + X */
	struct ZeroPageX {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.RAM[(CPU.Bus->ReadMemory(CPU.Registers.pc - 1)
				+ CPU.Registers.x) & 0xff];
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.RAM[(CPU.Bus->ReadMemory(CPU.Registers.pc - 1)
				+ CPU.Registers.x) & 0xff] = Src;
		}
	};

	/* ZP + Y */
	struct ZeroPageY {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.RAM[(CPU.Bus->ReadMemory(CPU.Registers.pc - 1)
				+ CPU.Registers.y) & 0xff];
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.RAM[(CPU.Bus->ReadMemory(CPU.Registers.pc - 1)
				+ CPU.Registers.y) & 0xff] = Src;
		}
	};

	/* Адрес + X + indirect */
	struct AbsoluteInd {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint16 AddressPage, AddressOffset;
			AddressOffset = CPU.Bus->ReadMemory(CPU.Registers.pc - 2) + CPU.Registers.x;
			AddressPage = (CPU.Bus->ReadMemory(CPU.Registers.pc - 1) << 8) + (AddressOffset & 0x100);
			return CPU.Bus->ReadMemory(AddressPage | (AddressOffset & 0xff)) |
				(CPU.Bus->ReadMemory(AddressPage | ((AddressOffset + 1) & 0xff)) << 8);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteMemory(GetAddr(CPU));
		}
	};

	/* ZP + X + indirect */
	struct ZeroPageInd {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint8 AddressOffset;
			AddressOffset = CPU.Bus->ReadMemory(CPU.Registers.pc - 1) + CPU.Registers.x;
			return CPU.RAM[AddressOffset] | (CPU.RAM[(uint8) (AddressOffset + 1)] << 8);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteMemory(GetAddr(CPU));
		}
	};

	/* ZP + indirect + Y */
	struct ZeroPageIndY {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint8 AddressOffset;
			AddressOffset = CPU.Bus->ReadMemory(CPU.Registers.pc - 1);
			return (CPU.RAM[(uint8) (AddressOffset + 1)] << 8) | (uint8) (CPU.RAM[AddressOffset]
				+ CPU.Registers.y);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteMemory(GetAddr(CPU));
		}
	};

	/* Адрес + indirect */
	struct Indirect {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint16 AddressPage;
			uint8 AddressOffset;
			AddressOffset = CPU.Bus->ReadMemory(CPU.Registers.pc - 2);
			AddressPage = CPU.Bus->ReadMemory(CPU.Registers.pc - 1) << 8;
			return CPU.Bus->ReadMemory(AddressPage | AddressOffset) |
				(CPU.Bus->ReadMemory(AddressPage | ((uint8) (AddressOffset + 1))) << 8);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteMemory(GetAddr(CPU));
		}
	};

public:
	inline explicit CCPU(_Bus *pBus) { Bus = pBus; }
	inline ~CCPU() {}

	/* Выполнить действие */
	inline void Clock(int DoClocks) {
		if ((Clocks -= DoClocks) == 0)
			Clocks = PerformOperation();
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) { return 0x00; }
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {}

	/* Отработать команду */
	int PerformOperation();
};

/* Махинации с классом */
struct CPU_rebind {
	template <class _Bus>
	struct rebind {
		typedef CCPU<_Bus> rebinded;
	};
};

/* Отработать такт */
template <class _Bus>
inline int CCPU<_Bus>::PerformOperation() {
	return 3;
}

}

#endif
