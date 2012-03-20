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

#include <cstring>
#include "device.h"
#include "clock.h"

namespace vpnes {

/* CPU */
template <class _Bus>
class CCPU: public CDevice<_Bus> {
	using CDevice<_Bus>::Bus;
private:
	/* Обработчик инструкции */
	typedef int (CCPU::*OpHandler)();

	/* Описание опкода */
	struct SOpCode {
		int Clocks; /* Число тактов */
		int Length; /* Длина команды */
		bool Bound; /* +1 такт при переходе страницы */
		OpHandler Handler; /* Обработчик */
	};

	/* Таблица опкодов */
	static const SOpCode OpCodes[256];

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
	uint8 RAM[0x0800];
	/* NMI pin */
	bool NMI;
	/* IRQ pin */
	bool IRQ;
	/* Зависание */
	bool Halt;
	/* Переменная для кеширования адреса */
	uint16 AddrCache;
	/* DMA */
	int OAM_DMA;
	/* DMA wrap */
	int DMA_wrap;
	/* Выполнить IRQ */
	bool raiseirq;

	/* Положить в стек */
	inline void PushByte(uint8 Src) {
		RAM[0x0100 + Registers.s--] = Src;
	}
	/* Достать из стека */
	inline uint8 PopByte() {
		return RAM[0x0100 + ++Registers.s];
	}
	/* Положить в стек слово */
	inline void PushWord(uint16 Src) {
		PushByte(Src >> 8);
		PushByte(Src & 0xff);
	}
	/* Достать из стека слово */
	inline uint16 PopWord() {
		return PopByte() | (PopByte() << 8);
	}

	/* Типы адресации */

	/* Аккумулятор */
	struct Accumulator {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Registers.a;
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Registers.a = Src;
		}
		inline static uint8 ReReadByte(CCPU &CPU) {
			return ReadByte(CPU);
		}
		inline static void ReWriteByte(CCPU &CPU, uint8 Src) {
			WriteByte(CPU, Src);
		}
	};

	/* Без параметра */
	struct Implied {
	};

	/* В параметре значение */
	struct Immediate {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1);
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(CPU.Registers.pc - 1);
		}
		inline static uint8 ReReadByte(CCPU &CPU) {
			return ReadByte(CPU);
		}
		inline static void ReWriteByte(CCPU &CPU, uint8 Src) {
			WriteByte(CPU, Src);
		}
	};

	/* В параметре адрес */
	struct Absolute {
		inline static uint16 GetAddr(CCPU &CPU) {
			return CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 2) |
				(CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1) << 8);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadCPUMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(GetAddr(CPU), Src);
		}
		inline static uint8 ReReadByte(CCPU &CPU) {
			CPU.AddrCache = GetAddr(CPU);
			return CPU.Bus->ReadCPUMemory(CPU.AddrCache);
		}
		inline static void ReWriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(CPU.AddrCache, Src);
		}
	};

	/* ZP */
	struct ZeroPage {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.RAM[CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1)];
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.RAM[CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1)] = Src;
		}
		inline static uint8 ReReadByte(CCPU &CPU) {
			return ReadByte(CPU);
		}
		inline static void ReWriteByte(CCPU &CPU, uint8 Src) {
			WriteByte(CPU, Src);
		}
	};

	/* PC + значение */
	struct Relative {
		inline static uint16 GetAddr(CCPU &CPU) {
			return CPU.Registers.pc + (sint8) CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1);
		}
	};

	/* Адрес + X */
	struct AbsoluteX {
		inline static uint16 GetAddr(CCPU &CPU) {
			return (CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 2) |
				(CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1) << 8)) + CPU.Registers.x;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadCPUMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(GetAddr(CPU), Src);
		}
		inline static uint8 ReReadByte(CCPU &CPU) {
			CPU.AddrCache = GetAddr(CPU);
			return CPU.Bus->ReadCPUMemory(CPU.AddrCache);
		}
		inline static void ReWriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(CPU.AddrCache, Src);
		}
	};

	/* Адрес + Y */
	struct AbsoluteY {
		inline static uint16 GetAddr(CCPU &CPU) {
			return (CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 2) |
				(CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1) << 8)) + CPU.Registers.y;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadCPUMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(GetAddr(CPU), Src);
		}
		inline static uint8 ReReadByte(CCPU &CPU) {
			CPU.AddrCache = GetAddr(CPU);
			return CPU.Bus->ReadCPUMemory(CPU.AddrCache);
		}
		inline static void ReWriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(CPU.AddrCache, Src);
		}
	};

	/* ZP + X */
	struct ZeroPageX {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.RAM[(CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1)
				+ CPU.Registers.x) & 0xff];
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.RAM[(CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1)
				+ CPU.Registers.x) & 0xff] = Src;
		}
		inline static uint8 ReReadByte(CCPU &CPU) {
			return ReadByte(CPU);
		}
		inline static void ReWriteByte(CCPU &CPU, uint8 Src) {
			WriteByte(CPU, Src);
		}
	};

	/* ZP + Y */
	struct ZeroPageY {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.RAM[(CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1)
				+ CPU.Registers.y) & 0xff];
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.RAM[(CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1)
				+ CPU.Registers.y) & 0xff] = Src;
		}
		inline static uint8 ReReadByte(CCPU &CPU) {
			return ReadByte(CPU);
		}
		inline static void ReWriteByte(CCPU &CPU, uint8 Src) {
			WriteByte(CPU, Src);
		}
	};

	/* Адрес + X + indirect */
	struct AbsoluteInd {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint16 AddressPage, AddressOffset;
			AddressOffset = CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 2) + CPU.Registers.x;
			AddressPage = (CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1) << 8) + (AddressOffset & 0x100);
			return CPU.Bus->ReadCPUMemory(AddressPage | (AddressOffset & 0xff)) |
				(CPU.Bus->ReadCPUMemory(AddressPage | ((AddressOffset + 1) & 0xff)) << 8);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadCPUMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(GetAddr(CPU), Src);
		}
		inline static uint8 ReReadByte(CCPU &CPU) {
			CPU.AddrCache = GetAddr(CPU);
			return CPU.Bus->ReadCPUMemory(CPU.AddrCache);
		}
		inline static void ReWriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(CPU.AddrCache, Src);
		}
	};

	/* ZP + X + indirect */
	struct ZeroPageInd {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint8 AddressOffset;
			AddressOffset = CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1) + CPU.Registers.x;
			return CPU.RAM[AddressOffset] | (CPU.RAM[(uint8) (AddressOffset + 1)] << 8);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadCPUMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(GetAddr(CPU), Src);
		}
		inline static uint8 ReReadByte(CCPU &CPU) {
			CPU.AddrCache = GetAddr(CPU);
			return CPU.Bus->ReadCPUMemory(CPU.AddrCache);
		}
		inline static void ReWriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(CPU.AddrCache, Src);
		}
	};

	/* ZP + indirect + Y */
	struct ZeroPageIndY {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint8 AddressOffset;
			AddressOffset = CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1);
			return ((CPU.RAM[(uint8) (AddressOffset + 1)] << 8) | CPU.RAM[AddressOffset])
				+ CPU.Registers.y;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadCPUMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(GetAddr(CPU), Src);
		}
		inline static uint8 ReReadByte(CCPU &CPU) {
			CPU.AddrCache = GetAddr(CPU);
			return CPU.Bus->ReadCPUMemory(CPU.AddrCache);
		}
		inline static void ReWriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(CPU.AddrCache, Src);
		}
	};

	/* Адрес + indirect */
	struct Indirect {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint16 AddressPage;
			uint8 AddressOffset;
			AddressOffset = CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 2);
			AddressPage = CPU.Bus->ReadCPUMemory(CPU.Registers.pc - 1) << 8;
			return CPU.Bus->ReadCPUMemory(AddressPage | AddressOffset) |
				(CPU.Bus->ReadCPUMemory(AddressPage | ((uint8) (AddressOffset + 1))) << 8);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Bus->ReadCPUMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(GetAddr(CPU), Src);
		}
		inline static uint8 ReReadByte(CCPU &CPU) {
			CPU.AddrCache = GetAddr(CPU);
			return CPU.Bus->ReadCPUMemory(CPU.AddrCache);
		}
		inline static void ReWriteByte(CCPU &CPU, uint8 Src) {
			CPU.Bus->WriteCPUMemory(CPU.AddrCache, Src);
		}
	};

public:
	inline explicit CCPU(_Bus *pBus) {
		Bus = pBus;
		/* Стартовые значения */
		memset(RAM, 0xff, 0x0800 * sizeof(uint8));
		RAM[0x0008] = 0xf7;
		RAM[0x0009] = 0xef;
		RAM[0x000a] = 0xdf;
		RAM[0x000f] = 0xbf;
		memset(&Registers, 0, sizeof(SRegisters));
		State.SetState(0);
	}
	inline ~CCPU() {}

	/* Отработать команду */
	inline int Clock();

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		return RAM[Address & 0x07ff];
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		RAM[Address & 0x07ff] = Src;
	}

	/* NMI pin */
	inline bool &GetNMIPin() { return NMI; }
	/* IRQ pin */
	inline bool &GetIRQPin() { return IRQ; }
	/* Halt flag */
	inline bool &GetHaltState() { return Halt; }
	/* DMA */
	inline int &GetDMA() { return OAM_DMA; }
	/* Direct Access */
	inline uint8 *DirectAccess() { return RAM; }

	/* Сброс */
	inline void Reset() {
		Registers.s -= 3;
		State.State |= 0x04; /* Interrupt */
		NMI = false;
		IRQ = false;
		Halt = false;
		CurBreak = false;
		Registers.pc = Bus->ReadCPUMemory(0xfffc) | (Bus->ReadCPUMemory(0xfffd) << 8);
		DMA_wrap = 0;
	}

private:
	/* Команды CPU */

	/* Неизвестная команда */
	int OpIllegal();
	/* Загрузить в A */
	template <class _Addr>
	int OpLDA();
	/* Загрузить в X */
	template <class _Addr>
	int OpLDX();
	/* Загрузить в Y */
	template <class _Addr>
	int OpLDY();
	/* Сохранить A */
	template <class _Addr>
	int OpSTA();
	/* Сохранить X */
	template <class _Addr>
	int OpSTX();
	/* Сохранить Y */
	template <class _Addr>
	int OpSTY();
	/* Сложение */
	template <class _Addr>
	int OpADC();
	/* Вычитание */
	template <class _Addr>
	int OpSBC();
	/* Инкремент */
	template <class _Addr>
	int OpINC();
	/* Инкремент X */
	template <class _Addr>
	int OpINX();
	/* Инкремент Y */
	template <class _Addr>
	int OpINY();
	/* Декремент */
	template <class _Addr>
	int OpDEC();
	/* Декремент X */
	template <class _Addr>
	int OpDEX();
	/* Декремент Y */
	template <class _Addr>
	int OpDEY();
	/* Сдвиг влево */
	template <class _Addr>
	int OpASL();
	/* Сдвиг вправо */
	template <class _Addr>
	int OpLSR();
	/* Циклический сдвиг влево */
	template <class _Addr>
	int OpROL();
	/* Циклический сдвиг вправо */
	template <class _Addr>
	int OpROR();
	/* Логическое и */
	template <class _Addr>
	int OpAND();
	/* Логическое или */
	template <class _Addr>
	int OpORA();
	/* Исключающаее или */
	template <class _Addr>
	int OpEOR();
	/* Сравнение с A */
	template <class _Addr>
	int OpCMP();
	/* Сравнение с X */
	template <class _Addr>
	int OpCPX();
	/* Сравнение с Y */
	template <class _Addr>
	int OpCPY();
	/* Битова проверка */
	template <class _Addr>
	int OpBIT();
	/* Переход по !C */
	template <class _Addr>
	int OpBCC();
	/* Переход по C */
	template <class _Addr>
	int OpBCS();
	/* Переход по !Z */
	template <class _Addr>
	int OpBNE();
	/* Переход по Z */
	template <class _Addr>
	int OpBEQ();
	/* Переход по !N */
	template <class _Addr>
	int OpBPL();
	/* Переход по N */
	template <class _Addr>
	int OpBMI();
	/* Переход по !V */
	template <class _Addr>
	int OpBVC();
	/* Переход по V */
	template <class _Addr>
	int OpBVS();
	/* A -> X */
	template <class _Addr>
	int OpTAX();
	/* X -> A */
	template <class _Addr>
	int OpTXA();
	/* A -> Y */
	template <class _Addr>
	int OpTAY();
	/* Y -> A */
	template <class _Addr>
	int OpTYA();
	/* S -> X */
	template <class _Addr>
	int OpTSX();
	/* X -> S */
	template <class _Addr>
	int OpTXS();
	/* Положить в стек A */
	template <class _Addr>
	int OpPHA();
	/* Достать и з стека A */
	template <class _Addr>
	int OpPLA();
	/* Положить в стек P */
	template <class _Addr>
	int OpPHP();
	/* Достать и з стека P */
	template <class _Addr>
	int OpPLP();
	/* Безусловный переход */
	template <class _Addr>
	int OpJMP();
	/* Вызов подпрограммы */
	template <class _Addr>
	int OpJSR();
	/* Выход из подпрограммы */
	template <class _Addr>
	int OpRTS();
	/* Выход из прерывания */
	template <class _Addr>
	int OpRTI();
	/* Установить C */
	template <class _Addr>
	int OpSEC();
	/* Установить D */
	template <class _Addr>
	int OpSED();
	/* Установить I */
	template <class _Addr>
	int OpSEI();
	/* Сбросить C */
	template <class _Addr>
	int OpCLC();
	/* Сбросить D */
	template <class _Addr>
	int OpCLD();
	/* Сбросить I */
	template <class _Addr>
	int OpCLI();
	/* Сбросить V */
	template <class _Addr>
	int OpCLV();
	/* Нет операции */
	template <class _Addr>
	int OpNOP();
	/* Вызвать прерывание */
	template <class _Addr>
	int OpBRK();
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
inline int CCPU<_Bus>::Clock() {
	uint8 opcode;
	int clocks;

	if (Halt) /* Зависли */
		return 1;
	if (CurBreak) { /* Обрабатываем прерывание */
		/* Прыгаем */
		if (NMI) {
			Registers.pc = Bus->ReadCPUMemory(0xfffa) | (Bus->ReadCPUMemory(0xfffb) << 8);
			NMI = false;
		} else
			Registers.pc = Bus->ReadCPUMemory(0xfffe) | (Bus->ReadCPUMemory(0xffff) << 8);
		CurBreak = false;
		return 3; /* 3 такта */
	} else
		raiseirq = NMI || (IRQ && !State.Interrupt());
	if (IRQ)
		IRQ = false;
	if (OAM_DMA >= 0) { /* Выполнить DMA */
		static_cast<typename _Bus::PPUClass *>(Bus->GetDeviceList()[_Bus::PPU])->SetDMA(OAM_DMA);
		OAM_DMA = -2;
		return 513 + DMA_wrap;
	} else if (OAM_DMA == -2) {
		static_cast<typename _Bus::PPUClass *>(Bus->GetDeviceList()[_Bus::PPU])->ProcessDMA(513 * 3);
		OAM_DMA = -1;
	}
	opcode = Bus->ReadCPUMemory(Registers.pc); /* Текущий опкод */
	/* Число тактов + 1 такт на пересечение страницы */
	clocks = OpCodes[opcode].Clocks;
	if (OpCodes[opcode].Bound && (((Registers.pc & 0xff) +
		OpCodes[opcode].Length) > 0x100))
		clocks++;
	Registers.pc += OpCodes[opcode].Length;
	clocks += (this->*OpCodes[opcode].Handler)();
	if (!CurBreak && raiseirq) {
		PushWord(Registers.pc); /* Следующая команда */
		PushByte(State.State); /* Сохраняем состояние */
		CurBreak = true;
		clocks += 4; /* 4 такта */
	}
	DMA_wrap ^= clocks & 0x01;
	return clocks;
}

/* Команды */

/* Неизвестный опкод */
template <class _Bus>
int CCPU<_Bus>::OpIllegal() {
	/* Вызываем зависание */
	Halt = true;
	return 0;
}

/* Загрузить в A */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpLDA() {
	Registers.a = _Addr::ReadByte(*this);
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
	return 0;
}

/* Загрузить в X */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpLDX() {
	Registers.x = _Addr::ReadByte(*this);
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
	return 0;
}

/* Загрузить в Y */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpLDY() {
	Registers.y = _Addr::ReadByte(*this);
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
	return 0;
}

/* Сохранить A */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpSTA() {
	_Addr::WriteByte(*this, Registers.a);
	return 0;
}

/* Сохранить X */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpSTX() {
	_Addr::WriteByte(*this, Registers.x);
	return 0;
}

/* Сохранить Y */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpSTY() {
	_Addr::WriteByte(*this, Registers.y);
	return 0;
}

/* Сложение */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpADC() {
	uint16 temp, src;

	src = _Addr::ReadByte(*this);
	temp = src + Registers.a + (State.State & 0x01);
	State.VFlag(((temp ^ Registers.a) & ~(Registers.a ^ src)) & 0x80);
	State.CFlag(temp >= 0x100);
	Registers.a = (uint8) temp;
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
	return 0;
}

/* Вычитание */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpSBC() {
	uint16 temp, src;

	src = _Addr::ReadByte(*this);
	temp = Registers.a - src - (~State.State & 0x01);
	State.VFlag(((temp ^ Registers.a) & (Registers.a ^ src)) & 0x80);
	State.CFlag(temp < 0x100);
	Registers.a = (uint8) temp;
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
	return 0;
}

/* Инкремент */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpINC() {
	uint8 src;

	src = _Addr::ReReadByte(*this);
	src++;
	_Addr::ReWriteByte(*this, src);
	State.NFlag(src);
	State.ZFlag(src);
	return 0;
}

/* Инкремент X */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpINX() {
	Registers.x++;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
	return 0;
}

/* Инкремент Y */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpINY() {
	Registers.y++;
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
	return 0;
}

/* Декремент */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpDEC() {
	uint8 src;

	src = _Addr::ReReadByte(*this);
	src--;
	_Addr::ReWriteByte(*this, src);
	State.NFlag(src);
	State.ZFlag(src);
	return 0;
}

/* Декремент X */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpDEX() {
	Registers.x--;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
	return 0;
}

/* Декремент Y */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpDEY() {
	Registers.y--;
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
	return 0;
}

/* Сдвиг влево */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpASL() {
	uint8 src;

	src = _Addr::ReReadByte(*this);
	State.CFlag(src & 0x80);
	src <<= 1;
	_Addr::ReWriteByte(*this, src);
	State.NFlag(src);
	State.ZFlag(src);
	return 0;
}

/* Сдвиг вправо */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpLSR() {
	uint8 src;

	src = _Addr::ReReadByte(*this);
	State.CFlag(src & 0x01);
	src >>= 1;
	_Addr::ReWriteByte(*this, src);
	State.NFlag(src);
	State.ZFlag(src);
	return 0;
}

/* Циклический сдвиг влево */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpROL() {
	uint8 src, temp;

	src = _Addr::ReReadByte(*this);
	temp = (src << 1) | (State.State & 0x01);
	State.CFlag(src & 0x80);
	_Addr::ReWriteByte(*this, temp);
	State.NFlag(temp);
	State.ZFlag(temp);
	return 0;
}

/* Циклический сдвиг вправо */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpROR() {
	uint8 src, temp;

	src = _Addr::ReReadByte(*this);
	temp = (src >> 1) | ((State.State & 0x01) << 7);
	State.CFlag(src & 0x01);
	_Addr::ReWriteByte(*this, temp);
	State.NFlag(temp);
	State.ZFlag(temp);
	return 0;
}

/* Логическое и */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpAND(){
	Registers.a &= _Addr::ReadByte(*this);
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
	return 0;
}

/* Логическое или */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpORA(){
	Registers.a |= _Addr::ReadByte(*this);
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
	return 0;
}

/* Исключающаее или */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpEOR() {
	Registers.a ^= _Addr::ReadByte(*this);
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
	return 0;
}

/* Сравнение с A */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpCMP() {
	uint16 temp;

	temp = Registers.a - _Addr::ReadByte(*this);
	State.CFlag(temp < 0x100);
	State.NFlag(temp);
	State.ZFlag(temp);
	return 0;
}

/* Сравнение с X */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpCPX() {
	uint16 temp;

	temp = Registers.x - _Addr::ReadByte(*this);
	State.CFlag(temp < 0x100);
	State.NFlag(temp);
	State.ZFlag(temp);
	return 0;
}

/* Сравнение с Y */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpCPY() {
	uint16 temp;

	temp = Registers.y - _Addr::ReadByte(*this);
	State.CFlag(temp < 0x100);
	State.NFlag(temp);
	State.ZFlag(temp);
	return 0;
}

/* Битова проверка */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpBIT() {
	uint16 src;

	src = _Addr::ReadByte(*this);
	State.VFlag(src & 0x40);
	State.NFlag(src);
	State.ZFlag(Registers.a & src);
	return 0;
}

/* Переход по !C */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpBCC() {
	uint16 lastpc;

	if (!State.Carry()) {
		lastpc = Registers.pc;
		Registers.pc = _Addr::GetAddr(*this);
		return ((lastpc & 0xff00) != (Registers.pc & 0xff00)) ? 2 : 1;
	}
	return 0;
}

/* Переход по C */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpBCS() {
	uint16 lastpc;

	if (State.Carry()) {
		lastpc = Registers.pc;
		Registers.pc = _Addr::GetAddr(*this);
		return ((lastpc & 0xff00) != (Registers.pc & 0xff00)) ? 2 : 1;
	}
	return 0;
}

/* Переход по !Z */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpBNE() {
	uint16 lastpc;

	if (!State.Zero()) {
		lastpc = Registers.pc;
		Registers.pc = _Addr::GetAddr(*this);
		return ((lastpc & 0xff00) != (Registers.pc & 0xff00)) ? 2 : 1;
	}
	return 0;
}

/* Переход по Z */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpBEQ() {
	uint16 lastpc;

	if (State.Zero()) {
		lastpc = Registers.pc;
		Registers.pc = _Addr::GetAddr(*this);
		return ((lastpc & 0xff00) != (Registers.pc & 0xff00)) ? 2 : 1;
	}
	return 0;
}

/* Переход по !N */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpBPL() {
	uint16 lastpc;

	if (!State.Negative()) {
		lastpc = Registers.pc;
		Registers.pc = _Addr::GetAddr(*this);
		return ((lastpc & 0xff00) != (Registers.pc & 0xff00)) ? 2 : 1;
	}
	return 0;
}

/* Переход по N */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpBMI() {
	uint16 lastpc;

	if (State.Negative()) {
		lastpc = Registers.pc;
		Registers.pc = _Addr::GetAddr(*this);
		return ((lastpc & 0xff00) != (Registers.pc & 0xff00)) ? 2 : 1;
	}
	return 0;
}

/* Переход по !V */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpBVC() {
	uint16 lastpc;

	if (!State.Overflow()) {
		lastpc = Registers.pc;
		Registers.pc = _Addr::GetAddr(*this);
		return ((lastpc & 0xff00) != (Registers.pc & 0xff00)) ? 2 : 1;
	}
	return 0;
}

/* Переход по V */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpBVS() {
	uint16 lastpc;

	if (State.Overflow()) {
		lastpc = Registers.pc;
		Registers.pc = _Addr::GetAddr(*this);
		return ((lastpc & 0xff00) != (Registers.pc & 0xff00)) ? 2 : 1;
	}
	return 0;
}

/* A_Addr:: X */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpTAX() {
	Registers.x = Registers.a;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
	return 0;
}

/* X_Addr:: A */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpTXA() {
	Registers.a = Registers.x;
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
	return 0;
}

/* A_Addr:: Y */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpTAY() {
	Registers.y = Registers.a;
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
	return 0;
}

/* Y_Addr:: A */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpTYA() {
	Registers.a = Registers.y;
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
	return 0;
}

/* S_Addr:: X */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpTSX() {
	Registers.x = Registers.s;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
	return 0;
}

/* X_Addr:: S */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpTXS() {
	Registers.s = Registers.x;
	return 0;
}

/* Положить в стек A */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpPHA() {
	PushByte(Registers.a);
	return 0;
}

/* Достать и з стека A */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpPLA() {
	Registers.a = PopByte();
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
	return 0;
}

/* Положить в стек P */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpPHP() {
	PushByte(State.State | 0x10); /* Флаг B не используется */
	return 0;
}

/* Достать и з стека P */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpPLP() {
	State.SetState(PopByte());
	static_cast<typename _Bus::APUClass *>(Bus->GetDeviceList()[_Bus::APU])->ForceIRQ();
	return 0;
}

/* Безусловный переход */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpJMP() {
	Registers.pc = _Addr::GetAddr(*this);
	return 0;
}

/* Вызов подпрограммы */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpJSR() {
	PushWord(Registers.pc - 1);
	Registers.pc = _Addr::GetAddr(*this);
	return 0;
}

/* Выход из подпрограммы */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpRTS() {
	Registers.pc = PopWord() + 1;
	return 0;
}

/* Выход из прерывания */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpRTI() {
	State.SetState(PopByte());
	Registers.pc = PopWord();
	static_cast<typename _Bus::APUClass *>(Bus->GetDeviceList()[_Bus::APU])->RaiseIRQ();
	if (!NMI)
		raiseirq = IRQ && !State.Interrupt();
	if (raiseirq)
		raiseirq = true;
	IRQ = false;
	return 0;
}

/* Установить C */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpSEC() {
	State.State |= 0x01;
	return 0;
}

/* Установить D */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpSED() {
	State.State |= 0x08;
	return 0;
}

/* Установить I */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpSEI() {
	State.State |= 0x04;
	return 0;
}

/* Сбросить C */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpCLC() {
	State.State &= 0xfe;
	return 0;
}

/* Сбросить D */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpCLD() {
	State.State &= 0xf7;
	return 0;
}

/* Сбросить I */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpCLI() {
	State.State &= 0xfb;
	static_cast<typename _Bus::APUClass *>(Bus->GetDeviceList()[_Bus::APU])->ForceIRQ();
	return 0;
}

/* Сбросить V */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpCLV() {
	State.State &= 0xbf;
	return 0;
}

/* Нет операции */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpNOP() {
	return 0;
}

/* Вызвать прерывание */
template <class _Bus>
template <class _Addr>
int CCPU<_Bus>::OpBRK() {
	/* BRK использует только 4 такта. Все остальное в главном цикле */
	PushWord(Registers.pc + 1);
	State.State |= 0x10;
	PushByte(State.State);
	State.State |= 0x04;
	CurBreak = true;
	return 0;
}

/* Таблица опкодов */
template <class _Bus>
const typename CCPU<_Bus>::SOpCode CCPU<_Bus>::OpCodes[256] = {
	{4, 1, false, &CCPU<_Bus>::OpBRK<CCPU<_Bus>::Implied>},     /* 0x00 */ /* BRK */
	{6, 2, false, &CCPU<_Bus>::OpORA<CCPU<_Bus>::ZeroPageInd>}, /* 0x01 */ /* ORA */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x02 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x03 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x04 */
	{2, 2, false, &CCPU<_Bus>::OpORA<CCPU<_Bus>::ZeroPage>},    /* 0x05 */ /* ORA */
	{5, 2, false, &CCPU<_Bus>::OpASL<CCPU<_Bus>::ZeroPage>},    /* 0x06 */ /* ASL */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x07 */
	{3, 1, false, &CCPU<_Bus>::OpPHP<CCPU<_Bus>::Implied>},     /* 0x08 */ /* PHP */
	{2, 2, false, &CCPU<_Bus>::OpORA<CCPU<_Bus>::Immediate>},   /* 0x09 */ /* ORA */
	{2, 1, false, &CCPU<_Bus>::OpASL<CCPU<_Bus>::Accumulator>}, /* 0x0a */ /* ASL */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x0b */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x0c */
	{4, 3, false, &CCPU<_Bus>::OpORA<CCPU<_Bus>::Absolute>},    /* 0x0d */ /* ORA */
	{6, 3, false, &CCPU<_Bus>::OpASL<CCPU<_Bus>::Absolute>},    /* 0x0e */ /* ASL */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x0f */
	{2, 2, false, &CCPU<_Bus>::OpBPL<CCPU<_Bus>::Relative>},    /* 0x10 */ /* BPL */
	{5, 2, true, &CCPU<_Bus>::OpORA<CCPU<_Bus>::ZeroPageIndY>}, /* 0x11 */ /* ORA */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x12 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x13 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x14 */
	{3, 2, false, &CCPU<_Bus>::OpORA<CCPU<_Bus>::ZeroPageX>},   /* 0x15 */ /* ORA */
	{6, 2, false, &CCPU<_Bus>::OpASL<CCPU<_Bus>::ZeroPageX>},   /* 0x16 */ /* ASL */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x17 */
	{2, 1, false, &CCPU<_Bus>::OpCLC<CCPU<_Bus>::Implied>},     /* 0x18 */ /* CLC */
	{4, 3, true, &CCPU<_Bus>::OpORA<CCPU<_Bus>::AbsoluteY>},    /* 0x19 */ /* ORA */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x1a */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x1b */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x1c */
	{4, 3, true, &CCPU<_Bus>::OpORA<CCPU<_Bus>::AbsoluteX>},    /* 0x1d */ /* ORA */
	{7, 3, false, &CCPU<_Bus>::OpASL<CCPU<_Bus>::AbsoluteX>},   /* 0x1e */ /* ASL */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x1f */
	{6, 3, false, &CCPU<_Bus>::OpJSR<CCPU<_Bus>::Absolute>},    /* 0x20 */ /* JSR */
	{6, 2, false, &CCPU<_Bus>::OpAND<CCPU<_Bus>::ZeroPageInd>}, /* 0x21 */ /* AND */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x22 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x23 */
	{3, 2, false, &CCPU<_Bus>::OpBIT<CCPU<_Bus>::ZeroPage>},    /* 0x24 */ /* BIT */
	{2, 2, false, &CCPU<_Bus>::OpAND<CCPU<_Bus>::ZeroPage>},    /* 0x25 */ /* AND */
	{5, 2, false, &CCPU<_Bus>::OpROL<CCPU<_Bus>::ZeroPage>},    /* 0x26 */ /* ROL */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x27 */
	{4, 1, false, &CCPU<_Bus>::OpPLP<CCPU<_Bus>::Implied>},     /* 0x28 */ /* PLP */
	{2, 2, false, &CCPU<_Bus>::OpAND<CCPU<_Bus>::Immediate>},   /* 0x29 */ /* AND */
	{2, 1, false, &CCPU<_Bus>::OpROL<CCPU<_Bus>::Accumulator>}, /* 0x2a */ /* ROL */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x2b */
	{4, 3, false, &CCPU<_Bus>::OpBIT<CCPU<_Bus>::Absolute>},    /* 0x2c */ /* BIT */
	{4, 3, false, &CCPU<_Bus>::OpAND<CCPU<_Bus>::Absolute>},    /* 0x2d */ /* AND */
	{6, 3, false, &CCPU<_Bus>::OpROL<CCPU<_Bus>::Absolute>},    /* 0x2e */ /* ROL */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x2f */
	{2, 2, false, &CCPU<_Bus>::OpBMI<CCPU<_Bus>::Relative>},    /* 0x30 */ /* BMI */
	{5, 2, true, &CCPU<_Bus>::OpAND<CCPU<_Bus>::ZeroPageIndY>}, /* 0x31 */ /* AND */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x32 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x33 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x34 */
	{3, 2, false, &CCPU<_Bus>::OpAND<CCPU<_Bus>::ZeroPageX>},   /* 0x35 */ /* AND */
	{6, 2, false, &CCPU<_Bus>::OpROL<CCPU<_Bus>::ZeroPageX>},   /* 0x36 */ /* ROL */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x37 */
	{2, 1, false, &CCPU<_Bus>::OpSEC<CCPU<_Bus>::Implied>},     /* 0x38 */ /* SEC */
	{4, 3, true, &CCPU<_Bus>::OpAND<CCPU<_Bus>::AbsoluteY>},    /* 0x39 */ /* AND */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x3a */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x3b */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x3c */
	{4, 3, true, &CCPU<_Bus>::OpAND<CCPU<_Bus>::AbsoluteX>},    /* 0x3d */ /* AMD */
	{7, 3, false, &CCPU<_Bus>::OpROL<CCPU<_Bus>::AbsoluteX>},   /* 0x3e */ /* ROL */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x3f */
	{6, 1, false, &CCPU<_Bus>::OpRTI<CCPU<_Bus>::Implied>},     /* 0x40 */ /* RTI */
	{6, 2, false, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::ZeroPageInd>}, /* 0x41 */ /* EOR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x42 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x43 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x44 */
	{3, 2, false, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::ZeroPage>},    /* 0x45 */ /* EOR */
	{5, 2, false, &CCPU<_Bus>::OpLSR<CCPU<_Bus>::ZeroPage>},    /* 0x46 */ /* LSR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x47 */
	{3, 1, false, &CCPU<_Bus>::OpPHA<CCPU<_Bus>::Implied>},     /* 0x48 */ /* PHA */
	{2, 2, false, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::Immediate>},   /* 0x49 */ /* EOR */
	{2, 1, false, &CCPU<_Bus>::OpLSR<CCPU<_Bus>::Accumulator>}, /* 0x4a */ /* LSR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x4b */
	{3, 3, false, &CCPU<_Bus>::OpJMP<CCPU<_Bus>::Absolute>},    /* 0x4c */ /* JMP */
	{4, 3, false, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::Absolute>},    /* 0x4d */ /* EOR */
	{6, 3, false, &CCPU<_Bus>::OpLSR<CCPU<_Bus>::Absolute>},    /* 0x4e */ /* LSR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x4f */
	{2, 2, false, &CCPU<_Bus>::OpBVC<CCPU<_Bus>::Relative>},    /* 0x50 */ /* BVC */
	{5, 2, true, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::ZeroPageIndY>}, /* 0x51 */ /* EOR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x52 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x53 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x54 */
	{4, 2, false, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::ZeroPageX>},   /* 0x55 */ /* EOR */
	{6, 2, false, &CCPU<_Bus>::OpLSR<CCPU<_Bus>::ZeroPageX>},   /* 0x56 */ /* LSR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x57 */
	{2, 1, false, &CCPU<_Bus>::OpCLI<CCPU<_Bus>::Implied>},     /* 0x58 */ /* CLI */
	{4, 3, true, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::AbsoluteY>},    /* 0x59 */ /* EOR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x5a */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x5b */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x5c */
	{4, 3, true, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::AbsoluteX>},    /* 0x5d */ /* EOR */
	{7, 3, false, &CCPU<_Bus>::OpLSR<CCPU<_Bus>::AbsoluteX>},   /* 0x5e */ /* LSR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x5f */
	{6, 1, false, &CCPU<_Bus>::OpRTS<CCPU<_Bus>::Implied>},     /* 0x60 */ /* RTS */
	{6, 2, false, &CCPU<_Bus>::OpADC<CCPU<_Bus>::ZeroPageInd>}, /* 0x61 */ /* ADC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x62 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x63 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x64 */
	{3, 2, false, &CCPU<_Bus>::OpADC<CCPU<_Bus>::ZeroPage>},    /* 0x65 */ /* ADC */
	{5, 2, false, &CCPU<_Bus>::OpROR<CCPU<_Bus>::ZeroPage>},    /* 0x66 */ /* ROR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x67 */
	{4, 1, false, &CCPU<_Bus>::OpPLA<CCPU<_Bus>::Implied>},     /* 0x68 */ /* PLA */
	{2, 2, false, &CCPU<_Bus>::OpADC<CCPU<_Bus>::Immediate>},   /* 0x69 */ /* ADC */
	{2, 1, false, &CCPU<_Bus>::OpROR<CCPU<_Bus>::Accumulator>}, /* 0x6a */ /* ROR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x6b */
	{5, 3, false, &CCPU<_Bus>::OpJMP<CCPU<_Bus>::Indirect>},    /* 0x6c */ /* JMP */
	{4, 3, false, &CCPU<_Bus>::OpADC<CCPU<_Bus>::Absolute>},    /* 0x6d */ /* ADC */
	{6, 3, false, &CCPU<_Bus>::OpROR<CCPU<_Bus>::Absolute>},    /* 0x6e */ /* ROR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x6f */
	{2, 2, false, &CCPU<_Bus>::OpBVS<CCPU<_Bus>::Relative>},    /* 0x70 */ /* BVS */
	{5, 2, true, &CCPU<_Bus>::OpADC<CCPU<_Bus>::ZeroPageIndY>}, /* 0x71 */ /* ADC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x72 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x73 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x74 */
	{4, 2, false, &CCPU<_Bus>::OpADC<CCPU<_Bus>::ZeroPageX>},   /* 0x75 */ /* ADC */
	{6, 2, false, &CCPU<_Bus>::OpROR<CCPU<_Bus>::ZeroPageX>},   /* 0x76 */ /* ROR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x77 */
	{2, 1, false, &CCPU<_Bus>::OpSEI<CCPU<_Bus>::Implied>},     /* 0x78 */ /* SEI */
	{4, 3, true, &CCPU<_Bus>::OpADC<CCPU<_Bus>::AbsoluteY>},    /* 0x79 */ /* ADC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x7a */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x7b */
	{5, 3, true, &CCPU<_Bus>::OpJMP<CCPU<_Bus>::AbsoluteInd>},  /* 0x7c */ /* JMP */
	{4, 3, true, &CCPU<_Bus>::OpADC<CCPU<_Bus>::AbsoluteX>},    /* 0x7d */ /* ADC */
	{7, 3, false, &CCPU<_Bus>::OpROR<CCPU<_Bus>::AbsoluteX>},   /* 0x7e */ /* ROR */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x7f */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x80 */
	{6, 2, false, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::ZeroPageInd>}, /* 0x81 */ /* STA */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x82 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x83 */
	{3, 2, false, &CCPU<_Bus>::OpSTY<CCPU<_Bus>::ZeroPage>},    /* 0x84 */ /* STY */
	{3, 2, false, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::ZeroPage>},    /* 0x85 */ /* STA */
	{3, 2, false, &CCPU<_Bus>::OpSTX<CCPU<_Bus>::ZeroPage>},    /* 0x86 */ /* STX */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x87 */
	{2, 1, false, &CCPU<_Bus>::OpDEY<CCPU<_Bus>::Implied>},     /* 0x88 */ /* DEY */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x89 */
	{2, 1, false, &CCPU<_Bus>::OpTXA<CCPU<_Bus>::Implied>},     /* 0x8a */ /* TXA */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x8b */
	{4, 3, false, &CCPU<_Bus>::OpSTY<CCPU<_Bus>::Absolute>},    /* 0x8c */ /* STY */
	{4, 3, false, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::Absolute>},    /* 0x8d */ /* STA */
	{4, 3, false, &CCPU<_Bus>::OpSTX<CCPU<_Bus>::Absolute>},    /* 0x8e */ /* STX */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x8f */
	{2, 2, false, &CCPU<_Bus>::OpBCC<CCPU<_Bus>::Relative>},    /* 0x90 */ /* BCC */
	{6, 2, false, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::ZeroPageIndY>},/* 0x91 */ /* STA */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x92 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x93 */
	{4, 2, false, &CCPU<_Bus>::OpSTY<CCPU<_Bus>::ZeroPageX>},   /* 0x94 */ /* STY */
	{4, 2, false, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::ZeroPageX>},   /* 0x95 */ /* STA */
	{4, 2, false, &CCPU<_Bus>::OpSTX<CCPU<_Bus>::ZeroPageY>},   /* 0x96 */ /* STX */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x97 */
	{2, 1, false, &CCPU<_Bus>::OpTYA<CCPU<_Bus>::Implied>},     /* 0x98 */ /* TYA */
	{5, 3, false, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::AbsoluteY>},   /* 0x99 */ /* STA */
	{2, 1, false, &CCPU<_Bus>::OpTXS<CCPU<_Bus>::Implied>},     /* 0x9a */ /* TXS */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x9b */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x9c */
	{5, 3, false, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::AbsoluteX>},   /* 0x9d */ /* STA */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x9e */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0x9f */
	{2, 2, false, &CCPU<_Bus>::OpLDY<CCPU<_Bus>::Immediate>},   /* 0xa0 */ /* LDY */
	{6, 2, false, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::ZeroPageInd>}, /* 0xa1 */ /* LDA */
	{2, 2, false, &CCPU<_Bus>::OpLDX<CCPU<_Bus>::Immediate>},   /* 0xa2 */ /* LDX */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xa3 */
	{3, 2, false, &CCPU<_Bus>::OpLDY<CCPU<_Bus>::ZeroPage>},    /* 0xa4 */ /* LDY */
	{3, 2, false, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::ZeroPage>},    /* 0xa5 */ /* LDA */
	{3, 2, false, &CCPU<_Bus>::OpLDX<CCPU<_Bus>::ZeroPage>},    /* 0xa6 */ /* LDX */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xa7 */
	{2, 1, false, &CCPU<_Bus>::OpTAY<CCPU<_Bus>::Implied>},     /* 0xa8 */ /* TAY */
	{2, 2, false, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::Immediate>},   /* 0xa9 */ /* LDA */
	{2, 1, false, &CCPU<_Bus>::OpTAX<CCPU<_Bus>::Implied>},     /* 0xaa */ /* TAX */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xab */
	{4, 3, false, &CCPU<_Bus>::OpLDY<CCPU<_Bus>::Absolute>},    /* 0xac */ /* LDY */
	{4, 3, false, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::Absolute>},    /* 0xad */ /* LDA */
	{4, 3, false, &CCPU<_Bus>::OpLDX<CCPU<_Bus>::Absolute>},    /* 0xae */ /* LDX */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xaf */
	{2, 2, false, &CCPU<_Bus>::OpBCS<CCPU<_Bus>::Relative>},    /* 0xb0 */ /* BCS */
	{5, 2, true, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::ZeroPageIndY>}, /* 0xb1 */ /* LDA */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xb2 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xb3 */
	{4, 2, false, &CCPU<_Bus>::OpLDY<CCPU<_Bus>::ZeroPageX>},   /* 0xb4 */ /* LDY */
	{4, 2, false, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::ZeroPageX>},   /* 0xb5 */ /* LDA */
	{4, 2, false, &CCPU<_Bus>::OpLDX<CCPU<_Bus>::ZeroPageY>},   /* 0xb6 */ /* LDX */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xb7 */
	{2, 1, false, &CCPU<_Bus>::OpCLV<CCPU<_Bus>::Implied>},     /* 0xb8 */ /* CLV */
	{4, 3, true, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::AbsoluteY>},    /* 0xb9 */ /* LDA */
	{2, 1, false, &CCPU<_Bus>::OpTSX<CCPU<_Bus>::Implied>},     /* 0xba */ /* TSX */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xbb */
	{4, 3, true, &CCPU<_Bus>::OpLDY<CCPU<_Bus>::AbsoluteX>},    /* 0xbc */ /* LDY */
	{4, 3, true, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::AbsoluteX>},    /* 0xbd */ /* LDA */
	{4, 3, true, &CCPU<_Bus>::OpLDX<CCPU<_Bus>::AbsoluteY>},    /* 0xbe */ /* LDX */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xbf */
	{2, 2, false, &CCPU<_Bus>::OpCPY<CCPU<_Bus>::Immediate>},   /* 0xc0 */ /* CPY */
	{6, 2, false, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::ZeroPageInd>}, /* 0xc1 */ /* CMP */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xc2 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xc3 */
	{3, 2, false, &CCPU<_Bus>::OpCPY<CCPU<_Bus>::ZeroPage>},    /* 0xc4 */ /* CPY */
	{3, 2, false, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::ZeroPage>},    /* 0xc5 */ /* CMP */
	{5, 2, false, &CCPU<_Bus>::OpDEC<CCPU<_Bus>::ZeroPage>},    /* 0xc6 */ /* DEC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xc7 */
	{2, 1, false, &CCPU<_Bus>::OpINY<CCPU<_Bus>::Implied>},     /* 0xc8 */ /* INY */
	{2, 2, false, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::Immediate>},   /* 0xc9 */ /* CMP */
	{2, 1, false, &CCPU<_Bus>::OpDEX<CCPU<_Bus>::Implied>},     /* 0xca */ /* DEX */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xcb */
	{4, 3, false, &CCPU<_Bus>::OpCPY<CCPU<_Bus>::Absolute>},    /* 0xcc */ /* CPY */
	{4, 3, false, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::Absolute>},    /* 0xcd */ /* CMP */
	{6, 3, false, &CCPU<_Bus>::OpDEC<CCPU<_Bus>::Absolute>},    /* 0xce */ /* DEC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xcf */
	{2, 2, false, &CCPU<_Bus>::OpBNE<CCPU<_Bus>::Relative>},    /* 0xd0 */ /* BNE */
	{5, 2, true, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::ZeroPageIndY>}, /* 0xd1 */ /* CMP */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xd2 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xd3 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xd4 */
	{4, 2, false, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::ZeroPageX>},   /* 0xd5 */ /* CMP */
	{6, 2, false, &CCPU<_Bus>::OpDEC<CCPU<_Bus>::ZeroPageX>},   /* 0xd6 */ /* DEC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xd7 */
	{2, 1, false, &CCPU<_Bus>::OpCLD<CCPU<_Bus>::Implied>},     /* 0xd8 */ /* CLD */
	{4, 3, true, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::AbsoluteY>},    /* 0xd9 */ /* CMP */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xda */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xdb */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xdc */
	{4, 3, true, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::AbsoluteX>},    /* 0xdd */ /* CMP */
	{7, 3, false, &CCPU<_Bus>::OpDEC<CCPU<_Bus>::AbsoluteX>},   /* 0xde */ /* DEC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xdf */
	{2, 2, false, &CCPU<_Bus>::OpCPX<CCPU<_Bus>::Immediate>},   /* 0xe0 */ /* CPX */
	{6, 2, false, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::ZeroPageInd>}, /* 0xe1 */ /* SBC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xe2 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xe3 */
	{3, 2, false, &CCPU<_Bus>::OpCPX<CCPU<_Bus>::ZeroPage>},    /* 0xe4 */ /* CPX */
	{3, 2, false, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::ZeroPage>},    /* 0xe5 */ /* SBC */
	{5, 2, false, &CCPU<_Bus>::OpINC<CCPU<_Bus>::ZeroPage>},    /* 0xe6 */ /* INC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xe7 */
	{2, 1, false, &CCPU<_Bus>::OpINX<CCPU<_Bus>::Implied>},     /* 0xe8 */ /* INX */
	{2, 2, false, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::Immediate>},   /* 0xe9 */ /* SBC */
	{2, 1, false, &CCPU<_Bus>::OpNOP<CCPU<_Bus>::Implied>},     /* 0xea */ /* NOP */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xeb */
	{4, 3, false, &CCPU<_Bus>::OpCPX<CCPU<_Bus>::Absolute>},    /* 0xec */ /* CPX */
	{4, 3, false, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::Absolute>},    /* 0xed */ /* SBC */
	{6, 3, false, &CCPU<_Bus>::OpINC<CCPU<_Bus>::Absolute>},    /* 0xee */ /* INC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xef */
	{2, 2, false, &CCPU<_Bus>::OpBEQ<CCPU<_Bus>::Relative>},    /* 0xf0 */ /* BEQ */
	{5, 2, true, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::ZeroPageIndY>}, /* 0xf1 */ /* SBC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xf2 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xf3 */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xf4 */
	{4, 2, false, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::ZeroPageX>},   /* 0xf5 */ /* SBC */
	{6, 2, false, &CCPU<_Bus>::OpINC<CCPU<_Bus>::ZeroPageX>},   /* 0xf6 */ /* INC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xf7 */
	{2, 1, false, &CCPU<_Bus>::OpSED<CCPU<_Bus>::Implied>},     /* 0xf8 */ /* SED */
	{4, 3, true, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::AbsoluteY>},    /* 0xf9 */ /* SBC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xfa */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xfb */
	{0, 1, false, &CCPU<_Bus>::OpIllegal},                      /* 0xfc */
	{4, 3, true, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::AbsoluteX>},    /* 0xfd */ /* SBC */
	{7, 3, false, &CCPU<_Bus>::OpINC<CCPU<_Bus>::AbsoluteX>},   /* 0xfe */ /* INC */
	{0, 1, false, &CCPU<_Bus>::OpIllegal}                       /* 0xff */
};

}

#endif
