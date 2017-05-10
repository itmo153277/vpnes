/**
 * @file
 *
 * Defines basic CPU
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

#ifndef VPNES_INCLUDE_CORE_CPU_HPP_
#define VPNES_INCLUDE_CORE_CPU_HPP_

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
class CCPU : public CEventDevice {
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
		    CCPU &device) {
			BankConfig::mapIO(iterRead, iterWrite, iterMod, openBus, dummy,
			    writeBuf, device.m_RAM);
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
		static std::size_t getBank(std::uint16_t addr, CCPU &device) {
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
	 * Address bus
	 */
	std::uint16_t m_AB;
	/**
	 * Data bus
	 */
	std::uint8_t m_DB;

	// TODO: Define all status registers

	/**
	 * Checks if can execute
	 *
	 * @return
	 */
	bool isReady() {
		return m_InternalClock < m_Clock;
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
		CEventDevice::resetClock(ticks);
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
	CCPU(CMotherBoard &motherBoard);
	/**
	 * Destroys the object
	 */
	~CCPU() = default;

	/**
	 * Adds CPU hooks
	 *
	 * @param bus CPU bus
	 */
	void addHooksCPU(CBus &bus) {
	}

	/**
	 * Gets pending time
	 *
	 * @return Pending time
	 */
	ticks_t getPending() {
		return m_InternalClock;
	}
};

}  // namespace core

}  // namespace vpnes

#endif /* VPNES_INCLUDE_CORE_CPU_HPP_ */
