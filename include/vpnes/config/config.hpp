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

#ifndef INCLUDE_VPNES_CONFIG_CONFIG_HPP_
#define INCLUDE_VPNES_CONFIG_CONFIG_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <string>
#include <boost/program_options.hpp>
#include <vpnes/vpnes.hpp>
#include <vpnes/config/gui.hpp>
#include <vpnes/config/core.hpp>

namespace po = boost::program_options;

namespace vpnes {

namespace config {

/**
 * Application configuration
 */
struct SApplicationConfig {
private:
	/**
	 * Path to rom
	 */
	std::string m_inputFile;

	/**
	 * Path to config file
	 */
	std::string m_configFile;

  SGuiConfig m_guiConfig;
	SCoreConfig m_coreConfig;

protected:
	/**
	 * Define options that will be available on CLI
	 * 
	 * @return po::options_description& for future use for parsing
	 */
	po::options_description defineCMDOptions();

public:
	/**
	 * Parses command line options
	 *
	 * @param argc Amount of parameters
	 * @param argv Array of parameters
	 */
	bool parseOptions(int argc, char **argv);
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
		return m_inputFile.size() > 0;
	}
	/**
	 * Sets input file
	 *
	 * @param fileName Input file path
	 */
	void setInputFile(const char *fileName) {
		m_inputFile = fileName;
	}
	/**
	 * Opens input file and constructs ifstream object
	 *
	 * @return std::ifstream for reading file
	 */
	std::ifstream getInputFile();

	SGuiConfig& getGuiConfig() {
		return m_guiConfig;
	}

	SCoreConfig& getCoreConfig() {
		return m_coreConfig;
	}
};

}  // namespace config

}  // namespace vpnes

#endif  // INCLUDE_VPNES_CONFIG_CONFIG_HPP_
