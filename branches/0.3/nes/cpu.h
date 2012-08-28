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
#include "manager.h"
#include "bus.h"

namespace vpnes {

namespace CPUID {

typedef CPUGroup<1>::ID::NoBatteryID StateID;
typedef CPUGroup<2>::ID::NoBatteryID RegistersID;
typedef CPUGroup<3>::ID::NoBatteryID RAMID;
typedef CPUGroup<4>::ID::NoBatteryID CycleDataID;
typedef CPUGroup<5>::ID::NoBatteryID InternalDataID;

}

/* CPU */
template <class _Bus, class _Settings>
class CCPU: public CDevice {
public:
	/* Делитель частоты */
	enum { ClockDivider = _Settings::CPU_Divider };
private:
	/* Шина */
	_Bus *Bus;

	/* Обработчик инструкции */
	typedef void (CCPU::*OpHandler)();

	/* Описание опкода */
	struct SOpcode {
		int Cycles; /* Номиальное число тактов */
		int Length; /* Длина команды */
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
	
	/* Встроенная память */
	uint8 *RAM;

	/* Данные о тактах */
	struct SCycleData {
		int NMICycle; /* Такт распознавания NMI */
		int IRQCycle; /* Такт распознавания IRQ */
		int Cycles; /* Всего тактов */
	} CycleData;

	/* Внутренние данные */
	struct SInternalData {
		bool Halt; /* Зависание */
	} InternalData;

	/* Обращения к памяти */
	inline uint8 ReadMemory(uint16 Address) {
		Bus->GetClock()->Clock(1);
		return Bus->ReadCPUMemory(Address);
	}
	inline void WriteMemory(uint16 Address, uint8 Src) {
		Bus->GetClock()->Clock(1);
		Bus->WriteCPUMemory(Address, Src);
	}

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
		PushByte(Src & 0x00ff);
	}
	/* Достать из стека слово */
	inline uint16 PopWord() {
		return PopByte() | (PopByte() << 8);
	}

	/* Адресация */

	/* Аккумулятор */
	struct Accumulator {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.Registers.a;
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.Registers.a = Src;
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			return ReadByte(CPU);
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			WriteByte(CPU, Src);
		}
	};

	/* Без параметра */
	struct Implied {
	};

	/* В параметре значение */
	struct Immediate {
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.ReadMemory(CPU.Registers.pc - 1);
		}
	};

	/* В параметре адрес */
	struct Absolute {
		inline static uint16 GetAddr(CCPU &CPU) {
			return CPU.ReadMemory(CPU.Registers.pc - 2) |
				(CPU.ReadMemory(CPU.Registers.pc - 1) << 8);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.ReadMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.WriteMemory(GetAddr(CPU), Src);
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			uint8 Res;

			Address = GetAddr(CPU);
			Res = CPU.ReadMemory(Address);
			CPU.WriteMemory(Address, Res); /* WTF?! 0_o */
			return Res;
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 Address, uint8 Src) {
			CPU.WriteMemory(Address, Src);
		}
	};

	/* ZP */
	struct ZeroPage {
		inline static uint8 ReadByte(CCPU &CPU) {
			/* 3 лишних такта... забить */
			return CPU.RAM[CPU.ReadMemory(CPU.Registers.pc - 1)];
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			/* Собственно и тут тоже... */
			CPU.RAM[CPU.ReadMemory(CPU.Registers.pc - 1)] = Src;
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			uint8 Res;

			Address = CPU.ReadMemory(CPU.Registers.pc - 1);
			Res = CPU.RAM[Address];
			//CPU.Bus->GetClock()->Clock(9); /* Неизвестное поведение */
			/* Опять-таки, а какая разница? */
			return Res;
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 Address, uint8 Src) {
			CPU.RAM[Address] = Src;
		}
	};

	/* PC + значение */
	struct Relative {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint16 Res;

			//CPU.Bus->GetClock()->Clock(??); /* Неизвестное поведение */
			Res = CPU.Registers.pc + (sint8) CPU.ReadMemory(CPU.Registers.pc - 1);
			if ((Res & 0xff00) != (CPU.Registers.pc & 0xff00))
				CPU.CycleData.Cycles++;
			return Res;
		}
	};

	/* Адрес + X */
	struct AbsoluteX {
		inline static uint16 GetAddr(CCPU &CPU, uint16 &Address) {
			Address = CPU.ReadMemory(CPU.Registers.pc - 2) + CPU.Registers.x;
			return (CPU.ReadMemory(CPU.Registers.pc - 1) << 8) + Address;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			uint16 Address, Page;

			Address = GetAddr(CPU, Page);
			if (Page & 0x0100) { /* Перепрыгнули страницу */
				CPU.ReadMemory(Address - 0x0100); /* Промазал Ha-Ha */
				CPU.CycleData.Cycles++;
			}
			return CPU.ReadMemory(Address);
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			uint16 Address, Page;

			Address = GetAddr(CPU, Page);
			CPU.ReadMemory(Address - (Page & 0x0100)); /* WTF?! */
			CPU.WriteMemory(Address, Src);
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			uint16 Page;
			uint8 Res;

			Address = GetAddr(CPU, Page);
			CPU.ReadMemory(Address - (Page & 0x0100));
			Res = CPU.ReadMemory(Address);
			CPU.WriteMemory(Address, Res); /* WTF?! */
			return Res;
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			CPU.WriteMemory(Address, Src);
		}
	};

	/* Адрес + Y */
	struct AbsoluteY {
		inline static uint16 GetAddr(CCPU &CPU, uint16 &Address) {
			Address = CPU.ReadMemory(CPU.Registers.pc - 2) + CPU.Registers.y;
			return (CPU.ReadMemory(CPU.Registers.pc - 1) << 8) + Address;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			uint16 Address, Page;

			Address = GetAddr(CPU, Page);
			if (Page & 0x0100) {
				CPU.ReadMemory(Address - 0x0100);
				CPU.CycleData.Cycles++;
			}
			return CPU.ReadMemory(Address);
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			uint16 Address, Page;

			Address = GetAddr(CPU, Page);
			CPU.ReadMemory(Address - (Page & 0x0100));
			CPU.WriteMemory(Address, Src);
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			uint16 Page;
			uint8 Res;

			Address = GetAddr(CPU, Page);
			CPU.ReadMemory(Address - (Page & 0x0100));
			Res = CPU.ReadMemory(Address);
			CPU.WriteMemory(Address, Res);
			return Res;
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			CPU.WriteMemory(Address, Src);
		}
	};

	/* ZP + X */
	struct ZeroPageX {
		inline static uint8 GetAddr(CCPU &CPU) {
			return CPU.ReadMemory(CPU.Registers.pc - 1) + CPU.Registers.x;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.RAM[GetAddr(CPU)];
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.RAM[GetAddr(CPU)] = Src;
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			Address = GetAddr(CPU);
			return CPU.RAM[Address];
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			CPU.RAM[Address] = Src;
		}
	};

	/* ZP + Y */
	struct ZeroPageY {
		inline static uint8 GetAddr(CCPU &CPU) {
			return CPU.ReadMemory(CPU.Registers.pc - 1) + CPU.Registers.y;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.RAM[GetAddr(CPU)];
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.RAM[GetAddr(CPU)] = Src;
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			Address = GetAddr(CPU);
			return CPU.RAM[Address];
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			CPU.RAM[Address] = Src;
		}
	};

	/* Адрес + indirect */
	struct AbsoluteInd {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint16 Address;

			Address = CPU.ReadMemory(CPU.Registers.pc - 2) |
				(CPU.ReadMemory(CPU.Registers.pc - 1) << 8);
			return CPU.ReadMemory(Address) | (CPU.ReadMemory(\
				(Address & 0xff00) | ((Address + 1) & 0x00ff)) << 8);
		}
	};

	/* ZP + X + indirect */
	struct ZeroPageInd {
		inline static uint16 GetAddr(CCPU &CPU) {
			uint8 AddressOffset;

			AddressOffset = CPU.ReadMemory(CPU.Registers.pc - 1) + CPU.Registers.x;
			CPU.Bus->GetClock()->Clock(3);
			return CPU.RAM[AddressOffset] | (CPU.RAM[(uint8) (AddressOffset + 1)] << 8);
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			return CPU.ReadMemory(GetAddr(CPU));
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			CPU.WriteMemory(GetAddr(CPU), Src);
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			uint8 Res;

			Address = GetAddr(CPU);
			Res = CPU.ReadMemory(Address);
			CPU.WriteMemory(Address, Res);
			return Res;
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			CPU.WriteMemory(Address, Src);
		}
	};

	/* ZP + indirect + Y */
	struct ZeroPageIndY {
		inline static uint16 GetAddr(CCPU &CPU, uint16 &Address) {
			uint8 AddressOffset;

			AddressOffset = CPU.ReadMemory(CPU.Registers.pc - 1);
			Address = CPU.RAM[AddressOffset] + CPU.Registers.y;
			return (CPU.RAM[(uint8) (AddressOffset + 1)] << 8) + Address;
		}
		inline static uint8 ReadByte(CCPU &CPU) {
			uint16 Address, Page;

			Address = GetAddr(CPU, Page);
			if (Page & 0x0100) {
				CPU.ReadMemory(Address - 0x0100);
				CPU.CycleData.Cycles++;
			}
			return CPU.ReadMemory(Address);
		}
		inline static void WriteByte(CCPU &CPU, uint8 Src) {
			uint16 Address, Page;

			Address = GetAddr(CPU, Page);
			CPU.ReadMemory(Address - (Page & 0x0100));
			CPU.WriteMemory(Address, Src);
		}
		inline static uint8 ReadByte_RW(CCPU &CPU, uint16 &Address) {
			uint16 Page;
			uint8 Res;

			Address = GetAddr(CPU, Page);
			CPU.ReadMemory(Address - (Page & 0x0100));
			Res = CPU.ReadMemory(Address);
			CPU.WriteMemory(Address, Res);
			return Res;
		}
		inline static void WriteByte_RW(CCPU &CPU, uint16 &Address, uint8 Src) {
			CPU.WriteMemory(Address, Src);
		}
	};

public:
	inline explicit CCPU(_Bus *pBus) {
		Bus = pBus;
		Bus->GetManager()->template SetPointer<CPUID::StateID>(\
			&State, sizeof(State));
		State.State = 0x20; /* Бит 5 всегда 1 */
		Bus->GetManager()->template SetPointer<CPUID::RegistersID>(\
			&Registers, sizeof(Registers));
		memset(&Registers, 0, sizeof(Registers));
		RAM = (uint8 *) Bus->GetManager()->template GetPointer<CPUID::RAMID>(\
			0x0800 * sizeof(uint8));
		memset(RAM, 0xff, 0x0800 * sizeof(uint8));
		Bus->GetManager()->template SetPointer<CPUID::CycleDataID>(\
			&CycleData, sizeof(CycleData));
		Bus->GetManager()->template SetPointer<CPUID::InternalDataID>(\
			&InternalData, sizeof(InternalData));
	}
	inline ~CCPU() {}

	/* Обработать такты */
	inline void Execute();

	/* Сброс */
	inline void Reset() {
		CycleData.Cycles = 7;
		Registers.s -= 3;
		State.State |= 0x04; /* Interrupt */
		memset(&InternalData, 0, sizeof(InternalData));
		Bus->GetClock()->Clock(5);
		Registers.pc = ReadMemory(0xfffc) | (ReadMemory(0xfffd) << 8);
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		return RAM[Address & 0x07ff];
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		RAM[Address & 0x07ff] = Src;
	}

	/* Получить число тактов */
	inline int GetCycles() const { return CycleData.Cycles; }
	/* RAM */
	inline const uint8 *GetRAM() const { return RAM; }
private:
	/* Команды CPU */

	/* Неизвестная команда */
	void OpIllegal();

	/* Управление памятью */

	/* Загрузить в A */
	template <class _Addr>
	void OpLDA();
	/* Загрузить в X */
	template <class _Addr>
	void OpLDX();
	/* Загрузить в Y */
	template <class _Addr>
	void OpLDY();
	/* Сохранить A */
	template <class _Addr>
	void OpSTA();
	/* Сохранить X */
	template <class _Addr>
	void OpSTX();
	/* Сохранить Y */
	template <class _Addr>
	void OpSTY();

	/* Арифметические операции */

	/* Сложение */
	template <class _Addr>
	void OpADC();
	/* Вычитание */
	template <class _Addr>
	void OpSBC();

	/* Инкремент / декремент */

	/* Инкремент */
	template <class _Addr>
	void OpINC();
	/* Инкремент X */
	template <class _Addr>
	void OpINX();
	/* Инкремент Y */
	template <class _Addr>
	void OpINY();
	/* Декремент */
	template <class _Addr>
	void OpDEC();
	/* Декремент X */
	template <class _Addr>
	void OpDEX();
	/* Декремент Y */
	template <class _Addr>
	void OpDEY();

	/* Сдвиги */

	/* Сдвиг влево */
	template <class _Addr>
	void OpASL();
	/* Сдвиг вправо */
	template <class _Addr>
	void OpLSR();
	/* Циклический сдвиг влево */
	template <class _Addr>
	void OpROL();
	/* Циклический сдвиг вправо */
	template <class _Addr>
	void OpROR();

	/* Логические операции */

	/* Логическое и */
	template <class _Addr>
	void OpAND();
	/* Логическое или */
	template <class _Addr>
	void OpORA();
	/* Исключающаее или */
	template <class _Addr>
	void OpEOR();

	/* Сравнения */

	/* Сравнение с A */
	template <class _Addr>
	void OpCMP();
	/* Сравнение с X */
	template <class _Addr>
	void OpCPX();
	/* Сравнение с Y */
	template <class _Addr>
	void OpCPY();
	/* Битова проверка */
	template <class _Addr>
	void OpBIT();

	/* Переходы */

	/* Переход по !C */
	template <class _Addr>
	void OpBCC();
	/* Переход по C */
	template <class _Addr>
	void OpBCS();
	/* Переход по !Z */
	template <class _Addr>
	void OpBNE();
	/* Переход по Z */
	template <class _Addr>
	void OpBEQ();
	/* Переход по !N */
	template <class _Addr>
	void OpBPL();
	/* Переход по N */
	template <class _Addr>
	void OpBMI();
	/* Переход по !V */
	template <class _Addr>
	void OpBVC();
	/* Переход по V */
	template <class _Addr>
	void OpBVS();

	/* Передача */

	/* A -> X */
	template <class _Addr>
	void OpTAX();
	/* X -> A */
	template <class _Addr>
	void OpTXA();
	/* A -> Y */
	template <class _Addr>
	void OpTAY();
	/* Y -> A */
	template <class _Addr>
	void OpTYA();
	/* S -> X */
	template <class _Addr>
	void OpTSX();
	/* X -> S */
	template <class _Addr>
	void OpTXS();

	/* Стек */

	/* Положить в стек A */
	template <class _Addr>
	void OpPHA();
	/* Достать и з стека A */
	template <class _Addr>
	void OpPLA();
	/* Положить в стек P */
	template <class _Addr>
	void OpPHP();
	/* Достать и з стека P */
	template <class _Addr>
	void OpPLP();

	/* Прыжки */

	/* Безусловный переход */
	template <class _Addr>
	void OpJMP();
	/* Вызов подпрограммы */
	template <class _Addr>
	void OpJSR();
	/* Выход из подпрограммы */
	template <class _Addr>
	void OpRTS();
	/* Выход из прерывания */
	template <class _Addr>
	void OpRTI();

	/* Управление флагами */

	/* Установить C */
	template <class _Addr>
	void OpSEC();
	/* Установить D */
	template <class _Addr>
	void OpSED();
	/* Установить I */
	template <class _Addr>
	void OpSEI();
	/* Сбросить C */
	template <class _Addr>
	void OpCLC();
	/* Сбросить D */
	template <class _Addr>
	void OpCLD();
	/* Сбросить I */
	template <class _Addr>
	void OpCLI();
	/* Сбросить V */
	template <class _Addr>
	void OpCLV();

	/* Другое */

	/* Нет операции */
	template <class _Addr>
	void OpNOP();
	/* Вызвать прерывание */
	template <class _Addr>
	void OpBRK();
};

template <class _Bus, class _Settings>
void CCPU<_Bus, _Settings>::Execute() {
	uint8 opcode;

	if (InternalData.Halt) /* Зависли */
		return;
	/* Текущий опкод */
	opcode = ReadMemory(Registers.pc);
	Registers.pc += Opcodes[opcode].Length;
	CycleData.Cycles = Opcodes[opcode].Cycles;
	/* Выполняем */
	(this->*Opcodes[opcode].Handler)();
}

/* Команды */

/* Неизвестный опкод */
template <class _Bus, class _Settings>
void CCPU<_Bus, _Settings>::OpIllegal() {
	VPNES_CPUHALT HaltData;

	/* Зависание */
	InternalData.Halt = true;
	HaltData.PC = Registers.pc;
	HaltData.A = Registers.a;
	HaltData.X = Registers.x;
	HaltData.Y = Registers.y;
	HaltData.S = Registers.s;
	HaltData.State = State.State;
	Bus->GetCallback()(VPNES_CALLBACK_CPUHALT, (void *) &HaltData);
}

/* Управление памятью */

/* Загрузить в A */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpLDA() {
	Registers.a = _Addr::ReadByte(*this);
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Загрузить в X */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpLDX() {
	Registers.x = _Addr::ReadByte(*this);
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* Загрузить в Y */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpLDY() {
	Registers.y = _Addr::ReadByte(*this);
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
}

/* Сохранить A */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpSTA() {
	_Addr::WriteByte(*this, Registers.a);
}

/* Сохранить X */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpSTX() {
	_Addr::WriteByte(*this, Registers.x);
}

/* Сохранить Y */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpSTY() {
	_Addr::WriteByte(*this, Registers.y);
}

/* Арифметические опрации */

/* Сложение */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpADC() {
	uint16 temp, src;

	src = _Addr::ReadByte(*this);
	temp = src + Registers.a + (State.State & 0x01);
	State.VFlag(((temp ^ Registers.a) & ~(Registers.a ^ src)) & 0x80);
	State.CFlag(temp >= 0x0100);
	Registers.a = (uint8) temp;
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Вычитание */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpSBC() {
	uint16 temp, src;

	src = _Addr::ReadByte(*this);
	temp = Registers.a - src - (~State.State & 0x01);
	State.VFlag(((temp ^ Registers.a) & (Registers.a ^ src)) & 0x80);
	State.CFlag(temp < 0x0100);
	Registers.a = (uint8) temp;
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Инкремент / декремент */

/* Инкремент */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpINC() {
	uint8 src;
	uint16 Address;

	src = _Addr::ReadByte_RW(*this, Address);
	src++;
	_Addr::WriteByte_RW(*this, Address, src);
	State.NFlag(src);
	State.ZFlag(src);
}

/* Инкремент X */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpINX() {
	Registers.x++;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* Инкремент Y */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpINY() {
	Registers.y++;
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
}

/* Декремент */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpDEC() {
	uint8 src;
	uint16 Address;

	src = _Addr::ReadByte_RW(*this, Address);
	src--;
	_Addr::WriteByte_RW(*this, Address, src);
	State.NFlag(src);
	State.ZFlag(src);
}

/* Декремент X */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpDEX() {
	Registers.x--;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* Декремент Y */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpDEY() {
	Registers.y--;
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
}

/* Сдвиги */

/* Сдвиг влево */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpASL() {
	uint8 src;
	uint16 Address;

	src = _Addr::ReadByte_RW(*this, Address);
	State.CFlag(src & 0x80);
	src <<= 1;
	_Addr::WriteByte_RW(*this, Address, src);
	State.NFlag(src);
	State.ZFlag(src);
}

/* Сдвиг вправо */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpLSR() {
	uint8 src;
	uint16 Address;

	src = _Addr::ReadByte_RW(*this, Address);
	State.CFlag(src & 0x01);
	src >>= 1;
	_Addr::WriteByte_RW(*this, Address, src);
	State.NFlag(src);
	State.ZFlag(src);
}

/* Циклический сдвиг влево */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpROL() {
	uint8 src, temp;
	uint16 Address;

	src = _Addr::ReadByte_RW(*this, Address);
	temp = (src << 1) | (State.State & 0x01);
	State.CFlag(src & 0x80);
	_Addr::WriteByte_RW(*this, Address, temp);
	State.NFlag(temp);
	State.ZFlag(temp);
}

/* Циклический сдвиг вправо */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpROR() {
	uint8 src, temp;
	uint16 Address;

	src = _Addr::ReadByte_RW(*this, Address);
	temp = (src >> 1) | ((State.State & 0x01) << 7);
	State.CFlag(src & 0x01);
	_Addr::WriteByte_RW(*this, Address, temp);
	State.NFlag(temp);
	State.ZFlag(temp);
}

/* Логические операции */

/* Логическое и */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpAND(){
	Registers.a &= _Addr::ReadByte(*this);
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Логическое или */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpORA(){
	Registers.a |= _Addr::ReadByte(*this);
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Исключающаее или */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpEOR() {
	Registers.a ^= _Addr::ReadByte(*this);
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Сравнения */

/* Сравнение с A */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpCMP() {
	uint16 temp;

	temp = Registers.a - _Addr::ReadByte(*this);
	State.CFlag(temp < 0x0100);
	State.NFlag(temp);
	State.ZFlag(temp);
}

/* Сравнение с X */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpCPX() {
	uint16 temp;

	temp = Registers.x - _Addr::ReadByte(*this);
	State.CFlag(temp < 0x0100);
	State.NFlag(temp);
	State.ZFlag(temp);
}

/* Сравнение с Y */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpCPY() {
	uint16 temp;

	temp = Registers.y - _Addr::ReadByte(*this);
	State.CFlag(temp < 0x0100);
	State.NFlag(temp);
	State.ZFlag(temp);
}

/* Битова проверка */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBIT() {
	uint16 src;

	src = _Addr::ReadByte(*this);
	State.VFlag(src & 0x40);
	State.NFlag(src);
	State.ZFlag(Registers.a & src);
}

/* Переходы */

/* Переход по !C */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBCC() {
	if (!State.Carry()) {
		Registers.pc = _Addr::GetAddr(*this);
		CycleData.Cycles++;
	}
}

/* Переход по C */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBCS() {
	if (State.Carry()) {
		Registers.pc = _Addr::GetAddr(*this);
		CycleData.Cycles++;
	}
}

/* Переход по !Z */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBNE() {
	if (!State.Zero()) {
		Registers.pc = _Addr::GetAddr(*this);
		CycleData.Cycles++;
	}
}

/* Переход по Z */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBEQ() {
	if (State.Zero()) {
		Registers.pc = _Addr::GetAddr(*this);
		CycleData.Cycles++;
	}
}

/* Переход по !N */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBPL() {
	if (!State.Negative()) {
		Registers.pc = _Addr::GetAddr(*this);
		CycleData.Cycles++;
	}
}

/* Переход по N */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBMI() {
	if (State.Negative()) {
		Registers.pc = _Addr::GetAddr(*this);
		CycleData.Cycles++;
	}
}

/* Переход по !V */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBVC() {
	if (!State.Overflow()) {
		Registers.pc = _Addr::GetAddr(*this);
		CycleData.Cycles++;
	}
}

/* Переход по V */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBVS() {
	if (State.Overflow()) {
		Registers.pc = _Addr::GetAddr(*this);
		CycleData.Cycles++;
	}
}

/* Копирование регистров */

/* A -> X */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpTAX() {
	Registers.x = Registers.a;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* X -> A */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpTXA() {
	Registers.a = Registers.x;
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* A -> Y */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpTAY() {
	Registers.y = Registers.a;
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
}

/* Y -> A */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpTYA() {
	Registers.a = Registers.y;
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* S -> X */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpTSX() {
	Registers.x = Registers.s;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* X -> S */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpTXS() {
	Registers.s = Registers.x;
}

/* Стек */

/* Положить в стек A */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpPHA() {
	PushByte(Registers.a);
}

/* Достать и з стека A */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpPLA() {
	Registers.a = PopByte();
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Положить в стек P */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpPHP() {
	PushByte(State.State | 0x10); /* Флаг B не используется */
}

/* Достать из стека P */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpPLP() {
	State.SetState(PopByte());
	// IRQ
}

/* Прыжки */

/* Безусловный переход */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpJMP() {
	Registers.pc = _Addr::GetAddr(*this);
}

/* Вызов подпрограммы */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpJSR() {
	PushWord(Registers.pc - 1);
	Registers.pc = _Addr::GetAddr(*this);
}

/* Выход из подпрограммы */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpRTS() {
	Registers.pc = PopWord() + 1;
}

/* Выход из прерывания */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpRTI() {
	State.SetState(PopByte());
	Registers.pc = PopWord();
	// IRQ
}

/* Управление флагами */

/* Установить C */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpSEC() {
	State.State |= 0x01;
}

/* Установить D */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpSED() {
	State.State |= 0x08;
}

/* Установить I */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpSEI() {
	State.State |= 0x04;
	// IRQ
}

/* Сбросить C */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpCLC() {
	State.State &= 0xfe;
}

/* Сбросить D */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpCLD() {
	State.State &= 0xf7;
}

/* Сбросить I */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpCLI() {
	State.State &= 0xfb;
	// IRQ
}

/* Сбросить V */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpCLV() {
	State.State &= 0xbf;
}

/* Другое */

/* Нет операции */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpNOP() {
}

/* Вызвать прерывание */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBRK() {
	/* BRK использует только 4 такта. Все остальное в главном цикле */
	PushWord(Registers.pc + 1);
	PushByte(State.State | 0x10);
	State.State |= 0x04;
	// IRQ
}

/* Таблица опкодов */
template <class _Bus, class _Settings>
const typename CCPU<_Bus, _Settings>::SOpcode CCPU<_Bus, _Settings>::Opcodes[256] = {
	{4, 1, &CCPU<_Bus, _Settings>::OpBRK<CCPU<_Bus, _Settings>::Implied>},      /* 0x00 */ /* BRK */
	{6, 2, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0x01 */ /* ORA */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x02 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x03 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x04 */
	{3, 2, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x05 */ /* ORA */
	{5, 2, &CCPU<_Bus, _Settings>::OpASL<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x06 */ /* ASL */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x07 */
	{3, 1, &CCPU<_Bus, _Settings>::OpPHP<CCPU<_Bus, _Settings>::Implied>},      /* 0x08 */ /* PHP */
	{2, 2, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::Immediate>},    /* 0x09 */ /* ORA */
	{2, 1, &CCPU<_Bus, _Settings>::OpASL<CCPU<_Bus, _Settings>::Accumulator>},  /* 0x0a */ /* ASL */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x0b */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x0c */
	{4, 3, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::Absolute>},     /* 0x0d */ /* ORA */
	{6, 3, &CCPU<_Bus, _Settings>::OpASL<CCPU<_Bus, _Settings>::Absolute>},     /* 0x0e */ /* ASL */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x0f */
	{2, 2, &CCPU<_Bus, _Settings>::OpBPL<CCPU<_Bus, _Settings>::Relative>},     /* 0x10 */ /* BPL */
	{5, 2, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0x11 */ /* ORA */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x12 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x13 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x14 */
	{4, 2, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x15 */ /* ORA */
	{6, 2, &CCPU<_Bus, _Settings>::OpASL<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x16 */ /* ASL */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x17 */
	{2, 1, &CCPU<_Bus, _Settings>::OpCLC<CCPU<_Bus, _Settings>::Implied>},      /* 0x18 */ /* CLC */
	{4, 3, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0x19 */ /* ORA */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x1a */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x1b */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x1c */
	{4, 3, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x1d */ /* ORA */
	{7, 3, &CCPU<_Bus, _Settings>::OpASL<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x1e */ /* ASL */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x1f */
	{6, 3, &CCPU<_Bus, _Settings>::OpJSR<CCPU<_Bus, _Settings>::Absolute>},     /* 0x20 */ /* JSR */
	{6, 2, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0x21 */ /* AND */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x22 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x23 */
	{3, 2, &CCPU<_Bus, _Settings>::OpBIT<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x24 */ /* BIT */
	{3, 2, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x25 */ /* AND */
	{5, 2, &CCPU<_Bus, _Settings>::OpROL<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x26 */ /* ROL */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x27 */
	{4, 1, &CCPU<_Bus, _Settings>::OpPLP<CCPU<_Bus, _Settings>::Implied>},      /* 0x28 */ /* PLP */
	{2, 2, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::Immediate>},    /* 0x29 */ /* AND */
	{2, 1, &CCPU<_Bus, _Settings>::OpROL<CCPU<_Bus, _Settings>::Accumulator>},  /* 0x2a */ /* ROL */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x2b */
	{4, 3, &CCPU<_Bus, _Settings>::OpBIT<CCPU<_Bus, _Settings>::Absolute>},     /* 0x2c */ /* BIT */
	{4, 3, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::Absolute>},     /* 0x2d */ /* AND */
	{6, 3, &CCPU<_Bus, _Settings>::OpROL<CCPU<_Bus, _Settings>::Absolute>},     /* 0x2e */ /* ROL */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x2f */
	{2, 2, &CCPU<_Bus, _Settings>::OpBMI<CCPU<_Bus, _Settings>::Relative>},     /* 0x30 */ /* BMI */
	{5, 2, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0x31 */ /* AND */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x32 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x33 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x34 */
	{4, 2, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x35 */ /* AND */
	{6, 2, &CCPU<_Bus, _Settings>::OpROL<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x36 */ /* ROL */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x37 */
	{2, 1, &CCPU<_Bus, _Settings>::OpSEC<CCPU<_Bus, _Settings>::Implied>},      /* 0x38 */ /* SEC */
	{4, 3, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0x39 */ /* AND */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x3a */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x3b */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x3c */
	{4, 3, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x3d */ /* AMD */
	{7, 3, &CCPU<_Bus, _Settings>::OpROL<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x3e */ /* ROL */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x3f */
	{6, 1, &CCPU<_Bus, _Settings>::OpRTI<CCPU<_Bus, _Settings>::Implied>},      /* 0x40 */ /* RTI */
	{6, 2, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0x41 */ /* EOR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x42 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x43 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x44 */
	{3, 2, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x45 */ /* EOR */
	{5, 2, &CCPU<_Bus, _Settings>::OpLSR<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x46 */ /* LSR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x47 */
	{3, 1, &CCPU<_Bus, _Settings>::OpPHA<CCPU<_Bus, _Settings>::Implied>},      /* 0x48 */ /* PHA */
	{2, 2, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::Immediate>},    /* 0x49 */ /* EOR */
	{2, 1, &CCPU<_Bus, _Settings>::OpLSR<CCPU<_Bus, _Settings>::Accumulator>},  /* 0x4a */ /* LSR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x4b */
	{3, 3, &CCPU<_Bus, _Settings>::OpJMP<CCPU<_Bus, _Settings>::Absolute>},     /* 0x4c */ /* JMP */
	{4, 3, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::Absolute>},     /* 0x4d */ /* EOR */
	{6, 3, &CCPU<_Bus, _Settings>::OpLSR<CCPU<_Bus, _Settings>::Absolute>},     /* 0x4e */ /* LSR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x4f */
	{2, 2, &CCPU<_Bus, _Settings>::OpBVC<CCPU<_Bus, _Settings>::Relative>},     /* 0x50 */ /* BVC */
	{5, 2, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0x51 */ /* EOR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x52 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x53 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x54 */
	{4, 2, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x55 */ /* EOR */
	{6, 2, &CCPU<_Bus, _Settings>::OpLSR<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x56 */ /* LSR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x57 */
	{2, 1, &CCPU<_Bus, _Settings>::OpCLI<CCPU<_Bus, _Settings>::Implied>},      /* 0x58 */ /* CLI */
	{4, 3, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0x59 */ /* EOR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x5a */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x5b */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x5c */
	{4, 3, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x5d */ /* EOR */
	{7, 3, &CCPU<_Bus, _Settings>::OpLSR<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x5e */ /* LSR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x5f */
	{6, 1, &CCPU<_Bus, _Settings>::OpRTS<CCPU<_Bus, _Settings>::Implied>},      /* 0x60 */ /* RTS */
	{6, 2, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0x61 */ /* ADC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x62 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x63 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x64 */
	{3, 2, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x65 */ /* ADC */
	{5, 2, &CCPU<_Bus, _Settings>::OpROR<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x66 */ /* ROR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x67 */
	{4, 1, &CCPU<_Bus, _Settings>::OpPLA<CCPU<_Bus, _Settings>::Implied>},      /* 0x68 */ /* PLA */
	{2, 2, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::Immediate>},    /* 0x69 */ /* ADC */
	{2, 1, &CCPU<_Bus, _Settings>::OpROR<CCPU<_Bus, _Settings>::Accumulator>},  /* 0x6a */ /* ROR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x6b */
	{5, 3, &CCPU<_Bus, _Settings>::OpJMP<CCPU<_Bus, _Settings>::AbsoluteInd>},  /* 0x6c */ /* JMP */
	{4, 3, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::Absolute>},     /* 0x6d */ /* ADC */
	{6, 3, &CCPU<_Bus, _Settings>::OpROR<CCPU<_Bus, _Settings>::Absolute>},     /* 0x6e */ /* ROR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x6f */
	{2, 2, &CCPU<_Bus, _Settings>::OpBVS<CCPU<_Bus, _Settings>::Relative>},     /* 0x70 */ /* BVS */
	{5, 2, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0x71 */ /* ADC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x72 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x73 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x74 */
	{4, 2, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x75 */ /* ADC */
	{6, 2, &CCPU<_Bus, _Settings>::OpROR<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x76 */ /* ROR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x77 */
	{2, 1, &CCPU<_Bus, _Settings>::OpSEI<CCPU<_Bus, _Settings>::Implied>},      /* 0x78 */ /* SEI */
	{4, 3, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0x79 */ /* ADC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x7a */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x7b */
	{0, 1, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x7c */
	{4, 3, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x7d */ /* ADC */
	{7, 3, &CCPU<_Bus, _Settings>::OpROR<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x7e */ /* ROR */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x7f */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x80 */
	{6, 2, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0x81 */ /* STA */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x82 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x83 */
	{3, 2, &CCPU<_Bus, _Settings>::OpSTY<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x84 */ /* STY */
	{3, 2, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x85 */ /* STA */
	{3, 2, &CCPU<_Bus, _Settings>::OpSTX<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x86 */ /* STX */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x87 */
	{2, 1, &CCPU<_Bus, _Settings>::OpDEY<CCPU<_Bus, _Settings>::Implied>},      /* 0x88 */ /* DEY */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x89 */
	{2, 1, &CCPU<_Bus, _Settings>::OpTXA<CCPU<_Bus, _Settings>::Implied>},      /* 0x8a */ /* TXA */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x8b */
	{4, 3, &CCPU<_Bus, _Settings>::OpSTY<CCPU<_Bus, _Settings>::Absolute>},     /* 0x8c */ /* STY */
	{4, 3, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::Absolute>},     /* 0x8d */ /* STA */
	{4, 3, &CCPU<_Bus, _Settings>::OpSTX<CCPU<_Bus, _Settings>::Absolute>},     /* 0x8e */ /* STX */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x8f */
	{2, 2, &CCPU<_Bus, _Settings>::OpBCC<CCPU<_Bus, _Settings>::Relative>},     /* 0x90 */ /* BCC */
	{6, 2, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0x91 */ /* STA */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x92 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x93 */
	{4, 2, &CCPU<_Bus, _Settings>::OpSTY<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x94 */ /* STY */
	{4, 2, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x95 */ /* STA */
	{4, 2, &CCPU<_Bus, _Settings>::OpSTX<CCPU<_Bus, _Settings>::ZeroPageY>},    /* 0x96 */ /* STX */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x97 */
	{2, 1, &CCPU<_Bus, _Settings>::OpTYA<CCPU<_Bus, _Settings>::Implied>},      /* 0x98 */ /* TYA */
	{5, 3, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0x99 */ /* STA */
	{2, 1, &CCPU<_Bus, _Settings>::OpTXS<CCPU<_Bus, _Settings>::Implied>},      /* 0x9a */ /* TXS */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x9b */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x9c */
	{5, 3, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x9d */ /* STA */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x9e */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x9f */
	{2, 2, &CCPU<_Bus, _Settings>::OpLDY<CCPU<_Bus, _Settings>::Immediate>},    /* 0xa0 */ /* LDY */
	{6, 2, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0xa1 */ /* LDA */
	{2, 2, &CCPU<_Bus, _Settings>::OpLDX<CCPU<_Bus, _Settings>::Immediate>},    /* 0xa2 */ /* LDX */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xa3 */
	{3, 2, &CCPU<_Bus, _Settings>::OpLDY<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xa4 */ /* LDY */
	{3, 2, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xa5 */ /* LDA */
	{3, 2, &CCPU<_Bus, _Settings>::OpLDX<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xa6 */ /* LDX */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xa7 */
	{2, 1, &CCPU<_Bus, _Settings>::OpTAY<CCPU<_Bus, _Settings>::Implied>},      /* 0xa8 */ /* TAY */
	{2, 2, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::Immediate>},    /* 0xa9 */ /* LDA */
	{2, 1, &CCPU<_Bus, _Settings>::OpTAX<CCPU<_Bus, _Settings>::Implied>},      /* 0xaa */ /* TAX */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xab */
	{4, 3, &CCPU<_Bus, _Settings>::OpLDY<CCPU<_Bus, _Settings>::Absolute>},     /* 0xac */ /* LDY */
	{4, 3, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::Absolute>},     /* 0xad */ /* LDA */
	{4, 3, &CCPU<_Bus, _Settings>::OpLDX<CCPU<_Bus, _Settings>::Absolute>},     /* 0xae */ /* LDX */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xaf */
	{2, 2, &CCPU<_Bus, _Settings>::OpBCS<CCPU<_Bus, _Settings>::Relative>},     /* 0xb0 */ /* BCS */
	{5, 2, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0xb1 */ /* LDA */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xb2 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xb3 */
	{4, 2, &CCPU<_Bus, _Settings>::OpLDY<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0xb4 */ /* LDY */
	{4, 2, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0xb5 */ /* LDA */
	{4, 2, &CCPU<_Bus, _Settings>::OpLDX<CCPU<_Bus, _Settings>::ZeroPageY>},    /* 0xb6 */ /* LDX */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xb7 */
	{2, 1, &CCPU<_Bus, _Settings>::OpCLV<CCPU<_Bus, _Settings>::Implied>},      /* 0xb8 */ /* CLV */
	{4, 3, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0xb9 */ /* LDA */
	{2, 1, &CCPU<_Bus, _Settings>::OpTSX<CCPU<_Bus, _Settings>::Implied>},      /* 0xba */ /* TSX */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xbb */
	{4, 3, &CCPU<_Bus, _Settings>::OpLDY<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0xbc */ /* LDY */
	{4, 3, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0xbd */ /* LDA */
	{4, 3, &CCPU<_Bus, _Settings>::OpLDX<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0xbe */ /* LDX */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xbf */
	{2, 2, &CCPU<_Bus, _Settings>::OpCPY<CCPU<_Bus, _Settings>::Immediate>},    /* 0xc0 */ /* CPY */
	{6, 2, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0xc1 */ /* CMP */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xc2 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xc3 */
	{3, 2, &CCPU<_Bus, _Settings>::OpCPY<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xc4 */ /* CPY */
	{3, 2, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xc5 */ /* CMP */
	{5, 2, &CCPU<_Bus, _Settings>::OpDEC<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xc6 */ /* DEC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xc7 */
	{2, 1, &CCPU<_Bus, _Settings>::OpINY<CCPU<_Bus, _Settings>::Implied>},      /* 0xc8 */ /* INY */
	{2, 2, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::Immediate>},    /* 0xc9 */ /* CMP */
	{2, 1, &CCPU<_Bus, _Settings>::OpDEX<CCPU<_Bus, _Settings>::Implied>},      /* 0xca */ /* DEX */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xcb */
	{4, 3, &CCPU<_Bus, _Settings>::OpCPY<CCPU<_Bus, _Settings>::Absolute>},     /* 0xcc */ /* CPY */
	{4, 3, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::Absolute>},     /* 0xcd */ /* CMP */
	{6, 3, &CCPU<_Bus, _Settings>::OpDEC<CCPU<_Bus, _Settings>::Absolute>},     /* 0xce */ /* DEC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xcf */
	{2, 2, &CCPU<_Bus, _Settings>::OpBNE<CCPU<_Bus, _Settings>::Relative>},     /* 0xd0 */ /* BNE */
	{5, 2, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0xd1 */ /* CMP */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xd2 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xd3 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xd4 */
	{4, 2, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0xd5 */ /* CMP */
	{6, 2, &CCPU<_Bus, _Settings>::OpDEC<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0xd6 */ /* DEC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xd7 */
	{2, 1, &CCPU<_Bus, _Settings>::OpCLD<CCPU<_Bus, _Settings>::Implied>},      /* 0xd8 */ /* CLD */
	{4, 3, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0xd9 */ /* CMP */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xda */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xdb */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xdc */
	{4, 3, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0xdd */ /* CMP */
	{7, 3, &CCPU<_Bus, _Settings>::OpDEC<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0xde */ /* DEC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xdf */
	{2, 2, &CCPU<_Bus, _Settings>::OpCPX<CCPU<_Bus, _Settings>::Immediate>},    /* 0xe0 */ /* CPX */
	{6, 2, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0xe1 */ /* SBC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xe2 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xe3 */
	{3, 2, &CCPU<_Bus, _Settings>::OpCPX<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xe4 */ /* CPX */
	{3, 2, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xe5 */ /* SBC */
	{5, 2, &CCPU<_Bus, _Settings>::OpINC<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xe6 */ /* INC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xe7 */
	{2, 1, &CCPU<_Bus, _Settings>::OpINX<CCPU<_Bus, _Settings>::Implied>},      /* 0xe8 */ /* INX */
	{2, 2, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::Immediate>},    /* 0xe9 */ /* SBC */
	{2, 1, &CCPU<_Bus, _Settings>::OpNOP<CCPU<_Bus, _Settings>::Implied>},      /* 0xea */ /* NOP */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xeb */
	{4, 3, &CCPU<_Bus, _Settings>::OpCPX<CCPU<_Bus, _Settings>::Absolute>},     /* 0xec */ /* CPX */
	{4, 3, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::Absolute>},     /* 0xed */ /* SBC */
	{6, 3, &CCPU<_Bus, _Settings>::OpINC<CCPU<_Bus, _Settings>::Absolute>},     /* 0xee */ /* INC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xef */
	{2, 2, &CCPU<_Bus, _Settings>::OpBEQ<CCPU<_Bus, _Settings>::Relative>},     /* 0xf0 */ /* BEQ */
	{5, 2, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0xf1 */ /* SBC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xf2 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xf3 */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xf4 */
	{4, 2, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0xf5 */ /* SBC */
	{6, 2, &CCPU<_Bus, _Settings>::OpINC<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0xf6 */ /* INC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xf7 */
	{2, 1, &CCPU<_Bus, _Settings>::OpSED<CCPU<_Bus, _Settings>::Implied>},      /* 0xf8 */ /* SED */
	{4, 3, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0xf9 */ /* SBC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xfa */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xfb */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xfc */
	{4, 3, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0xfd */ /* SBC */
	{7, 3, &CCPU<_Bus, _Settings>::OpINC<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0xfe */ /* INC */
	{1, 0, &CCPU<_Bus, _Settings>::OpIllegal}                                   /* 0xff */
};

/* Стандартный ЦПУ */
template <class _Settings>
struct StdCPU {
	template <class _Bus>
	class CPU: public CCPU<_Bus, _Settings> {
	public:
		inline explicit CPU(_Bus *pBus): CCPU<_Bus, _Settings>(pBus) {}
		inline ~CPU() {}
	};
};

}

#endif
