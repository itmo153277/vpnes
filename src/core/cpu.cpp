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

namespace vpnes {

namespace core {

namespace cpu {

/* Microcode compilation helpers */

/**
 * Defines CPU cycle
 */
struct Cycle {
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
struct Command {
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
struct Operation;

/**
 * Empty CPU operation
 */
template <>
struct Operation<> {
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
struct Operation<FirstCycle, OtherCycles...> {
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
			Operation<OtherCycles...>::template execute<0, Index + 1, Control>(
			    cpu);
		} else {
			Operation<OtherCycles...>::template execute<Offset - 1, Index,
			    Control>(cpu);
		}
	}
};

/**
 * Invokes operation
 */
template <std::size_t Index, std::size_t StartIndex, class Operation>
struct Invoker {
	/**
	 * Actual type
	 */
	using type = Invoker;
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
struct InvokeExpand;

/**
 * Empty invoker
 */
template <std::size_t Index, std::size_t StartIndex>
struct InvokeExpand<Index, StartIndex> {
	/**
	 * Actual type
	 */
	using type = InvokeExpand;
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
struct InvokeExpand<Index, StartIndex, FirstOperation, OtherOperations...>
    : std::conditional<(Index < FirstOperation::Size),
          Invoker<Index, StartIndex, FirstOperation>,
          typename InvokeExpand<Index - FirstOperation::Size, StartIndex,
              OtherOperations...>::type>::type {};

/**
 * Calculates operation offset in compiled microcode
 */
template <class Operation, class... Operations>
struct OperationOffset;

/**
 * Empty operation offset
 */
template <class Operation>
struct OperationOffset<Operation> {
	enum {
		Offset = 0  //!< Operation offset
	};
};

/**
 * Implementation for operation offset in compiled microcode
 */
template <class Operation, class FirstOperation, class... OtherOperations>
struct OperationOffset<Operation, FirstOperation, OtherOperations...> {
	enum {
		Offset =
		    std::is_same<Operation, FirstOperation>::value
		        ? 0
		        : FirstOperation::Size +
		              OperationOffset<Operation,
		                  OtherOperations...>::Offset  //!< Operation offset
	};
};

/**
 * Expands pack to operation offset
 */
template <class Operation, class OperationPack>
struct OperationOffsetExpand;

/**
 * Implementation of expansion to operation offset
 */
template <class Operation, class... Operations>
struct OperationOffsetExpand<Operation, class_pack<Operations...>> {
	using type = OperationOffset<Operation, Operations...>;
};

/**
 * Aggregates sizes from operation pack
 */
template <class OperationPack>
struct OperationSize {
	enum {
		Size = 0  //!< Operation sizes
	};
};

/**
 * Implements aggregation of operation sizes
 */
template <class FirstOperation, class... OtherOperations>
struct OperationSize<class_pack<FirstOperation, OtherOperations...>> {
	enum {
		Size =
		    FirstOperation::Size +
		    OperationSize<class_pack<OtherOperations...>>::Size  //!<
		                                                         //! Operation
		                                                         //! sizes
	};
};

/**
 * Invokes operation by index
 */
template <class OperationPack, class IndexPack>
struct Invoke;

/**
 * Implementation of invocation
 */
template <std::size_t... Indices, class... Operations>
struct Invoke<class_pack<Operations...>, index_pack<Indices...>> {
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
		    &(InvokeExpand<Indices, Indices,
		        Operations...>::type::template execute<Control>) ...};
		(*handlers[index])(cpu);
	}
};

/**
 * CPU Opcode
 */
template <std::uint8_t Code, class Operation>
struct Opcode {
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
struct FindOpcode;

/**
 * Empty operation if couldn't find opcode
 */
template <std::uint8_t Code>
struct FindOpcode<Code> : Opcode<Code, Operation<Cycle>> {};

/**
 * Looks up opcode using compiler class lookup
 */
template <std::uint8_t Code, class Operation, class... OtherOpcodes>
struct FindOpcode<Code, Opcode<Code, Operation>, OtherOpcodes...>
    : Opcode<Code, Operation> {};

/**
 * Looks up code in other opcodes
 */
template <std::uint8_t Code, class FirstOpcode, class... OtherOpcodes>
struct FindOpcode<Code, FirstOpcode, OtherOpcodes...>
    : FindOpcode<Code, OtherOpcodes...> {};

/**
 * Parses opcode
 */
template <class OpcodePack, class OperationPack, class IndexPack>
struct OpcodeParser;

/**
 * Implementation of opcode parser
 */
template <std::size_t... Codes, class... Opcodes, class OperationPack>
struct OpcodeParser<class_pack<Opcodes...>, OperationPack,
    index_pack<Codes...>> {
	/**
	 * Looks up code and returns index in compiled microcode
	 *
	 * @param code Opcode
	 * @return Index in compiled microcode
	 */
	static std::size_t parseOpcode(std::uint8_t code) {
		static const std::size_t codes[sizeof...(Codes)] = {
		    OperationOffsetExpand<
		        typename FindOpcode<Codes, Opcodes...>::operation,
		        OperationPack>::type::Offset...};
		return codes[code];
	}
};

/**
 * Converts opcode pack to operation pack
 */
template <class OpcodePack, class... Operations>
struct ExpandOperations;

/**
 * Implementation for converting opcode pack to operation pack
 */
template <class... Opcodes, class... Operations>
struct ExpandOperations<class_pack<Opcodes...>, Operations...>
    : distinct_class_pack<
          class_pack<Operations..., typename Opcodes::operation...>> {};

/**
 * CPU control
 */
template <class OpcodeControl>
struct Control {
	/**
	 * Opcode pack
	 */
	using opcode_pack = typename OpcodeControl::opcode_pack;
	/**
	 * Operation pack
	 */
	using operation_pack = typename ExpandOperations<opcode_pack,
	    typename OpcodeControl::opReset, Operation<Cycle>>::type;
	enum {
		ResetIndex = OperationOffsetExpand<typename OpcodeControl::opReset,
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
		Invoke<operation_pack,
		    typename make_index_pack<OperationSize<operation_pack>::Size>::
		        type>::template execute<Control>(cpu, index);
	}
	/**
	 * Looks up code and returns index in compiled microcode
	 *
	 * @param code Opcode
	 * @return Index in compiled microcode
	 */
	static std::size_t parseOpcode(std::uint8_t code) {
		return OpcodeParser<opcode_pack, operation_pack,
		    typename make_index_pack<0x100>::type>::parseOpcode(code);
	}
};

}  // namespace cpu

/**
 * Microcode
 */
struct CCPU::opcodes {
	// TODO: Define microcode

	/* Commands */
	struct cmdPHA : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_DB = cpu.m_A;
		}
	};
	struct cmdPHP : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_DB = cpu.packState();
		}
	};
	struct cmdPLA : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_A = cpu.m_DB;
		}
	};
	struct cmdPLP : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.unpackState(cpu.m_DB);
		}
	};
	struct cmdCLC : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_Carry = 0;
		}
	};
	struct cmdSEC : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_Carry = CPUFlagCarry;
		}
	};
	struct cmdCLD : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_Decimal = 0;
		}
	};
	struct cmdSED : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_Decimal = CPUFlagDecimal;
		}
	};
	struct cmdCLI : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_Interrupt = 0;
		}
	};
	struct cmdSEI : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_Interrupt = CPUFlagInterrupt;
		}
	};
	struct cmdCLV : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_Overflow = 0;
		}
	};
	struct cmdTAX : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_X = cpu.m_A;
			cpu.setNegativeFlag(cpu.m_X);
			cpu.setZeroFlag(cpu.m_X);
		}
	};
	struct cmdTAY : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_Y = cpu.m_A;
			cpu.setNegativeFlag(cpu.m_Y);
			cpu.setZeroFlag(cpu.m_Y);
		}
	};
	struct cmdTXA : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_A = cpu.m_X;
			cpu.setNegativeFlag(cpu.m_A);
			cpu.setZeroFlag(cpu.m_A);
		}
	};
	struct cmdTYA : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_A = cpu.m_Y;
			cpu.setNegativeFlag(cpu.m_A);
			cpu.setZeroFlag(cpu.m_A);
		}
	};
	struct cmdTXS : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_S = cpu.m_X;
		}
	};
	struct cmdTSX : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_X = cpu.m_S;
			cpu.setNegativeFlag(cpu.m_X);
			cpu.setZeroFlag(cpu.m_X);
		}
	};
	struct cmdINX : cpu::Command {
		static void execute(CCPU &cpu) {
			++cpu.m_X;
			cpu.setNegativeFlag(cpu.m_X);
			cpu.setZeroFlag(cpu.m_X);
		}
	};
	struct cmdDEX : cpu::Command {
		static void execute(CCPU &cpu) {
			--cpu.m_X;
			cpu.setNegativeFlag(cpu.m_X);
			cpu.setZeroFlag(cpu.m_X);
		}
	};
	struct cmdINY : cpu::Command {
		static void execute(CCPU &cpu) {
			++cpu.m_X;
			cpu.setNegativeFlag(cpu.m_X);
			cpu.setZeroFlag(cpu.m_X);
		}
	};
	struct cmdDEY : cpu::Command {
		static void execute(CCPU &cpu) {
			--cpu.m_X;
			cpu.setNegativeFlag(cpu.m_X);
			cpu.setZeroFlag(cpu.m_X);
		}
	};
	struct cmdBCC : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = cpu.m_Carry == 0;
		}
	};
	struct cmdBCS : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = cpu.m_Carry != 0;
		}
	};
	struct cmdBNE : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = cpu.m_Zero == 0;
		}
	};
	struct cmdBEQ : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = cpu.m_Zero != 0;
		}
	};
	struct cmdBPL : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = (cpu.m_Negative & CPUFlagNegative) == 0;
		}
	};
	struct cmdBMI : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = (cpu.m_Negative & CPUFlagNegative) != 0;
		}
	};
	struct cmdBVC : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = cpu.m_Overflow == 0;
		}
	};
	struct cmdBVS : cpu::Command {
		static void execute(CCPU &cpu) {
			cpu.m_BranchTaken = cpu.m_Overflow != 0;
		}
	};

	/* Cycles */

	/**
	 * Parsing next opcode
	 */
	struct ParseNext : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			if (!cpu.m_PendingINT) {
				++cpu.m_PC;
			} else {
				cpu.m_DB = 0;
			}
			cpu.m_AB = cpu.m_PC;
			Control::setEndPoint(cpu, Control::parseOpcode(cpu.m_DB));
		}
	};

	/* BRK */
	struct BRK01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			if (!cpu.m_PendingINT) {
				cpu.m_PC++;
			}
			cpu.m_AB = 0x0100 + cpu.m_S;
			cpu.m_DB = cpu.m_PC >> 8;
		}
	};
	struct BRK02 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + --cpu.m_S;
			cpu.m_DB = cpu.m_PC & 0x00ff;
		}
	};
	struct BRK03 : cpu::Cycle {
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
	struct BRK04 : cpu::Cycle {
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
	struct BRK05 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_DB, cpu.m_PC);
			cpu.m_Interrupt = CPUFlagInterrupt;
			cpu.m_PendingINT = false;
			++cpu.m_AB;
		}
	};
	struct BRK06 : cpu::Cycle {
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
	    cpu::Operation<BRK01, BRK02, BRK03, BRK04, BRK05, BRK06, ParseNext>;

	/* RTI */
	struct RTI01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + cpu.m_S;
		}
	};
	struct RTI02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + ++cpu.m_S;
		}
	};
	struct RTI03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.unpackState(cpu.m_DB);
			cpu.m_AB = 0x0100 + ++cpu.m_S;
		}
	};
	struct RTI04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = 0x0100 + ++cpu.m_S;
		}
	};
	struct RTI05 : cpu::Cycle {
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
	using opRTI = cpu::Operation<RTI01, RTI02, RTI03, RTI04, RTI05, ParseNext>;

	/* RTS */
	struct RTS01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + cpu.m_S;
		}
	};
	struct RTS02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + ++cpu.m_S;
		}
	};
	struct RTS03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = 0x0100 + ++cpu.m_S;
		}
	};
	struct RTS04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setHigh(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = cpu.m_PC;
		}
	};
	struct RTS05 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	/**
	 * RTS
	 */
	using opRTS = cpu::Operation<RTS01, RTS02, RTS03, RTS04, RTS05, ParseNext>;

	/* PHA/PHP */
	template <class Command>
	struct PHR01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + cpu.m_S;
			Command::execute(cpu);
		}
	};
	struct PHR02 : cpu::Cycle {
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
	using opPHR = cpu::Operation<PHR01<Command>, PHR02, ParseNext>;

	/* PLA/PLP */
	struct PLR01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + cpu.m_S;
		}
	};
	struct PLR02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + ++cpu.m_S;
		}
	};
	template <class Command>
	struct PLR03 : cpu::Cycle {
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
	using opPLR = cpu::Operation<PLR01, PLR02, PLR03<Command>, ParseNext>;

	/* JSR */
	struct JSR01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			++cpu.m_PC;
			cpu.m_AB = 0x0100 + cpu.m_S;
		}
	};
	struct JSR02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_DB = cpu.m_PC >> 8;
		}
	};
	struct JSR03 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + --cpu.m_S;
			cpu.m_DB = cpu.m_PC & 0xff;
		}
	};
	struct JSR04 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	struct JSR05 : cpu::Cycle {
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
	using opJSR = cpu::Operation<JSR01, JSR02, JSR03, JSR04, JSR05, ParseNext>;

	/* JMP Absolute */
	struct JMPAbs01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	struct JMPAbs02 : cpu::Cycle {
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
	using opJMPAbs = cpu::Operation<JMPAbs01, JMPAbs02, ParseNext>;

	/* JMP Absolute Indirect */
	struct JMPAbsInd01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	struct JMPAbsInd02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			++cpu.m_PC;
			cpu.setLow(cpu.m_OP, cpu.m_Abs);
			cpu.setHigh(cpu.m_DB, cpu.m_Abs);
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct JMPAbsInd03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_DB, cpu.m_PC);
			cpu.setLow(++cpu.m_OP, cpu.m_Abs);
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct JMPAbsInd04 : cpu::Cycle {
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
	using opJMPAbsInd = cpu::Operation<JMPAbsInd01, JMPAbsInd02, JMPAbsInd03,
	    JMPAbsInd04, ParseNext>;

	/* Branches */
	template <class Command>
	struct Branch01 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
			Command::execute(cpu);
		}
	};
	struct Branch02 : cpu::Cycle {
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
	struct Branch03 : cpu::Cycle {
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
	    cpu::Operation<Branch01<Command>, Branch02, Branch03, ParseNext>;

	/* Implied */
	template <class Command>
	struct Imp01 : cpu::Cycle {
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
	using opImp = cpu::Operation<Imp01<Command>, ParseNext>;

	/* Immediate */
	template <class Command>
	struct Imm01 : cpu::Cycle {
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
	using opImm = cpu::Operation<Imm01<Command>, ParseNext>;

	/* Absolute */
	/* Read */
	struct ReadAbs01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	struct ReadAbs02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_OP, cpu.m_Abs);
			cpu.setHigh(cpu.m_DB, cpu.m_Abs);
			++cpu.m_PC;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	template <class Command>
	struct ReadAbs03 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Read Absolute
	 */
	template <class Command>
	using opReadAbs =
	    cpu::Operation<ReadAbs01, ReadAbs02, ReadAbs03<Command>, ParseNext>;
	/* Modify */
	struct ModifyAbs01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	struct ModifyAbs02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_OP, cpu.m_Abs);
			cpu.setHigh(cpu.m_DB, cpu.m_Abs);
			++cpu.m_PC;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct ModifyAbs03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	template <class Command>
	struct ModifyAbs04 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct ModifyAbs05 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Modify Absolute
	 */
	template <class Command>
	using opModifyAbs = cpu::Operation<ModifyAbs01, ModifyAbs02, ModifyAbs03,
	    ModifyAbs04<Command>, ModifyAbs05, ParseNext>;
	/* Write */
	struct WriteAbs01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	template <class Command>
	struct WriteAbs02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_OP, cpu.m_Abs);
			cpu.setHigh(cpu.m_DB, cpu.m_Abs);
			++cpu.m_PC;
			Command::execute(cpu);
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct WriteAbs03 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Write Absolute
	 */
	template <class Command>
	using opWriteAbs =
	    cpu::Operation<WriteAbs01, WriteAbs02<Command>, WriteAbs03, ParseNext>;

	/* Zero-page */
	/* Read */
	struct ReadZP01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP = cpu.m_DB;
			++cpu.m_PC;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	template <class Command>
	struct ReadZP02 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Read Zero-page
	 */
	template <class Command>
	using opReadZP = cpu::Operation<ReadZP01, ReadZP02<Command>, ParseNext>;
	/* Modify */
	struct ModifyZP01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP = cpu.m_DB;
			++cpu.m_PC;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct ModifyZP02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	template <class Command>
	struct ModifyZP03 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct ModifyZP04 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Modify Zero-Page
	 */
	template <class Command>
	using opModifyZP = cpu::Operation<ModifyZP01, ModifyZP02,
	    ModifyZP03<Command>, ModifyZP04, ParseNext>;
	/* Write */
	template <class Command>
	struct WriteZP01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP = cpu.m_DB;
			++cpu.m_PC;
			Command::execute(cpu);
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct WriteZP02 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Write Zero-Page
	 */
	template <class Command>
	using opWriteZP = cpu::Operation<WriteZP01<Command>, WriteZP02, ParseNext>;

	/* Zero-page X */
	/* Read */
	struct ReadZPX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP = cpu.m_DB;
			++cpu.m_PC;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct ReadZPX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP += cpu.m_X;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	template <class Command>
	struct ReadZPX03 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Read Zero-page X
	 */
	template <class Command>
	using opReadZPX =
	    cpu::Operation<ReadZPX01, ReadZPX02, ReadZPX03<Command>, ParseNext>;
	/* Modify */
	struct ModifyZPX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP = cpu.m_DB;
			++cpu.m_PC;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct ModifyZPX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP += cpu.m_X;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct ModifyZPX03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	template <class Command>
	struct ModifyZPX04 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct ModifyZPX05 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Modify Zero-Page X
	 */
	template <class Command>
	using opModifyZPX = cpu::Operation<ModifyZPX01, ModifyZPX02, ModifyZPX03,
	    ModifyZPX04<Command>, ModifyZPX05, ParseNext>;
	/* Write */
	struct WriteZPX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP = cpu.m_DB;
			++cpu.m_PC;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	template <class Command>
	struct WriteZPX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP += cpu.m_X;
			Command::execute(cpu);
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct WriteZPX03 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Write Zero-Page X
	 */
	template <class Command>
	using opWriteZPX =
	    cpu::Operation<WriteZPX01, WriteZPX02<Command>, WriteZPX03, ParseNext>;

	/* Zero-page Y */
	/* Read */
	struct ReadZPY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP = cpu.m_DB;
			++cpu.m_PC;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct ReadZPY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP += cpu.m_Y;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	template <class Command>
	struct ReadZPY03 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Read Zero-page Y
	 */
	template <class Command>
	using opReadZPY =
	    cpu::Operation<ReadZPY01, ReadZPY02, ReadZPY03<Command>, ParseNext>;
	/* Modify */
	struct ModifyZPY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP = cpu.m_DB;
			++cpu.m_PC;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct ModifyZPY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP += cpu.m_Y;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct ModifyZPY03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	template <class Command>
	struct ModifyZPY04 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct ModifyZPY05 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Modify Zero-Page Y
	 */
	template <class Command>
	using opModifyZPY = cpu::Operation<ModifyZPY01, ModifyZPY02, ModifyZPY03,
	    ModifyZPY04<Command>, ModifyZPY05, ParseNext>;
	/* Write */
	struct WriteZPY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP = cpu.m_DB;
			++cpu.m_PC;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	template <class Command>
	struct WriteZPY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_ZP += cpu.m_Y;
			Command::execute(cpu);
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct WriteZPY03 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Write Zero-Page Y
	 */
	template <class Command>
	using opWriteZPY =
	    cpu::Operation<WriteZPY01, WriteZPY02<Command>, WriteZPY03, ParseNext>;

	/* Absolute X */
	/* Read */
	struct ReadAbsX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	struct ReadAbsX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_OP, cpu.m_OP16);
			cpu.setHigh(cpu.m_DB, cpu.m_OP16);
			cpu.m_OP16 += cpu.m_X;
			cpu.setLow(cpu.m_OP16 >> 8, cpu.m_Abs);
			cpu.setHigh(cpu.m_DB, cpu.m_Abs);
			++cpu.m_PC;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct ReadAbsX03 : cpu::Cycle {
		static bool skipCycle(CCPU &cpu) {
			return ((cpu.m_OP16 & 0xff00) == (cpu.m_Abs & 0xff00));
		}
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_Abs = cpu.m_OP16;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	template <class Command>
	struct ReadAbsX04 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Read Absolute X
	 */
	template <class Command>
	using opReadAbsX = cpu::Operation<ReadAbsX01, ReadAbsX02, ReadAbsX03,
	    ReadAbsX04<Command>, ParseNext>;
	/* Modify */
	struct ModifyAbsX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	struct ModifyAbsX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_OP, cpu.m_OP16);
			cpu.setHigh(cpu.m_DB, cpu.m_OP16);
			cpu.m_OP16 += cpu.m_X;
			cpu.setLow(cpu.m_OP16 >> 8, cpu.m_Abs);
			cpu.setHigh(cpu.m_DB, cpu.m_Abs);
			++cpu.m_PC;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct ModifyAbsX03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_Abs = cpu.m_OP16;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct ModifyAbsX04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	template <class Command>
	struct ModifyAbsX05 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct ModifyAbsX06 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Modify Absolute X
	 */
	template <class Command>
	using opModifyAbsX =
	    cpu::Operation<ModifyAbsX01, ModifyAbsX02, ModifyAbsX03, ModifyAbsX04,
	        ModifyAbsX05<Command>, ModifyAbsX06, ParseNext>;
	/* Write */
	struct WriteAbsX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	struct WriteAbsX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_OP, cpu.m_OP16);
			cpu.setHigh(cpu.m_DB, cpu.m_OP16);
			cpu.m_OP16 += cpu.m_X;
			cpu.setLow(cpu.m_OP16 >> 8, cpu.m_Abs);
			cpu.setHigh(cpu.m_DB, cpu.m_Abs);
			++cpu.m_PC;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	template <class Command>
	struct WriteAbsX03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_Abs = cpu.m_OP16;
			cpu.m_AB = cpu.m_Abs;
			Command::execute(cpu);
		}
	};
	struct WriteAbsX04 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Write Absolute X
	 */
	template <class Command>
	using opWriteAbsX = cpu::Operation<WriteAbsX01, WriteAbsX02,
	    WriteAbsX03<Command>, WriteAbsX04, ParseNext>;

	/* Absolute Y */
	/* Read */
	struct ReadAbsY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	struct ReadAbsY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_OP, cpu.m_OP16);
			cpu.setHigh(cpu.m_DB, cpu.m_OP16);
			cpu.m_OP16 += cpu.m_Y;
			cpu.setLow(cpu.m_OP16 >> 8, cpu.m_Abs);
			cpu.setHigh(cpu.m_DB, cpu.m_Abs);
			++cpu.m_PC;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct ReadAbsY03 : cpu::Cycle {
		static bool skipCycle(CCPU &cpu) {
			return ((cpu.m_OP16 & 0xff00) == (cpu.m_Abs & 0xff00));
		}
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_Abs = cpu.m_OP16;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	template <class Command>
	struct ReadAbsY04 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Read Absolute Y
	 */
	template <class Command>
	using opReadAbsY = cpu::Operation<ReadAbsY01, ReadAbsY02, ReadAbsY03,
	    ReadAbsY04<Command>, ParseNext>;
	/* Modify */
	struct ModifyAbsY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	struct ModifyAbsY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_OP, cpu.m_OP16);
			cpu.setHigh(cpu.m_DB, cpu.m_OP16);
			cpu.m_OP16 += cpu.m_Y;
			cpu.setLow(cpu.m_OP16 >> 8, cpu.m_Abs);
			cpu.setHigh(cpu.m_DB, cpu.m_Abs);
			++cpu.m_PC;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct ModifyAbsY03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_Abs = cpu.m_OP16;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	struct ModifyAbsY04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = cpu.m_ZP;
		}
	};
	template <class Command>
	struct ModifyAbsY05 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			Command::execute(cpu);
			cpu.m_AB = cpu.m_ZP;
		}
	};
	struct ModifyAbsY06 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Modify Absolute Y
	 */
	template <class Command>
	using opModifyAbsY =
	    cpu::Operation<ModifyAbsY01, ModifyAbsY02, ModifyAbsY03, ModifyAbsY04,
	        ModifyAbsY05<Command>, ModifyAbsY06, ParseNext>;
	/* Write */
	struct WriteAbsY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_OP = cpu.m_DB;
			cpu.m_AB = ++cpu.m_PC;
		}
	};
	struct WriteAbsY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_OP, cpu.m_OP16);
			cpu.setHigh(cpu.m_DB, cpu.m_OP16);
			cpu.m_OP16 += cpu.m_Y;
			cpu.setLow(cpu.m_OP16 >> 8, cpu.m_Abs);
			cpu.setHigh(cpu.m_DB, cpu.m_Abs);
			++cpu.m_PC;
			cpu.m_AB = cpu.m_Abs;
		}
	};
	template <class Command>
	struct WriteAbsY03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_Abs = cpu.m_OP16;
			cpu.m_AB = cpu.m_Abs;
			Command::execute(cpu);
		}
	};
	struct WriteAbsY04 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	/**
	 * Write Absolute Y
	 */
	template <class Command>
	using opWriteAbsY = cpu::Operation<WriteAbsY01, WriteAbsY02,
	    WriteAbsY03<Command>, WriteAbsY04, ParseNext>;

	/* RESET */
	struct Reset00 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	struct Reset01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = cpu.m_PC;
		}
	};
	struct Reset02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + cpu.m_S;
		}
	};
	struct Reset03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + --cpu.m_S;
		}
	};
	struct Reset04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.m_AB = 0x0100 + --cpu.m_S;
		}
	};
	struct Reset05 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			--cpu.m_S;
			cpu.m_AB = 0xfffc;
			cpu.m_PendingNMI = false;
		}
	};
	struct Reset06 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setLow(cpu.m_DB, cpu.m_PC);
			cpu.m_Interrupt = CPUFlagInterrupt;
			cpu.m_PendingINT = false;
			cpu.m_AB = 0xfffd;
		}
	};
	struct Reset07 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU &cpu) {
			cpu.setHigh(cpu.m_DB, cpu.m_PC);
			cpu.m_AB = cpu.m_PC;
		}
	};

	/**
	 * Reset operation
	 */
	using opReset = cpu::Operation<Reset00, Reset01, Reset02, Reset03, Reset04,
	    Reset05, Reset06, Reset07, ParseNext>;
	/**
	 * Opcodes
	 */
	using opcode_pack = class_pack<

	    // Flag manipulation

	    // CLC
	    cpu::Opcode<0x18, opImp<cmdCLC>>,
	    // SEC
	    cpu::Opcode<0x38, opImp<cmdSEC>>,
	    // CLI
	    cpu::Opcode<0x58, opImp<cmdCLI>>,
	    // SEI
	    cpu::Opcode<0x78, opImp<cmdSEI>>,
	    // CLV
	    cpu::Opcode<0xb8, opImp<cmdCLV>>,
	    // CLD
	    cpu::Opcode<0xd8, opImp<cmdCLD>>,
	    // SED
	    cpu::Opcode<0xf8, opImp<cmdSED>>,

	    // Transfer between registers

	    // TAX
	    cpu::Opcode<0xaa, opImp<cmdTAX>>,
	    // TAY
	    cpu::Opcode<0xa8, opImp<cmdTAY>>,
	    // TXA
	    cpu::Opcode<0x8a, opImp<cmdTXA>>,
	    // TYA
	    cpu::Opcode<0x98, opImp<cmdTYA>>,
	    // TXS
	    cpu::Opcode<0x9a, opImp<cmdTXS>>,
	    // TSX
	    cpu::Opcode<0xba, opImp<cmdTSX>>,

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
	    cpu::Opcode<0xe8, opImp<cmdINX>>,
	    // DEX
	    cpu::Opcode<0xca, opImp<cmdDEX>>,
	    // INY
	    cpu::Opcode<0xc8, opImp<cmdINY>>,
	    // DEY
	    cpu::Opcode<0x88, opImp<cmdDEY>>,
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
	    cpu::Opcode<0x90, opBranch<cmdBCC>>,
	    // BCS
	    cpu::Opcode<0xb0, opBranch<cmdBCS>>,
	    // BNE
	    cpu::Opcode<0xd0, opBranch<cmdBNE>>,
	    // BEQ
	    cpu::Opcode<0xf0, opBranch<cmdBEQ>>,
	    // BPL
	    cpu::Opcode<0x10, opBranch<cmdBPL>>,
	    // BMI
	    cpu::Opcode<0x30, opBranch<cmdBMI>>,
	    // BVC
	    cpu::Opcode<0x50, opBranch<cmdBVC>>,
	    // BVS
	    cpu::Opcode<0x70, opBranch<cmdBVS>>,

	    // Stack

	    // PHA
	    cpu::Opcode<0x48, opPHR<cmdPHA>>,
	    // PLA
	    cpu::Opcode<0x68, opPLR<cmdPLA>>,
	    // PHP
	    cpu::Opcode<0x08, opPHR<cmdPHP>>,
	    // PLP
	    cpu::Opcode<0x28, opPLR<cmdPLP>>,

	    // Jumps

	    // RTI
	    cpu::Opcode<0x40, opRTI>,
	    // RTS
	    cpu::Opcode<0x60, opRTS>,
	    // JSR
	    cpu::Opcode<0x20, opJSR>,
	    // JMP
	    cpu::Opcode<0x4c, opJMPAbs>, cpu::Opcode<0x6c, opJMPAbsInd>,

	    // Other

	    // BRK
	    cpu::Opcode<0x00, opBRK>,
	    // NOP
	    cpu::Opcode<0xea, opImp<cpu::Command>>,
	    cpu::Opcode<0x1a, opImp<cpu::Command>>,
	    cpu::Opcode<0x3a, opImp<cpu::Command>>,
	    cpu::Opcode<0x5a, opImp<cpu::Command>>,
	    cpu::Opcode<0x7a, opImp<cpu::Command>>,
	    cpu::Opcode<0xda, opImp<cpu::Command>>,
	    cpu::Opcode<0xfa, opImp<cpu::Command>>,
	    cpu::Opcode<0x80, opImm<cpu::Command>>,
	    cpu::Opcode<0x82, opImm<cpu::Command>>,
	    cpu::Opcode<0x89, opImm<cpu::Command>>,
	    cpu::Opcode<0xc2, opImm<cpu::Command>>,
	    cpu::Opcode<0xe2, opImm<cpu::Command>>
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
	using control = cpu::Control<opcodes>;

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
    , m_ZP()
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
