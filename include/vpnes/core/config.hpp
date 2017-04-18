/**
 * @file
 *
 * Defines class to configure NES
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

#ifndef VPNES_INCLUDE_CORE_CONFIG_HPP_
#define VPNES_INCLUDE_CORE_CONFIG_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdint>
#include <fstream>
#include <vpnes/vpnes.hpp>
#include <vpnes/gui/config.hpp>
#include <vpnes/core/nes.hpp>

namespace vpnes {

namespace core {

/**
 * Type of mirroring
 */
enum EMirroring {
	MirroringHorizontal,   //!< Horizontal mirroring
	MirorringVertical,     //!< Vertical mirroring
	MirorringSingleScreen1,//!< Single screen (1)
	MirorringSingleScreen2,//!< Single screen (2)
	MirorringFourScreens   //!< Four screens
};

/**
 * NES type
 */
enum ENESType {
	NESTypeAuto,    //!< Auto defined
	NESTypeNTSC,    //!< NTSC NES
	NESTypePAL,     //!< PAL NES
	NESTypeFC,      //!< Famicom
	NESTypeFamiclone//!< Famicom clone
};

/**
 * MMC Type
 */
enum EMMCType {
	MMCNROM//!< NROM
};

/**
 * Defines NES instance
 */
struct SNESConfig {
private:
	/**
	 * CHR ROM
	 */
	std::uint8_t *CHR;
	/**
	 * PRG ROM
	 */
	std::uint8_t *PRG;
	/**
	 * Trainer hack
	 */
	std::uint8_t *Trainer;
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
public:
	/**
	 * Configures the class
	 *
	 * @param appConfig Application configuration
	 * @param inputFile Input file stream
	 */
	void configure(const gui::SApplicationConfig &appConfig,
			std::ifstream &inputFile);
	/**
	 * Creates an instance of NES
	 *
	 * @return Instance of NES
	 */
	CNES createInstance();
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
	~SNESConfig();
};

}

}

#endif /* VPNES_INCLUDE_CORE_CONFIG_HPP_ */
