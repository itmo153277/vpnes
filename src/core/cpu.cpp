/**
 * @file
 * Implements basic CPU
 */
/*
 NES Emulator
 Copyright (C) 2012-2017  Ivanov Viktor

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

 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstddef>
#include <cstdint>
#include <utility>
#include <type_traits>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/device.hpp>
#include <vpnes/core/cpu.hpp>

using namespace ::std;
using namespace ::vpnes;
using namespace ::vpnes::core;

/* Microcode compilation helpers */

/**
 * Defines CPU cycle
 */
struct CPUCycle {
	enum {
		BusMode = CCPU::BusModeRead  //!< Current bus mode
	};
	enum {
		AckIRQ = false  //!< Check for IRQ
	};
	/**
	 * Checks if should skip this cycle
	 *
	 * @param cpu CPU
	 * @return True to skip
	 */
	static bool skipCycle(CCPU &cpu) {
		return false;
	}
	/**
	 * Executes the cycle
	 *
	 * @param cpu CPU
	 */
	template <class Control>
	static void execute(CCPU &cpu) {
	}
};

/**
 * Defines CPU command
 */
struct CPUCommand {
	/**
	 * Executes the command
	 *
	 * @param cpu CPU
	 */
	static void execute(CCPU &cpu) {
	}
};

/**
 * Defines CPU operation
 */
template <class... Cycles>
struct CPUOperation;

/**
 * Empty CPU operation
 */
template <>
struct CPUOperation<> {
	enum {
		Size = 0  //!< Operation size
	};
	/**
	 * Executes operation
	 *
	 * @param cpu CPU
	 */
	template <std::size_t Offset, std::size_t Index, class Control>
	static void execute(CCPU &cpu) {
	}
};

/**
 * Implementation of CPU operation
 */
template <class FirstCycle, class... OtherCycles>
struct CPUOperation<FirstCycle, OtherCycles...> {
	enum {
		Size = 1 + sizeof...(OtherCycles)  //!< Operation size
	};
	/**
	 * Executes operation
	 *
	 * @param cpu CPU
	 */
	template <std::size_t Offset, std::size_t Index, class Control>
	static void execute(CCPU &cpu) {
		if (Offset == 0) {
			if (!FirstCycle::skipCycle(cpu)) {
				if (Control::accessBus(cpu,
				        static_cast<CCPU::EBusMode>(FirstCycle::BusMode),
				        static_cast<bool>(FirstCycle::AckIRQ))) {
					FirstCycle::template execute<Control>(cpu);
				} else {
					Control::setEndPoint(cpu, Index);
				}
			}
			CPUOperation<OtherCycles...>::template execute<0, Index + 1,
			    Control>(cpu);
		} else {
			CPUOperation<OtherCycles...>::template execute<Offset - 1, Index,
			    Control>(cpu);
		}
	}
};

/**
 * Invokes operation
 */
template <std::size_t Index, std::size_t StartIndex, class Operation>
struct CPUInvoker {
	/**
	 * Actual type
	 */
	using type = CPUInvoker;
	/**
	 * Executes operation
	 *
	 * @param cpu CPU
	 */
	template <class Control>
	static void execute(CCPU &cpu) {
		Operation::template execute<Index, StartIndex, Control>(cpu);
	}
};

/**
 * Expands to invoker
 */
template <std::size_t Index, std::size_t StartIndex, class... Operations>
struct CPUInvokeExpand;

/**
 * Empty invoker
 */
template <std::size_t Index, std::size_t StartIndex>
struct CPUInvokeExpand<Index, StartIndex> {
	/**
	 * Actual type
	 */
	using type = CPUInvokeExpand;
	/**
	 * Executes operation
	 *
	 * @param cpu CPU
	 */
	template <class Control>
	static void execute(CCPU &cpu) {
	}
};

/**
 * Implementation of expanding to invoker
 */
template <std::size_t Index, std::size_t StartIndex, class FirstOperation,
    class... OtherOperations>
struct CPUInvokeExpand<Index, StartIndex, FirstOperation, OtherOperations...>
    : conditional<(Index < FirstOperation::Size),
          CPUInvoker<Index, StartIndex, FirstOperation>,
          typename CPUInvokeExpand<Index - FirstOperation::Size, StartIndex,
              OtherOperations...>::type>::type {};

/**
 * Calculates operation offset in compiled microcode
 */
template <class Operation, class... Operations>
struct CPUOperationOffset;

/**
 * Empty operation offset
 */
template <class Operation>
struct CPUOperationOffset<Operation> {
	enum {
		Offset = 0  //!< Operation offset
	};
};

/**
 * Implementation for operation offset in compiled microcode
 */
template <class Operation, class FirstOperation, class... OtherOperations>
struct CPUOperationOffset<Operation, FirstOperation, OtherOperations...> {
	enum {
		Offset =
		    is_same<Operation, FirstOperation>::value
		        ? 0
		        : FirstOperation::Size +
		              CPUOperationOffset<Operation,
		                  OtherOperations...>::Offset  //!< Operation offset
	};
};

/**
 * Expands pack to operation offset
 */
template <class Operation, class OperationPack>
struct CPUOperationOffsetExpand;

/**
 * Implementation of expansion to operation offset
 */
template <class Operation, class... Operations>
struct CPUOperationOffsetExpand<Operation, class_pack<Operations...>> {
	using type = CPUOperationOffset<Operation, Operations...>;
};

/**
 * Aggregates sizes from operation pack
 */
template <class OperationPack>
struct CPUOperationSize {
	enum {
		Size = 0  //!< Operation sizes
	};
};

/**
 * Implements aggregation of operation sizes
 */
template <class FirstOperation, class... OtherOperations>
struct CPUOperationSize<class_pack<FirstOperation, OtherOperations...>> {
	enum {
		Size = FirstOperation::Size +
		       CPUOperationSize<
		           class_pack<OtherOperations...>>::Size  //!< Operation sizes
	};
};

/**
 * Invokes operation by index
 */
template <class OperationPack, class IndexPack>
struct CPUInvoke;

template <std::size_t... Indices, class... Operations>
struct CPUInvoke<class_pack<Operations...>, index_pack<Indices...>> {
	/**
	 * Executes operation
	 *
	 * @param cpu CPU
	 * @param index Index
	 */
	template <class Control>
	static void execute(CCPU &cpu, std::size_t index) {
		typedef void (*handler)(CCPU & cpu);
		static const handler handlers[sizeof...(Indices)] = {
		    &(CPUInvokeExpand<Indices, Indices,
		        Operations...>::type::template execute<Control>) ...};
		(*handlers[index])(cpu);
	}
};

/**
 * CPU Opcode
 */
template <std::uint8_t Code, class Operation>
struct CPUOpcode {
	enum {
		code = Code  //!< Opcode code
	};
	/**
	 * Opcode operation
	 */
	using operation = Operation;
};

/**
 * Looks up opcode by code
 */
template <std::uint8_t Code, class... Opcodes>
struct CPUFindOpcode;

/**
 * Empty operation if couldn't find opcode
 */
template <std::uint8_t Code>
struct CPUFindOpcode<Code> : CPUOpcode<Code, CPUOperation<CPUCycle>> {};

/**
 * Looks up opcode using compiler class lookup
 */
template <std::uint8_t Code, class Operation, class... OtherOpcodes>
struct CPUFindOpcode<Code, CPUOpcode<Code, Operation>, OtherOpcodes...>
    : CPUOpcode<Code, Operation> {};

/**
 * Looks up code in other opcodes
 */
template <std::uint8_t Code, class FirstOpcode, class... OtherOpcodes>
struct CPUFindOpcode<Code, FirstOpcode, OtherOpcodes...>
    : CPUFindOpcode<Code, OtherOpcodes...> {};

/**
 * Parses opcode
 */
template <class OpcodePack, class OperationPack, class IndexPack>
struct CPUOpcodeParser;

/**
 * Implementation of opcode parser
 */
template <std::size_t... Codes, class... Opcodes, class OperationPack>
struct CPUOpcodeParser<class_pack<Opcodes...>, OperationPack,
    index_pack<Codes...>> {
	/**
	 * Looks up code and returns index in compiled microcode
	 *
	 * @param code Opcode
	 * @return Index in compiled microcode
	 */
	static std::size_t parseOpcode(std::uint8_t code) {
		static const std::size_t codes[sizeof...(Codes)] = {
		    CPUOperationOffsetExpand<
		        typename CPUFindOpcode<Codes, Opcodes...>::operation,
		        OperationPack>::type::Offset...};
		return codes[code];
	}
};

/**
 * Converts opcode pack to operation pack
 */
template <class OpcodePack, class... Operations>
struct CPUOperations;

/**
 * Implementation for converting opcode pack to operation pack
 */
template <class... Opcodes, class... Operations>
struct CPUOperations<class_pack<Opcodes...>, Operations...>
    : distinct_class_pack<
          class_pack<Operations..., typename Opcodes::operation...>> {};

/**
 * CPU control
 */
template <class OpcodeControl>
struct CPUControl {
	/**
	 * Opcode pack
	 */
	using opcode_pack = typename OpcodeControl::opcode_pack;
	/**
	 * Operation pack
	 */
	using operation_pack = typename CPUOperations<opcode_pack,
	    typename OpcodeControl::opReset, CPUOperation<CPUCycle>>::type;
	enum {
		ResetIndex = CPUOperationOffsetExpand<typename OpcodeControl::opReset,
		    operation_pack>::type::Offset  //!< Reset in compiled microcode
	};

	/**
	 * Sets a point since where to start next operation
	 *
	 * @param cpu CPU
	 * @param index Index
	 */
	static void setEndPoint(CCPU &cpu, std::size_t index) {
		OpcodeControl::setEndPoint(cpu, index);
	}
	/**
	 * Accesses the bus
	 *
	 * @param cpu CPU
	 * @param busMode Access mode
	 * @return If could or not
	 */
	static bool accessBus(CCPU &cpu, CCPU::EBusMode busMode, bool ackIRQ) {
		return OpcodeControl::accessBus(cpu, busMode, ackIRQ);
	}
	/**
	 * Executes microcode from index
	 *
	 * @param cpu CPU
	 * @param index Index
	 */
	static void execute(CCPU &cpu, std::size_t index) {
		CPUInvoke<operation_pack,
		    typename make_index_pack<CPUOperationSize<operation_pack>::Size>::
		        type>::template execute<CPUControl>(cpu, index);
	}
	/**
	 * Looks up code and returns index in compiled microcode
	 *
	 * @param code Opcode
	 * @return Index in compiled microcode
	 */
	static std::size_t parseOpcode(std::uint8_t code) {
		return CPUOpcodeParser<opcode_pack, operation_pack,
		    typename make_index_pack<0x100>::type>::parseOpcode(code);
	}
};

/* CPU Microcode */

/**
 * Microcode
 */
struct CCPU::opcodes {
	// TODO: Define microcode

	struct ParseNext : CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			if (!cpu.m_PendingIRQ) {
				cpu.m_PC++;
			} else {
				cpu.m_DB = 0;
			}
			cpu.m_AB = cpu.m_PC;
			Control::setEndPoint(cpu, Control::parseOpcode(cpu.m_DB));
		}
	};

	/* RESET */
	struct Reset00 : CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	struct Reset01 : CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	struct Reset02 : CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + cpu.m_S;
		}
	};
	struct Reset03 : CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + --cpu.m_S;
		}
	};
	struct Reset04 : CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + --cpu.m_S;
		}
	};
	struct Reset05 : CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			--cpu.m_S;
			cpu.m_AB = 0xfffc;
		}
	};
	struct Reset06 : CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_PC &= 0xff00;
			cpu.m_PC |= cpu.m_DB;
			cpu.m_AB = 0xfffd;
		}
	};
	struct Reset07 : CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_PC &= 0x00ff;
			cpu.m_PC |= cpu.m_DB << 8;
			cpu.m_AB = cpu.m_PC;
		}
	};

	/**
	 * Reset operation
	 */
	using opReset = CPUOperation<Reset00, Reset01, Reset02, Reset03, Reset04,
	    Reset05, Reset06, Reset07, ParseNext>;
	/**
	 * Opcodes
	 */
	using opcode_pack = class_pack<>;
	/**
	 * Control
	 */
	using control = CPUControl<opcodes>;

	/**
	 * Sets a point since where to start next operation
	 *
	 * @param cpu CPU
	 * @param index Index
	 */
	static void setEndPoint(CCPU &cpu, std::size_t index) {
		cpu.m_CurrentIndex = index;
	}
	/**
	 * Accesses the bus
	 *
	 * @param cpu CPU
	 * @param busMode Access mode
	 * @return If could or not
	 */
	static bool accessBus(CCPU &cpu, CCPU::EBusMode busMode, bool ackIRQ) {
		if (!cpu.isReady()) {
			return false;
		}
		// TODO : Acknowledge IRQ
		switch (busMode) {
		case BusModeRead:
			cpu.m_DB = cpu.m_MotherBoard->getBusCPU().readMemory(cpu.m_AB);
			break;
		case BusModeWrite:
			cpu.m_MotherBoard->getBusCPU().writeMemory(cpu.m_DB, cpu.m_AB);
			break;
		}
		// TODO : Use CPU divider
		cpu.m_InternalClock += 12;
		return true;
	}
};

/* CCPU */

/**
 * Constructs the object
 *
 * @param motherBoard Motherboard
 */
CCPU::CCPU(CMotherBoard &motherBoard)
    : CEventDevice()
    , m_MotherBoard(&motherBoard)
    , m_InternalClock()
    , m_CurrentIndex(opcodes::control::ResetIndex)
    , m_RAM{}
    , m_PendingIRQ()
    , m_AB()
    , m_DB()
    , m_PC()
    , m_S() {
}

/**
 * Simulation routine
 */
void CCPU::execute() {
	while (isReady()) {
		opcodes::control::execute(*this, m_CurrentIndex);
	}
}
