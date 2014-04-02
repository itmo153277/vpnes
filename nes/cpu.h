/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2014  Ivanov Viktor

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
#include "frontend.h"
#include "clock.h"
#include "bus.h"

namespace vpnes {

namespace CPUID {

typedef CPUGroup<1>::ID::NoBatteryID RAMID;
typedef CPUGroup<2>::ID::NoBatteryID InternalDataID;
typedef CPUGroup<3>::ID::NoBatteryID StateID;
typedef CPUGroup<4>::ID::NoBatteryID RegistersID;
typedef CPUGroup<5>::ID::NoBatteryID CycleDataID;

}

/* CPU */
template <class _Bus, class _Settings>
class CCPU {
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

	/* Внутренние данные */
	struct SInternalData: public ManagerID<CPUID::InternalDataID> {
		enum {
			Reset = 0,
			Halt,
			NotReady,
			Run
		} State; /* Текущее состояние */
		enum {
			IRQLow = 0,
			IRQReady,
			IRQExecute,
			IRQDelay
		} IRQState; /* Состояние IRQ */
		int IRQ; /* Флаг IRQ */
		bool BranchTaken; /* Изменить логику получения опкода */
		sint8 BranchOperand; /* Параметр для перехода */
	} InternalData;

	/* Данные о тактах */
	struct SCycleData: public ManagerID<CPUID::CycleDataID> {
		int NMI; /* Такт распознавания NMI */
		int IRQ; /* Такт распознавания IRQ */
		int Cycles; /* Всего тактов */
	} CycleData;

	/* Регистр состояния */
	struct SState: public ManagerID<CPUID::StateID> {
		int Negative;
		int Overflow;
		int Decimal;
		int Interrupt;
		int Zero;
		int Carry;
		inline void SetState(uint8 State) { Negative = State & 0x80;
			Overflow = State & 0x40; Decimal = State & 0x08;
			Interrupt = State & 0x04; Zero = State & 0x02; Carry = State & 0x01; }
		inline uint8 GetState() { uint8 State = 0x20; State |= Negative & 0x80;
			State |= Overflow; State |= Decimal; State |= Interrupt;
			State |= Zero; State |= Carry; return State; }
		inline void NFlag(uint16 Src) { Negative = Src; }
		inline void VFlag(int Flag) { if (Flag) Overflow = 0x40; else Overflow = 0; }
		inline void ZFlag(uint8 Src) { if (Src) Zero = 0; else Zero = 0x02; }
		inline void CFlag(int Flag) { if (Flag) Carry = 0x01; else Carry = 0; }
	} State;

	/* Регистры */
	struct SRegisters: public ManagerID<CPUID::RegistersID> {
		uint8 a; /* Аккумулятор */
		uint8 x; /* X */
		uint8 y; /* Y */
		uint8 s; /* S */
		uint16 pc; /* PC */
	} Registers;

	/* Встроенная память */
	uint8 *RAM;

	/* Обращения к памяти */
	inline uint8 ReadMemory(uint16 Address) {
		Bus->IncrementClock(ClockDivider);
		return Bus->ReadCPU(Address);
	}
	inline void WriteMemory(uint16 Address, uint8 Src) {
		Bus->IncrementClock(ClockDivider);
		Bus->WriteCPU(Address, Src);
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

	/* Обновление состояния IRQ в соответстии с флагом */
	inline void UpdateIRQState() {
		if (State.Interrupt)
			InternalData.IRQState = SInternalData::IRQLow;
		else
			InternalData.IRQState = SInternalData::IRQReady;
	}

	/* Декодер операции */
	const SOpcode *GetNextOpcode();

	/* Обработать IRQ */
	void ProcessIRQ();

	/* Проверка IRQ */
	bool DetectIRQ();

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
		inline static uint16 GetAddrWithStack(CCPU &CPU) {
			uint16 Addr;

			Addr = CPU.ReadMemory(CPU.Registers.pc - 2);
			CPU.Bus->IncrementClock(3 * ClockDivider);
			Addr |= CPU.ReadMemory(CPU.Registers.pc - 1) << 8;
			return Addr;
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
		inline static void SetupBranch(CCPU &CPU, int Flag) {
			CPU.InternalData.BranchOperand = (sint8) CPU.ReadMemory(CPU.Registers.pc - 1);
			if (Flag)
				CPU.InternalData.BranchTaken = true;
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
				CPU.CycleData.Cycles += ClockDivider;
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
			CPU.Bus->IncrementClock(3 * ClockDivider);
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
			CPU.Bus->IncrementClock(2 * ClockDivider);
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
	CCPU(_Bus *pBus) {
		Bus = pBus;
		RAM = static_cast<uint8 *>(Bus->GetManager()->\
			template GetPointer<ManagerID<CPUID::RAMID> >(0x0800 * sizeof(uint8)));
		memset(RAM, 0xff, 0x0800 * sizeof(uint8));
		Bus->GetManager()->template SetPointer<SInternalData>(&InternalData);
		Bus->GetManager()->template SetPointer<SState>(&State);
		memset(&State, 0, sizeof(State));
		Bus->GetManager()->template SetPointer<SRegisters>(&Registers);
		memset(&Registers, 0, sizeof(Registers));
		Bus->GetManager()->template SetPointer<SCycleData>(&CycleData);
		memset(&CycleData, 0, sizeof(CycleData));
		CycleData.IRQ = -1;
		CycleData.NMI = -1;
	}
	inline ~CCPU() {}

	/* Обработать такты */
	void Execute();
	/* Сброс */
	inline void Reset() {
		memset(&InternalData, 0, sizeof(InternalData));
		InternalData.State = SInternalData::Reset;
	}
	/* Начать панику */
	inline void Panic() {
		InternalData.State = SInternalData::Halt;
		Bus->GetFrontend()->Panic();
	}

	/* Чтение памяти */
	inline uint8 ReadByte(uint16 Address) {
		return RAM[Address & 0x07ff];
	}
	/* Запись памяти */
	inline void WriteByte(uint16 Address, uint8 Src) {
		RAM[Address & 0x07ff] = Src;
	}
private:
	/* Команды CPU */

	/* Управление переходит к APU */
	void OpNotReady();
	/* Неизвестная команда */
	void OpIllegal();
	/* Сброс */
	void OpReset();
	/* Векторизация */
	void OpIRQVector();
	/* Нет операции */
	template <class _Addr>
	void OpNOP();
	/* Вызвать прерывание */
	template <class _Addr>
	void OpBRK();

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
};

/* Обработчик тактов */
template <class _Bus, class _Settings>
void CCPU<_Bus, _Settings>::Execute() {
	const SOpcode *NextOpcode;
	int ClockCounter;

	Bus->ResetInternalClock();
	ClockCounter = 0;
	while (ClockCounter < Bus->GetClock()->GetWaitClocks()) {
		NextOpcode = GetNextOpcode();
		if (NextOpcode == NULL)
			break;
		CycleData.Cycles += NextOpcode->Cycles;
		(this->*(NextOpcode->Handler))();
		ClockCounter += CycleData.Cycles;
		Bus->Synchronize(ClockCounter);
	}
}

/* Декодер операции */
template <class _Bus, class _Settings>
const typename CCPU<_Bus, _Settings>::SOpcode *CCPU<_Bus, _Settings>::GetNextOpcode() {
	uint8 opcode;
	static const SOpcode NotReadyOpcode = {
		0, 0, &CCPU<_Bus, _Settings>::OpNotReady
	};
	static const SOpcode ResetOpcode = {
		7 * ClockDivider, 0, &CCPU<_Bus, _Settings>::OpReset
	};
	static const SOpcode IRQOpcode = {
		3 * ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIRQVector
	};

	ProcessIRQ();
	if (InternalData.IRQState == SInternalData::IRQExecute)
		return &IRQOpcode;
	if (InternalData.State == SInternalData::NotReady)
		return &NotReadyOpcode;
	switch (InternalData.State) {
		case SInternalData::Reset:
			return &ResetOpcode;
		case SInternalData::Halt:
			return NULL;
		default:
			opcode = ReadMemory(Registers.pc);
			if (InternalData.BranchTaken) {
				uint16 Res;

				InternalData.BranchTaken = false;
				Res = Registers.pc + InternalData.BranchOperand;
				if ((Res & 0xff00) != (Registers.pc & 0xff00)) {
					ReadMemory((Registers.pc & 0xff00) | (Res & 0x00ff));
					CycleData.Cycles += 2 * ClockDivider;
					ProcessIRQ();
				} else
					CycleData.Cycles += ClockDivider;
				Registers.pc = Res;
				opcode = ReadMemory(Registers.pc);
			}
			if (DetectIRQ())
				return &Opcodes[0];
			else
				Registers.pc += Opcodes[opcode].Length;
			return &Opcodes[opcode];
	}
	return NULL;
}

/* Обработка IRQ */
template <class _Bus, class _Settings>
void CCPU<_Bus, _Settings>::ProcessIRQ() {
	/* Отсчитываем такты до прерываний */
	if (CycleData.IRQ > 0) {
		CycleData.IRQ -= CycleData.Cycles;
		if (CycleData.IRQ < 0)
			CycleData.IRQ = 0;
	}
	if (CycleData.NMI > 0) {
		CycleData.NMI -= CycleData.Cycles;
		if (CycleData.NMI < 0)
			CycleData.NMI = 0;
	}
	CycleData.Cycles = 0;
}

/* Проверка на IRQ */
template <class _Bus, class _Settings>
bool CCPU<_Bus, _Settings>::DetectIRQ() {
	if (InternalData.IRQState == SInternalData::IRQDelay) {
		/* Пропускаем на этой операции */
		UpdateIRQState();
		return false;
	}
	if ((CycleData.NMI == 0) || (CycleData.IRQ == 0 &&
		InternalData.IRQState == SInternalData::IRQReady)) {
		ReadMemory(Registers.pc);
		InternalData.IRQState = SInternalData::IRQExecute;
		return true;
	}
	return false;
}

/* Команды */

/* Переход управления к APU */
template <class _Bus, class _Settings>
void CCPU<_Bus, _Settings>::OpNotReady() {
	CycleData.Cycles += Bus->GetAPU()->Execute();
	InternalData.State = SInternalData::Run;
}

/* Сброс */
template <class _Bus, class _Settings>
void CCPU<_Bus, _Settings>::OpReset() {
	Registers.s -= 3; /* Игнорируем чтение из RAM */
	State.Interrupt = 0x04;
	Bus->IncrementClock(5 * ClockDivider); /* Игнорируем провереку векторизации */
	Registers.pc = ReadMemory(0xfffc) | (ReadMemory(0xfffd) << 8);
	InternalData.State = SInternalData::Run;
}

/* Векторизация */
template <class _Bus, class _Settings>
void CCPU<_Bus, _Settings>::OpIRQVector() {
	if (CycleData.IRQ < 3)
		CycleData.IRQ = -1;
	UpdateIRQState();
	if (CycleData.NMI == 0) {
		Registers.pc = ReadMemory(0xfffa) | (ReadMemory(0xfffb) << 8);
		CycleData.NMI = -1;
	} else
		Registers.pc = ReadMemory(0xfffe) | (ReadMemory(0xffff) << 8);
}

/* Неизвестный опкод */
template <class _Bus, class _Settings>
void CCPU<_Bus, _Settings>::OpIllegal() {
	/* Зависание */
	Panic();
}

/* Нет операции */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpNOP() {
}

/* Вызвать прерывание */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBRK() {
	uint8 src = State.GetState();

	if (InternalData.IRQState != SInternalData::IRQExecute) {
		ReadMemory(Registers.pc++);
		src |= 0x10;
		InternalData.IRQState = SInternalData::IRQExecute;
	}
	State.Interrupt = 0x04;
	PushWord(Registers.pc);
	PushByte(src);
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
	temp = src + Registers.a + State.Carry;
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
	temp = Registers.a - src - (State.Carry ^ 0x01);
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
	ReadMemory(Registers.pc);
	Registers.x++;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* Инкремент Y */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpINY() {
	ReadMemory(Registers.pc);
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
	ReadMemory(Registers.pc);
	Registers.x--;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* Декремент Y */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpDEY() {
	ReadMemory(Registers.pc);
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
	temp = (src << 1) | State.Carry;
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
	temp = (src >> 1) | (State.Carry << 7);
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
	_Addr::SetupBranch(*this, !State.Carry);
}

/* Переход по C */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBCS() {
	_Addr::SetupBranch(*this, State.Carry);
}

/* Переход по !Z */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBNE() {
	_Addr::SetupBranch(*this, !State.Zero);
}

/* Переход по Z */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBEQ() {
	_Addr::SetupBranch(*this, State.Zero);
}

/* Переход по !N */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBPL() {
	_Addr::SetupBranch(*this, ~State.Negative & 0x80);
}

/* Переход по N */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBMI() {
	_Addr::SetupBranch(*this, State.Negative & 0x80);
}

/* Переход по !V */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBVC() {
	_Addr::SetupBranch(*this, !State.Overflow);
}

/* Переход по V */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpBVS() {
	_Addr::SetupBranch(*this, State.Overflow);
}

/* Копирование регистров */

/* A -> X */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpTAX() {
	ReadMemory(Registers.pc);
	Registers.x = Registers.a;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* X -> A */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpTXA() {
	ReadMemory(Registers.pc);
	Registers.a = Registers.x;
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* A -> Y */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpTAY() {
	ReadMemory(Registers.pc);
	Registers.y = Registers.a;
	State.NFlag(Registers.y);
	State.ZFlag(Registers.y);
}

/* Y -> A */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpTYA() {
	ReadMemory(Registers.pc);
	Registers.a = Registers.y;
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* S -> X */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpTSX() {
	ReadMemory(Registers.pc);
	Registers.x = Registers.s;
	State.NFlag(Registers.x);
	State.ZFlag(Registers.x);
}

/* X -> S */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpTXS() {
	ReadMemory(Registers.pc);
	Registers.s = Registers.x;
}

/* Стек */

/* Положить в стек A */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpPHA() {
	ReadMemory(Registers.pc);
	PushByte(Registers.a);
}

/* Достать и з стека A */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpPLA() {
	ReadMemory(Registers.pc);
	Registers.a = PopByte();
	State.NFlag(Registers.a);
	State.ZFlag(Registers.a);
}

/* Положить в стек P */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpPHP() {
	ReadMemory(Registers.pc);
	PushByte(State.GetState() | 0x10); /* Флаг B не используется */
}

/* Достать из стека P */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpPLP() {
	int Interrupt = State.Interrupt;

	ReadMemory(Registers.pc);
	State.SetState(PopByte());
	if (InternalData.IRQ) {
		if (Interrupt && !State.Interrupt) {
			InternalData.IRQState = SInternalData::IRQDelay;
			if (CycleData.IRQ < 0)
				CycleData.IRQ = 0;
		} else if (!Interrupt && State.Interrupt) {
			if (CycleData.IRQ <= CycleData.Cycles) {
				InternalData.IRQState = SInternalData::IRQReady;
				CycleData.IRQ = 0;
			} else
				InternalData.IRQState = SInternalData::IRQLow;
		}
	} else
		UpdateIRQState();
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
	Registers.pc = _Addr::GetAddrWithStack(*this);
}

/* Выход из подпрограммы */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpRTS() {
	ReadMemory(Registers.pc);
	Registers.pc = PopWord() + 1;
}

/* Выход из прерывания */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpRTI() {
	ReadMemory(Registers.pc);
	State.SetState(PopByte());
	Registers.pc = PopWord();
	UpdateIRQState();
	if (InternalData.IRQ && (CycleData.IRQ < 0))
		CycleData.IRQ = 0;
}

/* Управление флагами */

/* Установить C */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpSEC() {
	ReadMemory(Registers.pc);
	State.Carry = 0x01;
}

/* Установить D */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpSED() {
	ReadMemory(Registers.pc);
	State.Decimal = 0x08;
}

/* Установить I */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpSEI() {
	ReadMemory(Registers.pc);
	if (!State.Interrupt) {
		if (InternalData.IRQ && (CycleData.IRQ <= CycleData.Cycles)) {
			InternalData.IRQState = SInternalData::IRQReady;
			CycleData.IRQ = 0;
		} else
			InternalData.IRQState = SInternalData::IRQLow;
		State.Interrupt = 0x04;
	}
}

/* Сбросить C */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpCLC() {
	ReadMemory(Registers.pc);
	State.Carry = 0;
}

/* Сбросить D */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpCLD() {
	ReadMemory(Registers.pc);
	State.Decimal = 0;
}

/* Сбросить I */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpCLI() {
	ReadMemory(Registers.pc);
	if (State.Interrupt) {
		if (InternalData.IRQ) {
			InternalData.IRQState = SInternalData::IRQDelay;
			if (CycleData.IRQ < 0)
				CycleData.IRQ = 0;
		} else
			InternalData.IRQState = SInternalData::IRQReady;
		State.Interrupt = 0;
	}
}

/* Сбросить V */
template <class _Bus, class _Settings>
template <class _Addr>
void CCPU<_Bus, _Settings>::OpCLV() {
	ReadMemory(Registers.pc);
	State.Overflow = 0;
}

/* Таблица опкодов */
template <class _Bus, class _Settings>
const typename CCPU<_Bus, _Settings>::SOpcode CCPU<_Bus, _Settings>::Opcodes[256] = {
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpBRK<CCPU<_Bus, _Settings>::Implied>},      /* 0x00 */ /* BRK */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0x01 */ /* ORA */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x02 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x03 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x04 */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x05 */ /* ORA */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpASL<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x06 */ /* ASL */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x07 */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpPHP<CCPU<_Bus, _Settings>::Implied>},      /* 0x08 */ /* PHP */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::Immediate>},    /* 0x09 */ /* ORA */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpASL<CCPU<_Bus, _Settings>::Accumulator>},  /* 0x0a */ /* ASL */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x0b */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x0c */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::Absolute>},     /* 0x0d */ /* ORA */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpASL<CCPU<_Bus, _Settings>::Absolute>},     /* 0x0e */ /* ASL */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x0f */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpBPL<CCPU<_Bus, _Settings>::Relative>},     /* 0x10 */ /* BPL */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0x11 */ /* ORA */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x12 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x13 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x14 */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x15 */ /* ORA */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpASL<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x16 */ /* ASL */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x17 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpCLC<CCPU<_Bus, _Settings>::Implied>},      /* 0x18 */ /* CLC */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0x19 */ /* ORA */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x1a */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x1b */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x1c */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpORA<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x1d */ /* ORA */
	{7 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpASL<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x1e */ /* ASL */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x1f */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpJSR<CCPU<_Bus, _Settings>::Absolute>},     /* 0x20 */ /* JSR */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0x21 */ /* AND */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x22 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x23 */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpBIT<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x24 */ /* BIT */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x25 */ /* AND */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpROL<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x26 */ /* ROL */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x27 */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpPLP<CCPU<_Bus, _Settings>::Implied>},      /* 0x28 */ /* PLP */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::Immediate>},    /* 0x29 */ /* AND */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpROL<CCPU<_Bus, _Settings>::Accumulator>},  /* 0x2a */ /* ROL */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x2b */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpBIT<CCPU<_Bus, _Settings>::Absolute>},     /* 0x2c */ /* BIT */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::Absolute>},     /* 0x2d */ /* AND */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpROL<CCPU<_Bus, _Settings>::Absolute>},     /* 0x2e */ /* ROL */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x2f */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpBMI<CCPU<_Bus, _Settings>::Relative>},     /* 0x30 */ /* BMI */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0x31 */ /* AND */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x32 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x33 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x34 */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x35 */ /* AND */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpROL<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x36 */ /* ROL */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x37 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpSEC<CCPU<_Bus, _Settings>::Implied>},      /* 0x38 */ /* SEC */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0x39 */ /* AND */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x3a */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x3b */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x3c */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpAND<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x3d */ /* AMD */
	{7 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpROL<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x3e */ /* ROL */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x3f */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpRTI<CCPU<_Bus, _Settings>::Implied>},      /* 0x40 */ /* RTI */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0x41 */ /* EOR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x42 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x43 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x44 */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x45 */ /* EOR */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLSR<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x46 */ /* LSR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x47 */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpPHA<CCPU<_Bus, _Settings>::Implied>},      /* 0x48 */ /* PHA */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::Immediate>},    /* 0x49 */ /* EOR */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpLSR<CCPU<_Bus, _Settings>::Accumulator>},  /* 0x4a */ /* LSR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x4b */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpJMP<CCPU<_Bus, _Settings>::Absolute>},     /* 0x4c */ /* JMP */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::Absolute>},     /* 0x4d */ /* EOR */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpLSR<CCPU<_Bus, _Settings>::Absolute>},     /* 0x4e */ /* LSR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x4f */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpBVC<CCPU<_Bus, _Settings>::Relative>},     /* 0x50 */ /* BVC */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0x51 */ /* EOR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x52 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x53 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x54 */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x55 */ /* EOR */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLSR<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x56 */ /* LSR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x57 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpCLI<CCPU<_Bus, _Settings>::Implied>},      /* 0x58 */ /* CLI */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0x59 */ /* EOR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x5a */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x5b */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x5c */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpEOR<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x5d */ /* EOR */
	{7 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpLSR<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x5e */ /* LSR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x5f */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpRTS<CCPU<_Bus, _Settings>::Implied>},      /* 0x60 */ /* RTS */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0x61 */ /* ADC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x62 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x63 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x64 */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x65 */ /* ADC */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpROR<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x66 */ /* ROR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x67 */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpPLA<CCPU<_Bus, _Settings>::Implied>},      /* 0x68 */ /* PLA */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::Immediate>},    /* 0x69 */ /* ADC */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpROR<CCPU<_Bus, _Settings>::Accumulator>},  /* 0x6a */ /* ROR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x6b */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpJMP<CCPU<_Bus, _Settings>::AbsoluteInd>},  /* 0x6c */ /* JMP */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::Absolute>},     /* 0x6d */ /* ADC */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpROR<CCPU<_Bus, _Settings>::Absolute>},     /* 0x6e */ /* ROR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x6f */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpBVS<CCPU<_Bus, _Settings>::Relative>},     /* 0x70 */ /* BVS */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0x71 */ /* ADC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x72 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x73 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x74 */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x75 */ /* ADC */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpROR<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x76 */ /* ROR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x77 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpSEI<CCPU<_Bus, _Settings>::Implied>},      /* 0x78 */ /* SEI */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0x79 */ /* ADC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x7a */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x7b */
	{0 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x7c */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpADC<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x7d */ /* ADC */
	{7 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpROR<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x7e */ /* ROR */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x7f */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x80 */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0x81 */ /* STA */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x82 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x83 */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSTY<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x84 */ /* STY */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x85 */ /* STA */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSTX<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0x86 */ /* STX */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x87 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpDEY<CCPU<_Bus, _Settings>::Implied>},      /* 0x88 */ /* DEY */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x89 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpTXA<CCPU<_Bus, _Settings>::Implied>},      /* 0x8a */ /* TXA */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x8b */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpSTY<CCPU<_Bus, _Settings>::Absolute>},     /* 0x8c */ /* STY */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::Absolute>},     /* 0x8d */ /* STA */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpSTX<CCPU<_Bus, _Settings>::Absolute>},     /* 0x8e */ /* STX */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x8f */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpBCC<CCPU<_Bus, _Settings>::Relative>},     /* 0x90 */ /* BCC */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0x91 */ /* STA */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x92 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x93 */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSTY<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x94 */ /* STY */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0x95 */ /* STA */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSTX<CCPU<_Bus, _Settings>::ZeroPageY>},    /* 0x96 */ /* STX */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x97 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpTYA<CCPU<_Bus, _Settings>::Implied>},      /* 0x98 */ /* TYA */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0x99 */ /* STA */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpTXS<CCPU<_Bus, _Settings>::Implied>},      /* 0x9a */ /* TXS */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x9b */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x9c */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpSTA<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0x9d */ /* STA */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x9e */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0x9f */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLDY<CCPU<_Bus, _Settings>::Immediate>},    /* 0xa0 */ /* LDY */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0xa1 */ /* LDA */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLDX<CCPU<_Bus, _Settings>::Immediate>},    /* 0xa2 */ /* LDX */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xa3 */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLDY<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xa4 */ /* LDY */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xa5 */ /* LDA */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLDX<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xa6 */ /* LDX */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xa7 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpTAY<CCPU<_Bus, _Settings>::Implied>},      /* 0xa8 */ /* TAY */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::Immediate>},    /* 0xa9 */ /* LDA */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpTAX<CCPU<_Bus, _Settings>::Implied>},      /* 0xaa */ /* TAX */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xab */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpLDY<CCPU<_Bus, _Settings>::Absolute>},     /* 0xac */ /* LDY */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::Absolute>},     /* 0xad */ /* LDA */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpLDX<CCPU<_Bus, _Settings>::Absolute>},     /* 0xae */ /* LDX */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xaf */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpBCS<CCPU<_Bus, _Settings>::Relative>},     /* 0xb0 */ /* BCS */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0xb1 */ /* LDA */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xb2 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xb3 */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLDY<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0xb4 */ /* LDY */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0xb5 */ /* LDA */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpLDX<CCPU<_Bus, _Settings>::ZeroPageY>},    /* 0xb6 */ /* LDX */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xb7 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpCLV<CCPU<_Bus, _Settings>::Implied>},      /* 0xb8 */ /* CLV */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0xb9 */ /* LDA */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpTSX<CCPU<_Bus, _Settings>::Implied>},      /* 0xba */ /* TSX */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xbb */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpLDY<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0xbc */ /* LDY */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpLDA<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0xbd */ /* LDA */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpLDX<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0xbe */ /* LDX */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xbf */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpCPY<CCPU<_Bus, _Settings>::Immediate>},    /* 0xc0 */ /* CPY */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0xc1 */ /* CMP */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xc2 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xc3 */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpCPY<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xc4 */ /* CPY */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xc5 */ /* CMP */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpDEC<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xc6 */ /* DEC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xc7 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpINY<CCPU<_Bus, _Settings>::Implied>},      /* 0xc8 */ /* INY */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::Immediate>},    /* 0xc9 */ /* CMP */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpDEX<CCPU<_Bus, _Settings>::Implied>},      /* 0xca */ /* DEX */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xcb */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpCPY<CCPU<_Bus, _Settings>::Absolute>},     /* 0xcc */ /* CPY */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::Absolute>},     /* 0xcd */ /* CMP */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpDEC<CCPU<_Bus, _Settings>::Absolute>},     /* 0xce */ /* DEC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xcf */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpBNE<CCPU<_Bus, _Settings>::Relative>},     /* 0xd0 */ /* BNE */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0xd1 */ /* CMP */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xd2 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xd3 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xd4 */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0xd5 */ /* CMP */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpDEC<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0xd6 */ /* DEC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xd7 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpCLD<CCPU<_Bus, _Settings>::Implied>},      /* 0xd8 */ /* CLD */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0xd9 */ /* CMP */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xda */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xdb */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xdc */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpCMP<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0xdd */ /* CMP */
	{7 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpDEC<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0xde */ /* DEC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xdf */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpCPX<CCPU<_Bus, _Settings>::Immediate>},    /* 0xe0 */ /* CPX */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::ZeroPageInd>},  /* 0xe1 */ /* SBC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xe2 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xe3 */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpCPX<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xe4 */ /* CPX */
	{3 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xe5 */ /* SBC */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpINC<CCPU<_Bus, _Settings>::ZeroPage>},     /* 0xe6 */ /* INC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xe7 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpINX<CCPU<_Bus, _Settings>::Implied>},      /* 0xe8 */ /* INX */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::Immediate>},    /* 0xe9 */ /* SBC */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpNOP<CCPU<_Bus, _Settings>::Implied>},      /* 0xea */ /* NOP */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xeb */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpCPX<CCPU<_Bus, _Settings>::Absolute>},     /* 0xec */ /* CPX */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::Absolute>},     /* 0xed */ /* SBC */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpINC<CCPU<_Bus, _Settings>::Absolute>},     /* 0xee */ /* INC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xef */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpBEQ<CCPU<_Bus, _Settings>::Relative>},     /* 0xf0 */ /* BEQ */
	{5 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::ZeroPageIndY>}, /* 0xf1 */ /* SBC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xf2 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xf3 */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xf4 */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0xf5 */ /* SBC */
	{6 * CCPU<_Bus, _Settings>::ClockDivider, 2, &CCPU<_Bus, _Settings>::OpINC<CCPU<_Bus, _Settings>::ZeroPageX>},    /* 0xf6 */ /* INC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xf7 */
	{2 * CCPU<_Bus, _Settings>::ClockDivider, 1, &CCPU<_Bus, _Settings>::OpSED<CCPU<_Bus, _Settings>::Implied>},      /* 0xf8 */ /* SED */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::AbsoluteY>},    /* 0xf9 */ /* SBC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xfa */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xfb */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal},                                  /* 0xfc */
	{4 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpSBC<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0xfd */ /* SBC */
	{7 * CCPU<_Bus, _Settings>::ClockDivider, 3, &CCPU<_Bus, _Settings>::OpINC<CCPU<_Bus, _Settings>::AbsoluteX>},    /* 0xfe */ /* INC */
	{1 * CCPU<_Bus, _Settings>::ClockDivider, 0, &CCPU<_Bus, _Settings>::OpIllegal}                                   /* 0xff */
};

}

#endif
