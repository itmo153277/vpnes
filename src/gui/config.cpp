/*
 * config.cpp
 *
 * Implements configuration parsing
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

#include <cassert>
#include <cstring>
#include <iostream>
#include <vpnes/gui/config.hpp>

using namespace ::std;
using namespace ::vpnes;
using namespace ::vpnes::gui;

/* SApplicationConfig */

/**
 * Sets default values
 */
SApplicationConfig::SApplicationConfig() :
		inputFile(NULL) {

}

/**
 * Destroys the object
 */
SApplicationConfig::~SApplicationConfig() {
	delete[] inputFile;
}

/**
 * Parses command line options
 *
 * @param argc Amount of parameters
 * @param argv Array of parameters
 */
void SApplicationConfig::parseOptions(int argc, char **argv) {
	//TODO: implement
	if (argc >= 2) {
		setInputFile(argv[1]);
	}
}

/**
 * Sets input file
 *
 * @param fileName Input file path
 */
void SApplicationConfig::setInputFile(const char *fileName) {
	delete[] inputFile;
	char *tempFile = new char[strlen(fileName)];
	strcpy(tempFile, fileName);
	inputFile = tempFile;
}

/**
 * Opens input file and constructs ifstream object
 *
 * @return std::ifstream for reading file
 */
ifstream SApplicationConfig::getInputFile() {
	assert(inputFile != NULL);
	ifstream file;
	file.exceptions(file.exceptions() | fstream::failbit);
	file.open(inputFile, fstream::binary);
	return file;
}
