/**
 * @file
 * TAP log formatter for boost unit tests
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

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log_formatter.hpp>
#include <boost/test/execution_monitor.hpp>
#include <iostream>

using namespace boost;
using namespace boost::unit_test;

/**
 * Simple implementation of TAP log formatter
 */
class tap_formatter : public unit_test_log_formatter {
private:
	bool ng = false;
	const_string module_name;

public:
	void log_start(std::ostream &os, counter_t num_tests) {
		os << "1.." << num_tests << std::endl;
	}
	void log_finish(std::ostream &os) {
		os.flush();
	}
	void log_build_info(std::ostream &) {
	}
	void test_unit_start(std::ostream &os, test_unit const &tu) {
		os << "# Starting " << tu.p_type_name << ' ' << tu.p_name << std::endl;
		if (tu.p_type == TUT_CASE) {
			ng = false;
		}
		if (tu.p_type == TUT_SUITE) {
			module_name = tu.p_name;
		}
	}
	void test_unit_finish(
	    std::ostream &os, test_unit const &tu, unsigned long elapsed) {
		if (tu.p_type == TUT_CASE) {
			if (ng) {
				os << "not ";
			}
			os << "ok " << tu.p_name << " (" << module_name << ')' << std::endl;
		}
		if (tu.p_type == TUT_SUITE) {
			module_name.clear();
		}
		os << "# Finished " << tu.p_type_name << ' ' << tu.p_name << std::endl;
	}
	void test_unit_skipped(
	    std::ostream &os, test_unit const &tu, const_string reason) {
		if (tu.p_type == TUT_CASE) {
			os << "ok " << tu.p_name << " (" << module_name
			   << ") # SKIP: " << reason << std::endl;
		}
		os << "# Skipped " << tu.p_type_name << ' ' << tu.p_name << ": "
		   << reason << std::endl;
	}
	void test_unit_skipped(std::ostream &os, test_unit const &tu) {
		if (tu.p_type == TUT_CASE) {
			os << "ok " << tu.p_name << " (" << module_name << ") # SKIP"
			   << std::endl;
		}
		os << "# Skipped " << tu.p_type_name << ' ' << tu.p_name << std::endl;
	}
	void test_unit_aborted(std::ostream &os, test_unit const &tu) {
		os << "# Aborted " << tu.p_type_name << ' ' << tu.p_name << std::endl;
	}
	void log_exception_start(std::ostream &os,
	    log_checkpoint_data const &checkpoint,
	    execution_exception const &exception) {
		auto &loc = exception.where();
		os << "# Exception at ";
		if (!loc.m_function.is_empty()) {
			os << loc.m_function << ' ';
		}
		os << loc.m_file_name << ':' << loc.m_line_num << " "
		   << exception.what() << std::endl;
		if (!checkpoint.m_file_name.is_empty()) {
			os << "# Last checkpoint was at " << checkpoint.m_file_name << ':'
			   << checkpoint.m_line_num << ' ' << checkpoint.m_message
			   << std::endl;
		}
	}
	void log_exception_finish(std::ostream &os) {
		os.flush();
		ng = true;
	}
	void log_entry_start(
	    std::ostream &os, log_entry_data const &data, log_entry_types type) {
		if (type == unit_test_log_formatter::BOOST_UTL_ET_ERROR ||
		    type == unit_test_log_formatter::BOOST_UTL_ET_FATAL_ERROR) {
			ng = true;
			os << "# Assertion failed at " << data.m_file_name << ':'
			   << data.m_line_num << std::endl;
		}
		os << "# ";
	}
	void log_entry_value(std::ostream &os, const_string str) {
		os << str;
	}
	void log_entry_finish(std::ostream &os) {
		os << std::endl;
	}
	void entry_context_start(std::ostream &, log_level) {
	}
	void log_entry_context(std::ostream &, log_level, const_string) {
	}
	void entry_context_finish(std::ostream &, log_level) {
	}
};

/**
 * Global configuration
 */
struct TestConfiguration {
	TestConfiguration() {
		unit_test_log.set_formatter(new tap_formatter());
		unit_test_log.set_threshold_level(log_test_units);
	}
	~TestConfiguration() {
	}
};

BOOST_TEST_GLOBAL_CONFIGURATION(TestConfiguration);
