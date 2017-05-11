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

/* Microcode compilation helpers */

namespace vpnes {

namespace core {

namespace cpu {

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
    : std::conditional<(Index < FirstOperation::Size),
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
		    std::is_same<Operation, FirstOperation>::value
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

/**
 * Implementation of invokation
 */
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
	 * @param ackIRQ Acknoledge IRQ
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

}  // namespace cpu

/* CPU Microcode */

/**
 * Microcode
 */
struct CCPU::opcodes {
	// TODO: Define microcode

	/* Commands */
	struct cmdPHA : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_DB = cpu.m_A;
		}
	};
	struct cmdPHP : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_DB = cpu.packState();
		}
	};
	struct cmdPLA : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_A = cpu.m_DB;
		}
	};
	struct cmdPLP : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.unpackState(cpu.m_DB);
		}
	};
	struct cmdCLC : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_Carry = 0;
		}
	};
	struct cmdSEC : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_Carry = CPUFlagCarry;
		}
	};
	struct cmdCLD : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_Decimal = 0;
		}
	};
	struct cmdSED : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_Decimal = CPUFlagDecimal;
		}
	};
	struct cmdCLI : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_Interrupt = 0;
		}
	};
	struct cmdSEI : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_Interrupt = CPUFlagInterrupt;
		}
	};
	struct cmdCLV : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_Overflow = 0;
		}
	};
	struct cmdTAX : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_X = cpu.m_A;
			cpu.setNegativeFlag(cpu.m_X);
			cpu.setZeroFlag(cpu.m_X);
		}
	};
	struct cmdTAY : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_Y = cpu.m_A;
			cpu.setNegativeFlag(cpu.m_Y);
			cpu.setZeroFlag(cpu.m_Y);
		}
	};
	struct cmdTXA : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_A = cpu.m_X;
			cpu.setNegativeFlag(cpu.m_A);
			cpu.setZeroFlag(cpu.m_A);
		}
	};
	struct cmdTYA : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_A = cpu.m_Y;
			cpu.setNegativeFlag(cpu.m_A);
			cpu.setZeroFlag(cpu.m_A);
		}
	};
	struct cmdTXS : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_S = cpu.m_X;
		}
	};
	struct cmdTSX : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_X = cpu.m_S;
			cpu.setNegativeFlag(cpu.m_X);
			cpu.setZeroFlag(cpu.m_X);
		}
	};
	struct cmdINX : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			++cpu.m_X;
			cpu.setNegativeFlag(cpu.m_X);
			cpu.setZeroFlag(cpu.m_X);
		}
	};
	struct cmdDEX : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			--cpu.m_X;
			cpu.setNegativeFlag(cpu.m_X);
			cpu.setZeroFlag(cpu.m_X);
		}
	};
	struct cmdINY : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			++cpu.m_X;
			cpu.setNegativeFlag(cpu.m_X);
			cpu.setZeroFlag(cpu.m_X);
		}
	};
	struct cmdDEY : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			--cpu.m_X;
			cpu.setNegativeFlag(cpu.m_X);
			cpu.setZeroFlag(cpu.m_X);
		}
	};
	struct cmdBCC : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = cpu.m_Carry == 0;
		}
	};
	struct cmdBCS : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = cpu.m_Carry != 0;
		}
	};
	struct cmdBNE : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = cpu.m_Zero == 0;
		}
	};
	struct cmdBEQ : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = cpu.m_Zero != 0;
		}
	};
	struct cmdBPL : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = (cpu.m_Negative & CPUFlagNegative) == 0;
		}
	};
	struct cmdBMI : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = (cpu.m_Negative & CPUFlagNegative) != 0;
		}
	};
	struct cmdBVC : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = cpu.m_Overflow == 0;
		}
	};
	struct cmdBVS : cpu::CPUCommand {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = cpu.m_Overflow != 0;
		}
	};

	/* Cycles */

	/**
	 * Parsing next opcode
	 */
	struct ParseNext : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			if (!cpu.m_PendingINT) {
				cpu.m_PC++;
			} else {
				cpu.m_DB = 0;
			}
			cpu.m_AB = cpu.m_PC;
			Control::setEndPoint(cpu, Control::parseOpcode(cpu.m_DB));
		}
	};

	/* BRK */
	struct BRK01 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			if (!cpu.m_PendingINT) {
				cpu.m_PC++;
			}
			cpu.m_AB = 0x0100 + cpu.m_S;
			cpu.m_DB = cpu.m_PC >> 8;
		}
	};
	struct BRK02 : cpu::CPUCycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + --cpu.m_S;
			cpu.m_DB = cpu.m_PC & 0x00ff;
		}
	};
	struct BRK03 : cpu::CPUCycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + --cpu.m_S;
			cpu.m_DB = cpu.packState();
			if (!cpu.m_PendingINT) {
				cpu.m_DB |= CPUFlagBreak;
			}
		}
	};
	struct BRK04 : cpu::CPUCycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			--cpu.m_S;
			cpu.processInterrupts();
			if (cpu.m_PendingNMI) {
				cpu.m_PendingNMI = false;
				cpu.m_AB = 0xfffa;
			} else {
				cpu.m_AB = 0xfffe;
			}
		}
	};
	struct BRK05 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_DB, cpu.m_PC);
			cpu.m_Interrupt = CPUFlagInterrupt;
			cpu.m_PendingINT = false;
			++cpu.m_AB;
		}
	};
	struct BRK06 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setHigh(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * BRK
	 */
	using opBRK =
	    cpu::CPUOperation<BRK01, BRK02, BRK03, BRK04, BRK05, BRK06, ParseNext>;

	/* RTI */
	struct RTI01 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + cpu.m_S;
		}
	};
	struct RTI02 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + ++cpu.m_S;
		}
	};
	struct RTI03 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.unpackState(cpu.m_DB);
			cpu.m_AB = 0x0100 + ++cpu.m_S;
		}
	};
	struct RTI04 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = 0x0100 + ++cpu.m_S;
		}
	};
	struct RTI05 : cpu::CPUCycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setHigh(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * RTI
	 */
	using opRTI =
	    cpu::CPUOperation<RTI01, RTI02, RTI03, RTI04, RTI05, ParseNext>;

	/* RTS */
	struct RTS01 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + cpu.m_S;
		}
	};
	struct RTS02 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + ++cpu.m_S;
		}
	};
	struct RTS03 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = 0x0100 + ++cpu.m_S;
		}
	};
	struct RTS04 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setHigh(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = cpu.m_PC;
		}
	};
	struct RTS05 : cpu::CPUCycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	/**
	 * RTS
	 */
	using opRTS =
	    cpu::CPUOperation<RTS01, RTS02, RTS03, RTS04, RTS05, ParseNext>;

	/* PHA/PHP */
	template <class Command>
	struct PHR01 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + cpu.m_S;
			Command::execute(cpu);
		}
	};
	struct PHR02 : cpu::CPUCycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			--cpu.m_S;
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * PHA/PHP
	 */
	template <class Command>
	using opPHR = cpu::CPUOperation<PHR01<Command>, PHR02, ParseNext>;

	/* PLA/PLP */
	struct PLR01 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + cpu.m_S;
		}
	};
	struct PLR02 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + ++cpu.m_S;
		}
	};
	template <class Command>
	struct PLR03 : cpu::CPUCycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * PLA/PLP
	 */
	template <class Command>
	using opPLR = cpu::CPUOperation<PLR01, PLR02, PLR03<Command>, ParseNext>;

	/* JSR */
	struct JSR01 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			++cpu.m_PC;
			cpu.m_AB = 0x0100 + cpu.m_S;
		}
	};
	struct JSR02 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_DB = cpu.m_PC >> 8;
		}
	};
	struct JSR03 : cpu::CPUCycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + --cpu.m_S;
			cpu.m_DB = cpu.m_PC & 0xff;
		}
	};
	struct JSR04 : cpu::CPUCycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	struct JSR05 : cpu::CPUCycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_OP, cpu.m_PC);
			cpu.setHigh(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * JSR
	 */
	using opJSR =
	    cpu::CPUOperation<JSR01, JSR02, JSR03, JSR04, JSR05, ParseNext>;

	/* JMP Absolute */
	struct JMPAbs01 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = cpu.m_PC;
		}
	};
	struct JMPAbs02 : cpu::CPUCycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_OP, cpu.m_PC);
			cpu.setHigh(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * JMP Absolute
	 */
	using opJMPAbs = cpu::CPUOperation<JMPAbs01, JMPAbs02, ParseNext>;

	/* JMP Absolute Indirect */
	struct JMPAbsInd01 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	struct JMPAbsInd02 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			++cpu.m_PC;
			cpu.setLow(cpu.m_OP, cpu.m_Abs);
			cpu.setHigh(cpu.m_DB, cpu.m_Abs);
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct JMPAbsInd03 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_DB, cpu.m_PC);
			cpu.setLow(++cpu.m_OP, cpu.m_Abs);
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct JMPAbsInd04 : cpu::CPUCycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setHigh(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * JMP Absolute Indirect
	 */
	using opJMPAbsInd = cpu::CPUOperation<JMPAbsInd01, JMPAbsInd02, JMPAbsInd03,
	    JMPAbsInd04, ParseNext>;

	/* Branches */
	template <class Command>
	struct Branch01 : cpu::CPUCycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
			Command::execute(cpu);
		}
	};
	struct Branch02 : cpu::CPUCycle {
		static bool skipCycle(CCPU &cpu) {
			return !cpu.m_BranchTaken;
		}
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP16 = cpu.m_PC + static_cast<std::int8_t>(cpu.m_OP);
			cpu.setLow(cpu.m_OP16 & 0xff, cpu.m_PC);
			cpu.m_AB = cpu.m_PC;
		}
	};
	struct Branch03 : cpu::CPUCycle {
		enum { AckIRQ = true };
		static bool skipCycle(CCPU &cpu) {
			return !cpu.m_BranchTaken ||
			       ((cpu.m_OP16 & 0xff00) == (cpu.m_PC & 0xff00));
		}
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_PC = cpu.m_OP16;
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Branch
	 */
	template <class Command>
	using opBranch =
	    cpu::CPUOperation<Branch01<Command>, Branch02, Branch03, ParseNext>;

	/* Implied */
	template <class Command>
	struct Imp01 : cpu::CPUCycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Implied
	 */
	template <class Command>
	using opImp = cpu::CPUOperation<Imp01<Command>, ParseNext>;

	/* Immediate */
	template <class Command>
	struct Imm01 : cpu::CPUCycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	/**
	 * Immediate
	 */
	template <class Command>
	using opImm = cpu::CPUOperation<Imm01<Command>, ParseNext>;

	/* RESET */
	struct Reset00 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	struct Reset01 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	struct Reset02 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + cpu.m_S;
		}
	};
	struct Reset03 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + --cpu.m_S;
		}
	};
	struct Reset04 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + --cpu.m_S;
		}
	};
	struct Reset05 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			--cpu.m_S;
			cpu.m_AB = 0xfffc;
			cpu.m_PendingNMI = false;
		}
	};
	struct Reset06 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_DB, cpu.m_PC);
			cpu.m_Interrupt = CPUFlagInterrupt;
			cpu.m_PendingINT = false;
			cpu.m_AB = 0xfffd;
		}
	};
	struct Reset07 : cpu::CPUCycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setHigh(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = cpu.m_PC;
		}
	};

	/**
	 * Reset operation
	 */
	using opReset = cpu::CPUOperation<Reset00, Reset01, Reset02, Reset03,
	    Reset04, Reset05, Reset06, Reset07, ParseNext>;
	/**
	 * Opcodes
	 */
	using opcode_pack = class_pack<

	    // Flag manipulation

	    // CLC
	    cpu::CPUOpcode<0x18, opImp<cmdCLC>>,
	    // SEC
	    cpu::CPUOpcode<0x38, opImp<cmdSEC>>,
	    // CLI
	    cpu::CPUOpcode<0x58, opImp<cmdCLI>>,
	    // SEI
	    cpu::CPUOpcode<0x78, opImp<cmdSEI>>,
	    // CLV
	    cpu::CPUOpcode<0xb8, opImp<cmdCLV>>,
	    // CLD
	    cpu::CPUOpcode<0xd8, opImp<cmdCLD>>,
	    // SED
	    cpu::CPUOpcode<0xf8, opImp<cmdSED>>,

	    // Transfer between registers

	    // TAX
	    cpu::CPUOpcode<0xaa, opImp<cmdTAX>>,
	    // TAY
	    cpu::CPUOpcode<0xa8, opImp<cmdTAY>>,
	    // TXA
	    cpu::CPUOpcode<0x8a, opImp<cmdTXA>>,
	    // TYA
	    cpu::CPUOpcode<0x98, opImp<cmdTYA>>,
	    // TXS
	    cpu::CPUOpcode<0x9a, opImp<cmdTXS>>,
	    // TSX
	    cpu::CPUOpcode<0xba, opImp<cmdTSX>>,

	    // Load / Save

	    // LDA
	    // STA
	    // LDX
	    // STX
	    // LDY
	    // STY

	    // Arithmetic

	    // ADC
	    // SBC

	    // Increment / Decrement

	    // INX
	    cpu::CPUOpcode<0xe8, opImp<cmdINX>>,
	    // DEX
	    cpu::CPUOpcode<0xca, opImp<cmdDEX>>,
	    // INY
	    cpu::CPUOpcode<0xc8, opImp<cmdINY>>,
	    // DEY
	    cpu::CPUOpcode<0x88, opImp<cmdDEY>>,
	    // INC
	    // DEC

	    // Shifts

	    // ASL
	    // LSR
	    // ROL
	    // ROR

	    // Logic

	    // AND
	    // ORA
	    // EOR

	    // Comparison

	    // CMP
	    // CPX
	    // CPY
	    // BIT

	    // Branches

	    // BCC
	    cpu::CPUOpcode<0x90, opBranch<cmdBCC>>,
	    // BCS
	    cpu::CPUOpcode<0xb0, opBranch<cmdBCS>>,
	    // BNE
	    cpu::CPUOpcode<0xd0, opBranch<cmdBNE>>,
	    // BEQ
	    cpu::CPUOpcode<0xf0, opBranch<cmdBEQ>>,
	    // BPL
	    cpu::CPUOpcode<0x10, opBranch<cmdBPL>>,
	    // BMI
	    cpu::CPUOpcode<0x30, opBranch<cmdBMI>>,
	    // BVC
	    cpu::CPUOpcode<0x50, opBranch<cmdBVC>>,
	    // BVS
	    cpu::CPUOpcode<0x70, opBranch<cmdBVS>>,

	    // Stack

	    // PHA
	    cpu::CPUOpcode<0x48, opPHR<cmdPHA>>,
	    // PLA
	    cpu::CPUOpcode<0x68, opPLR<cmdPLA>>,
	    // PHP
	    cpu::CPUOpcode<0x08, opPHR<cmdPHP>>,
	    // PLP
	    cpu::CPUOpcode<0x28, opPLR<cmdPLP>>,

	    // Jumps

	    // RTI
	    cpu::CPUOpcode<0x40, opRTI>,
	    // RTS
	    cpu::CPUOpcode<0x60, opRTS>,
	    // JSR
	    cpu::CPUOpcode<0x20, opJSR>,
	    // JMP
	    cpu::CPUOpcode<0x4c, opJMPAbs>, cpu::CPUOpcode<0x6c, opJMPAbsInd>,

	    // Other

	    // BRK
	    cpu::CPUOpcode<0x00, opBRK>,
	    // NOP
	    cpu::CPUOpcode<0xea, opImp<cpu::CPUCommand>>,
	    cpu::CPUOpcode<0x1a, opImp<cpu::CPUCommand>>,
	    cpu::CPUOpcode<0x3a, opImp<cpu::CPUCommand>>,
	    cpu::CPUOpcode<0x5a, opImp<cpu::CPUCommand>>,
	    cpu::CPUOpcode<0x7a, opImp<cpu::CPUCommand>>,
	    cpu::CPUOpcode<0xda, opImp<cpu::CPUCommand>>,
	    cpu::CPUOpcode<0xfa, opImp<cpu::CPUCommand>>,
	    cpu::CPUOpcode<0x80, opImm<cpu::CPUCommand>>,
	    cpu::CPUOpcode<0x82, opImm<cpu::CPUCommand>>,
	    cpu::CPUOpcode<0x89, opImm<cpu::CPUCommand>>,
	    cpu::CPUOpcode<0xc2, opImm<cpu::CPUCommand>>,
	    cpu::CPUOpcode<0xe2, opImm<cpu::CPUCommand>>
	    //...

	    // Undocumented

	    // SLO
	    // RLA
	    // SRE
	    // RRA
	    // SAX
	    // LAX
	    // DCP
	    // ISC
	    // ANC
	    // ALR
	    // ARR
	    // XAA
	    // LAX
	    // SBC
	    // AHX
	    // SHY
	    // SHX
	    // TAS
	    // LAS

	    >;
	/**
	 * Control
	 */
	using control = cpu::CPUControl<opcodes>;

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
	 * @param ackIRQ Acknowledge IRQ
	 * @return If could or not
	 */
	static bool accessBus(CCPU &cpu, CCPU::EBusMode busMode, bool ackIRQ) {
		if (!cpu.isReady()) {
			return false;
		}
		if (ackIRQ) {
			cpu.processInterrupts();
		}
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
    , m_PendingNMI()
    , m_PendingINT()
    , m_AB()
    , m_DB()
    , m_PC()
    , m_S()
    , m_A()
    , m_X()
    , m_Y()
    , m_OP()
    , m_OP16()
    , m_Abs()
    , m_BranchTaken()
    , m_Negative()
    , m_Overflow()
    , m_Decimal()
    , m_Interrupt()
    , m_Zero()
    , m_Carry() {
}

/**
 * Simulation routine
 */
void CCPU::execute() {
	while (isReady()) {
		opcodes::control::execute(*this, m_CurrentIndex);
	}
}

}  // namespace core

}  // namespace vpnes
