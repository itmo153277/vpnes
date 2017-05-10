/**
 * @file
 *
 * Defines motherboard
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

#ifndef VPNES_INCLUDE_CORE_MBOARD_HPP_
#define VPNES_INCLUDE_CORE_MBOARD_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cassert>
#include <cstdint>
#include <utility>
#include <type_traits>
#include <memory>
#include <vector>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/frontend.hpp>
#include <vpnes/core/device.hpp>
#include <vpnes/core/bus.hpp>

namespace vpnes {

namespace core {

/**
 * Basic motherboard
 */
class CMotherBoard : public CGeneratorDevice, public CEventManager {
private:
	/**
	 * Devices that can be run
	 */
	std::vector<CClockedDevice *> m_Devices;
	/**
	 * Current running device
	 */
	CClockedDevice *m_CurrentDevice;
	/**
	 * PPU bus
	 */
	std::unique_ptr<CBus> m_BusPPU;
	/**
	 * CPU bus
	 */
	std::unique_ptr<CBus> m_BusCPU;
	/**
	 * Front-end
	 */
	CFrontEnd *m_FrontEnd;
	/**
	 * Estimated frame time
	 */
	ticks_t m_FrameTime;
	/**
	 * Frequency
	 */
	double m_Freq;

	/**
	 * Adds hooks for PPU bus
	 */
	void addHooksPPU() {
	}
	/**
	 * Adds hooks for PPU bus
	 *
	 * @param device Current device
	 * @param otherDevices Other devices
	 */
	template <class FirstDevice, class... OtherDevices>
	void addHooksPPU(FirstDevice &device, OtherDevices &... otherDevices) {
		device.addHooksPPU(*m_BusPPU);
		addHooksPPU(otherDevices...);
	}
	/**
	 * Adds hooks for CPU bus
	 */
	void addHooksCPU() {
	}
	/**
	 * Adds hooks for CPU bus
	 *
	 * @param device Current device
	 * @param otherDevices Other devices
	 */
	template <class FirstDevice, class... OtherDevices>
	void addHooksCPU(FirstDevice &device, OtherDevices &... otherDevices) {
		device.addHooksCPU(*m_BusCPU);
		addHooksPPU(otherDevices...);
	}

	/**
	 * Handles frame ending
	 *
	 * @param event Frame ending event
	 */
	void handleFrameEnd(CEvent *event) {
		m_FrontEnd->handleFrameRender(event->getFireTime() * m_Freq);
		resetClock(event->getFireTime());
		event->setFireTime(m_FrameTime);
	}

protected:
	/**
	 * Executes the simulation
	 */
	void execute() {
		for (CClockedDevice *device : m_Devices) {
			m_CurrentDevice = device;
			device->simulate(m_Clock);
		}
		m_CurrentDevice = nullptr;
	}
	/**
	 * Synchronizes clock of running device
	 *
	 * @param ticks New time
	 */
	void sync(ticks_t ticks) {
		if (m_CurrentDevice) {
			m_CurrentDevice->setClock(ticks);
		}
	}

public:
	/**
	 * Deleted default constructor
	 */
	CMotherBoard() = delete;
	/**
	 * Constructs the object
	 *
	 * @param frontEnd Front-end
	 * @param frequency Frequency
	 * @param frameTime Basic frame time
	 */
	CMotherBoard(CFrontEnd &frontEnd, double frequency, ticks_t frameTime)
	    : CGeneratorDevice(true)
	    , CEventManager()
	    , m_CurrentDevice()
	    , m_FrontEnd(&frontEnd)
	    , m_FrameTime(frameTime)
	    , m_Freq(frequency) {
		registerEvent(*this, "FRAME_RENDER_END", m_FrameTime, true,
		    &CMotherBoard::handleFrameEnd);
	}
	/**
	 * Deleted copy constructor
	 *
	 * @param s Copied value
	 */
	CMotherBoard(const CMotherBoard &s) = delete;
	/**
	 * Destroys the object
	 */
	~CMotherBoard() = default;

	/**
	 * Registers running devices
	 *
	 * @param devices Devices
	 */
	template <class... Devices>
	void registerSimDevices(Devices &... devices) {
		static_assert(
		    cond_and<std::is_base_of<CClockedDevice, Devices>...>::value,
		    "Only for clocked devices");
		m_Devices = {&devices...};
	}
	/**
	 * Creates new PPU bus
	 *
	 * @param devices Devices on PPU bus
	 */
	template <class... Devices>
	void addBusPPU(Devices &... devices) {
		m_BusPPU.reset(
		    new CBusConfig<typename Devices::PPUConfig...>(0x00, devices...));
		addHooksPPU(devices...);
	}
	/**
	 * Creates new CPU bus
	 *
	 * @param devices Devices on CPU bus
	 */
	template <class... Devices>
	void addBusCPU(Devices &... devices) {
		m_BusCPU.reset(
		    new CBusConfig<typename Devices::CPUConfig...>(0x40, devices...));
		addHooksCPU(devices...);
	}

	/**
	 * Gets PPU bus
	 *
	 * @return PPU bus
	 */
	CBus &getBusPPU() const {
		assert(m_BusPPU);
		return *m_BusPPU;
	}
	/**
	 * Gets CPU bus
	 *
	 * @return CPU bus
	 */
	CBus &getBusCPU() const {
		assert(m_BusCPU);
		return *m_BusCPU;
	}

	double getFrequency() const {
		return m_Freq;
	}
	/**
	 * Resets the clock by ticks amount
	 *
	 * @param ticks Amount of ticks
	 */
	void resetClock(ticks_t ticks) {
		CGeneratorDevice::resetClock(ticks);
		for (CClockedDevice *device : m_Devices) {
			device->resetClock(ticks);
		}
	}
	/**
	 * Gets pending time
	 *
	 * @return Pending time
	 */
	ticks_t getPending() {
		if (m_CurrentDevice) {
			return m_CurrentDevice->getPending();
		} else {
			return m_Clock;
		}
	}
};

}  // namespace core

}  // namespace vpnes

#endif /* VPNES_INCLUDE_CORE_MBOARD_HPP_ */
