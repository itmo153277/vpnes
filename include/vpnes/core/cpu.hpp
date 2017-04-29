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
	struct CPUConfig : public BusConfigBase<CCPU> {
		/**
		 * Banks config
		 */
		typedef banks::BankConfig<banks::ReadWrite<0x0000, 0x2000, 0x0800>>
		    BankConfig;

		/**
		 * Maps to read map
		 *
		 * @param iter Read map iterator
		 * @param openBus Open bus
		 * @param device Device
		 */
		static void mapRead(
		    MemoryMap::iterator iter, std::uint8_t *openBus, CCPU &device) {
			BankConfig::mapRead(iter, openBus, device.m_RAM);
		}
		/**
		 * Maps to write map
		 *
		 * @param iter Write map iterator
		 * @param dummy Dummy value
		 * @param device Device
		 */
		static void mapWrite(
		    MemoryMap::iterator iter, std::uint8_t *dummy, CCPU &device) {
			BankConfig::mapWrite(iter, dummy, device.m_RAM);
		}
		/**
		 * Maps to mod map
		 *
		 * @param iter Mod map iterator
		 * @param writeBuf Write buffer
		 * @param device Device
		 */
		static void mapMod(
		    MemoryMap::iterator iter, std::uint8_t *writeBuf, CCPU &device) {
			BankConfig::mapMod(iter, writeBuf, device.m_RAM);
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

private:
	/**
	 * CPU RAM
	 */
	std::uint8_t m_RAM[0x0800];

protected:
	/**
	 * Simulation routine
	 */
	void execute() {
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
	CCPU(CMotherBoard &motherBoard) : CEventDevice() {
	}
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
		return 0;
	}
};

} /* core */

} /* vpnes */

#endif /* VPNES_INCLUDE_CORE_CPU_HPP_ */
