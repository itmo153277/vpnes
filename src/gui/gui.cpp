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

using namespace ::std;
using namespace ::std::chrono;
using namespace ::vpnes;
using namespace ::vpnes::gui;
using namespace ::vpnes::core;

/* CGUI */

/**
 * Constructor
 */
CGUI::CGUI() : m_Jitter(), m_TimeOverhead(), m_Time(), m_Config() {
}

/**
 * Starts GUI
 *
 * @param argc Amount of parameters
 * @param argv Array of parameters
 * @return Exit code
 */
int CGUI::startGUI(int argc, char **argv) {
	m_Config.parseOptions(argc, argv);
	if (!m_Config.hasInputFile()) {
		cerr << "Usage:" << endl;
		cerr << argv[0] << " path_to_rom.nes" << endl;
		return 0;
	}
	try {
		ifstream inputFile = m_Config.getInputFile();
		SNESConfig nesConfig;
		nesConfig.configure(m_Config, inputFile);
		inputFile.close();
		unique_ptr<CNES> nes(nesConfig.createInstance(this));
		m_Jitter = 0;
		m_TimeOverhead = 0;
		m_Time = high_resolution_clock::now();
		nes->powerUp();
	} catch (const invalid_argument &e) {
		cerr << e.what() << endl;
	} catch (const exception &e) {
		cerr << "Unknown error: " << e.what() << endl;
		cerr << strerror(errno) << endl;
	}
	return 0;
}

/**
 * Frame-ready callback
 *
 * @param frameTime Frame time
 */
void CGUI::handleFrameRender(double frameTime) {
	high_resolution_clock::time_point lastTime = m_Time;
	m_Jitter += frameTime;
	if (m_Jitter > m_TimeOverhead) {
		long waitTime = static_cast<long>(m_Jitter - m_TimeOverhead);
		this_thread::sleep_for(milliseconds(waitTime));
		m_Time = high_resolution_clock::now();
		long actualWait =
		    duration_cast<milliseconds>(m_Time - lastTime).count();
		m_TimeOverhead += (actualWait - waitTime) / 2;
		m_Jitter -= actualWait;
	} else {
		m_Time = high_resolution_clock::now();
		m_Jitter -= duration_cast<milliseconds>(m_Time - lastTime).count();
	}
}
