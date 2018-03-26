/**
 * @file
 *
 * Defines basic PPU
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

#ifndef INCLUDE_VPNES_CORE_PPU_HPP_
#define INCLUDE_VPNES_CORE_PPU_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstddef>
#include <cstdint>
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
	struct CPUConfig : BusConfigBase<CPPU> {
		/**
		 * Banks config
		 */
		typedef banks::BankConfig<banks::ReadWrite<0x2000, 0x2000, 1>>
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
		    CPPU *device) {
			BankConfig::mapIO(iterRead, iterWrite, iterMod, openBus, dummy,
			    writeBuf, &device->m_IOBuf);
		}
		/**
		 * Checks if device is enabled
		 *
		 * @param addr Address
		 * @return True if enabled
		 */
		static bool isDeviceEnabled(std::uint16_t addr) {
			return (addr >= 0x2000) && (addr < 0x4000);
		}
		/**
		 * Determines active bank
		 *
		 * @param addr Address
		 * @param device Device
		 * @return Active bank
		 */
		static std::size_t getBank(std::uint16_t addr, const CPPU &device) {
			return 0;
		}
	};

private:
	/**
	 * Motherboard
	 */
	CMotherBoard *m_MotherBoard;
	/**
	 * Palette
	 */
	std::uint8_t m_Palette[0x0020];
	/**
	 * IO Buffer
	 */
	std::uint8_t m_IOBuf;
	/**
	 * Estimated frame time
	 */
	ticks_t m_FrameTime;
	/**
	 * Frequency
	 */
	double m_Freq;

	/**
	 * Object sizes
	 */
	enum EObjectSizeMode {
		PPUObjectSize8x8 = 8,   //!< 8x8 size
		PPUObjectSize8x16 = 16  //!< 8x16 size
	};

	/**
	 * Address
	 */
	std::uint16_t m_Addr_v;
	/**
	 * Address buffer
	 */
	std::uint16_t m_Addr_t;
	/**
	 * Page for background tiles
	 */
	std::uint16_t m_BackgroundPage;
	/**
	 * Page for object tiles
	 */
	std::uint16_t m_ObjectPage;
	/**
	 * Index of pixel in shift register
	 */
	std::size_t m_RenderIndex;
	/**
	 * Vertical increment of PPU address
	 */
	int m_IncrementVertically;
	/**
	 * Generate non-maskable interrupt
	 */
	int m_GenerateNMI;
	/**
	 * Enable background render
	 */
	int m_EnableBackgound;
	/**
	 * Do not render first 8px of screen for background
	 */
	int m_ClipBackground;
	/**
	 * Enable object render
	 */
	int m_EnableObjects;
	/**
	 * Do not render first 8px of screen for objects
	 */
	int m_ClipObjects;
	/**
	 * Grayscale mode
	 */
	int m_Grayscale;
	/**
	 * Object size
	 */
	EObjectSizeMode m_ObjectSize;
	/**
	 * Tint mode
	 */
	std::size_t m_TintIndex;
	/**
	 * Object 0 was hit
	 */
	int m_Object0Hit;
	/**
	 * Object evaluation had overflowed
	 */
	int m_ObjectOverflow;
	/**
	 * Vertical blank is active
	 */
	int m_VerticalBlank;
	/**
	 * Trigger for first/second write to 5/6
	 */
	bool m_WriteTrigger;

	/**
	 * Control register values
	 */
	enum {
		PPUControlIncrementVertically = 0x04,  //!< Increment address vertically
		PPUControlGenerateNMI = 0x80,          //!< Generate NMI on VBlank
		PPUControlGrayscale = 0x01,            //!< Grayscale mode
		PPUControlBGPageSelector = 0x10,       //!< Selector for background page
		PPUControlObjectPageSelector = 0x08,   //!< Selector for object page
		PPUControlObjectSizeSelector = 0x20,   //!< Selector for object size
		PPUControlClipBackground =
		    0x02,  //!< Clip first 8px of screen for background
		PPUControlClipObjects = 0x04,  //!< Clip first 8px of screen for objects
		PPUControlEnableBackground = 0x08,  //!< Enable background rendering
		PPUControlEnableObjects = 0x10      //!< Enable object rendering
	};

	/**
	 * Status values
	 */
	enum {
		PPUStateVerticalBlank = 0x80,  //!< Vertical blank period
		PPUStateObject0Hit = 0x40,     //!< Object 0 hit background pixel
		PPUStateObjectOberflow = 0x20  //!< Object evaluation overflowed
	};

	/**
	 * Checks if rendering is active
	 *
	 * @return Active or not
	 */
	bool isRenderingEnabled() {
		return m_EnableBackgound || m_EnableObjects;
	}

	/**
	 * Reads register
	 *
	 * @param addr register address
	 */
	void readReg(std::uint16_t addr) {
		// TODO(me): respect read timings
		switch (addr & 7) {
		case 2:
			m_IOBuf &= 0x1f;
			m_IOBuf |= m_VerticalBlank;
			m_IOBuf |= m_Object0Hit;
			m_IOBuf |= m_ObjectOverflow;
			// m_VerticalBlank = 0;
			m_VerticalBlank ^= PPUStateVerticalBlank;  // For debug
			break;
		case 4:
			// TODO(me): implement OAM access
			break;
		case 7:
			// TODO(me): implement PPU memory access
			break;
		}
	}
	/**
	 * Writes to register
	 *
	 * @param val Value
	 * @param addr Address
	 */
	void writeReg(std::uint8_t val, std::uint16_t addr) {
		// TODO(me): respect write timings
		switch (addr & 7) {
		case 0:
			m_Addr_t = (m_Addr_t & 0x73ff) | ((val & 0x03) << 10);
			m_IncrementVertically = val & PPUControlIncrementVertically;
			m_GenerateNMI = val & PPUControlGenerateNMI;
			m_BackgroundPage = (val & PPUControlBGPageSelector) << 8;
			m_ObjectPage = (val & PPUControlObjectPageSelector) << 9;
			m_ObjectSize = (val & PPUControlObjectSizeSelector)
			                   ? PPUObjectSize8x16
			                   : PPUObjectSize8x8;
			break;
		case 1:
			m_Grayscale = val & PPUControlGrayscale;
			m_EnableBackgound = val & PPUControlEnableBackground;
			m_ClipBackground = val & PPUControlClipBackground;
			m_EnableObjects = val & PPUControlEnableObjects;
			m_ClipObjects = val & PPUControlClipObjects;
			m_TintIndex = val >> 5;
			break;
		case 2:
			// TODO(me): implement object memory access
			break;
		case 4:
			// TODO(me): implement object memory access
			break;
		case 5:
			m_WriteTrigger = !m_WriteTrigger;
			if (m_WriteTrigger) {
				m_RenderIndex = 0x10 | ((~val & 0x07) << 1);
				m_Addr_t = (m_Addr_t & 0x7fe0) | (val >> 3);
			} else {
				m_Addr_t = (m_Addr_t & 0x0c1f) | ((val & 0x07) << 12) |
				           ((val & 0xf8) << 2);
			}
			break;
		case 6:
			m_WriteTrigger = !m_WriteTrigger;
			if (m_WriteTrigger) {
				m_Addr_t = (m_Addr_t & 0x00ff) | ((val & 0x3f) << 8);
			} else {
				m_Addr_t = (m_Addr_t & 0x7f00) | val;
			}
			break;
		case 7:
			// TODO(me): implement PPU memory access
			break;
		}
	}

	/**
	 * Handles frame ending
	 *
	 * @param event Frame ending event
	 */
	void handleFrameEnd(CMotherBoard::CEvent *event) {
		m_MotherBoard->getFrontEnd()->handleFrameRender(
		    event->getFireTime() * m_Freq);
		m_MotherBoard->resetClock(event->getFireTime());
		event->setFireTime(m_FrameTime);
	}

protected:
	/**
	 * Simulation routine
	 */
	void execute() {
		// TODO(me): implement rendering
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
	 * @param frequency Frequency
	 * @param frameTime Basic frame time
	 */
	CPPU(CMotherBoard *motherBoard, double frequency, std::size_t frameTime)
	    : CEventDevice()
	    , m_MotherBoard(motherBoard)
	    , m_IOBuf()
	    , m_FrameTime(frameTime)
	    , m_Freq(frequency)
	    , m_Addr_v(0)
	    , m_Addr_t(0)
	    , m_BackgroundPage(0)
	    , m_ObjectPage(0)
	    , m_RenderIndex(0x1e)
	    , m_IncrementVertically(0)
	    , m_GenerateNMI(0)
	    , m_EnableBackgound(0)
	    , m_ClipBackground(0)
	    , m_EnableObjects(0)
	    , m_ClipObjects(0)
	    , m_Grayscale(0)
	    , m_ObjectSize(PPUObjectSize8x8)
	    , m_TintIndex(0)
	    , m_Object0Hit(0)
	    , m_ObjectOverflow(0)
	    , m_VerticalBlank(0)
	    , m_WriteTrigger(false) {
		m_MotherBoard->registerEvent(this, m_MotherBoard, "FRAME_RENDER_END",
		    m_FrameTime, true, &CPPU::handleFrameEnd);
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
	void addHooksCPU(CBus *bus) {
		for (std::uint16_t addr = 0x2000; addr < 0x4000; addr++) {
			bus->addPreReadHook(addr, this, &CPPU::readReg);
			bus->addWriteHook(addr, this, &CPPU::writeReg);
		}
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

#endif  // INCLUDE_VPNES_CORE_PPU_HPP_
