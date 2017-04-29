/**
 * @file
 *
 * Defines basic PPU
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

#ifndef VPNES_INCLUDE_CORE_PPU_HPP_
#define VPNES_INCLUDE_CORE_PPU_HPP_

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
 * Basic PPU
 */
class CPPU : public CEventDevice {
public:
	/**
	 * CPU bus config
	 */
	struct CPUConfig : public BusConfigBase<CPPU> {
		/**
		 * Banks config
		 */
		typedef banks::BankConfig<banks::ReadWrite<0x2000, 0x2000, 1>>
		    BankConfig;

		/**
		 * Maps to read map
		 *
		 * @param iter Read map iterator
		 * @param openBus Open bus
		 * @param device Device
		 */
		static void mapRead(
		    MemoryMap::iterator iter, std::uint8_t *openBus, CPPU &device) {
			BankConfig::mapRead(iter, openBus, &device.m_IOBuf);
		}
		/**
		 * Maps to write map
		 *
		 * @param iter Write map iterator
		 * @param dummy Dummy value
		 * @param device Device
		 */
		static void mapWrite(
		    MemoryMap::iterator iter, std::uint8_t *dummy, CPPU &device) {
			BankConfig::mapWrite(iter, dummy, &device.m_IOBuf);
		}
		/**
		 * Maps to mod map
		 *
		 * @param iter Mod map iterator
		 * @param writeBuf Write buffer
		 * @param device Device
		 */
		static void mapMod(
		    MemoryMap::iterator iter, std::uint8_t *writeBuf, CPPU &device) {
			BankConfig::mapMod(iter, writeBuf, &device.m_IOBuf);
		}
		/**
		 * Checks if device is enabled
		 *
		 * @param addr Address
		 * @return True if enabled
		 */
		static bool isDeviceEnabled(std::uint16_t addr) {
			return (addr >= 0x2000) && (addr < 0x4000);
		}
		/**
		 * Determines active bank
		 *
		 * @param addr Address
		 * @param device Device
		 * @return Active bank
		 */
		static std::size_t getBank(std::uint16_t addr, CPPU &device) {
			return 0;
		}
	};
	/**
	 * PPU bus config
	 */
	struct PPUConfig : public BusConfigBase<CPPU> {
		/**
		 * Banks config
		 */
		typedef banks::BankConfig<banks::ReadWrite<0x3f00, 0x2000, 0x0020>>
		    BankConfig;

		/**
		 * Maps to read map
		 *
		 * @param iter Read map iterator
		 * @param openBus Open bus
		 * @param device Device
		 */
		static void mapRead(
		    MemoryMap::iterator iter, std::uint8_t *openBus, CPPU &device) {
			BankConfig::mapRead(iter, openBus, device.m_Palette);
		}
		/**
		 * Maps to write map
		 *
		 * @param iter Write map iterator
		 * @param dummy Dummy value
		 * @param device Device
		 */
		static void mapWrite(
		    MemoryMap::iterator iter, std::uint8_t *dummy, CPPU &device) {
			BankConfig::mapWrite(iter, dummy, device.m_Palette);
		}
		/**
		 * Maps to mod map
		 *
		 * @param iter Mod map iterator
		 * @param writeBuf Write buffer
		 * @param device Device
		 */
		static void mapMod(
		    MemoryMap::iterator iter, std::uint8_t *writeBuf, CPPU &device) {
			BankConfig::mapMod(iter, writeBuf, device.m_Palette);
		}
		/**
		 * Checks if device is enabled
		 *
		 * @param addr Address
		 * @return True if enabled
		 */
		static bool isDeviceEnabled(std::uint16_t addr) {
			return (addr & 0x3f00);
		}
		/**
		 * Determines active bank
		 *
		 * @param addr Address
		 * @param device Device
		 * @return Active bank
		 */
		static bool getBank(std::uint16_t addr, CPPU &device) {
			return 0;
		}
	};

private:
	/**
	 * Motherboard
	 */
	CMotherBoard *m_MotherBoard;
	/**
	 * Palette
	 */
	std::uint8_t m_Palette[0x0020];
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
	CPPU() = delete;
	/**
	 * Constructs the object
	 *
	 * @param motherBoard Motherboard
	 */
	CPPU(CMotherBoard &motherBoard)
	    : CEventDevice(), m_MotherBoard(&motherBoard), m_IOBuf() {
	}
	/**
	 * Destroys the object
	 */
	~CPPU() = default;

	/**
	 * Adds CPU hooks
	 *
	 * @param bus CPU bus
	 */
	void addHooksCPU(CBus &bus) {
		for (std::size_t addr = 0x2000; addr < 0x4000; addr++) {
			bus.addPreReadHook(addr, *this, &CPPU::readReg);
			bus.addWriteHook(addr, *this, &CPPU::writeReg);
		}
	}
	/**
	 * Adds PPU hooks
	 *
	 * @param bus PPU bus
	 */
	void addHooksPPU(CBus &bus) {
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

#endif /* VPNES_INCLUDE_CORE_PPU_HPP_ */
