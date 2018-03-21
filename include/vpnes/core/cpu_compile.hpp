/**
 * @file
 * Defines microcode compilation helpers for cpu
 */
/*
 NES Emulator
 Copyright (C) 2012-2018  Ivanov Viktor

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

#ifndef INCLUDE_VPNES_CORE_CPU_COMPILE_HPP_
#define INCLUDE_VPNES_CORE_CPU_COMPILE_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vpnes/vpnes.hpp>
#include <vpnes/core/cpu.hpp>

namespace vpnes {

namespace core {

namespace cpu {

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
	static bool skipCycle(CCPU *cpu) {
		return false;
	}
	/**
	 * Executes the cycle
	 *
	 * @param cpu CPU
	 */
	template <class Control>
	static void execute(CCPU *cpu) {
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
	static void execute(CCPU *cpu) {
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
	static void execute(CCPU *cpu) {
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
	static void execute(CCPU *cpu) {
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
 * Expands pack to operation offset
 */
template <class OperationPack>
struct OperationOffsetExpand {
	template <class Operation>
	struct type {
		enum { offset = 0 };
	};
};

/**
 * Implementation of expansion to operation offset
 */
template <class FirstOperation, class... OtherOperations>
struct OperationOffsetExpand<class_pack<FirstOperation, OtherOperations...>> {
	template <class Operation>
	struct type {
		enum {
			offset = std::is_same<Operation, FirstOperation>::value
			             ? 0
			             : FirstOperation::Size +
			                   OperationOffsetExpand<
			                       class_pack<OtherOperations...>>::
			                       template type<Operation>::offset
		};
	};
};

/**
 * Invokes operation
 */
template <std::size_t Offset, std::size_t Index, class Operation>
struct Invoker {
	template <class Control>
	static void execute(CCPU *cpu) {
		Operation::template execute<Offset, Index, Control>(cpu);
	}
};

/**
 * Clones invoker with parameters needed
 */
template <std::size_t Index, class Operation, class IndexPack>
struct InvokeExpandHelper;

/**
 * Clones invoker with parameters needed
 */

template <std::size_t Index, class Operation, std::size_t... Indices>
struct InvokeExpandHelper<Index, Operation, index_pack<Indices...>>
    : class_pack<Invoker<Indices, Index + Indices, Operation>...> {};

/**
 * Expands invoke
 */
template <std::size_t Offset, class CurrentPack>
struct InvokeExpand;

/**
 * Empty invoke
 */
template <std::size_t Offset>
struct InvokeExpand<Offset, class_pack<>> : class_pack<> {};

/**
 * Implementation of invoke expansion
 */
template <std::size_t Offset, class FirstOperation, class... OtherOperations>
struct InvokeExpand<Offset, class_pack<FirstOperation, OtherOperations...>>
    : merge_class_pack<
          typename InvokeExpandHelper<Offset, FirstOperation,
              typename make_index_pack<FirstOperation::Size>::type>::type,
          typename InvokeExpand<Offset + FirstOperation::Size,
              class_pack<OtherOperations...>>::type> {};

/**
 * Invokes operation by index
 */
template <class OperationPack>
struct Invoke;

/**
 * Implementation of invocation
 */
template <class... Operations>
struct Invoke<class_pack<Operations...>> {
	/**
	 * Executes operation
	 *
	 * @param cpu CPU
	 * @param index Index
	 */
	template <class Control>
	static void execute(CCPU *cpu, std::size_t index) {
		typedef void (*handler)(CCPU *);
		static const handler handlers[sizeof...(Operations)] = {
		    (&Operations::template execute<Control>) ...};
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
 * Stores code and offset in compile time
 */
template <std::uint8_t Code, std::size_t Offset>
struct OpcodeOffset {
	using type = OpcodeOffset;
	enum { offset = Offset };
	enum { code = Code };
};

/**
 * Expands operations into opcode data
 */
template <class OperationOffsets, class OpcodePack>
struct OpcodeData;

/**
 * Implements expansion for opcodes
 */
template <class OperationOffsets, class... Opcodes>
struct OpcodeData<OperationOffsets, class_pack<Opcodes...>>
    : class_pack<OpcodeOffset<Opcodes::code,
          OperationOffsets::template type<
              typename Opcodes::operation>::offset>...> {};

/**
 * Declares opcode finder
 */
template <std::size_t DefaultOffset, class OpcodeOffsetPack>
struct FindOpcode;

/**
 * Empty opcode finder
 */
template <std::size_t DefaultOffset>
struct FindOpcode<DefaultOffset, class_pack<>> {
	template <std::uint8_t Code>
	using type = OpcodeOffset<Code, DefaultOffset>;
};

/**
 * Implements opcode finder
 */
template <std::size_t DefaultOffset, class FirstOffset, class... OtherOffsets>
struct FindOpcode<DefaultOffset, class_pack<FirstOffset, OtherOffsets...>> {
	template <std::uint8_t Code>
	using type =
	    typename std::conditional<Code == FirstOffset::code, FirstOffset,
	        typename FindOpcode<DefaultOffset,
	            class_pack<OtherOffsets...>>::template type<Code>>::type;
};

/**
 * Parses opcode
 */
template <class OpcodeFinder, class IndexPack>
struct OpcodeParser;

/**
 * Implementation of opcode parser
 */
template <std::size_t... Codes, class OpcodeFinder>
struct OpcodeParser<OpcodeFinder, index_pack<Codes...>> {
	/**
	 * Looks up code and returns index in compiled microcode
	 *
	 * @param code Opcode
	 * @return Index in compiled microcode
	 */
	static std::size_t parseOpcode(std::uint8_t code) {
		static const std::size_t codes[sizeof...(Codes)] = {
		    OpcodeFinder::template type<Codes>::offset...};
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
	 * Operation to jam CPU
	 */
	using operation_jam = Operation<Cycle>;
	/**
	 * Operation pack
	 */
	using operation_pack = typename ExpandOperations<opcode_pack,
	    typename OpcodeControl::opReset, operation_jam>::type;
	/**
	 * Operation offsets
	 */
	struct operation_offset : OperationOffsetExpand<operation_pack> {};
	/**
	 * Opcode data
	 */
	using opcode_data =
	    typename OpcodeData<operation_offset, opcode_pack>::type;
	/**
	 * Opcode finder
	 */
	using opcode_finder =
	    FindOpcode<operation_offset::template type<operation_jam>::offset,
	        opcode_data>;
	enum {
		ResetIndex =
		    operation_offset::template type<typename OpcodeControl::opReset>::
		        offset  //!< Reset in compiled microcode
	};

	/**
	 * Sets a point since where to start next operation
	 *
	 * @param cpu CPU
	 * @param index Index
	 */
	static void setEndPoint(CCPU *cpu, std::size_t index) {
		OpcodeControl::setEndPoint(cpu, index);
	}
	/**
	 * Accesses the bus
	 *
	 * @param cpu CPU
	 * @param busMode Access mode
	 * @param ackIRQ Acknowledge IRQ
	 * @return If could or not
	 */
	static bool accessBus(CCPU *cpu, CCPU::EBusMode busMode, bool ackIRQ) {
		return OpcodeControl::accessBus(cpu, busMode, ackIRQ);
	}
	/**
	 * Executes microcode from index
	 *
	 * @param cpu CPU
	 * @param index Index
	 */
	static void execute(CCPU *cpu, std::size_t index) {
		Invoke<typename InvokeExpand<0,
		    operation_pack>::type>::template execute<Control>(cpu, index);
	}
	/**
	 * Looks up code and returns index in compiled microcode
	 *
	 * @param code Opcode
	 * @return Index in compiled microcode
	 */
	static std::size_t parseOpcode(std::uint8_t code) {
		return OpcodeParser<opcode_finder,
		    typename make_index_pack<0x100>::type>::parseOpcode(code);
	}
};

}  // namespace cpu

}  // namespace core

}  // namespace vpnes

#endif  // INCLUDE_VPNES_CORE_CPU_COMPILE_HPP_
