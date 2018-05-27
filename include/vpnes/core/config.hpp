/**
 * @file
 *
 * Defines class to configure NES
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

#ifndef INCLUDE_VPNES_CORE_CONFIG_HPP_
#define INCLUDE_VPNES_CORE_CONFIG_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstddef>
#include <cstdint>
#include <vector>
#include <fstream>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/frontend.hpp>
#include <vpnes/core/nes.hpp>
#include <vpnes/config/core.hpp>

namespace vpnes {

namespace core {

/**
 * Type of mirroring
 */
enum EMirroring {
	MirroringHorizontal,     //!< Horizontal mirroring
	MirroringVertical,       //!< Vertical mirroring
	MirroringSingleScreen1,  //!< Single screen (1)
	MirroringSingleScreen2,  //!< Single screen (2)
	MirroringFourScreens     //!< Four screens
};

/**
 * NES type
 */
enum ENESType {
	NESTypeNTSC,       //!< NTSC NES
	NESTypePAL,        //!< PAL NES
	NESTypeFC,         //!< Famicom
	NESTypeFamiclone,  //!< Famicom clone
};

/**
 * MMC Type
 */
enum EMMCType {
	MMCNROM128,  //!< NROM-128
	MMCNROM256,  //!< NROM-256
	MMCAmount    //!< Total amount
};

/**
 * Defines NES instance
 */
struct SNESConfig {
	/**
	 * CHR ROM
	 */
	std::vector<std::uint8_t> CHR;
	/**
	 * PRG ROM
	 */
	std::vector<std::uint8_t> PRG;
	/**
	 * Trainer hack
	 */
	std::vector<std::uint8_t> Trainer;
	/**
	 * PRG size
	 */
	std::size_t PRGSize;
	/**
	 * CHR size
	 */
	std::size_t CHRSize;
	/**
	 * RAM size
	 */
	std::size_t RAMSize;
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
	 * Configures the class
	 *
	 * @param coreConfig Configuration for core emulator settings
	 * @param inputFile Input file stream
	 */
	void configure(
	    const config::SCoreConfig &coreConfig, std::ifstream *inputFile);
	/**
	 * Creates an instance of NES
	 *
	 * @param frontEnd Front-end
	 * @return Instance of NES
	 */
	virtual CNES *createInstance(CFrontEnd *frontEnd);
	/**
	 * Default constructor
	 */
	SNESConfig();
	/**
	 * Deleted default copy constructor
	 *
	 * @param s Copied value
	 */
	SNESConfig(const SNESConfig &s) = delete;
	/**
	 * Default destructor
	 */
	virtual ~SNESConfig() = default;
};

}  // namespace core

}  // namespace vpnes

#endif  // INCLUDE_VPNES_CORE_CONFIG_HPP_
