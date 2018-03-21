/**
 * @file
 *
 * Defines NROM MMC
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

#ifndef INCLUDE_VPNES_CORE_MAPPERS_NROM_HPP_
#define INCLUDE_VPNES_CORE_MAPPERS_NROM_HPP_

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
	struct CPUConfig : BusConfigBase<CNROM> {
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
		    CNROM *device) {
			BankConfig::mapIO(iterRead, iterWrite, iterMod, openBus, dummy,
			    writeBuf, nullptr, device->m_RAM.data(),
			    device->m_RAM.data() + (0x0800 & (device->m_RAM.size() - 1)),
			    device->m_RAM.data() + (0x1000 & (device->m_RAM.size() - 1)),
			    device->m_RAM.data() + (0x1800 & (device->m_RAM.size() - 1)),
			    device->m_PRG.data(),
			    device->m_PRG.data() + (0x4000 & (device->m_PRG.size() - 1)));
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
		static std::size_t getBank(std::uint16_t addr, const CNROM &device) {
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
	struct PPUConfig : BusConfigBase<CNROM> {
		/**
		 * Banks config
		 */
		typedef banks::BankConfig<banks::ReadOnlyWithConflict<0x0000, 0x2000,
		                              0x2000>,  // CHR ROM 0x0000 - 0x1fff
		    banks::ReadWrite<0x0000, 0x2000,
		        0x2000>,  // CHR RAM 0x0000 - 0x1fff
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
		    CNROM *device) {
			switch (device->m_Mirroring) {
			case MirroringHorizontal:
				BankConfig::mapIO(iterRead, iterWrite, iterMod, openBus, dummy,
				    writeBuf, device->m_CHR.data(), device->m_CHR.data(),
				    device->m_NameTable, device->m_NameTable + 0x0400,
				    device->m_NameTable, device->m_NameTable + 0x0400,
				    device->m_NameTable, device->m_NameTable + 0x0400,
				    device->m_NameTable, device->m_NameTable + 0x0400);
				break;
			case MirroringVertical:
				BankConfig::mapIO(iterRead, iterWrite, iterMod, openBus, dummy,
				    writeBuf, device->m_CHR.data(), device->m_CHR.data(),
				    device->m_NameTable, device->m_NameTable,
				    device->m_NameTable + 0x0400, device->m_NameTable + 0x0400,
				    device->m_NameTable, device->m_NameTable,
				    device->m_NameTable + 0x0400, device->m_NameTable + 0x0400);
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
		static std::size_t getBank(std::uint16_t addr, const CNROM &device) {
			if (addr < 0x2000) {
				return device.m_CHRBank;
			} else {
				return 1 + ((addr >> 10) & 7);
			}
		}
	};

private:
	/**
	 * PRG ROM
	 */
	std::vector<std::uint8_t> m_PRG;
	/**
	 * CHR ROM / CHR RAM
	 */
	std::vector<std::uint8_t> m_CHR;
	/**
	 * PRG RAM
	 */
	std::vector<std::uint8_t> m_RAM;
	/**
	 * Mirroring
	 */
	EMirroring m_Mirroring;
	/**
	 * Nametable
	 */
	std::uint8_t m_NameTable[0x0800];
	/**
	 * CHR Bank
	 */
	std::size_t m_CHRBank;

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
	 * @param config NES config
	 */
	CNROM(CMotherBoard *motherBoard, const SNESConfig &config);
	/**
	 * Destroys the object
	 */
	~CNROM() = default;

	/**
	 * Adds CPU hooks
	 *
	 * @param bus CPU bus
	 */
	void addHooksCPU(CBus *bus) {
	}
	/**
	 * Adds PPU hooks
	 *
	 * @param bus PPU bus
	 */
	void addHooksPPU(CBus *bus) {
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

#endif  // INCLUDE_VPNES_CORE_MAPPERS_NROM_HPP_
