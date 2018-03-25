/**
 * @file
 * Defines microcode compilation helpers for ppu
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

#ifndef INCLUDE_VPNES_CORE_PPU_COMPILE_HPP_
#define INCLUDE_VPNES_CORE_PPU_COMPILE_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vpnes/vpnes.hpp>
#include <vpnes/core/device.hpp>
#include <vpnes/core/ppu.hpp>

namespace vpnes {

namespace core {

namespace ppu {

/**
 * PPU Unit
 */
class CPPUUnit : public CClockedDevice {
protected:
	/**
	 * Link to PPU
	 */
	CPPU *m_PPU;
	/**
	 * Internal clock
	 */
	ticks_t m_InternalClock;
	/**
	 * Resets the clock by ticks amount
	 *
	 * @param ticks Amount of ticks
	 */
	void resetClock(ticks_t ticks) {
		m_InternalClock -= ticks;
	}

public:
	/**
	 * Deleted default constructor
	 */
	CPPUUnit() = delete;
	/**
	 * Constructs the object
	 *
	 * @param ppu PPU
	 */
	explicit CPPUUnit(CPPU *&ppu) : m_PPU(ppu), m_InternalClock() {
	}
	/**
	 * Default destructor
	 */
	~CPPUUnit() = default;

	/**
	 * Gets pending time
	 *
	 * @return Pending time
	 */
	ticks_t getPending() const {
		return m_InternalClock;
	}
	/**
	 * Resolves dependency between units
	 *
	 * @param dependentUnit Unit dependent of this one
	 */
	void resolveDependency(const CPPUUnit &dependentUnit) {
		simulate(dependentUnit.getPending());
	}
};

}  // namespace ppu

}  // namespace core

}  // namespace vpnes

#endif  // INCLUDE_VPNES_CORE_PPU_COMPILE_HPP_
