/**
 * @file
 *
 * Defines application config
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

#ifndef INCLUDE_VPNES_GUI_CONFIG_HPP_
#define INCLUDE_VPNES_GUI_CONFIG_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <string>
#include <vpnes/vpnes.hpp>

namespace vpnes {

namespace gui {

/**
 * Application configuration
 */
struct SApplicationConfig {
private:
	/**
	 * File name
	 */
	std::string inputFile;

protected:
	/**
	 * Parse command line option
	 *
	 * @param name Option name
	 * @param value Argument
	 * @return Valid option or not
	 */
	virtual bool parseOption(
	    const std::string &name, const std::string &value) {
		return false;
	}
	/**
	 * Parse command line option
	 *
	 * @param name Option name
	 * @return Valid option or not
	 */
	virtual bool parseOption(const std::string &name) {
		return false;
	}

public:
	/**
	 * Parses command line options
	 *
	 * @param argc Amount of parameters
	 * @param argv Array of parameters
	 */
	void parseOptions(int argc, char **argv);
	/**
	 * Sets default values
	 */
	SApplicationConfig();
	/**
	 * Deleted default copy constructor
	 *
	 * @param s Copied value
	 */
	SApplicationConfig(const SApplicationConfig &s) = delete;
	/**
	 * Destroys the object
	 */
	virtual ~SApplicationConfig() = default;

	/**
	 * Checks if the program has an input file
	 *
	 * @return True if input file is present
	 */
	inline bool hasInputFile() const noexcept {
		return inputFile.size() > 0;
	}
	/**
	 * Sets input file
	 *
	 * @param fileName Input file path
	 */
	void setInputFile(const char *fileName) {
		inputFile = fileName;
	}
	/**
	 * Opens input file and constructs ifstream object
	 *
	 * @return std::ifstream for reading file
	 */
	std::ifstream getInputFile();
};

}  // namespace gui

}  // namespace vpnes

#endif  // INCLUDE_VPNES_GUI_CONFIG_HPP_
