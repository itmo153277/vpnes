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
typedef CPUGroup<4>::ID::NoBatteryID InternalDataID;

}

/* CPU */
template <class _Bus>
class CCPU: public CDevice {
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

	/* Такты */
	int Cycles;

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

#define IRQLow     0 /* Не обрабатывать IRQ */
#define IRQReady   IRQLow   + 1 /* Проверить на обработку IRQ */
#define IRQStart   IRQReady + 1 /* Начать ообработку IRQ */
#define IRQCheck   IRQStart + 1 /* Проверить для немедленной обработки IRQ */
#define IRQSave    IRQCheck + 1 /* Сохранить контекст */
#define IRQExecute IRQSave  + 1 /* Выполнить IRQ */

	/* Внутренние данные */
	struct SInternalData {
		bool Halt; /* Зависание */
		bool NMI; /* Не маскируемое прерывание */
		bool IRQ; /* Прерывание */
		int IRQTrigger;
	} InternalData;

	/* Обращения к памяти */
	inline uint8 ReadMemory(uint16 Address) {
		uint8 Res;

		Res = Bus->ReadCPUMemory(Address);
		Bus->GetClock()->Clock(3);
		return Res;
	}
	inline void WriteMemory(uint16 Address, uint8 Src) {
		Bus->WriteCPUMemory(Address, Src);
		Bus->GetClock()->Clock(3);
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

	/* Обновить состояние триггера */
	inline void UpdateTrigger() {
		if (State.Interrupt())
			InternalData.IRQTrigger = IRQLow;
		else
			InternalData.IRQTrigger = IRQReady;
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
				CPU.Cycles++;
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
				CPU.Cycles++;
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
				CPU.Cycles++;
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
			CPU.Bus->GetClock()->Clock(9);
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
				CPU.Cycles++;
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
		memset(&State, 0, sizeof(State));
		Bus->GetManager()->template SetPointer<CPUID::RegistersID>(\
			&Registers, sizeof(Registers));
		memset(&Registers, 0, sizeof(Registers));
		RAM = (uint8 *) Bus->GetManager()->template GetPointer<CPUID::RAMID>(\
			0x0800 * sizeof(uint8));
		Bus->GetManager()->template SetPointer<CPUID::InternalDataID>(\
			&InternalData, sizeof(InternalData));
	}
	inline ~CCPU() {}

	/* Обработать такты */
	inline int Execute();

	/* Сброс */
	inline void Reset() {
		Registers.s -= 3;
		State.State |= 0x04; /* Interrupt */
		memset(&InternalData, 0, sizeof(InternalData));
		InternalData.IRQ = true;
		Registers.pc = Bus->ReadCPUMemory(0xfffc) | (Bus->ReadCPUMemory(0xfffd) << 8);
	}

	/* Чтение памяти */
	inline uint8 ReadAddress(uint16 Address) {
		return RAM[Address & 0x07ff];
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
		RAM[Address & 0x07ff] = Src;
	}

	/* Генерировать NMI */
	inline void GenerateNMI() {
		InternalData.NMI = true;
		InternalData.IRQTrigger = IRQStart;
	}
	/* IRQ */
	inline bool &GetIRQPin() { return InternalData.IRQ; }
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

template <class _Bus>
int CCPU<_Bus>::Execute() {
	uint8 opcode;

	if (InternalData.Halt) /* Зависли */
		return 3;
	Cycles = Bus->GetAPU()->WasteCycles();
	if (Cycles > 0) /* Заняты APU */
		return Cycles;
	if (((InternalData.IRQTrigger == IRQReady) ||
		(InternalData.IRQTrigger == IRQCheck)) &
		InternalData.IRQ)
		InternalData.IRQTrigger++;
	switch (InternalData.IRQTrigger) {
		case IRQStart:
			InternalData.IRQTrigger = IRQSave;
			break;
		case IRQSave:
			PushWord(Registers.pc); /* Следующая команда */
			PushByte(State.State); /* Сохраняем состояние */
			InternalData.IRQTrigger = IRQExecute;
			return 12; /* 4 такта */
		case IRQExecute: /* Выполняем прерываение */
			/* Прыгаем */
			if (InternalData.NMI) {
				Registers.pc = Bus->ReadCPUMemory(0xfffa) |
					(Bus->ReadCPUMemory(0xfffb) << 8);
				InternalData.NMI = false;
			} else
				Registers.pc = Bus->ReadCPUMemory(0xfffe) |
					(Bus->ReadCPUMemory(0xffff) << 8);
			UpdateTrigger();
			return 9; /* 3 такта */
	}
	/* Текущий опкод */
	opcode = ReadMemory(Registers.pc);
	Registers.pc += Opcodes[opcode].Length;
	Cycles = Opcodes[opcode].Cycles;
	/* Выполняем */
	(this->*Opcodes[opcode].Handler)();
	return Cycles * 3;
}

/* Команды */

/* Неизвестный опкод */
template <class _Bus>
void CCPU<_Bus>::OpIllegal() {
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
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpLDA() {
	Registers.a = _Addr::ReadByte(*this);
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Загрузить в X */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpLDX() {
	Registers.x = _Addr::ReadByte(*this);
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* Загрузить в Y */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpLDY() {
	Registers.y = _Addr::ReadByte(*this);
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
}

/* Сохранить A */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpSTA() {
	_Addr::WriteByte(*this, Registers.a);
}

/* Сохранить X */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpSTX() {
	_Addr::WriteByte(*this, Registers.x);
}

/* Сохранить Y */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpSTY() {
	_Addr::WriteByte(*this, Registers.y);
}

/* Арифметические опрации */

/* Сложение */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpADC() {
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
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpSBC() {
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
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpINC() {
	uint8 src;
	uint16 Address;

	src = _Addr::ReadByte_RW(*this, Address);
	src++;
	_Addr::WriteByte_RW(*this, Address, src);
	State.NFlag(src);
	State.ZFlag(src);
}

/* Инкремент X */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpINX() {
	Registers.x++;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* Инкремент Y */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpINY() {
	Registers.y++;
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
}

/* Декремент */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpDEC() {
	uint8 src;
	uint16 Address;

	src = _Addr::ReadByte_RW(*this, Address);
	src--;
	_Addr::WriteByte_RW(*this, Address, src);
	State.NFlag(src);
	State.ZFlag(src);
}

/* Декремент X */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpDEX() {
	Registers.x--;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* Декремент Y */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpDEY() {
	Registers.y--;
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
}

/* Сдвиги */

/* Сдвиг влево */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpASL() {
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
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpLSR() {
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
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpROL() {
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
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpROR() {
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
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpAND(){
	Registers.a &= _Addr::ReadByte(*this);
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Логическое или */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpORA(){
	Registers.a |= _Addr::ReadByte(*this);
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Исключающаее или */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpEOR() {
	Registers.a ^= _Addr::ReadByte(*this);
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Сравнения */

/* Сравнение с A */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpCMP() {
	uint16 temp;

	temp = Registers.a - _Addr::ReadByte(*this);
	State.CFlag(temp < 0x0100);
	State.NFlag(temp);
	State.ZFlag(temp);
}

/* Сравнение с X */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpCPX() {
	uint16 temp;

	temp = Registers.x - _Addr::ReadByte(*this);
	State.CFlag(temp < 0x0100);
	State.NFlag(temp);
	State.ZFlag(temp);
}

/* Сравнение с Y */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpCPY() {
	uint16 temp;

	temp = Registers.y - _Addr::ReadByte(*this);
	State.CFlag(temp < 0x0100);
	State.NFlag(temp);
	State.ZFlag(temp);
}

/* Битова проверка */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpBIT() {
	uint16 src;

	src = _Addr::ReadByte(*this);
	State.VFlag(src & 0x40);
	State.NFlag(src);
	State.ZFlag(Registers.a & src);
}

/* Переходы */

/* Переход по !C */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpBCC() {
	if (!State.Carry()) {
		Registers.pc = _Addr::GetAddr(*this);
		Cycles++;
	}
}

/* Переход по C */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpBCS() {
	if (State.Carry()) {
		Registers.pc = _Addr::GetAddr(*this);
		Cycles++;
	}
}

/* Переход по !Z */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpBNE() {
	if (!State.Zero()) {
		Registers.pc = _Addr::GetAddr(*this);
		Cycles++;
	}
}

/* Переход по Z */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpBEQ() {
	if (State.Zero()) {
		Registers.pc = _Addr::GetAddr(*this);
		Cycles++;
	}
}

/* Переход по !N */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpBPL() {
	if (!State.Negative()) {
		Registers.pc = _Addr::GetAddr(*this);
		Cycles++;
	}
}

/* Переход по N */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpBMI() {
	if (State.Negative()) {
		Registers.pc = _Addr::GetAddr(*this);
		Cycles++;
	}
}

/* Переход по !V */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpBVC() {
	if (!State.Overflow()) {
		Registers.pc = _Addr::GetAddr(*this);
		Cycles++;
	}
}

/* Переход по V */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpBVS() {
	if (State.Overflow()) {
		Registers.pc = _Addr::GetAddr(*this);
		Cycles++;
	}
}

/* Копирование регистров */

/* A -> X */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpTAX() {
	Registers.x = Registers.a;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* X -> A */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpTXA() {
	Registers.a = Registers.x;
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* A -> Y */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpTAY() {
	Registers.y = Registers.a;
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
}

/* Y -> A */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpTYA() {
	Registers.a = Registers.y;
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* S -> X */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpTSX() {
	Registers.x = Registers.s;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* X -> S */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpTXS() {
	Registers.s = Registers.x;
}

/* Стек */

/* Положить в стек A */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpPHA() {
	PushByte(Registers.a);
}

/* Достать и з стека A */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpPLA() {
	Registers.a = PopByte();
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Положить в стек P */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpPHP() {
	PushByte(State.State | 0x10); /* Флаг B не используется */
}

/* Достать из стека P */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpPLP() {
	State.SetState(PopByte());
	UpdateTrigger();
}

/* Прыжки */

/* Безусловный переход */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpJMP() {
	Registers.pc = _Addr::GetAddr(*this);
}

/* Вызов подпрограммы */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpJSR() {
	PushWord(Registers.pc - 1);
	Registers.pc = _Addr::GetAddr(*this);
}

/* Выход из подпрограммы */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpRTS() {
	Registers.pc = PopWord() + 1;
}

/* Выход из прерывания */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpRTI() {
	State.SetState(PopByte());
	Registers.pc = PopWord();
	if (!State.Interrupt())
		InternalData.IRQTrigger = IRQCheck;
}

/* Управление флагами */

/* Установить C */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpSEC() {
	State.State |= 0x01;
}

/* Установить D */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpSED() {
	State.State |= 0x08;
}

/* Установить I */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpSEI() {
	State.State |= 0x04;
}

/* Сбросить C */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpCLC() {
	State.State &= 0xfe;
}

/* Сбросить D */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpCLD() {
	State.State &= 0xf7;
}

/* Сбросить I */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpCLI() {
	State.State &= 0xfb;
	UpdateTrigger();
}

/* Сбросить V */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpCLV() {
	State.State &= 0xbf;
}

/* Другое */

/* Нет операции */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpNOP() {
}

/* Вызвать прерывание */
template <class _Bus>
template <class _Addr>
void CCPU<_Bus>::OpBRK() {
	/* BRK использует только 4 такта. Все остальное в главном цикле */
	PushWord(Registers.pc + 1);
	State.State |= 0x10;
	PushByte(State.State);
	State.State |= 0x04;
	InternalData.IRQTrigger = IRQExecute;
}

/* Таблица опкодов */
template <class _Bus>
const typename CCPU<_Bus>::SOpcode CCPU<_Bus>::Opcodes[256] = {
	{4, 1, &CCPU<_Bus>::OpBRK<CCPU<_Bus>::Implied>},      /* 0x00 */ /* BRK */
	{6, 2, &CCPU<_Bus>::OpORA<CCPU<_Bus>::ZeroPageInd>},  /* 0x01 */ /* ORA */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x02 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x03 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x04 */
	{2, 2, &CCPU<_Bus>::OpORA<CCPU<_Bus>::ZeroPage>},     /* 0x05 */ /* ORA */
	{5, 2, &CCPU<_Bus>::OpASL<CCPU<_Bus>::ZeroPage>},     /* 0x06 */ /* ASL */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x07 */
	{3, 1, &CCPU<_Bus>::OpPHP<CCPU<_Bus>::Implied>},      /* 0x08 */ /* PHP */
	{2, 2, &CCPU<_Bus>::OpORA<CCPU<_Bus>::Immediate>},    /* 0x09 */ /* ORA */
	{2, 1, &CCPU<_Bus>::OpASL<CCPU<_Bus>::Accumulator>},  /* 0x0a */ /* ASL */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x0b */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x0c */
	{4, 3, &CCPU<_Bus>::OpORA<CCPU<_Bus>::Absolute>},     /* 0x0d */ /* ORA */
	{6, 3, &CCPU<_Bus>::OpASL<CCPU<_Bus>::Absolute>},     /* 0x0e */ /* ASL */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x0f */
	{2, 2, &CCPU<_Bus>::OpBPL<CCPU<_Bus>::Relative>},     /* 0x10 */ /* BPL */
	{5, 2, &CCPU<_Bus>::OpORA<CCPU<_Bus>::ZeroPageIndY>}, /* 0x11 */ /* ORA */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x12 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x13 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x14 */
	{3, 2, &CCPU<_Bus>::OpORA<CCPU<_Bus>::ZeroPageX>},    /* 0x15 */ /* ORA */
	{6, 2, &CCPU<_Bus>::OpASL<CCPU<_Bus>::ZeroPageX>},    /* 0x16 */ /* ASL */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x17 */
	{2, 1, &CCPU<_Bus>::OpCLC<CCPU<_Bus>::Implied>},      /* 0x18 */ /* CLC */
	{4, 3, &CCPU<_Bus>::OpORA<CCPU<_Bus>::AbsoluteY>},    /* 0x19 */ /* ORA */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x1a */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x1b */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x1c */
	{4, 3, &CCPU<_Bus>::OpORA<CCPU<_Bus>::AbsoluteX>},    /* 0x1d */ /* ORA */
	{7, 3, &CCPU<_Bus>::OpASL<CCPU<_Bus>::AbsoluteX>},    /* 0x1e */ /* ASL */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x1f */
	{6, 3, &CCPU<_Bus>::OpJSR<CCPU<_Bus>::Absolute>},     /* 0x20 */ /* JSR */
	{6, 2, &CCPU<_Bus>::OpAND<CCPU<_Bus>::ZeroPageInd>},  /* 0x21 */ /* AND */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x22 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x23 */
	{3, 2, &CCPU<_Bus>::OpBIT<CCPU<_Bus>::ZeroPage>},     /* 0x24 */ /* BIT */
	{2, 2, &CCPU<_Bus>::OpAND<CCPU<_Bus>::ZeroPage>},     /* 0x25 */ /* AND */
	{5, 2, &CCPU<_Bus>::OpROL<CCPU<_Bus>::ZeroPage>},     /* 0x26 */ /* ROL */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x27 */
	{4, 1, &CCPU<_Bus>::OpPLP<CCPU<_Bus>::Implied>},      /* 0x28 */ /* PLP */
	{2, 2, &CCPU<_Bus>::OpAND<CCPU<_Bus>::Immediate>},    /* 0x29 */ /* AND */
	{2, 1, &CCPU<_Bus>::OpROL<CCPU<_Bus>::Accumulator>},  /* 0x2a */ /* ROL */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x2b */
	{4, 3, &CCPU<_Bus>::OpBIT<CCPU<_Bus>::Absolute>},     /* 0x2c */ /* BIT */
	{4, 3, &CCPU<_Bus>::OpAND<CCPU<_Bus>::Absolute>},     /* 0x2d */ /* AND */
	{6, 3, &CCPU<_Bus>::OpROL<CCPU<_Bus>::Absolute>},     /* 0x2e */ /* ROL */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x2f */
	{2, 2, &CCPU<_Bus>::OpBMI<CCPU<_Bus>::Relative>},     /* 0x30 */ /* BMI */
	{5, 2, &CCPU<_Bus>::OpAND<CCPU<_Bus>::ZeroPageIndY>}, /* 0x31 */ /* AND */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x32 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x33 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x34 */
	{3, 2, &CCPU<_Bus>::OpAND<CCPU<_Bus>::ZeroPageX>},    /* 0x35 */ /* AND */
	{6, 2, &CCPU<_Bus>::OpROL<CCPU<_Bus>::ZeroPageX>},    /* 0x36 */ /* ROL */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x37 */
	{2, 1, &CCPU<_Bus>::OpSEC<CCPU<_Bus>::Implied>},      /* 0x38 */ /* SEC */
	{4, 3, &CCPU<_Bus>::OpAND<CCPU<_Bus>::AbsoluteY>},    /* 0x39 */ /* AND */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x3a */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x3b */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x3c */
	{4, 3, &CCPU<_Bus>::OpAND<CCPU<_Bus>::AbsoluteX>},    /* 0x3d */ /* AMD */
	{7, 3, &CCPU<_Bus>::OpROL<CCPU<_Bus>::AbsoluteX>},    /* 0x3e */ /* ROL */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x3f */
	{6, 1, &CCPU<_Bus>::OpRTI<CCPU<_Bus>::Implied>},      /* 0x40 */ /* RTI */
	{6, 2, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::ZeroPageInd>},  /* 0x41 */ /* EOR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x42 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x43 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x44 */
	{3, 2, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::ZeroPage>},     /* 0x45 */ /* EOR */
	{5, 2, &CCPU<_Bus>::OpLSR<CCPU<_Bus>::ZeroPage>},     /* 0x46 */ /* LSR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x47 */
	{3, 1, &CCPU<_Bus>::OpPHA<CCPU<_Bus>::Implied>},      /* 0x48 */ /* PHA */
	{2, 2, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::Immediate>},    /* 0x49 */ /* EOR */
	{2, 1, &CCPU<_Bus>::OpLSR<CCPU<_Bus>::Accumulator>},  /* 0x4a */ /* LSR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x4b */
	{3, 3, &CCPU<_Bus>::OpJMP<CCPU<_Bus>::Absolute>},     /* 0x4c */ /* JMP */
	{4, 3, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::Absolute>},     /* 0x4d */ /* EOR */
	{6, 3, &CCPU<_Bus>::OpLSR<CCPU<_Bus>::Absolute>},     /* 0x4e */ /* LSR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x4f */
	{2, 2, &CCPU<_Bus>::OpBVC<CCPU<_Bus>::Relative>},     /* 0x50 */ /* BVC */
	{5, 2, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::ZeroPageIndY>}, /* 0x51 */ /* EOR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x52 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x53 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x54 */
	{4, 2, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::ZeroPageX>},    /* 0x55 */ /* EOR */
	{6, 2, &CCPU<_Bus>::OpLSR<CCPU<_Bus>::ZeroPageX>},    /* 0x56 */ /* LSR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x57 */
	{2, 1, &CCPU<_Bus>::OpCLI<CCPU<_Bus>::Implied>},      /* 0x58 */ /* CLI */
	{4, 3, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::AbsoluteY>},    /* 0x59 */ /* EOR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x5a */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x5b */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x5c */
	{4, 3, &CCPU<_Bus>::OpEOR<CCPU<_Bus>::AbsoluteX>},    /* 0x5d */ /* EOR */
	{7, 3, &CCPU<_Bus>::OpLSR<CCPU<_Bus>::AbsoluteX>},    /* 0x5e */ /* LSR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x5f */
	{6, 1, &CCPU<_Bus>::OpRTS<CCPU<_Bus>::Implied>},      /* 0x60 */ /* RTS */
	{6, 2, &CCPU<_Bus>::OpADC<CCPU<_Bus>::ZeroPageInd>},  /* 0x61 */ /* ADC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x62 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x63 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x64 */
	{3, 2, &CCPU<_Bus>::OpADC<CCPU<_Bus>::ZeroPage>},     /* 0x65 */ /* ADC */
	{5, 2, &CCPU<_Bus>::OpROR<CCPU<_Bus>::ZeroPage>},     /* 0x66 */ /* ROR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x67 */
	{4, 1, &CCPU<_Bus>::OpPLA<CCPU<_Bus>::Implied>},      /* 0x68 */ /* PLA */
	{2, 2, &CCPU<_Bus>::OpADC<CCPU<_Bus>::Immediate>},    /* 0x69 */ /* ADC */
	{2, 1, &CCPU<_Bus>::OpROR<CCPU<_Bus>::Accumulator>},  /* 0x6a */ /* ROR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x6b */
	{5, 3, &CCPU<_Bus>::OpJMP<CCPU<_Bus>::AbsoluteInd>},  /* 0x6c */ /* JMP */
	{4, 3, &CCPU<_Bus>::OpADC<CCPU<_Bus>::Absolute>},     /* 0x6d */ /* ADC */
	{6, 3, &CCPU<_Bus>::OpROR<CCPU<_Bus>::Absolute>},     /* 0x6e */ /* ROR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x6f */
	{2, 2, &CCPU<_Bus>::OpBVS<CCPU<_Bus>::Relative>},     /* 0x70 */ /* BVS */
	{5, 2, &CCPU<_Bus>::OpADC<CCPU<_Bus>::ZeroPageIndY>}, /* 0x71 */ /* ADC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x72 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x73 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x74 */
	{4, 2, &CCPU<_Bus>::OpADC<CCPU<_Bus>::ZeroPageX>},    /* 0x75 */ /* ADC */
	{6, 2, &CCPU<_Bus>::OpROR<CCPU<_Bus>::ZeroPageX>},    /* 0x76 */ /* ROR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x77 */
	{2, 1, &CCPU<_Bus>::OpSEI<CCPU<_Bus>::Implied>},      /* 0x78 */ /* SEI */
	{4, 3, &CCPU<_Bus>::OpADC<CCPU<_Bus>::AbsoluteY>},    /* 0x79 */ /* ADC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x7a */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x7b */
	{0, 1, &CCPU<_Bus>::OpIllegal},                       /* 0x7c */
	{4, 3, &CCPU<_Bus>::OpADC<CCPU<_Bus>::AbsoluteX>},    /* 0x7d */ /* ADC */
	{7, 3, &CCPU<_Bus>::OpROR<CCPU<_Bus>::AbsoluteX>},    /* 0x7e */ /* ROR */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x7f */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x80 */
	{6, 2, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::ZeroPageInd>},  /* 0x81 */ /* STA */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x82 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x83 */
	{3, 2, &CCPU<_Bus>::OpSTY<CCPU<_Bus>::ZeroPage>},     /* 0x84 */ /* STY */
	{3, 2, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::ZeroPage>},     /* 0x85 */ /* STA */
	{3, 2, &CCPU<_Bus>::OpSTX<CCPU<_Bus>::ZeroPage>},     /* 0x86 */ /* STX */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x87 */
	{2, 1, &CCPU<_Bus>::OpDEY<CCPU<_Bus>::Implied>},      /* 0x88 */ /* DEY */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x89 */
	{2, 1, &CCPU<_Bus>::OpTXA<CCPU<_Bus>::Implied>},      /* 0x8a */ /* TXA */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x8b */
	{4, 3, &CCPU<_Bus>::OpSTY<CCPU<_Bus>::Absolute>},     /* 0x8c */ /* STY */
	{4, 3, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::Absolute>},     /* 0x8d */ /* STA */
	{4, 3, &CCPU<_Bus>::OpSTX<CCPU<_Bus>::Absolute>},     /* 0x8e */ /* STX */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x8f */
	{2, 2, &CCPU<_Bus>::OpBCC<CCPU<_Bus>::Relative>},     /* 0x90 */ /* BCC */
	{6, 2, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::ZeroPageIndY>}, /* 0x91 */ /* STA */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x92 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x93 */
	{4, 2, &CCPU<_Bus>::OpSTY<CCPU<_Bus>::ZeroPageX>},    /* 0x94 */ /* STY */
	{4, 2, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::ZeroPageX>},    /* 0x95 */ /* STA */
	{4, 2, &CCPU<_Bus>::OpSTX<CCPU<_Bus>::ZeroPageY>},    /* 0x96 */ /* STX */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x97 */
	{2, 1, &CCPU<_Bus>::OpTYA<CCPU<_Bus>::Implied>},      /* 0x98 */ /* TYA */
	{5, 3, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::AbsoluteY>},    /* 0x99 */ /* STA */
	{2, 1, &CCPU<_Bus>::OpTXS<CCPU<_Bus>::Implied>},      /* 0x9a */ /* TXS */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x9b */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x9c */
	{5, 3, &CCPU<_Bus>::OpSTA<CCPU<_Bus>::AbsoluteX>},    /* 0x9d */ /* STA */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x9e */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0x9f */
	{2, 2, &CCPU<_Bus>::OpLDY<CCPU<_Bus>::Immediate>},    /* 0xa0 */ /* LDY */
	{6, 2, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::ZeroPageInd>},  /* 0xa1 */ /* LDA */
	{2, 2, &CCPU<_Bus>::OpLDX<CCPU<_Bus>::Immediate>},    /* 0xa2 */ /* LDX */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xa3 */
	{3, 2, &CCPU<_Bus>::OpLDY<CCPU<_Bus>::ZeroPage>},     /* 0xa4 */ /* LDY */
	{3, 2, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::ZeroPage>},     /* 0xa5 */ /* LDA */
	{3, 2, &CCPU<_Bus>::OpLDX<CCPU<_Bus>::ZeroPage>},     /* 0xa6 */ /* LDX */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xa7 */
	{2, 1, &CCPU<_Bus>::OpTAY<CCPU<_Bus>::Implied>},      /* 0xa8 */ /* TAY */
	{2, 2, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::Immediate>},    /* 0xa9 */ /* LDA */
	{2, 1, &CCPU<_Bus>::OpTAX<CCPU<_Bus>::Implied>},      /* 0xaa */ /* TAX */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xab */
	{4, 3, &CCPU<_Bus>::OpLDY<CCPU<_Bus>::Absolute>},     /* 0xac */ /* LDY */
	{4, 3, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::Absolute>},     /* 0xad */ /* LDA */
	{4, 3, &CCPU<_Bus>::OpLDX<CCPU<_Bus>::Absolute>},     /* 0xae */ /* LDX */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xaf */
	{2, 2, &CCPU<_Bus>::OpBCS<CCPU<_Bus>::Relative>},     /* 0xb0 */ /* BCS */
	{5, 2, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::ZeroPageIndY>}, /* 0xb1 */ /* LDA */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xb2 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xb3 */
	{4, 2, &CCPU<_Bus>::OpLDY<CCPU<_Bus>::ZeroPageX>},    /* 0xb4 */ /* LDY */
	{4, 2, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::ZeroPageX>},    /* 0xb5 */ /* LDA */
	{4, 2, &CCPU<_Bus>::OpLDX<CCPU<_Bus>::ZeroPageY>},    /* 0xb6 */ /* LDX */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xb7 */
	{2, 1, &CCPU<_Bus>::OpCLV<CCPU<_Bus>::Implied>},      /* 0xb8 */ /* CLV */
	{4, 3, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::AbsoluteY>},    /* 0xb9 */ /* LDA */
	{2, 1, &CCPU<_Bus>::OpTSX<CCPU<_Bus>::Implied>},      /* 0xba */ /* TSX */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xbb */
	{4, 3, &CCPU<_Bus>::OpLDY<CCPU<_Bus>::AbsoluteX>},    /* 0xbc */ /* LDY */
	{4, 3, &CCPU<_Bus>::OpLDA<CCPU<_Bus>::AbsoluteX>},    /* 0xbd */ /* LDA */
	{4, 3, &CCPU<_Bus>::OpLDX<CCPU<_Bus>::AbsoluteY>},    /* 0xbe */ /* LDX */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xbf */
	{2, 2, &CCPU<_Bus>::OpCPY<CCPU<_Bus>::Immediate>},    /* 0xc0 */ /* CPY */
	{6, 2, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::ZeroPageInd>},  /* 0xc1 */ /* CMP */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xc2 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xc3 */
	{3, 2, &CCPU<_Bus>::OpCPY<CCPU<_Bus>::ZeroPage>},     /* 0xc4 */ /* CPY */
	{3, 2, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::ZeroPage>},     /* 0xc5 */ /* CMP */
	{5, 2, &CCPU<_Bus>::OpDEC<CCPU<_Bus>::ZeroPage>},     /* 0xc6 */ /* DEC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xc7 */
	{2, 1, &CCPU<_Bus>::OpINY<CCPU<_Bus>::Implied>},      /* 0xc8 */ /* INY */
	{2, 2, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::Immediate>},    /* 0xc9 */ /* CMP */
	{2, 1, &CCPU<_Bus>::OpDEX<CCPU<_Bus>::Implied>},      /* 0xca */ /* DEX */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xcb */
	{4, 3, &CCPU<_Bus>::OpCPY<CCPU<_Bus>::Absolute>},     /* 0xcc */ /* CPY */
	{4, 3, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::Absolute>},     /* 0xcd */ /* CMP */
	{6, 3, &CCPU<_Bus>::OpDEC<CCPU<_Bus>::Absolute>},     /* 0xce */ /* DEC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xcf */
	{2, 2, &CCPU<_Bus>::OpBNE<CCPU<_Bus>::Relative>},     /* 0xd0 */ /* BNE */
	{5, 2, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::ZeroPageIndY>}, /* 0xd1 */ /* CMP */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xd2 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xd3 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xd4 */
	{4, 2, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::ZeroPageX>},    /* 0xd5 */ /* CMP */
	{6, 2, &CCPU<_Bus>::OpDEC<CCPU<_Bus>::ZeroPageX>},    /* 0xd6 */ /* DEC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xd7 */
	{2, 1, &CCPU<_Bus>::OpCLD<CCPU<_Bus>::Implied>},      /* 0xd8 */ /* CLD */
	{4, 3, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::AbsoluteY>},    /* 0xd9 */ /* CMP */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xda */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xdb */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xdc */
	{4, 3, &CCPU<_Bus>::OpCMP<CCPU<_Bus>::AbsoluteX>},    /* 0xdd */ /* CMP */
	{7, 3, &CCPU<_Bus>::OpDEC<CCPU<_Bus>::AbsoluteX>},    /* 0xde */ /* DEC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xdf */
	{2, 2, &CCPU<_Bus>::OpCPX<CCPU<_Bus>::Immediate>},    /* 0xe0 */ /* CPX */
	{6, 2, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::ZeroPageInd>},  /* 0xe1 */ /* SBC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xe2 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xe3 */
	{3, 2, &CCPU<_Bus>::OpCPX<CCPU<_Bus>::ZeroPage>},     /* 0xe4 */ /* CPX */
	{3, 2, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::ZeroPage>},     /* 0xe5 */ /* SBC */
	{5, 2, &CCPU<_Bus>::OpINC<CCPU<_Bus>::ZeroPage>},     /* 0xe6 */ /* INC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xe7 */
	{2, 1, &CCPU<_Bus>::OpINX<CCPU<_Bus>::Implied>},      /* 0xe8 */ /* INX */
	{2, 2, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::Immediate>},    /* 0xe9 */ /* SBC */
	{2, 1, &CCPU<_Bus>::OpNOP<CCPU<_Bus>::Implied>},      /* 0xea */ /* NOP */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xeb */
	{4, 3, &CCPU<_Bus>::OpCPX<CCPU<_Bus>::Absolute>},     /* 0xec */ /* CPX */
	{4, 3, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::Absolute>},     /* 0xed */ /* SBC */
	{6, 3, &CCPU<_Bus>::OpINC<CCPU<_Bus>::Absolute>},     /* 0xee */ /* INC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xef */
	{2, 2, &CCPU<_Bus>::OpBEQ<CCPU<_Bus>::Relative>},     /* 0xf0 */ /* BEQ */
	{5, 2, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::ZeroPageIndY>}, /* 0xf1 */ /* SBC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xf2 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xf3 */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xf4 */
	{4, 2, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::ZeroPageX>},    /* 0xf5 */ /* SBC */
	{6, 2, &CCPU<_Bus>::OpINC<CCPU<_Bus>::ZeroPageX>},    /* 0xf6 */ /* INC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xf7 */
	{2, 1, &CCPU<_Bus>::OpSED<CCPU<_Bus>::Implied>},      /* 0xf8 */ /* SED */
	{4, 3, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::AbsoluteY>},    /* 0xf9 */ /* SBC */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xfa */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xfb */
	{1, 0, &CCPU<_Bus>::OpIllegal},                       /* 0xfc */
	{4, 3, &CCPU<_Bus>::OpSBC<CCPU<_Bus>::AbsoluteX>},    /* 0xfd */ /* SBC */
	{7, 3, &CCPU<_Bus>::OpINC<CCPU<_Bus>::AbsoluteX>},    /* 0xfe */ /* INC */
	{1, 0, &CCPU<_Bus>::OpIllegal}                        /* 0xff */
};

}

#endif
