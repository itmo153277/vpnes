/**
 * @file
 * Implements basic CPU
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
#include <vpnes/core/cpu_compile.hpp>

namespace vpnes {

namespace core {

/**
 * Microcode
 */
struct CCPU::opcodes {
	/* Commands */
	struct cmdPHA : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_DB = cpu->m_A;
		}
	};
	struct cmdPHP : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_DB = cpu->packState() | CPUFlagBreak;
		}
	};
	struct cmdPLA : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_A = cpu->m_DB;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdPLP : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->unpackState(cpu->m_DB);
		}
	};
	struct cmdCLC : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_Carry = 0;
		}
	};
	struct cmdSEC : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_Carry = CPUFlagCarry;
		}
	};
	struct cmdCLD : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_Decimal = 0;
		}
	};
	struct cmdSED : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_Decimal = CPUFlagDecimal;
		}
	};
	struct cmdCLI : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_Interrupt = 0;
		}
	};
	struct cmdSEI : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_Interrupt = CPUFlagInterrupt;
		}
	};
	struct cmdCLV : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_Overflow = 0;
		}
	};
	struct cmdTAX : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_X = cpu->m_A;
			cpu->setNegativeFlag(cpu->m_X);
			cpu->setZeroFlag(cpu->m_X);
		}
	};
	struct cmdTAY : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_Y = cpu->m_A;
			cpu->setNegativeFlag(cpu->m_Y);
			cpu->setZeroFlag(cpu->m_Y);
		}
	};
	struct cmdTXA : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_A = cpu->m_X;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdTYA : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_A = cpu->m_Y;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdTXS : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_S = cpu->m_X;
		}
	};
	struct cmdTSX : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_X = cpu->m_S;
			cpu->setNegativeFlag(cpu->m_X);
			cpu->setZeroFlag(cpu->m_X);
		}
	};
	struct cmdINX : cpu::Command {
		static void execute(CCPU *cpu) {
			++cpu->m_X;
			cpu->setNegativeFlag(cpu->m_X);
			cpu->setZeroFlag(cpu->m_X);
		}
	};
	struct cmdDEX : cpu::Command {
		static void execute(CCPU *cpu) {
			--cpu->m_X;
			cpu->setNegativeFlag(cpu->m_X);
			cpu->setZeroFlag(cpu->m_X);
		}
	};
	struct cmdINY : cpu::Command {
		static void execute(CCPU *cpu) {
			++cpu->m_Y;
			cpu->setNegativeFlag(cpu->m_Y);
			cpu->setZeroFlag(cpu->m_Y);
		}
	};
	struct cmdDEY : cpu::Command {
		static void execute(CCPU *cpu) {
			--cpu->m_Y;
			cpu->setNegativeFlag(cpu->m_Y);
			cpu->setZeroFlag(cpu->m_Y);
		}
	};
	struct cmdBCC : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_BranchTaken = cpu->m_Carry == 0;
		}
	};
	struct cmdBCS : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_BranchTaken = cpu->m_Carry != 0;
		}
	};
	struct cmdBNE : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_BranchTaken = cpu->m_Zero == 0;
		}
	};
	struct cmdBEQ : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_BranchTaken = cpu->m_Zero != 0;
		}
	};
	struct cmdBPL : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_BranchTaken = (cpu->m_Negative & CPUFlagNegative) == 0;
		}
	};
	struct cmdBMI : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_BranchTaken = (cpu->m_Negative & CPUFlagNegative) != 0;
		}
	};
	struct cmdBVC : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_BranchTaken = cpu->m_Overflow == 0;
		}
	};
	struct cmdBVS : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_BranchTaken = cpu->m_Overflow != 0;
		}
	};
	struct cmdLDA : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_A = cpu->m_DB;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdSTA : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_DB = cpu->m_A;
		}
	};
	struct cmdLDX : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_X = cpu->m_DB;
			cpu->setNegativeFlag(cpu->m_X);
			cpu->setZeroFlag(cpu->m_X);
		}
	};
	struct cmdSTX : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_DB = cpu->m_X;
		}
	};
	struct cmdLDY : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_Y = cpu->m_DB;
			cpu->setNegativeFlag(cpu->m_Y);
			cpu->setZeroFlag(cpu->m_Y);
		}
	};
	struct cmdSTY : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_DB = cpu->m_Y;
		}
	};
	struct cmdAND : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_A &= cpu->m_DB;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdORA : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_A |= cpu->m_DB;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdEOR : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_A ^= cpu->m_DB;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdINC : cpu::Command {
		static void execute(CCPU *cpu) {
			++cpu->m_OP;
			cpu->setNegativeFlag(cpu->m_OP);
			cpu->setZeroFlag(cpu->m_OP);
			cpu->m_DB = cpu->m_OP;
		}
	};
	struct cmdDEC : cpu::Command {
		static void execute(CCPU *cpu) {
			--cpu->m_OP;
			cpu->setNegativeFlag(cpu->m_OP);
			cpu->setZeroFlag(cpu->m_OP);
			cpu->m_DB = cpu->m_OP;
		}
	};
	struct cmdCMP : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint16_t dummy = cpu->m_A - cpu->m_DB;
			cpu->setCarryFlag(dummy < 0x0100);
			cpu->setNegativeFlag(dummy);
			cpu->setZeroFlag(dummy & 0xff);
		}
	};
	struct cmdCPX : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint16_t dummy = cpu->m_X - cpu->m_DB;
			cpu->setCarryFlag(dummy < 0x0100);
			cpu->setNegativeFlag(dummy);
			cpu->setZeroFlag(dummy & 0xff);
		}
	};
	struct cmdCPY : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint16_t dummy = cpu->m_Y - cpu->m_DB;
			cpu->setCarryFlag(dummy < 0x0100);
			cpu->setNegativeFlag(dummy);
			cpu->setZeroFlag(dummy & 0xff);
		}
	};
	struct cmdBIT : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->setOverflowFlag((cpu->m_DB & 0x40) != 0);
			cpu->setNegativeFlag(cpu->m_DB);
			cpu->setZeroFlag(cpu->m_A & cpu->m_DB);
		}
	};
	struct cmdADC : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint16_t dummy = cpu->m_DB + cpu->m_A + cpu->m_Carry;
			cpu->setOverflowFlag(
			    ((dummy ^ cpu->m_A) & ~(cpu->m_A ^ cpu->m_DB) & 0x80) != 0);
			cpu->setCarryFlag(dummy >= 0x0100);
			cpu->m_A = dummy & 0xff;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdSBC : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint16_t dummy =
			    cpu->m_A - cpu->m_DB - (cpu->m_Carry ^ CPUFlagCarry);
			cpu->setOverflowFlag(
			    ((dummy ^ cpu->m_A) & (cpu->m_A ^ cpu->m_DB) & 0x80) != 0);
			cpu->setCarryFlag(dummy < 0x0100);
			cpu->m_A = dummy & 0xff;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdROL : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint8_t dummy = cpu->m_OP & 0x80;
			cpu->m_OP <<= 1;
			cpu->m_OP |= cpu->m_Carry;
			cpu->setCarryFlag(dummy != 0);
			cpu->setNegativeFlag(cpu->m_OP);
			cpu->setZeroFlag(cpu->m_OP);
			cpu->m_DB = cpu->m_OP;
		}
	};
	struct cmdROR : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint8_t dummy = cpu->m_OP & 0x01;
			cpu->m_OP >>= 1;
			cpu->m_OP |= (cpu->m_Carry << 7);
			cpu->setCarryFlag(dummy != 0);
			cpu->setNegativeFlag(cpu->m_OP);
			cpu->setZeroFlag(cpu->m_OP);
			cpu->m_DB = cpu->m_OP;
		}
	};
	struct cmdASL : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->setCarryFlag((cpu->m_OP & 0x80) != 0);
			cpu->m_OP <<= 1;
			cpu->setNegativeFlag(cpu->m_OP);
			cpu->setZeroFlag(cpu->m_OP);
			cpu->m_DB = cpu->m_OP;
		}
	};
	struct cmdLSR : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->setCarryFlag((cpu->m_OP & 0x01) != 0);
			cpu->m_OP >>= 1;
			cpu->setNegativeFlag(cpu->m_OP);
			cpu->setZeroFlag(cpu->m_OP);
			cpu->m_DB = cpu->m_OP;
		}
	};
	struct cmdROLA : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint8_t dummy = cpu->m_A & 0x80;
			cpu->m_A <<= 1;
			cpu->m_A |= cpu->m_Carry;
			cpu->setCarryFlag(dummy != 0);
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdRORA : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint8_t dummy = cpu->m_A & 0x01;
			cpu->m_A >>= 1;
			cpu->m_A |= (cpu->m_Carry << 7);
			cpu->setCarryFlag(dummy != 0);
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdASLA : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->setCarryFlag((cpu->m_A & 0x80) != 0);
			cpu->m_A <<= 1;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdLSRA : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->setCarryFlag((cpu->m_A & 0x01) != 0);
			cpu->m_A >>= 1;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdSLO : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->setCarryFlag((cpu->m_OP & 0x80) != 0);
			cpu->m_OP <<= 1;
			cpu->m_A |= cpu->m_OP;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
			cpu->m_DB = cpu->m_OP;
		}
	};
	struct cmdRLA : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint8_t dummy = cpu->m_OP & 0x80;
			cpu->m_OP <<= 1;
			cpu->m_OP |= cpu->m_Carry;
			cpu->m_A &= cpu->m_OP;
			cpu->setCarryFlag(dummy != 0);
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
			cpu->m_DB = cpu->m_OP;
		}
	};
	struct cmdSRE : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->setCarryFlag((cpu->m_OP & 0x01) != 0);
			cpu->m_OP >>= 1;
			cpu->m_A ^= cpu->m_OP;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
			cpu->m_DB = cpu->m_OP;
		}
	};
	struct cmdRRA : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint8_t dummy = cpu->m_OP & 0x01;
			cpu->m_OP >>= 1;
			cpu->m_OP |= (cpu->m_Carry << 7);
			cpu->setCarryFlag(dummy != 0);
			std::uint16_t dummySum = cpu->m_OP + cpu->m_A + cpu->m_Carry;
			cpu->setOverflowFlag(
			    ((dummySum ^ cpu->m_A) & ~(cpu->m_A ^ cpu->m_OP) & 0x80) != 0);
			cpu->setCarryFlag(dummySum >= 0x0100);
			cpu->m_A = dummySum & 0xff;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
			cpu->m_DB = cpu->m_OP;
		}
	};
	struct cmdSAX : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_DB = cpu->m_A & cpu->m_X;
		}
	};
	struct cmdLAX : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_A = cpu->m_DB;
			cpu->m_X = cpu->m_DB;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdDCP : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint16_t dummy = cpu->m_A - --cpu->m_OP;
			cpu->setCarryFlag(dummy < 0x0100);
			cpu->setNegativeFlag(dummy);
			cpu->setZeroFlag(dummy);
			cpu->m_DB = cpu->m_OP;
		}
	};
	struct cmdISC : cpu::Command {
		static void execute(CCPU *cpu) {
			++cpu->m_OP;
			std::uint16_t dummy =
			    cpu->m_A - cpu->m_OP - (cpu->m_Carry ^ CPUFlagCarry);
			cpu->setOverflowFlag(
			    ((dummy ^ cpu->m_A) & (cpu->m_A ^ cpu->m_OP) & 0x80) != 0);
			cpu->setCarryFlag(dummy < 0x0100);
			cpu->m_A = dummy & 0xff;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
			cpu->m_DB = cpu->m_OP;
		}
	};
	struct cmdANC : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_A &= cpu->m_DB;
			cpu->setCarryFlag((cpu->m_A & 0x80) != 0);
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdALR : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_A &= cpu->m_DB;
			cpu->setCarryFlag((cpu->m_A & 0x01) != 0);
			cpu->m_A >>= 1;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};
	struct cmdARR : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_A &= cpu->m_DB;
			cpu->m_A >>= 1;
			cpu->m_A |= cpu->m_Carry << 7;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
			cpu->setOverflowFlag(
			    (((cpu->m_A >> 6) ^ (cpu->m_A >> 5)) & 0x01) != 0);
			cpu->setCarryFlag((cpu->m_A & 0x40) != 0);
		}
	};
	struct cmdXAA : cpu::Command {
		static void execute(CCPU *cpu) {
			std::uint8_t dummy = cpu->m_X & cpu->m_A & cpu->m_DB;
			cpu->setNegativeFlag(dummy);
			cpu->setZeroFlag(dummy);
			cpu->m_A &= cpu->m_X & (dummy | 0xef);
		}
	};
	struct cmdAXS : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_X &= cpu->m_A;
			cpu->setCarryFlag(cpu->m_X >= cpu->m_DB);
			cpu->m_X -= cpu->m_DB;
			cpu->setNegativeFlag(cpu->m_X);
			cpu->setZeroFlag(cpu->m_X);
		}
	};
	struct cmdSHA : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_DB = cpu->m_A & cpu->m_X & ((cpu->m_OP16 >> 8) + 1);
			if ((cpu->m_AB & 0xff00) != (cpu->m_OP16 & 0xff00)) {
				cpu->m_Abs = (cpu->m_Abs & 0xff) | (cpu->m_DB << 8);
			}
		}
	};
	struct cmdSHX : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_DB = cpu->m_X & ((cpu->m_OP16 >> 8) + 1);
			if ((cpu->m_AB & 0xff00) != (cpu->m_OP16 & 0xff00)) {
				cpu->m_Abs = (cpu->m_Abs & 0xff) | (cpu->m_DB << 8);
			}
		}
	};
	struct cmdSHY : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_DB = cpu->m_Y & ((cpu->m_OP16 >> 8) + 1);
			if ((cpu->m_AB & 0xff00) != (cpu->m_OP16 & 0xff00)) {
				cpu->m_Abs = (cpu->m_Abs & 0xff) | (cpu->m_DB << 8);
			}
		}
	};
	struct cmdTAS : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_S = cpu->m_A & cpu->m_X;
			cpu->m_DB = cpu->m_S & ((cpu->m_OP16 >> 8) + 1);
			if ((cpu->m_AB & 0xff00) != (cpu->m_OP16 & 0xff00)) {
				cpu->m_Abs = (cpu->m_Abs & 0xff) | (cpu->m_DB << 8);
			}
		}
	};
	struct cmdLAS : cpu::Command {
		static void execute(CCPU *cpu) {
			cpu->m_S &= cpu->m_OP;
			cpu->m_A = cpu->m_S;
			cpu->m_X = cpu->m_S;
			cpu->setNegativeFlag(cpu->m_A);
			cpu->setZeroFlag(cpu->m_A);
		}
	};

	/* Cycles */

	/**
	 * Parsing next opcode
	 */
	struct ParseNext : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			if (!cpu->m_PendingINT) {
				++cpu->m_PC;
			} else {
				cpu->m_DB = 0;
			}
			cpu->m_AB = cpu->m_PC;
			Control::setEndPoint(cpu, Control::parseOpcode(cpu->m_DB));
		}
	};

	/* BRK */
	struct BRK01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			if (!cpu->m_PendingINT) {
				cpu->m_PC++;
			}
			cpu->m_AB = 0x0100 + cpu->m_S;
			cpu->m_DB = cpu->m_PC >> 8;
		}
	};
	struct BRK02 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + --cpu->m_S;
			cpu->m_DB = cpu->m_PC & 0x00ff;
		}
	};
	struct BRK03 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + --cpu->m_S;
			cpu->m_DB = cpu->packState();
			if (!cpu->m_PendingINT) {
				cpu->m_DB |= CPUFlagBreak;
			}
		}
	};
	struct BRK04 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			--cpu->m_S;
			cpu->processInterrupts();
			if (cpu->m_PendingNMI) {
				cpu->m_PendingNMI = false;
				cpu->m_AB = 0xfffa;
			} else {
				cpu->m_AB = 0xfffe;
			}
		}
	};
	struct BRK05 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_DB, &cpu->m_PC);
			cpu->m_Interrupt = CPUFlagInterrupt;
			cpu->m_PendingINT = false;
			++cpu->m_AB;
		}
	};
	struct BRK06 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setHigh(cpu->m_DB, &cpu->m_PC);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * BRK
	 */
	struct opBRK
	    : cpu::Operation<BRK01, BRK02, BRK03, BRK04, BRK05, BRK06, ParseNext> {
	};

	/* RTI */
	struct RTI01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + cpu->m_S;
		}
	};
	struct RTI02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + ++cpu->m_S;
		}
	};
	struct RTI03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->unpackState(cpu->m_DB);
			cpu->m_AB = 0x0100 + ++cpu->m_S;
		}
	};
	struct RTI04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_DB, &cpu->m_PC);
			cpu->m_AB = 0x0100 + ++cpu->m_S;
		}
	};
	struct RTI05 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setHigh(cpu->m_DB, &cpu->m_PC);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * RTI
	 */
	struct opRTI
	    : cpu::Operation<RTI01, RTI02, RTI03, RTI04, RTI05, ParseNext> {};

	/* RTS */
	struct RTS01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + cpu->m_S;
		}
	};
	struct RTS02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + ++cpu->m_S;
		}
	};
	struct RTS03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_DB, &cpu->m_PC);
			cpu->m_AB = 0x0100 + ++cpu->m_S;
		}
	};
	struct RTS04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setHigh(cpu->m_DB, &cpu->m_PC);
			cpu->m_AB = cpu->m_PC;
		}
	};
	struct RTS05 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	/**
	 * RTS
	 */
	struct opRTS
	    : cpu::Operation<RTS01, RTS02, RTS03, RTS04, RTS05, ParseNext> {};

	/* PHA/PHP */
	template <class Command>
	struct PHR01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + cpu->m_S;
			Command::execute(cpu);
		}
	};
	struct PHR02 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			--cpu->m_S;
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * PHA/PHP
	 */
	template <class Command>
	struct opPHR : cpu::Operation<PHR01<Command>, PHR02, ParseNext> {};

	/* PLA/PLP */
	struct PLR01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + cpu->m_S;
		}
	};
	struct PLR02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + ++cpu->m_S;
		}
	};
	template <class Command>
	struct PLR03 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * PLA/PLP
	 */
	template <class Command>
	struct opPLR : cpu::Operation<PLR01, PLR02, PLR03<Command>, ParseNext> {};

	/* JSR */
	struct JSR01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = 0x0100 + cpu->m_S;
		}
	};
	struct JSR02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_DB = cpu->m_PC >> 8;
		}
	};
	struct JSR03 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + --cpu->m_S;
			cpu->m_DB = cpu->m_PC & 0xff;
		}
	};
	struct JSR04 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			--cpu->m_S;
			cpu->m_AB = cpu->m_PC;
		}
	};
	struct JSR05 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_PC);
			cpu->setHigh(cpu->m_DB, &cpu->m_PC);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * JSR
	 */
	struct opJSR
	    : cpu::Operation<JSR01, JSR02, JSR03, JSR04, JSR05, ParseNext> {};

	/* JMP Absolute */
	struct JMPAbs01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	struct JMPAbs02 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_PC);
			cpu->setHigh(cpu->m_DB, &cpu->m_PC);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * JMP Absolute
	 */
	struct opJMPAbs : cpu::Operation<JMPAbs01, JMPAbs02, ParseNext> {};

	/* JMP Absolute Indirect */
	struct JMPAbsInd01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	struct JMPAbsInd02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			++cpu->m_PC;
			cpu->setLow(cpu->m_OP, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct JMPAbsInd03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_DB, &cpu->m_PC);
			cpu->setLow(++cpu->m_OP, &cpu->m_Abs);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct JMPAbsInd04 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setHigh(cpu->m_DB, &cpu->m_PC);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * JMP Absolute Indirect
	 */
	struct opJMPAbsInd : cpu::Operation<JMPAbsInd01, JMPAbsInd02, JMPAbsInd03,
	                         JMPAbsInd04, ParseNext> {};

	/* Branches */
	template <class Command>
	struct Branch01 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			Command::execute(cpu);
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	struct Branch02 : cpu::Cycle {
		static bool skipCycle(CCPU *cpu) {
			return !cpu->m_BranchTaken;
		}
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP16 = cpu->m_PC + static_cast<std::int8_t>(cpu->m_OP);
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_PC);
			cpu->m_AB = cpu->m_PC;
		}
	};
	struct Branch03 : cpu::Cycle {
		enum { AckIRQ = true };
		static bool skipCycle(CCPU *cpu) {
			return !cpu->m_BranchTaken ||
			       ((cpu->m_OP16 & 0xff00) == (cpu->m_PC & 0xff00));
		}
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_PC = cpu->m_OP16;
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Branch
	 */
	template <class Command>
	struct opBranch
	    : cpu::Operation<Branch01<Command>, Branch02, Branch03, ParseNext> {};

	/* Implied */
	template <class Command>
	struct Imp01 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Implied
	 */
	template <class Command>
	struct opImp : cpu::Operation<Imp01<Command>, ParseNext> {};

	/* Immediate */
	template <class Command>
	struct Imm01 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	/**
	 * Immediate
	 */
	template <class Command>
	struct opImm : cpu::Operation<Imm01<Command>, ParseNext> {};

	/* Absolute */
	/* Read */
	struct ReadAbs01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	struct ReadAbs02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			++cpu->m_PC;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ReadAbs03 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Read Absolute
	 */
	template <class Command>
	struct opReadAbs
	    : cpu::Operation<ReadAbs01, ReadAbs02, ReadAbs03<Command>, ParseNext> {
	};
	/* Modify */
	struct ModifyAbs01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	struct ModifyAbs02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			++cpu->m_PC;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyAbs03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ModifyAbs04 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyAbs05 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Modify Absolute
	 */
	template <class Command>
	struct opModifyAbs : cpu::Operation<ModifyAbs01, ModifyAbs02, ModifyAbs03,
	                         ModifyAbs04<Command>, ModifyAbs05, ParseNext> {};
	/* Write */
	struct WriteAbs01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	template <class Command>
	struct WriteAbs02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			++cpu->m_PC;
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct WriteAbs03 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Write Absolute
	 */
	template <class Command>
	struct opWriteAbs : cpu::Operation<WriteAbs01, WriteAbs02<Command>,
	                        WriteAbs03, ParseNext> {};

	/* Zero-page */
	/* Read */
	struct ReadZP01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	template <class Command>
	struct ReadZP02 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Read Zero-page
	 */
	template <class Command>
	struct opReadZP : cpu::Operation<ReadZP01, ReadZP02<Command>, ParseNext> {};
	/* Modify */
	struct ModifyZP01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZP02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	template <class Command>
	struct ModifyZP03 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZP04 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Modify Zero-Page
	 */
	template <class Command>
	struct opModifyZP : cpu::Operation<ModifyZP01, ModifyZP02,
	                        ModifyZP03<Command>, ModifyZP04, ParseNext> {};
	/* Write */
	template <class Command>
	struct WriteZP01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			Command::execute(cpu);
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct WriteZP02 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Write Zero-Page
	 */
	template <class Command>
	struct opWriteZP
	    : cpu::Operation<WriteZP01<Command>, WriteZP02, ParseNext> {};

	/* Zero-page X */
	/* Read */
	struct ReadZPX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ReadZPX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP += cpu->m_X;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	template <class Command>
	struct ReadZPX03 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Read Zero-page X
	 */
	template <class Command>
	struct opReadZPX
	    : cpu::Operation<ReadZPX01, ReadZPX02, ReadZPX03<Command>, ParseNext> {
	};
	/* Modify */
	struct ModifyZPX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZPX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP += cpu->m_X;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZPX03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	template <class Command>
	struct ModifyZPX04 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZPX05 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Modify Zero-Page X
	 */
	template <class Command>
	struct opModifyZPX : cpu::Operation<ModifyZPX01, ModifyZPX02, ModifyZPX03,
	                         ModifyZPX04<Command>, ModifyZPX05, ParseNext> {};
	/* Write */
	struct WriteZPX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	template <class Command>
	struct WriteZPX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP += cpu->m_X;
			Command::execute(cpu);
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct WriteZPX03 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Write Zero-Page X
	 */
	template <class Command>
	struct opWriteZPX : cpu::Operation<WriteZPX01, WriteZPX02<Command>,
	                        WriteZPX03, ParseNext> {};

	/* Zero-page Y */
	/* Read */
	struct ReadZPY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ReadZPY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP += cpu->m_Y;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	template <class Command>
	struct ReadZPY03 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Read Zero-page Y
	 */
	template <class Command>
	struct opReadZPY
	    : cpu::Operation<ReadZPY01, ReadZPY02, ReadZPY03<Command>, ParseNext> {
	};
	/* Modify */
	struct ModifyZPY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZPY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP += cpu->m_Y;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZPY03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	template <class Command>
	struct ModifyZPY04 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZPY05 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Modify Zero-Page Y
	 */
	template <class Command>
	struct opModifyZPY : cpu::Operation<ModifyZPY01, ModifyZPY02, ModifyZPY03,
	                         ModifyZPY04<Command>, ModifyZPY05, ParseNext> {};
	/* Write */
	struct WriteZPY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	template <class Command>
	struct WriteZPY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP += cpu->m_Y;
			Command::execute(cpu);
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct WriteZPY03 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Write Zero-Page Y
	 */
	template <class Command>
	struct opWriteZPY : cpu::Operation<WriteZPY01, WriteZPY02<Command>,
	                        WriteZPY03, ParseNext> {};

	/* Absolute X */
	/* Read */
	struct ReadAbsX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	struct ReadAbsX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_OP16);
			cpu->setHigh(cpu->m_DB, &cpu->m_OP16);
			cpu->m_OP16 += cpu->m_X;
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			++cpu->m_PC;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ReadAbsX03 : cpu::Cycle {
		static bool skipCycle(CCPU *cpu) {
			return ((cpu->m_OP16 & 0xff00) == (cpu->m_Abs & 0xff00));
		}
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_Abs = cpu->m_OP16;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ReadAbsX04 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Read Absolute X
	 */
	template <class Command>
	struct opReadAbsX : cpu::Operation<ReadAbsX01, ReadAbsX02, ReadAbsX03,
	                        ReadAbsX04<Command>, ParseNext> {};
	/* Modify */
	struct ModifyAbsX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	struct ModifyAbsX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_OP16);
			cpu->setHigh(cpu->m_DB, &cpu->m_OP16);
			cpu->m_OP16 += cpu->m_X;
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			++cpu->m_PC;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyAbsX03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_Abs = cpu->m_OP16;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyAbsX04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ModifyAbsX05 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyAbsX06 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Modify Absolute X
	 */
	template <class Command>
	struct opModifyAbsX
	    : cpu::Operation<ModifyAbsX01, ModifyAbsX02, ModifyAbsX03, ModifyAbsX04,
	          ModifyAbsX05<Command>, ModifyAbsX06, ParseNext> {};
	/* Write */
	struct WriteAbsX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	struct WriteAbsX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_OP16);
			cpu->setHigh(cpu->m_DB, &cpu->m_OP16);
			cpu->m_OP16 += cpu->m_X;
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			++cpu->m_PC;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct WriteAbsX03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_Abs = cpu->m_OP16;
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct WriteAbsX04 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Write Absolute X
	 */
	template <class Command>
	struct opWriteAbsX : cpu::Operation<WriteAbsX01, WriteAbsX02,
	                         WriteAbsX03<Command>, WriteAbsX04, ParseNext> {};

	/* Absolute Y */
	/* Read */
	struct ReadAbsY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	struct ReadAbsY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_OP16);
			cpu->setHigh(cpu->m_DB, &cpu->m_OP16);
			cpu->m_OP16 += cpu->m_Y;
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			++cpu->m_PC;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ReadAbsY03 : cpu::Cycle {
		static bool skipCycle(CCPU *cpu) {
			return ((cpu->m_OP16 & 0xff00) == (cpu->m_Abs & 0xff00));
		}
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_Abs = cpu->m_OP16;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ReadAbsY04 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Read Absolute Y
	 */
	template <class Command>
	struct opReadAbsY : cpu::Operation<ReadAbsY01, ReadAbsY02, ReadAbsY03,
	                        ReadAbsY04<Command>, ParseNext> {};
	/* Modify */
	struct ModifyAbsY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	struct ModifyAbsY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_OP16);
			cpu->setHigh(cpu->m_DB, &cpu->m_OP16);
			cpu->m_OP16 += cpu->m_Y;
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			++cpu->m_PC;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyAbsY03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_Abs = cpu->m_OP16;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyAbsY04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ModifyAbsY05 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyAbsY06 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Modify Absolute Y
	 */
	template <class Command>
	struct opModifyAbsY
	    : cpu::Operation<ModifyAbsY01, ModifyAbsY02, ModifyAbsY03, ModifyAbsY04,
	          ModifyAbsY05<Command>, ModifyAbsY06, ParseNext> {};
	/* Write */
	struct WriteAbsY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_PC;
		}
	};
	struct WriteAbsY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_OP16);
			cpu->setHigh(cpu->m_DB, &cpu->m_OP16);
			cpu->m_OP16 += cpu->m_Y;
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			++cpu->m_PC;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct WriteAbsY03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_Abs = cpu->m_OP16;
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct WriteAbsY04 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Write Absolute Y
	 */
	template <class Command>
	struct opWriteAbsY : cpu::Operation<WriteAbsY01, WriteAbsY02,
	                         WriteAbsY03<Command>, WriteAbsY04, ParseNext> {};

	/* ZP X Indirect */
	/* Read */
	struct ReadZPXInd01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ReadZPXInd02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP += cpu->m_X;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ReadZPXInd03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_ZP;
		}
	};
	struct ReadZPXInd04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ReadZPXInd05 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Read ZP X Indirect
	 */
	template <class Command>
	struct opReadZPXInd
	    : cpu::Operation<ReadZPXInd01, ReadZPXInd02, ReadZPXInd03, ReadZPXInd04,
	          ReadZPXInd05<Command>, ParseNext> {};
	/* Modify */
	struct ModifyZPXInd01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZPXInd02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP += cpu->m_X;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZPXInd03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_ZP;
		}
	};
	struct ModifyZPXInd04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyZPXInd05 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ModifyZPXInd06 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyZPXInd07 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Modify ZP X Indirect
	 */
	template <class Command>
	struct opModifyZPXInd
	    : cpu::Operation<ModifyZPXInd01, ModifyZPXInd02, ModifyZPXInd03,
	          ModifyZPXInd04, ModifyZPXInd05, ModifyZPXInd06<Command>,
	          ModifyZPXInd07, ParseNext> {};
	/* Write */
	struct WriteZPXInd01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct WriteZPXInd02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP += cpu->m_X;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct WriteZPXInd03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_ZP;
		}
	};
	template <class Command>
	struct WriteZPXInd04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct WriteZPXInd05 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Write ZP X Indirect
	 */
	template <class Command>
	struct opWriteZPXInd
	    : cpu::Operation<WriteZPXInd01, WriteZPXInd02, WriteZPXInd03,
	          WriteZPXInd04<Command>, WriteZPXInd05, ParseNext> {};

	/* ZP Y Indirect */
	/* Read */
	struct ReadZPYInd01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ReadZPYInd02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP += cpu->m_Y;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ReadZPYInd03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_ZP;
		}
	};
	struct ReadZPYInd04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ReadZPYInd05 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Read ZP Y Indirect
	 */
	template <class Command>
	struct opReadZPYInd
	    : cpu::Operation<ReadZPYInd01, ReadZPYInd02, ReadZPYInd03, ReadZPYInd04,
	          ReadZPYInd05<Command>, ParseNext> {};
	/* Modify */
	struct ModifyZPYInd01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZPYInd02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP += cpu->m_Y;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZPYInd03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_ZP;
		}
	};
	struct ModifyZPYInd04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyZPYInd05 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ModifyZPYInd06 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyZPYInd07 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Modify ZP Y Indirect
	 */
	template <class Command>
	struct opModifyZPYInd
	    : cpu::Operation<ModifyZPYInd01, ModifyZPYInd02, ModifyZPYInd03,
	          ModifyZPYInd04, ModifyZPYInd05, ModifyZPYInd06<Command>,
	          ModifyZPYInd07, ParseNext> {};
	/* Write */
	struct WriteZPYInd01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct WriteZPYInd02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP += cpu->m_Y;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct WriteZPYInd03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_ZP;
		}
	};
	template <class Command>
	struct WriteZPYInd04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct WriteZPYInd05 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Write ZP Y Indirect
	 */
	template <class Command>
	struct opWriteZPYInd
	    : cpu::Operation<WriteZPYInd01, WriteZPYInd02, WriteZPYInd03,
	          WriteZPYInd04<Command>, WriteZPYInd05, ParseNext> {};

	/* ZP Indirect X */
	/* Read */
	struct ReadZPIndX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ReadZPIndX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_ZP;
		}
	};
	struct ReadZPIndX03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_OP16);
			cpu->setHigh(cpu->m_DB, &cpu->m_OP16);
			cpu->m_OP16 += cpu->m_X;
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ReadZPIndX04 : cpu::Cycle {
		static bool skipCycle(CCPU *cpu) {
			return ((cpu->m_OP16 & 0xff00) == (cpu->m_Abs & 0xff00));
		}
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_Abs = cpu->m_OP16;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ReadZPIndX05 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Read ZP Indirect X
	 */
	template <class Command>
	struct opReadZPIndX
	    : cpu::Operation<ReadZPIndX01, ReadZPIndX02, ReadZPIndX03, ReadZPIndX04,
	          ReadZPIndX05<Command>, ParseNext> {};
	/* Modify */
	struct ModifyZPIndX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZPIndX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_ZP;
		}
	};
	struct ModifyZPIndX03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_OP16);
			cpu->setHigh(cpu->m_DB, &cpu->m_OP16);
			cpu->m_OP16 += cpu->m_X;
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyZPIndX04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_Abs = cpu->m_OP16;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyZPIndX05 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ModifyZPIndX06 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyZPIndX07 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Modify ZP Indirect X
	 */
	template <class Command>
	struct opModifyZPIndX
	    : cpu::Operation<ModifyZPIndX01, ModifyZPIndX02, ModifyZPIndX03,
	          ModifyZPIndX04, ModifyZPIndX05, ModifyZPIndX06<Command>,
	          ModifyZPIndX07, ParseNext> {};
	/* Write */
	struct WriteZPIndX01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct WriteZPIndX02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_ZP;
		}
	};
	struct WriteZPIndX03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_OP16);
			cpu->setHigh(cpu->m_DB, &cpu->m_OP16);
			cpu->m_OP16 += cpu->m_X;
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct WriteZPIndX04 : cpu::Cycle {
		static bool skipCycle(CCPU *cpu) {
			return ((cpu->m_OP16 & 0xff00) == (cpu->m_Abs & 0xff00));
		}
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_Abs = cpu->m_OP16;
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct WriteZPIndX05 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Write ZP Indirect X
	 */
	template <class Command>
	struct opWriteZPIndX
	    : cpu::Operation<WriteZPIndX01, WriteZPIndX02, WriteZPIndX03,
	          WriteZPIndX04<Command>, WriteZPIndX05, ParseNext> {};

	/* ZP Indirect Y */
	/* Read */
	struct ReadZPIndY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ReadZPIndY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_ZP;
		}
	};
	struct ReadZPIndY03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_OP16);
			cpu->setHigh(cpu->m_DB, &cpu->m_OP16);
			cpu->m_OP16 += cpu->m_Y;
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ReadZPIndY04 : cpu::Cycle {
		static bool skipCycle(CCPU *cpu) {
			return ((cpu->m_OP16 & 0xff00) == (cpu->m_Abs & 0xff00));
		}
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_Abs = cpu->m_OP16;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ReadZPIndY05 : cpu::Cycle {
		enum { AckIRQ = true };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Read ZP Indirect Y
	 */
	template <class Command>
	struct opReadZPIndY
	    : cpu::Operation<ReadZPIndY01, ReadZPIndY02, ReadZPIndY03, ReadZPIndY04,
	          ReadZPIndY05<Command>, ParseNext> {};
	/* Modify */
	struct ModifyZPIndY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct ModifyZPIndY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_ZP;
		}
	};
	struct ModifyZPIndY03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_OP16);
			cpu->setHigh(cpu->m_DB, &cpu->m_OP16);
			cpu->m_OP16 += cpu->m_Y;
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyZPIndY04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_Abs = cpu->m_OP16;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyZPIndY05 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct ModifyZPIndY06 : cpu::Cycle {
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct ModifyZPIndY07 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Modify ZP Indirect Y
	 */
	template <class Command>
	struct opModifyZPIndY
	    : cpu::Operation<ModifyZPIndY01, ModifyZPIndY02, ModifyZPIndY03,
	          ModifyZPIndY04, ModifyZPIndY05, ModifyZPIndY06<Command>,
	          ModifyZPIndY07, ParseNext> {};
	/* Write */
	struct WriteZPIndY01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_ZP = cpu->m_DB;
			++cpu->m_PC;
			cpu->m_AB = cpu->m_ZP;
		}
	};
	struct WriteZPIndY02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_OP = cpu->m_DB;
			cpu->m_AB = ++cpu->m_ZP;
		}
	};
	struct WriteZPIndY03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_OP, &cpu->m_OP16);
			cpu->setHigh(cpu->m_DB, &cpu->m_OP16);
			cpu->m_OP16 += cpu->m_Y;
			cpu->setLow(cpu->m_OP16 & 0xff, &cpu->m_Abs);
			cpu->setHigh(cpu->m_DB, &cpu->m_Abs);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	template <class Command>
	struct WriteZPIndY04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_Abs = cpu->m_OP16;
			Command::execute(cpu);
			cpu->m_AB = cpu->m_Abs;
		}
	};
	struct WriteZPIndY05 : cpu::Cycle {
		enum { AckIRQ = true };
		enum { BusMode = BusModeWrite };
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Write ZP Indirect Y
	 */
	template <class Command>
	struct opWriteZPIndY
	    : cpu::Operation<WriteZPIndY01, WriteZPIndY02, WriteZPIndY03,
	          WriteZPIndY04<Command>, WriteZPIndY05, ParseNext> {};

	/* RESET */
	struct Reset00 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	struct Reset01 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = cpu->m_PC;
		}
	};
	struct Reset02 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + cpu->m_S;
		}
	};
	struct Reset03 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + --cpu->m_S;
		}
	};
	struct Reset04 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->m_AB = 0x0100 + --cpu->m_S;
		}
	};
	struct Reset05 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			--cpu->m_S;
			cpu->m_AB = 0xfffc;
			cpu->m_PendingNMI = false;
		}
	};
	struct Reset06 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setLow(cpu->m_DB, &cpu->m_PC);
			cpu->m_Interrupt = CPUFlagInterrupt;
			cpu->m_PendingINT = false;
			cpu->m_AB = 0xfffd;
		}
	};
	struct Reset07 : cpu::Cycle {
		template <class Control>
		static void execute(CCPU *cpu) {
			cpu->setHigh(cpu->m_DB, &cpu->m_PC);
			cpu->m_AB = cpu->m_PC;
		}
	};
	/**
	 * Reset operation
	 */
	struct opReset : cpu::Operation<Reset00, Reset01, Reset02, Reset03, Reset04,
	                     Reset05, Reset06, Reset07, ParseNext> {};
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
	    cpu::Opcode<0xa9, opImm<cmdLDA>>, cpu::Opcode<0xa5, opReadZP<cmdLDA>>,
	    cpu::Opcode<0xb5, opReadZPX<cmdLDA>>,
	    cpu::Opcode<0xad, opReadAbs<cmdLDA>>,
	    cpu::Opcode<0xbd, opReadAbsX<cmdLDA>>,
	    cpu::Opcode<0xb9, opReadAbsY<cmdLDA>>,
	    cpu::Opcode<0xa1, opReadZPXInd<cmdLDA>>,
	    cpu::Opcode<0xb1, opReadZPIndY<cmdLDA>>,
	    // STA
	    cpu::Opcode<0x85, opWriteZP<cmdSTA>>,
	    cpu::Opcode<0x95, opWriteZPX<cmdSTA>>,
	    cpu::Opcode<0x8d, opWriteAbs<cmdSTA>>,
	    cpu::Opcode<0x9d, opWriteAbsX<cmdSTA>>,
	    cpu::Opcode<0x99, opWriteAbsY<cmdSTA>>,
	    cpu::Opcode<0x81, opWriteZPXInd<cmdSTA>>,
	    cpu::Opcode<0x91, opWriteZPIndY<cmdSTA>>,
	    // LDX
	    cpu::Opcode<0xa2, opImm<cmdLDX>>, cpu::Opcode<0xa6, opReadZP<cmdLDX>>,
	    cpu::Opcode<0xb6, opReadZPY<cmdLDX>>,
	    cpu::Opcode<0xae, opReadAbs<cmdLDX>>,
	    cpu::Opcode<0xbe, opReadAbsY<cmdLDX>>,
	    // STX
	    cpu::Opcode<0x86, opWriteZP<cmdSTX>>,
	    cpu::Opcode<0x96, opWriteZPY<cmdSTX>>,
	    cpu::Opcode<0x8e, opWriteAbs<cmdSTX>>,
	    // LDY
	    cpu::Opcode<0xa0, opImm<cmdLDY>>, cpu::Opcode<0xa4, opReadZP<cmdLDY>>,
	    cpu::Opcode<0xb4, opReadZPX<cmdLDY>>,
	    cpu::Opcode<0xac, opReadAbs<cmdLDY>>,
	    cpu::Opcode<0xbc, opReadAbsX<cmdLDY>>,
	    // STY
	    cpu::Opcode<0x84, opWriteZP<cmdSTY>>,
	    cpu::Opcode<0x94, opWriteZPX<cmdSTY>>,
	    cpu::Opcode<0x8c, opWriteAbs<cmdSTY>>,

	    // Arithmetic

	    // ADC
	    cpu::Opcode<0x69, opImm<cmdADC>>, cpu::Opcode<0x65, opReadZP<cmdADC>>,
	    cpu::Opcode<0x75, opReadZPX<cmdADC>>,
	    cpu::Opcode<0x6d, opReadAbs<cmdADC>>,
	    cpu::Opcode<0x7d, opReadAbsX<cmdADC>>,
	    cpu::Opcode<0x79, opReadAbsY<cmdADC>>,
	    cpu::Opcode<0x61, opReadZPXInd<cmdADC>>,
	    cpu::Opcode<0x71, opReadZPIndY<cmdADC>>,
	    // SBC
	    cpu::Opcode<0xe9, opImm<cmdSBC>>, cpu::Opcode<0xe5, opReadZP<cmdSBC>>,
	    cpu::Opcode<0xf5, opReadZPX<cmdSBC>>,
	    cpu::Opcode<0xed, opReadAbs<cmdSBC>>,
	    cpu::Opcode<0xfd, opReadAbsX<cmdSBC>>,
	    cpu::Opcode<0xf9, opReadAbsY<cmdSBC>>,
	    cpu::Opcode<0xe1, opReadZPXInd<cmdSBC>>,
	    cpu::Opcode<0xf1, opReadZPIndY<cmdSBC>>,
	    cpu::Opcode<0xeb, opImm<cmdSBC>>,

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
	    cpu::Opcode<0xe6, opModifyZP<cmdINC>>,
	    cpu::Opcode<0xf6, opModifyZPX<cmdINC>>,
	    cpu::Opcode<0xee, opModifyAbs<cmdINC>>,
	    cpu::Opcode<0xfe, opModifyAbsX<cmdINC>>,
	    // DEC
	    cpu::Opcode<0xc6, opModifyZP<cmdDEC>>,
	    cpu::Opcode<0xd6, opModifyZPX<cmdDEC>>,
	    cpu::Opcode<0xce, opModifyAbs<cmdDEC>>,
	    cpu::Opcode<0xde, opModifyAbsX<cmdDEC>>,

	    // Shifts

	    // ASL
	    cpu::Opcode<0x0a, opImp<cmdASLA>>,
	    cpu::Opcode<0x06, opModifyZP<cmdASL>>,
	    cpu::Opcode<0x16, opModifyZPX<cmdASL>>,
	    cpu::Opcode<0x0e, opModifyAbs<cmdASL>>,
	    cpu::Opcode<0x1e, opModifyAbsX<cmdASL>>,
	    // LSR
	    cpu::Opcode<0x4a, opImp<cmdLSRA>>,
	    cpu::Opcode<0x46, opModifyZP<cmdLSR>>,
	    cpu::Opcode<0x56, opModifyZPX<cmdLSR>>,
	    cpu::Opcode<0x4e, opModifyAbs<cmdLSR>>,
	    cpu::Opcode<0x5e, opModifyAbsX<cmdLSR>>,
	    // ROL
	    cpu::Opcode<0x2a, opImp<cmdROLA>>,
	    cpu::Opcode<0x26, opModifyZP<cmdROL>>,
	    cpu::Opcode<0x36, opModifyZPX<cmdROL>>,
	    cpu::Opcode<0x2e, opModifyAbs<cmdROL>>,
	    cpu::Opcode<0x3e, opModifyAbsX<cmdROL>>,
	    // ROR
	    cpu::Opcode<0x6a, opImp<cmdRORA>>,
	    cpu::Opcode<0x66, opModifyZP<cmdROR>>,
	    cpu::Opcode<0x76, opModifyZPX<cmdROR>>,
	    cpu::Opcode<0x6e, opModifyAbs<cmdROR>>,
	    cpu::Opcode<0x7e, opModifyAbsX<cmdROR>>,

	    // Logic

	    // AND
	    cpu::Opcode<0x29, opImm<cmdAND>>, cpu::Opcode<0x25, opReadZP<cmdAND>>,
	    cpu::Opcode<0x35, opReadZPX<cmdAND>>,
	    cpu::Opcode<0x2d, opReadAbs<cmdAND>>,
	    cpu::Opcode<0x3d, opReadAbsX<cmdAND>>,
	    cpu::Opcode<0x39, opReadAbsY<cmdAND>>,
	    cpu::Opcode<0x21, opReadZPXInd<cmdAND>>,
	    cpu::Opcode<0x31, opReadZPIndY<cmdAND>>,
	    // ORA
	    cpu::Opcode<0x09, opImm<cmdORA>>, cpu::Opcode<0x05, opReadZP<cmdORA>>,
	    cpu::Opcode<0x15, opReadZPX<cmdORA>>,
	    cpu::Opcode<0x0d, opReadAbs<cmdORA>>,
	    cpu::Opcode<0x1d, opReadAbsX<cmdORA>>,
	    cpu::Opcode<0x19, opReadAbsY<cmdORA>>,
	    cpu::Opcode<0x01, opReadZPXInd<cmdORA>>,
	    cpu::Opcode<0x11, opReadZPIndY<cmdORA>>,
	    // EOR
	    cpu::Opcode<0x49, opImm<cmdEOR>>, cpu::Opcode<0x45, opReadZP<cmdEOR>>,
	    cpu::Opcode<0x55, opReadZPX<cmdEOR>>,
	    cpu::Opcode<0x4d, opReadAbs<cmdEOR>>,
	    cpu::Opcode<0x5d, opReadAbsX<cmdEOR>>,
	    cpu::Opcode<0x59, opReadAbsY<cmdEOR>>,
	    cpu::Opcode<0x41, opReadZPXInd<cmdEOR>>,
	    cpu::Opcode<0x51, opReadZPIndY<cmdEOR>>,

	    // Comparison

	    // CMP
	    cpu::Opcode<0xc9, opImm<cmdCMP>>, cpu::Opcode<0xc5, opReadZP<cmdCMP>>,
	    cpu::Opcode<0xd5, opReadZPX<cmdCMP>>,
	    cpu::Opcode<0xcd, opReadAbs<cmdCMP>>,
	    cpu::Opcode<0xdd, opReadAbsX<cmdCMP>>,
	    cpu::Opcode<0xd9, opReadAbsY<cmdCMP>>,
	    cpu::Opcode<0xc1, opReadZPXInd<cmdCMP>>,
	    cpu::Opcode<0xd1, opReadZPIndY<cmdCMP>>,
	    // CPX
	    cpu::Opcode<0xe0, opImm<cmdCPX>>, cpu::Opcode<0xe4, opReadZP<cmdCPX>>,
	    cpu::Opcode<0xec, opReadAbs<cmdCPX>>,
	    // CPY
	    cpu::Opcode<0xc0, opImm<cmdCPY>>, cpu::Opcode<0xc4, opReadZP<cmdCPY>>,
	    cpu::Opcode<0xcc, opReadAbs<cmdCPY>>,
	    // BIT
	    cpu::Opcode<0x24, opReadZP<cmdBIT>>,
	    cpu::Opcode<0x2c, opReadAbs<cmdBIT>>,

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
	    cpu::Opcode<0xe2, opImm<cpu::Command>>,
	    cpu::Opcode<0x04, opReadZP<cpu::Command>>,
	    cpu::Opcode<0x44, opReadZP<cpu::Command>>,
	    cpu::Opcode<0x64, opReadZP<cpu::Command>>,
	    cpu::Opcode<0x14, opReadZPX<cpu::Command>>,
	    cpu::Opcode<0x34, opReadZPX<cpu::Command>>,
	    cpu::Opcode<0x54, opReadZPX<cpu::Command>>,
	    cpu::Opcode<0x74, opReadZPX<cpu::Command>>,
	    cpu::Opcode<0xd4, opReadZPX<cpu::Command>>,
	    cpu::Opcode<0xf4, opReadZPX<cpu::Command>>,
	    cpu::Opcode<0x0c, opReadAbs<cpu::Command>>,
	    cpu::Opcode<0x1c, opReadAbsX<cpu::Command>>,
	    cpu::Opcode<0x3c, opReadAbsX<cpu::Command>>,
	    cpu::Opcode<0x5c, opReadAbsX<cpu::Command>>,
	    cpu::Opcode<0x7c, opReadAbsX<cpu::Command>>,
	    cpu::Opcode<0xdc, opReadAbsX<cpu::Command>>,
	    cpu::Opcode<0xfc, opReadAbsX<cpu::Command>>,

	    // Undocumented

	    // SLO
	    cpu::Opcode<0x03, opModifyZPXInd<cmdSLO>>,
	    cpu::Opcode<0x07, opModifyZP<cmdSLO>>,
	    cpu::Opcode<0x0f, opModifyAbs<cmdSLO>>,
	    cpu::Opcode<0x13, opModifyZPIndY<cmdSLO>>,
	    cpu::Opcode<0x17, opModifyZPX<cmdSLO>>,
	    cpu::Opcode<0x1b, opModifyAbsY<cmdSLO>>,
	    cpu::Opcode<0x1f, opModifyAbsX<cmdSLO>>,
	    // RLA
	    cpu::Opcode<0x23, opModifyZPXInd<cmdRLA>>,
	    cpu::Opcode<0x27, opModifyZP<cmdRLA>>,
	    cpu::Opcode<0x2f, opModifyAbs<cmdRLA>>,
	    cpu::Opcode<0x33, opModifyZPIndY<cmdRLA>>,
	    cpu::Opcode<0x37, opModifyZPX<cmdRLA>>,
	    cpu::Opcode<0x3b, opModifyAbsY<cmdRLA>>,
	    cpu::Opcode<0x3f, opModifyAbsX<cmdRLA>>,
	    // SRE
	    cpu::Opcode<0x43, opModifyZPXInd<cmdSRE>>,
	    cpu::Opcode<0x47, opModifyZP<cmdSRE>>,
	    cpu::Opcode<0x4f, opModifyAbs<cmdSRE>>,
	    cpu::Opcode<0x53, opModifyZPIndY<cmdSRE>>,
	    cpu::Opcode<0x57, opModifyZPX<cmdSRE>>,
	    cpu::Opcode<0x5b, opModifyAbsY<cmdSRE>>,
	    cpu::Opcode<0x5f, opModifyAbsX<cmdSRE>>,
	    // RRA
	    cpu::Opcode<0x63, opModifyZPXInd<cmdRRA>>,
	    cpu::Opcode<0x67, opModifyZP<cmdRRA>>,
	    cpu::Opcode<0x6f, opModifyAbs<cmdRRA>>,
	    cpu::Opcode<0x73, opModifyZPIndY<cmdRRA>>,
	    cpu::Opcode<0x77, opModifyZPX<cmdRRA>>,
	    cpu::Opcode<0x7b, opModifyAbsY<cmdRRA>>,
	    cpu::Opcode<0x7f, opModifyAbsX<cmdRRA>>,
	    // SAX
	    cpu::Opcode<0x83, opWriteZPXInd<cmdSAX>>,
	    cpu::Opcode<0x87, opWriteZP<cmdSAX>>,
	    cpu::Opcode<0x8f, opWriteAbs<cmdSAX>>,
	    cpu::Opcode<0x97, opWriteZPY<cmdSAX>>,
	    // LAX
	    cpu::Opcode<0xa3, opReadZPXInd<cmdLAX>>,
	    cpu::Opcode<0xa7, opReadZP<cmdLAX>>,
	    cpu::Opcode<0xaf, opReadAbs<cmdLAX>>,
	    cpu::Opcode<0xb3, opReadZPIndY<cmdLAX>>,
	    cpu::Opcode<0xb7, opReadZPY<cmdLAX>>,
	    cpu::Opcode<0xbf, opReadAbsY<cmdLAX>>, cpu::Opcode<0xab, opImm<cmdLAX>>,
	    // DCP
	    cpu::Opcode<0xc3, opModifyZPXInd<cmdDCP>>,
	    cpu::Opcode<0xc7, opModifyZP<cmdDCP>>,
	    cpu::Opcode<0xcf, opModifyAbs<cmdDCP>>,
	    cpu::Opcode<0xd3, opModifyZPIndY<cmdDCP>>,
	    cpu::Opcode<0xd7, opModifyZPX<cmdDCP>>,
	    cpu::Opcode<0xdb, opModifyAbsY<cmdDCP>>,
	    cpu::Opcode<0xdf, opModifyAbsX<cmdDCP>>,
	    // ISC
	    cpu::Opcode<0xe3, opModifyZPXInd<cmdISC>>,
	    cpu::Opcode<0xe7, opModifyZP<cmdISC>>,
	    cpu::Opcode<0xef, opModifyAbs<cmdISC>>,
	    cpu::Opcode<0xf3, opModifyZPIndY<cmdISC>>,
	    cpu::Opcode<0xf7, opModifyZPX<cmdISC>>,
	    cpu::Opcode<0xfb, opModifyAbsY<cmdISC>>,
	    cpu::Opcode<0xff, opModifyAbsX<cmdISC>>,
	    // ANC
	    cpu::Opcode<0x0b, opImm<cmdANC>>, cpu::Opcode<0x2b, opImm<cmdANC>>,
	    // ALR
	    cpu::Opcode<0x4b, opImm<cmdALR>>,
	    // ARR
	    cpu::Opcode<0x6b, opImm<cmdARR>>,
	    // XAA
	    cpu::Opcode<0x8b, opImm<cmdXAA>>,
	    // AXS
	    cpu::Opcode<0xcb, opImm<cmdAXS>>,
	    // SHA
	    cpu::Opcode<0x9f, opWriteAbsY<cmdSHA>>,
	    cpu::Opcode<0x93, opWriteZPIndY<cmdSHA>>,
	    // SHY
	    cpu::Opcode<0x9c, opWriteAbsX<cmdSHY>>,
	    // SHX
	    cpu::Opcode<0x9e, opWriteAbsY<cmdSHX>>,
	    // TAS
	    cpu::Opcode<0x9b, opWriteAbsY<cmdTAS>>,
	    // LAS
	    cpu::Opcode<0xbb, opReadAbsY<cmdLAS>>

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
	static void setEndPoint(CCPU *cpu, std::size_t index) {
		cpu->m_CurrentIndex = index;
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
		if (!cpu->isReady()) {
			return false;
		}
		if (ackIRQ) {
			cpu->processInterrupts();
		}
		switch (busMode) {
		case BusModeRead:
			cpu->m_DB = cpu->m_MotherBoard->getBusCPU()->readMemory(cpu->m_AB);
			break;
		case BusModeWrite:
			cpu->m_MotherBoard->getBusCPU()->writeMemory(cpu->m_DB, cpu->m_AB);
			break;
		}
		// TODO(me) : Use CPU divider
		cpu->m_InternalClock += 12;
		return true;
	}
};

/* CCPU */

/**
 * Constructs the object
 *
 * @param motherBoard Motherboard
 */
CCPU::CCPU(CMotherBoard *motherBoard)
    : CClockedDevice()
    , m_MotherBoard(motherBoard)
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
		opcodes::control::execute(this, m_CurrentIndex);
	}
}

}  // namespace core

}  // namespace vpnes
