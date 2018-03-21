/**
 * @file
 *
 * Defines iNES file format
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

#ifndef INCLUDE_VPNES_CORE_INES_HPP_
#define INCLUDE_VPNES_CORE_INES_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstddef>
#include <cstdint>
#include <vector>
#include <fstream>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/config.hpp>

namespace vpnes {

namespace core {

namespace ines {

#pragma pack(push, 1)
/**
 * iNES header structure
 */
typedef struct iNES_Header {
	/**
	 * iNES signature
	 */
	char Signature[4];
	/**
	 * PRG ROM size in 16KB
	 */
	std::uint8_t PRGSize;
	/**
	 * CHR ROM size in 8KB
	 */
	std::uint8_t CHRSize;
	/**
	 * Flags
	 */
	std::uint8_t Flags;
	/**
	 * More flags
	 */
	std::uint8_t Flags_ex;
	/**
	 * PRG RAM size in 8KB
	 */
	std::uint8_t RAMSize;
	/**
	 * TV system (unofficial)
	 */
	std::uint8_t TV_system;
	/**
	 * Unofficial flags
	 */
	std::uint8_t Flags_unofficial;
	/**
	 * Must be 0
	 */
	std::uint8_t Reserved;
	/**
	 * Must be 0, check if it is a bad ROM
	 */
	std::uint32_t BadROM;
} iNES_Header;
#pragma pack(pop)

/**
 * Defines basic data from ROM
 */
struct SNESData {
	/**
	 * PRG ROM size
	 */
	std::size_t PRGSize;
	/**
	 * CHR ROM size
	 */
	std::size_t CHRSize;
	/**
	 * PRG RAM size
	 */
	std::size_t RAMSize;
	/**
	 * Battery backed PRG RAM size
	 */
	std::size_t BatterySize;
	/**
	 * MMC type
	 */
	EMMCType MMCType;
	/**
	 * Mirroring type
	 */
	EMirroring Mirroring;
	/**
	 * NES Type
	 */
	ENESType NESType;
	/**
	 * PRG ROM
	 */
	std::vector<std::uint8_t> PRG;
	/**
	 * CHR ROM
	 */
	std::vector<std::uint8_t> CHR;
	/**
	 * Trainer hacks
	 */
	std::vector<std::uint8_t> Trainer;
	/**
	 * Constructs the structure from the data
	 * @param ROM
	 */
	explicit SNESData(std::ifstream *ROM);
	/**
	 * Deleted default constructor
	 */
	SNESData() = delete;
	/**
	 * Deleted default copy constructor
	 *
	 * @param s Copied value
	 */
	SNESData(const SNESData &s) = delete;
	/**
	 * Destroys the structure
	 */
	~SNESData() = default;
};

}  // namespace ines

}  // namespace core

}  // namespace vpnes

#endif  // INCLUDE_VPNES_CORE_INES_HPP_
