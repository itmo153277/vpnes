/**
 * @file
 *
 * Defines NROM MMC
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

#ifndef VPNES_INCLUDE_CORE_MAPPERS_NROM_HPP_
#define VPNES_INCLUDE_CORE_MAPPERS_NROM_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/device.hpp>
#include <vpnes/core/bus.hpp>
#include <vpnes/core/mboard.hpp>
#include <vpnes/core/config.hpp>

namespace vpnes {

namespace core {

/**
 * NROM mapper
 */
class CNROM : public CEventDevice {
public:
	/**
	 * CPU bus config
	 */
	struct CPUConfig : public BusConfigBase<CNROM> {
		/**
		 * Banks config
		 */
		typedef banks::BankConfig<banks::OpenBus,
		    banks::ReadWrite<0x6000, 0x0800,
		        0x0800>,  // PRG RAM 0x6000 - 0x67ff
		    banks::ReadWrite<0x6200, 0x0800,
		        0x0800>,  // PRG RAM 0x6800 - 0x6fff
		    banks::ReadWrite<0x6000, 0x0800,
		        0x0800>,  // PRG RAM 0x7000 - 0x77ff
		    banks::ReadWrite<0x6000, 0x0800,
		        0x0800>,  // PRG RAM 0x7800 - 0x7fff
		    banks::ReadOnlyWithConflict<0x8000, 0x4000,
		        0x4000>,  // PRG ROM 0x8000 - 0xbfff
		    banks::ReadOnlyWithConflict<0xc000, 0x4000,
		        0x4000>>  // PRG ROM 0xc000 - 0xffff
		    BankConfig;

		/**
		 * Maps to read map
		 *
		 * @param iter Read map iterator
		 * @param openBus Open bus
		 * @param device Device
		 */
		static void mapRead(
		    MemoryMap::iterator iter, std::uint8_t *openBus, CNROM &device) {
			BankConfig::mapRead(iter, openBus, nullptr, device.m_RAM.data(),
			    device.m_RAM.data() + (0x0800 & (device.m_RAM.size() - 1)),
			    device.m_RAM.data() + (0x1000 & (device.m_RAM.size() - 1)),
			    device.m_RAM.data() + (0x1800 & (device.m_RAM.size() - 1)),
			    device.m_PRG.data(),
			    device.m_PRG.data() + (0x4000 & (device.m_PRG.size() - 1)));
		}
		/**
		 * Maps to write map
		 *
		 * @param iter Write map iterator
		 * @param dummy Dummy value
		 * @param device Device
		 */
		static void mapWrite(
		    MemoryMap::iterator iter, std::uint8_t *dummy, CNROM &device) {
			BankConfig::mapWrite(iter, dummy, nullptr, device.m_RAM.data(),
			    device.m_RAM.data() + (0x0800 & (device.m_RAM.size() - 1)),
			    device.m_RAM.data() + (0x1000 & (device.m_RAM.size() - 1)),
			    device.m_RAM.data() + (0x1800 & (device.m_RAM.size() - 1)),
			    device.m_PRG.data(),
			    device.m_PRG.data() + (0x4000 & (device.m_PRG.size() - 1)));
		}
		/**
		 * Maps to mod map
		 *
		 * @param iter Mod map iterator
		 * @param writeBuf Write buffer
		 * @param device Device
		 */
		static void mapMod(
		    MemoryMap::iterator iter, std::uint8_t *writeBuf, CNROM &device) {
			BankConfig::mapMod(iter, writeBuf, nullptr, device.m_RAM.data(),
			    device.m_RAM.data() + (0x0800 & (device.m_RAM.size() - 1)),
			    device.m_RAM.data() + (0x1000 & (device.m_RAM.size() - 1)),
			    device.m_RAM.data() + (0x1800 & (device.m_RAM.size() - 1)),
			    device.m_PRG.data(),
			    device.m_PRG.data() + (0x4000 & (device.m_PRG.size() - 1)));
		}
		/**
		 * Checks if device is enabled
		 *
		 * @param addr Address
		 * @return True if enabled
		 */
		static bool isDeviceEnabled(std::uint16_t addr) {
			return (addr > 0x6000);
		}
		/**
		 * Determines active bank
		 *
		 * @param addr Address
		 * @param device Device
		 * @return Active bank
		 */
		static std::size_t getBank(std::uint16_t addr, CNROM &device) {
			if (addr < 0x8000) {
				if (device.m_RAM.size() == 0) {
					return 0;
				}
				return 1 + ((addr >> 11) & 3);
			} else if (addr < 0xc000) {
				return 5;
			} else {
				return 6;
			}
		}
	};
	/**
	 * PPU bus config
	 */
	struct PPUConfig : public BusConfigBase<CNROM> {
		/**
		 * Banks config
		 */
		typedef banks::BankConfig<banks::ReadOnlyWithConflict<0x0000, 0x2000,
		                              0x2000>,  // CHR ROM 0x0000 - 0x1fff
		    banks::ReadWrite<0x2000, 0x0400,
		        0x0400>,  // NameTable 1 0x2000 - 0x23ff
		    banks::ReadWrite<0x2000, 0x0400,
		        0x0400>,  // NameTable 2 0x2400 - 0x27ff
		    banks::ReadWrite<0x2000, 0x0400,
		        0x0400>,  // NameTable 3 0x2800 - 0x2bff
		    banks::ReadWrite<0x2000, 0x0400,
		        0x0400>,  // NameTable 4 0x2c00 - 0x3000
		    banks::ReadWrite<0x2000, 0x0400,
		        0x0400>,  // NameTable 1 0x3000 - 0x33ff
		    banks::ReadWrite<0x2000, 0x0400,
		        0x0400>,  // NameTable 2 0x3400 - 0x37ff
		    banks::ReadWrite<0x2000, 0x0400,
		        0x0400>,  // NameTable 3 0x3800 - 0x3bff
		    banks::ReadWrite<0x2000, 0x0400, 0x0400>>  // NameTable 4 0x3c00 -
		                                               // 0x3000
		    BankConfig;

		/**
		 * Maps to read map
		 *
		 * @param iter Read map iterator
		 * @param openBus Open bus
		 * @param device Device
		 */
		static void mapRead(
		    MemoryMap::iterator iter, std::uint8_t *openBus, CNROM &device) {
			switch (device.m_Mirroring) {
			case MirroringHorizontal:
				BankConfig::mapRead(iter, openBus, device.m_CHR.data(),
				    device.m_NameTable, device.m_NameTable + 0x0400,
				    device.m_NameTable, device.m_NameTable + 0x0400,
				    device.m_NameTable, device.m_NameTable + 0x0400,
				    device.m_NameTable, device.m_NameTable + 0x0400);
				break;
			case MirroringVertical:
				BankConfig::mapRead(iter, openBus, device.m_CHR.data(),
				    device.m_NameTable, device.m_NameTable,
				    device.m_NameTable + 0x0400, device.m_NameTable + 0x0400,
				    device.m_NameTable, device.m_NameTable,
				    device.m_NameTable + 0x0400, device.m_NameTable + 0x0400);
				break;
			default:
				assert(false);
			}
		}
		/**
		 * Maps to write map
		 *
		 * @param iter Write map iterator
		 * @param dummy Dummy value
		 * @param device Device
		 */
		static void mapWrite(
		    MemoryMap::iterator iter, std::uint8_t *dummy, CNROM &device) {
			switch (device.m_Mirroring) {
			case MirroringHorizontal:
				BankConfig::mapWrite(iter, dummy, device.m_CHR.data(),
				    device.m_NameTable, device.m_NameTable + 0x0400,
				    device.m_NameTable, device.m_NameTable + 0x0400,
				    device.m_NameTable, device.m_NameTable + 0x0400,
				    device.m_NameTable, device.m_NameTable + 0x0400);
				break;
			case MirroringVertical:
				BankConfig::mapWrite(iter, dummy, device.m_CHR.data(),
				    device.m_NameTable, device.m_NameTable,
				    device.m_NameTable + 0x0400, device.m_NameTable + 0x0400,
				    device.m_NameTable, device.m_NameTable,
				    device.m_NameTable + 0x0400, device.m_NameTable + 0x0400);
				break;
			default:
				assert(false);
			}
		}
		/**
		 * Maps to mod map
		 *
		 * @param iter Mod map iterator
		 * @param writeBuf Write buffer
		 * @param device Device
		 */
		static void mapMod(
		    MemoryMap::iterator iter, std::uint8_t *writeBuf, CNROM &device) {
			switch (device.m_Mirroring) {
			case MirroringHorizontal:
				BankConfig::mapMod(iter, writeBuf, device.m_CHR.data(),
				    device.m_NameTable, device.m_NameTable + 0x0400,
				    device.m_NameTable, device.m_NameTable + 0x0400,
				    device.m_NameTable, device.m_NameTable + 0x0400,
				    device.m_NameTable, device.m_NameTable + 0x0400);
				break;
			case MirroringVertical:
				BankConfig::mapMod(iter, writeBuf, device.m_CHR.data(),
				    device.m_NameTable, device.m_NameTable,
				    device.m_NameTable + 0x0400, device.m_NameTable + 0x0400,
				    device.m_NameTable, device.m_NameTable,
				    device.m_NameTable + 0x0400, device.m_NameTable + 0x0400);
				break;
			default:
				assert(false);
			}
		}
		/**
		 * Checks if device is enabled
		 *
		 * @param addr Address
		 * @return True if enabled
		 */
		static bool isDeviceEnabled(std::uint16_t addr) {
			// This will override PPU palette access to the bus
			// which is not needed since it has different access point
			return true;
		}
		/**
		 * Determines active bank
		 *
		 * @param addr Address
		 * @param device Device
		 * @return Active bank
		 */
		static std::size_t getBank(std::uint16_t addr, CNROM &device) {
			return 0;
		}
	};

private:
	std::vector<std::uint8_t> m_PRG;
	std::vector<std::uint8_t> m_CHR;
	std::vector<std::uint8_t> m_RAM;
	EMirroring m_Mirroring;
	std::uint8_t m_NameTable[0x0800];

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
	CNROM() = delete;
	/**
	 * Constructs the object
	 *
	 * @param motherBoard Motherboard
	 */
	CNROM(CMotherBoard &motherBoard, const SNESConfig &config)
	    : m_PRG(config.PRG)
	    , m_CHR(config.CHR)
	    , m_RAM(config.RAMSize)
	    , m_Mirroring(config.Mirroring) {
	}
	/**
	 * Destroys the object
	 */
	~CNROM() = default;

	/**
	 * Adds CPU hooks
	 *
	 * @param bus CPU bus
	 */
	void addHooksCPU(CBus &bus) {
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

#endif /* VPNES_INCLUDE_CORE_MAPPERS_NROM_HPP_ */
