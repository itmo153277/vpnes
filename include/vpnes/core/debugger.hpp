/**
 * @file
 *
 * Defines debugger for NES
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

#ifndef INCLUDE_VPNES_CORE_DEBUGGER_HPP_
#define INCLUDE_VPNES_CORE_DEBUGGER_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdint>
#include <functional>
#include <vpnes/vpnes.hpp>

namespace vpnes {

namespace core {

/**
 * NES Debugger
 */
class CDebugger {
public:
	/**
	 * Hook in device
	 */
	typedef std::function<void(std::uint16_t addr, std::uint8_t val)>
	    addrHook_t;
	/**
	 * Hook address read on CPU bus
	 *
	 * @param address Address
	 * @param hook Hook
	 */
	virtual void hookCPURead(std::uint16_t addr, addrHook_t hook) = 0;
	/**
	 * Hook address write on CPU bus
	 *
	 * @param addr Address
	 * @param hook Hook
	 */
	virtual void hookCPUWrite(std::uint16_t addr, addrHook_t hook) = 0;
	/**
	 * Direct read from CPU bus
	 *
	 * @param addr Address
	 * @return Value on CPU bus
	 */
	virtual std::uint8_t directCPURead(std::uint16_t addr) = 0;
	/**
	 * Direct write to CPU bus
	 *
	 * @param addr Address
	 * @param val Value
	 */
	virtual void directCPUWrite(std::uint16_t addr, std::uint8_t val) = 0;
	/**
	 * Constructor
	 */
	CDebugger() = default;
	/**
	 * Deleted copy constructor
	 *
	 * @param s Source value
	 */
	CDebugger(const CDebugger &s) = delete;
	/**
	 * Destructor
	 */
	virtual ~CDebugger() = default;
};

}  // namespace core

}  // namespace vpnes

#endif  // INCLUDE_VPNES_CORE_DEBUGGER_HPP_
