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
	typedef BusConfigBase<CPPU> CPUConfig;
	/**
	 * PPU bus config
	 */
	typedef BusConfigBase<CPPU> PPUConfig;

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
	CPPU(CMotherBoard &motherBoard) {
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
