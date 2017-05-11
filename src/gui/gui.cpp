/**
 * @file
 *
 * Implements cross-platform console ui
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cerrno>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <memory>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <vpnes/vpnes.hpp>
#include <vpnes/gui/config.hpp>
#include <vpnes/gui/gui.hpp>
#include <vpnes/core/config.hpp>
#include <vpnes/core/nes.hpp>

namespace vpnes {

namespace gui {

/* CGUI */

/**
 * Constructor
 */
vpnes::gui::CGUI::CGUI() : m_Jitter(), m_TimeOverhead(), m_Time(), m_Config() {
}

/**
 * Starts GUI
 *
 * @param argc Amount of parameters
 * @param argv Array of parameters
 * @return Exit code
 */
int vpnes::gui::CGUI::startGUI(int argc, char **argv) {
	m_Config.parseOptions(argc, argv);
	if (!m_Config.hasInputFile()) {
		std::cerr << "Usage:" << std::endl;
		std::cerr << argv[0] << " path_to_rom.nes" << std::endl;
		return 0;
	}
	try {
		std::ifstream inputFile = m_Config.getInputFile();
		core::SNESConfig nesConfig;
		nesConfig.configure(m_Config, inputFile);
		inputFile.close();
		std::unique_ptr<core::CNES> nes(nesConfig.createInstance(*this));
		m_Jitter = 0;
		m_TimeOverhead = 0;
		m_Time = std::chrono::high_resolution_clock::now();
		nes->powerUp();
	} catch (const std::invalid_argument &e) {
		std::cerr << e.what() << std::endl;
	} catch (const std::exception &e) {
		std::cerr << "Unknown error: " << e.what() << std::endl;
		std::cerr << std::strerror(errno) << std::endl;
	}
	return 0;
}

/**
 * Frame-ready callback
 *
 * @param frameTime Frame time
 */
void vpnes::gui::CGUI::handleFrameRender(double frameTime) {
#ifdef VPNES_MEASURE_FPS
	static int curFrame = 0;
	curFrame++;
	std::chrono::high_resolution_clock::time_point lastTime = m_Time;
	std::chrono::high_resolution_clock::time_point newTime =
	    std::chrono::high_resolution_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(
	        newTime - lastTime)
	        .count() > 1000) {
		m_Time = newTime;
		cerr << "FPS: "
		     << curFrame * 1000.0 /
		            std::chrono::duration_cast<std::chrono::milliseconds>(
		                newTime - lastTime)
		                .count()
		     << std::endl;
		curFrame = 0;
	}
#else
	std::chrono::high_resolution_clock::time_point lastTime = m_Time;
	m_Jitter += frameTime;
	if (m_Jitter > m_TimeOverhead) {
		long waitTime = static_cast<long>(m_Jitter - m_TimeOverhead);
		std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
		m_Time = std::chrono::high_resolution_clock::now();
		long actualWait = static_cast<long>(
		    std::chrono::duration_cast<std::chrono::milliseconds>(
		        m_Time - lastTime)
		        .count());
		m_TimeOverhead += (actualWait - waitTime) / 2;
		m_Jitter -= actualWait;
	} else {
		m_Time = std::chrono::high_resolution_clock::now();
		m_Jitter -= std::chrono::duration_cast<std::chrono::milliseconds>(
		    m_Time - lastTime)
		                .count();
	}
#endif
}

}  // namespace gui

}  // namespace vpnes
