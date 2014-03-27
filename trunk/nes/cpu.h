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
	} InternalData;

	/* Данные о тактах */
	struct SCycleData: public ManagerID<CPUID::CycleDataID> {
		int NMI; /* Такт распознавания NMI */
		int IRQ; /* Такт распознавания IRQ */
		int Cycles; /* Всего тактов */
		int AddCycles;
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
		return Bus->ReadCPUMemory(Address);
	}
	inline void WriteMemory(uint16 Address, uint8 Src) {
		Bus->IncrementClock(ClockDivider);
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
				CPU.InternalClock += 2 * ClockDivider;
			else
				CPU.CycleData.AddCycles += ClockDivider;
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
			template GetPointer<ManagerID<CPUID::RAMID> >(0x800 * sizeof(uint8)));
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
	inline uint8 ReadAddress(uint16 Address) {
		return RAM[Address & 0x07ff];
	}
	/* Запись памяти */
	inline void WriteAddress(uint16 Address, uint8 Src) {
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
};

/* Обработчик тактов */
template <class _Bus, class _Settings>
void CCPU<_Bus, _Settings>::Execute() {
	const SOpcode *NextOpcode;
	int ClockCounter;

	if (InternalData.State == SInternalData::Halt)
		return;
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
		0, 0, &OpNotReady
	};
	static const SOpcode ResetOpcode = {
		7 * ClockDivider, 0, &OpReset
	};
	static const SOpcode IRQOpcode = {
		3 * ClockDivider, 0, &OpIRQVector
	};

	ProcessIRQ();
	if (InternalData.State == SInternalData::NotReady)
		return &NotReadyOpcode;
	if (InternalData.IRQState == SInternalData::IRQExecute)
		return &IRQOpcode;
	switch (InternalData.State) {
		case SInternalData::Reset:
			return &ResetOpcode;
		case SInternalData::Halt:
			return NULL;
		default:
			opcode = ReadMemory(Registers.pc);
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
	if (CycleData.AddCycles > 0) {
		Bus->IncrementClock(CycleData.AddCycles);
		CycleData.Cycles = CycleData.AddCycles;
		CycleData.AddCycles = 0;
	} else
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

	/* BRK использует только 5 тактов. Все остальное в главном цикле */
	if (InternalData.IRQState != SInternalData::IRQExecute) {
		ReadMemory(Registers.pc++);
		src |= 0x10;
		InternalData.IRQState = SInternalData::IRQExecute;
	}
	State.Interrupt = 0x04;
	PushWord(Registers.pc);
	PushByte(src);
}

}

#endif
