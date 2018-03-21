/**
 * @file
 *
 * Defines main GUI classes
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

#ifndef INCLUDE_VPNES_GUI_GUI_HPP_
#define INCLUDE_VPNES_GUI_GUI_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#include <cstddef>
#include <memory>
#include <chrono>
#include <vpnes/vpnes.hpp>
#include <vpnes/gui/config.hpp>
#include <vpnes/core/frontend.hpp>
#include <vpnes/core/nes.hpp>

namespace vpnes {

namespace gui {

/**
 * Main GUI class
 */
class CGUI : public core::CFrontEnd {
protected:
	/**
	 * Jitter
	 */
	double m_Jitter;
	/**
	 * Time overhead
	 */
	double m_TimeOverhead;
	/**
	 * Frame start point
	 */
	std::chrono::high_resolution_clock::time_point m_Time;
	/**
	 * Application configuration
	 */
	SApplicationConfig m_Config;
	/**
	 * NES
	 */
	std::unique_ptr<core::CNES> m_NES;
	/**
	 * Window
	 */
	::SDL_Window *m_Window;
	/**
	 * Main renderer
	 */
	::SDL_Renderer *m_Renderer;
	/**
	 * Main screen buffer
	 */
	::SDL_Texture *m_ScreenBuffer;

	/**
	 * (Re-)init main window
	 *
	 * @param width Width
	 * @param height Height
	 */
	void initMainWindow(std::size_t width, std::size_t height);

public:
	/**
	 * Constructor
	 */
	CGUI();
	/**
	 * Deleted default copy constructor
	 *
	 * @param s Copied value
	 */
	CGUI(const CGUI &s) = delete;
	/**
	 * Destroyer
	 */
	~CGUI();
	/**
	 * Starts GUI
	 *
	 * @param argc Amount of parameters
	 * @param argv Array of parameters
	 * @return Exit code
	 */
	int startGUI(int argc, char **argv);
	/**
	 * Frame-ready callback
	 *
	 * @param frameTime Frame time
	 */
	void handleFrameRender(double frameTime);
};

}  // namespace gui

}  // namespace vpnes

#endif  // INCLUDE_VPNES_GUI_GUI_HPP_
