/**
 * @file
 *
 * Defines main GUI classes
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

#ifndef VPNES_INCLUDE_GUI_GUI_HPP_
#define VPNES_INCLUDE_GUI_GUI_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vpnes/vpnes.hpp>
#include <vpnes/gui/config.hpp>

namespace vpnes {

namespace gui {

/**
 * Main GUI class
 */
class CGUI {
protected:
	/**
	 * Application configuration
	 */
	SApplicationConfig config;
public:
	/**
	 * Constructor
	 */
	CGUI() = default;
	/**
	 * Destroyer
	 */
	~CGUI() = default;
	/**
	 * Starts GUI
	 *
	 * @param argc Amount of parameters
	 * @param argv Array of parameters
	 * @return Exit code
	 */
	int startGUI(int argc, char **argv);
};

}

}

#endif /* VPNES_INCLUDE_GUI_GUI_HPP_ */
