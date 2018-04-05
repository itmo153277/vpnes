/**
 * @file
 *
 * Helps to create factories
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

#ifndef INCLUDE_VPNES_CORE_MAPPERS_HELPER_HPP_
#define INCLUDE_VPNES_CORE_MAPPERS_HELPER_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <array>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/device.hpp>
#include <vpnes/core/bus.hpp>
#include <vpnes/core/mboard.hpp>
#include <vpnes/core/cpu.hpp>
#include <vpnes/core/ppu.hpp>
#include <vpnes/core/apu.hpp>

namespace vpnes {

namespace core {

namespace factory {

/**
 * Basic NES implementation based on config
 */
template <class Config, class MMCType, class... Devices>
class CNESHelper : public CNES {
private:
	/**
	 * Motherboard
	 */
	CMotherBoard m_MotherBoard;
	/**
	 * CPU
	 */
	CCPU m_CPU;
	/**
	 * PPU
	 */
	CPPU m_PPU;
	/**
	 * APU
	 */
	CAPU m_APU;
	/**
	 * MMC
	 */
	MMCType m_MMC;

public:
	/**
	 * Deleted default constructor
	 */
	CNESHelper() = delete;
	/**
	 * Constructs the object
	 *
	 * @param config NES config
	 * @param frontEnd GUI callback
	 * @param devices Additional devices
	 */
	explicit CNESHelper(
	    const SNESConfig &config, CFrontEnd *frontEnd, Devices *... devices)
	    : CNES()
	    , m_MotherBoard(frontEnd)
	    , m_CPU(&m_MotherBoard)
	    , m_PPU(&m_MotherBoard, Config::getFrequency(), Config::FrameTime)
	    , m_APU(&m_MotherBoard)
	    , m_MMC(&m_MotherBoard, config) {
		m_MotherBoard.addBusCPU(&m_CPU, &m_APU, &m_PPU, &m_MMC, devices...);
		m_MotherBoard.addBusPPU(&m_MMC, devices...);
		m_MotherBoard.registerSimDevices(
		    &m_CPU, &m_APU, &m_PPU, &m_MMC);
	}
	/**
	 * Starts the simulation
	 */
	void powerUp() {
		m_MotherBoard.simulate();
	}
	/**
	 * Stops the emulation
	 */
	void turnOff() {
		m_MotherBoard.setEnabled(false);
	}
};

/**
 * NTSC NES settings
 */
struct SConfigNTSC {
	enum { FrameTime = 4 * 341 * 262 };  //!< Frame Time

	/**
	 * Bus frequency
	 *
	 * @return Frequency
	 */
	static constexpr double getFrequency() {
		return 44.0 / 945000.0;
	}
};

/**
 * Basic NES factory
 *
 * @param config NES config
 * @param frontEnd Front-end
 * @param devices Additional devices
 * @return NES
 */
template <class T, class... Devices>
CNES *factoryNES(
    const SNESConfig &config, CFrontEnd *frontEnd, Devices *... devices) {
	return new CNESHelper<SConfigNTSC, T, Devices...>(
	    config, frontEnd, devices...);
}

}  // namespace factory

}  // namespace core

}  // namespace vpnes

#endif  // INCLUDE_VPNES_CORE_MAPPERS_HELPER_HPP_
