/**
 * @file
 *
 * Defines basic CPU
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

#ifndef INCLUDE_VPNES_CORE_CPU_HPP_
#define INCLUDE_VPNES_CORE_CPU_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstddef>
#include <cstdint>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/device.hpp>
#include <vpnes/core/bus.hpp>
#include <vpnes/core/mboard.hpp>

namespace vpnes {

namespace core {

/**
 * Basic CPU
 */
class CCPU : public CClockedDevice {
public:
	/**
	 * CPU bus config
	 */
	struct CPUConfig : BusConfigBase<CCPU> {
		/**
		 * Banks config
		 */
		typedef banks::BankConfig<banks::ReadWrite<0x0000, 0x2000, 0x0800>>
		    BankConfig;

		/**
		 * Maps IO
		 *
		 * @param iterRead Read iterator
		 * @param iterWrite Write iterator
		 * @param iterMod Mod iterator
		 * @param openBus Open bus
		 * @param dummy Dummy write
		 * @param writeBuf Write bus
		 * @param device Device
		 */
		static void mapIO(MemoryMap::iterator iterRead,
		    MemoryMap::iterator iterWrite, MemoryMap::iterator iterMod,
		    std::uint8_t *openBus, std::uint8_t *dummy, std::uint8_t *writeBuf,
		    CCPU *device) {
			BankConfig::mapIO(iterRead, iterWrite, iterMod, openBus, dummy,
			    writeBuf, device->m_RAM);
		}
		/**
		 * Checks if device is enabled
		 *
		 * @param addr Address
		 * @return True if enabled
		 */
		static bool isDeviceEnabled(std::uint16_t addr) {
			return (addr < 0x2000);
		}
		/**
		 * Determines active bank
		 *
		 * @param addr Address
		 * @param device Device
		 * @return Active bank
		 */
		static std::size_t getBank(std::uint16_t addr, const CCPU &device) {
			return 0;
		}
	};
	/**
	 * Bus mode
	 */
	enum EBusMode {
		BusModeRead,  //!< Read from bus
		BusModeWrite  //!< Write to bus
	};

private:
	/**
	 * Internal opcodes implementation
	 */
	struct opcodes;
	/**
	 * Motherboard
	 */
	CMotherBoard *m_MotherBoard;
	/**
	 * Internal clock
	 */
	ticks_t m_InternalClock;
	/**
	 * Current index in compiled microcode
	 */
	std::size_t m_CurrentIndex;
	/**
	 * CPU RAM
	 */
	std::uint8_t m_RAM[0x0800];
	/**
	 * Pending IRQ
	 */
	bool m_PendingIRQ;
	/**
	 * Pending NMI
	 */
	bool m_PendingNMI;
	/**
	 * Pending INT (IRQ && I || NMI)
	 */
	bool m_PendingINT;
	/**
	 * Address bus
	 */
	std::uint16_t m_AB;
	/**
	 * Data bus
	 */
	std::uint8_t m_DB;
	/**
	 * PC
	 */
	std::uint16_t m_PC;
	/**
	 * Stack
	 */
	std::uint8_t m_S;
	/**
	 * A
	 */
	std::uint8_t m_A;
	/**
	 * X
	 */
	std::uint8_t m_X;
	/**
	 * Y
	 */
	std::uint8_t m_Y;
	/**
	 * Operand
	 */
	std::uint8_t m_OP;
	/**
	 * 16-bit operand
	 */
	std::uint16_t m_OP16;
	/**
	 * Absolute address
	 */
	std::uint16_t m_Abs;
	/**
	 * Zero-page address
	 */
	std::uint8_t m_ZP;
	/**
	 * Flag to check if we jump by branch
	 */
	bool m_BranchTaken;
	/**
	 * Negative flag
	 */
	int m_Negative;
	/**
	 * Overflow flag
	 */
	int m_Overflow;
	/**
	 * Decimal flag
	 */
	int m_Decimal;
	/**
	 * Interrupt flag
	 */
	int m_Interrupt;
	/**
	 * Zero flag
	 */
	int m_Zero;
	/**
	 * Carry flag
	 */
	int m_Carry;
	/**
	 * Default flag values
	 */
	enum {
		CPUFlagNegative = 0x80,   //!< Negative
		CPUFlagOverflow = 0x40,   //!< Overflow
		CPUFlagBreak = 0x10,      //!< Break
		CPUFlagDecimal = 0x08,    //!< Decimal
		CPUFlagInterrupt = 0x04,  //!< Interrupt
		CPUFlagZero = 0x02,       //!< Zero
		CPUFlagCarry = 0x01       //!< Carry
	};

	// TODO(me): Define all status registers

	/**
	 * Checks if can execute
	 *
	 * @return
	 */
	bool isReady() {
		return m_InternalClock < m_Clock;
	}
	/**
	 * Packs flags
	 *
	 * @return Packed state
	 */
	std::uint8_t packState() {
		std::uint8_t state = 0x20;
		state |= m_Negative & CPUFlagNegative;
		state |= m_Overflow;
		state |= m_Decimal;
		state |= m_Zero;
		state |= m_Carry;
		return state;
	}
	/**
	 * Unpacks flags from state
	 *
	 * @param state Packed state
	 */
	void unpackState(std::uint8_t state) {
		m_Negative = state;
		m_Overflow = state & CPUFlagOverflow;
		m_Decimal = state & CPUFlagDecimal;
		m_Interrupt = state & CPUFlagInterrupt;
		m_Zero = state & CPUFlagZero;
		m_Carry = state & CPUFlagCarry;
	}
	/**
	 * Sets negative
	 *
	 * @param s Value
	 */
	void setNegativeFlag(std::uint16_t s) {
		m_Negative = s;
	}
	/**
	 * Sets overflow
	 *
	 * @param flag Flag value
	 */
	void setOverflowFlag(bool flag) {
		m_Overflow = flag ? CPUFlagOverflow : 0;
	}
	/**
	 * Sets zero
	 *
	 * @param s Value
	 */
	void setZeroFlag(std::uint8_t s) {
		m_Zero = (s == 0) ? CPUFlagZero : 0;
	}
	/**
	 * Set carry
	 *
	 * @param flag Flag value
	 */
	void setCarryFlag(bool flag) {
		m_Carry = flag ? CPUFlagCarry : 0;
	}
	/**
	 * Sets low byte
	 *
	 * @param s Source
	 * @param d Destination
	 */
	void setLow(std::uint8_t s, std::uint16_t *d) {
		*d &= 0xff00;
		*d |= s;
	}
	/**
	 * Sets high byte
	 *
	 * @param s Source
	 * @param d Destination
	 */
	void setHigh(std::uint8_t s, std::uint16_t *d) {
		*d &= 0x00ff;
		*d |= s << 8;
	}
	/**
	 * Processes interrupts
	 */
	void processInterrupts() {
		// TODO(me) : Update interrupt flags
	}

protected:
	/**
	 * Simulation routine
	 */
	void execute();
	/**
	 * Resets the clock by ticks amount
	 *
	 * @param ticks Amount of ticks
	 */
	void resetClock(ticks_t ticks) {
		m_InternalClock -= ticks;
	}

public:
	/**
	 * Deleted default constructor
	 */
	CCPU() = delete;
	/**
	 * Constructs the object
	 *
	 * @param motherBoard Motherboard
	 */
	explicit CCPU(CMotherBoard *motherBoard);
	/**
	 * Destroys the object
	 */
	~CCPU() = default;

	/**
	 * Adds CPU hooks
	 *
	 * @param bus CPU bus
	 */
	void addHooksCPU(CBus *bus) {
	}

	/**
	 * Gets pending time
	 *
	 * @return Pending time
	 */
	ticks_t getPending() const {
		return m_InternalClock;
	}
};

}  // namespace core

}  // namespace vpnes

#endif  // INCLUDE_VPNES_CORE_CPU_HPP_
