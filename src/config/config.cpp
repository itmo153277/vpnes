/**
 * @file
 *
 * Implements configuration parsing
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

#include <cassert>
#include <cstring>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/bind.hpp>
#include <vpnes/config/config.hpp>
#include <vpnes/config/gui.hpp>
#include <vpnes/config/core.hpp>

namespace po = boost::program_options;

namespace vpnes {

namespace config {

/* SApplicationConfig */

/**
 * Sets default values
 */
SApplicationConfig::SApplicationConfig() :
	m_inputFile(),
	m_configFile(),
	m_guiConfig(),
	m_coreConfig() {
}

void printUsage(std::ostream&, const po::options_description&, std::string);

/**
 * Parses command line options
 *
 * @param argc Amount of parameters
 * @param argv Array of parameters
 * @return true if at least rom supplied (in config or command line) 
 * 	and no error happened during options parsing
 */
bool SApplicationConfig::parseOptions(int argc, char **argv) {
	po::positional_options_description rom;
	rom.add("rom", 1);

	auto cmd_options = defineCMDOptions();
	po::variables_map variables_map;

	try {
		store(po::command_line_parser(argc, argv)
			.options(defineCMDOptions())
			.positional(rom)
			.run(), variables_map);

		po::notify(variables_map);
	} catch (std::exception& e) {
		std::cout << e.what() << "\n";
		printUsage(std::cout, cmd_options, argv[0]);

		return false;
	}

	// TODO(arhont375): should not fail on version/help print
	if (variables_map.count("version")) {
		std::cout << PACKAGE_BUILD << std::endl;

		return false;
	} else if (variables_map.count("help") || m_inputFile.empty()) {
		printUsage(std::cout, cmd_options, argv[0]);

		return false;
	}

	return true;
}

/**
 * Opens input file and constructs ifstream object
 *
 * @return std::ifstream for reading file
 */
std::ifstream SApplicationConfig::getInputFile() {
	assert(m_inputFile.size() > 0);

	std::ifstream file;

	file.exceptions(file.exceptions() | std::fstream::failbit);
	file.open(m_inputFile, std::fstream::binary);

	return file;
}

/**
 * Define options that will be available on CLI 
 * 
 * @return po::options_description 
 */
po::options_description SApplicationConfig::defineCMDOptions() {
	po::positional_options_description rom;
	rom.add("input-file", 1);

	po::options_description general("General options");
	general.add_options()
		("version,v", "print version string")
		("help", "produce help message")
		("config,c",
			po::value<std::string>(&m_configFile)->default_value("config.yaml"),
			"path to configuration file")
		("rom",
			po::value<std::string>(&m_inputFile),
			"path to rom file");

	po::options_description ui("UI options");
	ui.add_options()
		("width",
			po::value<int>()
				->notifier(boost::bind(&SGuiConfig::setWidth, &m_guiConfig, _1))
				->default_value(512),
			"set screen width")
		("height",
			po::value<int>()
				->notifier(boost::bind(&SGuiConfig::setHeight, &m_guiConfig, _1))
				->default_value(448),
			"set screen height");

	po::options_description cmd_options;
	cmd_options
		.add(general)
		.add(ui);

	return cmd_options;
}

void printUsage(std::ostream& out,
							  const po::options_description& options,
								std::string path_to_bin) {
	out << "Usage: " << path_to_bin << " [options] <path_to_rom>" << std::endl;
	out << options;
}


}  // namespace config

}  // namespace vpnes
