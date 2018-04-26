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

#include <cassert>
#include <cstdint>
#include <array>
#include <unordered_map>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/debugger.hpp>
#include <vpnes/core/nes.hpp>
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
 * Implementation for debugger class
 */
class CDebuggerHelper : public CDebugger {
private:
	class CDebugDevice : public CDevice {
	private:
		/**
		 * Hooks for CPU reads
		 */
		std::unordered_multimap<std::uint16_t, addrHook_t> m_HooksCPURead;
		/**
		 * Hooks for CPU writes
		 */
		std::unordered_multimap<std::uint16_t, addrHook_t> m_HooksCPUWrite;
		/**
		 * Motherboard
		 */
		CMotherBoard *m_MotherBoard;

		/**
		 * Reads to CPU bus
		 *
		 * @param val Value
		 * @param addr Address
		 */
		void readCPU(std::uint8_t val, std::uint16_t addr) {
			auto range = m_HooksCPURead.equal_range(addr);
			assert(range.first != range.second);
			for (auto iter = range.first; iter != range.second; ++iter) {
				iter->second(addr, val);
			}
		}
		/**
		 * Writes to CPU bus
		 *
		 * @param val Value
		 * @param addr Address
		 */
		void writeCPU(std::uint8_t val, std::uint16_t addr) {
			auto range = m_HooksCPUWrite.equal_range(addr);
			assert(range.first != range.second);
			for (auto iter = range.first; iter != range.second; ++iter) {
				iter->second(addr, val);
			}
		}

	public:
		/**
		 * Deleted default constructor
		 */
		CDebugDevice() = delete;
		/**
		 * Constructor
		 *
		 * @param motherBoard Motherboard
		 */
		explicit CDebugDevice(CMotherBoard *motherBoard)
		    : m_HooksCPURead(), m_HooksCPUWrite(), m_MotherBoard(motherBoard) {
		}

		/**
		 * Hook address read on CPU bus
		 *
		 * @param addr Address
		 * @param hook Hook
		 */
		void hookCPURead(std::uint16_t addr, addrHook_t hook) {
			if (m_HooksCPURead.find(addr) == m_HooksCPURead.end()) {
				m_MotherBoard->getBusCPU()->addPostReadHook(
				    addr, this, &CDebugDevice::readCPU);
			}
			m_HooksCPURead.emplace(addr, hook);
		}
		/**
		 * Hook address write on CPU bus
		 *
		 * @param addr Address
		 * @param hook Hook
		 */
		void hookCPUWrite(std::uint16_t addr, addrHook_t hook) {
			if (m_HooksCPUWrite.find(addr) == m_HooksCPUWrite.end()) {
				m_MotherBoard->getBusCPU()->addWriteHook(
				    addr, this, &CDebugDevice::writeCPU);
			}
			m_HooksCPUWrite.emplace(addr, hook);
		}
	};

	/**
	 * Motherboard
	 */
	CMotherBoard *m_MotherBoard;
	/**
	 * Debug device
	 */
	CDebugDevice m_DebugDevice;

public:
	/**
	 * Deleted default constructor
	 */
	CDebuggerHelper() = delete;
	/**
	 * Constructor
	 *
	 * @param motherBoard Motherboard
	 */
	explicit CDebuggerHelper(CMotherBoard *motherBoard)
	    : m_MotherBoard(motherBoard), m_DebugDevice(motherBoard) {
	}

	/**
	 * Hook address read on CPU bus
	 *
	 * @param addr Address
	 * @param hook Hook
	 */
	void hookCPURead(std::uint16_t addr, addrHook_t hook) {
		m_DebugDevice.hookCPURead(addr, hook);
	}
	/**
	 * Hook address write on CPU bus
	 *
	 * @param addr Address
	 * @param hook Hook
	 */
	void hookCPUWrite(std::uint16_t addr, addrHook_t hook) {
		m_DebugDevice.hookCPUWrite(addr, hook);
	}
	/**
	 * Direct read from CPU bus
	 *
	 * @param addr Address
	 * @return Value on CPU bus
	 */
	std::uint8_t directCPURead(std::uint16_t addr) {
		return m_MotherBoard->getBusCPU()->readMemory(addr, true);
	}
	/**
	 * Direct write to CPU bus
	 *
	 * @param addr Address
	 * @param val Value
	 */
	void directCPUWrite(std::uint16_t addr, std::uint8_t val) {
		m_MotherBoard->getBusCPU()->writeMemory(val, addr, true);
	}
};

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
	/**
	 * Debugger
	 */
	CDebuggerHelper m_Debugger;

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
	    , m_MMC(&m_MotherBoard, config)
	    , m_Debugger(&m_MotherBoard) {
		m_MotherBoard.addBusCPU(&m_CPU, &m_APU, &m_PPU, &m_MMC, devices...);
		m_MotherBoard.addBusPPU(&m_MMC, devices...);
		m_MotherBoard.registerSimDevices(&m_CPU, &m_APU, &m_PPU, &m_MMC);
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
		m_MotherBoard.setClock(m_MotherBoard.getPending());
	}
	/**
	 * Debugger for NES
	 *
	 * @return Debugger for NES
	 */
	CDebugger *getDebugger() {
		return &m_Debugger;
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
