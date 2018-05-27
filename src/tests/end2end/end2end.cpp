/**
 * @file
 * End-to-end test
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

#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <stdexcept>
#include <memory>
#include <chrono>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vpnes/vpnes.hpp>
#include <vpnes/config/config.hpp>
#include <vpnes/core/frontend.hpp>
#include <vpnes/core/config.hpp>

/**
 * Configuration for end-to-end testing
 */
struct SConfig : vpnes::config::SApplicationConfig {
public:
	/**
	 * Checks if timeout was specified
	 *
	 * @return True if specified
	 */
	bool hasTimeout() const {
		return true;
	}

	/**
	 * Gets timeout value
	 *
	 * @return Timeout value
	 */
	int getTimeout() const {
		return 60;
	}
};

/**
 * Frontend for testing
 */
class CTestFrontEnd : public vpnes::core::CFrontEnd {
public:
	/**
	 * Duration type
	 */
	typedef std::chrono::high_resolution_clock::duration duration_t;

private:
	/**
	 * Jitter
	 */
	double m_Jitter;
	/**
	 * Current time
	 */
	duration_t m_Time;
	/**
	 * Time limit
	 */
	duration_t m_TimeLimit;

public:
	/**
	 * Constructor
	 *
	 * @param timeLimit Running time limit
	 */
	explicit CTestFrontEnd(duration_t timeLimit)
	    : m_Jitter(), m_Time(), m_TimeLimit(timeLimit) {
	}
	/**
	 * Deleted default copy constructor
	 *
	 * @param s Copied value
	 */
	CTestFrontEnd(const CTestFrontEnd &s) = delete;
	/**
	 * Destructor
	 */
	~CTestFrontEnd() = default;
	/**
	 * Frame-ready callback
	 *
	 * @param frameTime Frame time
	 */
	void handleFrameRender(double frameTime) {
		m_Jitter += frameTime;
		auto millTime = static_cast<std::int64_t>(m_Jitter);
		m_Time += std::chrono::duration_cast<duration_t>(
		    std::chrono::milliseconds(millTime));
		m_Jitter -= millTime;
		if (m_Time > m_TimeLimit) {
			throw std::runtime_error("Timeout");
		}
	}
};

/**
 * Checks if NES is in valid state for test debug output
 *
 * @param nes NES
 * @return Is valid or not
 */
bool checkValidState(vpnes::core::CNES *nes) {
	auto debugger = nes->getDebugger();
	return debugger->directCPURead(0x6001) == 0xde &&
	       debugger->directCPURead(0x6002) == 0xb0 &&
	       debugger->directCPURead(0x6003) == 0x61;
}

/**
 * Entry point for e2e tester
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Exit code
 */
int main(int argc, char **argv) {
	try {
		SConfig config;
		config.parseOptions(argc, argv);
		if (!config.hasInputFile()) {
			throw std::invalid_argument("No input file specified");
		}
		if (!config.hasTimeout()) {
			throw std::invalid_argument("No timeout specified");
		}
		std::ifstream inputFile = config.getInputFile();
		vpnes::core::SNESConfig nesConfig;
		nesConfig.configure(config.getCoreConfig(), &inputFile);
		inputFile.close();
		int result = EXIT_FAILURE;
		bool inProgress = false;
		auto time = std::chrono::seconds(config.getTimeout());
		auto frontEnd = std::make_unique<CTestFrontEnd>(time);
		std::unique_ptr<vpnes::core::CNES> nes(
		    nesConfig.createInstance(frontEnd.get()));
		nes->getDebugger()->hookCPUWrite(0x6000, [&](std::uint16_t addr,
		                                             std::uint8_t val) {
			std::uint16_t readAddress;
			std::stringstream str;
			switch (val) {
			case 0x80:  // Start test
				if (inProgress) {
					throw std::invalid_argument("wrong state");
				}
				inProgress = true;
				break;
			case 0x81:  // Input required - skipping
				if (!checkValidState(nes.get()) || !inProgress) {
					throw std::invalid_argument("wrong state");
				}
				nes->turnOff();
				result = EXIT_SUCCESS;
				break;
			default:
				if (!checkValidState(nes.get()) || !inProgress) {
					throw std::invalid_argument("wrong state");
				}
				if (val >= 0x80) {
					throw std::invalid_argument("wrong result code");
				}
				for (readAddress = 0x6004; readAddress < 0x8000;
				     ++readAddress) {
					std::uint8_t readValue =
					    nes->getDebugger()->directCPURead(readAddress);
					if (readValue == 0) {
						break;
					}
					if (!std::isalnum(readValue) && !std::ispunct(readValue) &&
					    !std::isspace(readValue)) {
						throw std::invalid_argument(
						    "tried to print an invalid character");
					}
					str << readValue;
				}
				std::cout << str.str() << std::endl;
				nes->turnOff();
				if (val == 0) {
					result = EXIT_SUCCESS;
				}
			}
		});
		nes->powerUp();
		return result;
	} catch (const std::invalid_argument &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	} catch (const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	} catch (const std::exception &e) {
		std::cerr << "Unknown error: " << e.what() << std::endl;
		std::cerr << std::strerror(errno) << std::endl;
		return EXIT_FAILURE;
	}
}
