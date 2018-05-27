/**
 * @file
 *
 * Implements cross-platform console ui
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <memory>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vpnes/vpnes.hpp>
#include <vpnes/config/config.hpp>
#include <vpnes/config/gui.hpp>
#include <vpnes/gui/gui.hpp>
#include <vpnes/core/config.hpp>
#include <vpnes/core/nes.hpp>

namespace vpnes {

namespace gui {

/* CGUI */

/**
 * Constructor
 */
CGUI::CGUI()
    : m_Jitter()
    , m_TimeOverhead()
    , m_Time()
    , m_Config()
    , m_Window()
    , m_Renderer()
    , m_ScreenBuffer() {
	std::atexit(::SDL_Quit);
}

/**
 * Destroyer
 */
CGUI::~CGUI() {
	::SDL_DestroyWindow(m_Window);
}

/**
 * (Re-)init main window
 *
 * @param width Width
 * @param height Height
 */
void CGUI::initMainWindow(std::size_t width, std::size_t height) {
	if (!m_Window) {
		m_Window = ::SDL_CreateWindow(PACKAGE_STRING, SDL_WINDOWPOS_UNDEFINED,
		    SDL_WINDOWPOS_UNDEFINED, width, height,
		    SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
		if (!m_Window) {
			throw std::invalid_argument(SDL_GetError());
		}
		m_Renderer = ::SDL_CreateRenderer(m_Window, -1,
		    SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
		if (!m_Renderer) {
			throw std::invalid_argument(SDL_GetError());
		}
	} else {
		::SDL_DestroyTexture(m_ScreenBuffer);
		::SDL_SetWindowSize(m_Window, width, height);
	}
	int realWidth, realHeight;
	Uint32 pixelFormat;
	::SDL_GetRendererOutputSize(m_Renderer, &realWidth, &realHeight);
	pixelFormat = ::SDL_GetWindowPixelFormat(m_Window);
	if (pixelFormat == SDL_PIXELFORMAT_UNKNOWN) {
		pixelFormat = SDL_PIXELFORMAT_RGBA32;
	}
	m_ScreenBuffer = ::SDL_CreateTexture(m_Renderer, pixelFormat,
	    SDL_RENDERER_TARGETTEXTURE, realWidth, realHeight);
	if (!m_ScreenBuffer) {
		throw std::invalid_argument(SDL_GetError());
	}
}

/**
 * Starts GUI
 *
 * @param argc Amount of parameters
 * @param argv Array of parameters
 * @return Exit code
 */
int CGUI::startGUI(config::SApplicationConfig* appConfig) {
	try {
		if (::SDL_Init(SDL_INIT_EVERYTHING) < 0) {
			throw std::invalid_argument(SDL_GetError());
		}

		core::SNESConfig nesConfig;

		std::ifstream inputFile = appConfig->getInputFile();
		nesConfig.configure(appConfig->getCoreConfig(), &inputFile);
		inputFile.close();

		auto uiConfig = appConfig->getGuiConfig();
		initMainWindow(uiConfig.getWidth(), uiConfig.getHeight());

		m_NES.reset(nesConfig.createInstance(this));

		m_Jitter = 0;
		m_TimeOverhead = 0;
		m_Time = std::chrono::high_resolution_clock::now();

		m_NES->powerUp();
	} catch (const std::invalid_argument &e) {
		std::cerr << e.what() << std::endl;
	} catch (const std::exception &e) {
		std::cerr << "Unknown error: " << e.what() << std::endl;
		std::cerr << std::strerror(errno) << std::endl;
	}

	return EXIT_SUCCESS;
}

/**
 * Frame-ready callback
 *
 * @param frameTime Frame time
 */
void CGUI::handleFrameRender(double frameTime) {
	::SDL_Event event;
	::SDL_RenderCopy(m_Renderer, m_ScreenBuffer, nullptr, nullptr);
	::SDL_RenderPresent(m_Renderer);
	while (::SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			m_NES->turnOff();
			break;
		}
	}
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
		std::stringstream ss;
		ss << "FPS: "
		   << curFrame * 1000.0 /
		          std::chrono::duration_cast<std::chrono::milliseconds>(
		              newTime - lastTime)
		              .count();
		::SDL_SetWindowTitle(m_Window, ss.str().c_str());
		curFrame = 0;
	}
#else
	std::chrono::high_resolution_clock::time_point lastTime = m_Time;
	m_Jitter += frameTime;
	if (m_Jitter > m_TimeOverhead) {
		int64_t waitTime = static_cast<std::int64_t>(m_Jitter - m_TimeOverhead);
		std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
		m_Time = std::chrono::high_resolution_clock::now();
		int64_t actualWait = static_cast<std::int64_t>(
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
