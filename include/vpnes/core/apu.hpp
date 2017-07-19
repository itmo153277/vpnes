/**
 * @file
 *
 * Defines basic APU
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

#ifndef VPNES_INCLUDE_CORE_APU_HPP_
#define VPNES_INCLUDE_CORE_APU_HPP_

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
 * Basic APU
 */
class CAPU : public CEventDevice {
public:
	/**
	 * CPU bus config
	 */
	struct CPUConfig : BusConfigBase<CAPU> {
		/**
		 * Banks config
		 */
		typedef banks::BankConfig<banks::ReadWrite<0x4000, 0x0020, 1>>
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
		    CAPU &device) {
			BankConfig::mapIO(iterRead, iterWrite, iterMod, openBus, dummy,
			    writeBuf, &device.m_IOBuf);
		}
		/**
		 * Checks if device is enabled
		 *
		 * @param addr Address
		 * @return True if enabled
		 */
		static bool isDeviceEnabled(std::uint16_t addr) {
			return (addr >= 0x4000) && (addr < 0x4020);
		}
		/**
		 * Determines active bank
		 *
		 * @param addr Address
		 * @param device Device
		 * @return Active bank
		 */
		static std::size_t getBank(std::uint16_t addr, CAPU &device) {
			return 0;
		}
	};

private:
	/**
	 * IO Buffer
	 */
	std::uint8_t m_IOBuf;

	/**
	 * Reads register
	 *
	 * @param addr register address
	 */
	void readReg(std::uint16_t addr) {
	}
	/**
	 * Writes to register
	 *
	 * @param val Value
	 * @param addr Address
	 */
	void writeReg(std::uint8_t val, std::uint16_t addr) {
	}

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
	CAPU() = delete;
	/**
	 * Constructs the object
	 *
	 * @param motherBoard Motherboard
	 */
	CAPU(CMotherBoard &motherBoard) : CEventDevice(), m_IOBuf() {
	}
	/**
	 * Destroys the object
	 */
	~CAPU() = default;

	/**
	 * Adds CPU hooks
	 *
	 * @param bus CPU bus
	 */
	void addHooksCPU(CBus &bus) {
		for (std::uint16_t addr = 0x4000; addr < 0x4020; addr++) {
			bus.addPreReadHook(addr, *this, &CAPU::readReg);
			bus.addWriteHook(addr, *this, &CAPU::writeReg);
		}
	}

	/**
	 * Gets pending time
	 *
	 * @return Pending time
	 */
	ticks_t getPending() const {
		return 0;
	}
};

}  // namespace core

}  // namespace vpnes

#endif /* VPNES_INCLUDE_CORE_APU_HPP_ */
