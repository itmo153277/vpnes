/**
 * @file
 *
 * Defines bus classes
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

#ifndef VPNES_INCLUDE_CORE_BUS_HPP_
#define VPNES_INCLUDE_CORE_BUS_HPP_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <type_traits>
#include <vector>
#include <vpnes/vpnes.hpp>
#include <vpnes/core/device.hpp>

namespace vpnes {

namespace core {

/**
 * Memory map
 */
typedef std::vector<std::uint8_t *> MemoryMap;

/**
 * Defines basic bus
 */
class CBaseBus {
protected:
	/**
	 * Open bus value
	 */
	std::uint8_t m_OpenBus;
	/**
	 * Write value
	 */
	std::uint8_t m_WriteBuf;
	/**
	 * Dummy write buffer
	 */
	std::uint8_t m_DummyWrite;
	/**
	 * Constructs the object
	 */
	CBaseBus() :
			m_OpenBus(0x40), m_WriteBuf(), m_DummyWrite() {
	}
	/**
	 * Deleted copy constructor
	 *
	 * @param s Copied value
	 */
	CBaseBus(const CBaseBus &s) = delete;
public:
	/**
	 * Destroys the object
	 */
	virtual ~CBaseBus() = default;

	/**
	 * Reads memory from the bus
	 *
	 * @param addr Address
	 * @return Value at the address
	 */
	virtual std::uint8_t readMemory(std::uint16_t addr) = 0;
	/**
	 * Writes memory on the bus
	 *
	 * @param s Value
	 * @param addr Address
	 */
	virtual void writeMemory(std::uint8_t s, std::uint16_t addr) = 0;

	//TODO: add hooks
};

namespace Banks {

/**
 * Base bank meta class
 */
struct BaseBank {
	enum {
		ReadSize = 0 //!< Requested read bytes
	};
	enum {
		WriteSize = 0 //!< Requested write bytes
	};
	enum {
		ModSize = 0 //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Buffer pointer
	 */
	static void mapRead(MemoryMap::iterator iter, std::uint8_t *openBus,
			std::uint8_t *buf) {
		assert(false);
	}
	/**
	 * Maps to write map
	 *
	 * @param iter Write map iterator
	 * @param dummy Dummy value
	 * @param buf Buffer pointer
	 */
	static void mapWrite(MemoryMap::iterator iter, std::uint8_t *dummy,
			std::uint8_t *buf) {
		assert(false);
	}
	/**
	 * Maps to mod map
	 *
	 * @param iter Mod map iterator
	 * @param writeBuf Write buffer
	 * @param buf Buffer pointer
	 */
	static void mapMod(MemoryMap::iterator iter, std::uint8_t *writeBuf,
			std::uint8_t *buf) {
		assert(false);
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(MemoryMap::iterator iter,
			std::uint16_t addr) {
		assert(false);
		return iter;
	}
	/**
	 * Gives write iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Write iterator
	 */
	static MemoryMap::iterator getAddrWrite(MemoryMap::iterator iter,
			std::uint16_t addr) {
		assert(false);
		return iter;
	}
	/**
	 * Gives mod iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Mod iterator
	 */
	static MemoryMap::iterator getAddrMod(MemoryMap::iterator iter,
			std::uint16_t addr) {
		assert(false);
		return iter;
	}
private:
	/**
	 * Prevent class from constructing
	 */
	BaseBank();
};

/**
 * Open bus bank (reads open bus, writes nothing, no bus conflict
 */
struct OpenBus: BaseBank {
	enum {
		ReadSize = 1 //!< Requested read bytes
	};
	enum {
		WriteSize = 1 //!< Requested write bytes
	};
	enum {
		ModSize = 1 //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Buffer pointer
	 */
	static void mapRead(MemoryMap::iterator iter, std::uint8_t *openBus,
			std::uint8_t *buf) {
		*iter = openBus;
	}
	/**
	 * Maps to write map
	 *
	 * @param iter Write map iterator
	 * @param dummy Dummy value
	 * @param buf Buffer pointer
	 */
	static void mapWrite(MemoryMap::iterator iter, std::uint8_t *dummy,
			std::uint8_t *buf) {
		*iter = dummy;
	}
	/**
	 * Maps to mod map
	 *
	 * @param iter Mod map iterator
	 * @param writeBuf Write buffer
	 * @param buf Buffer pointer
	 */
	static void mapMod(MemoryMap::iterator iter, std::uint8_t *writeBuf,
			std::uint8_t *buf) {
		*iter = writeBuf;
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter;
	}
	/**
	 * Gives write iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Write iterator
	 */
	static MemoryMap::iterator getAddrWrite(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter;
	}
	/**
	 * Gives mod iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Mod iterator
	 */
	static MemoryMap::iterator getAddrMod(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter;
	}
};

/**
 * Read only bank (reads buf, writes nothing, no bus conflict
 */
template<std::uint16_t Base, std::size_t Size, std::size_t BufSize>
struct ReadOnly: BaseBank {
	enum {
		ReadSize = Size //!< Requested read bytes
	};
	enum {
		WriteSize = 1 //!< Requested write bytes
	};
	enum {
		ModSize = 1 //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Buffer pointer
	 */
	static void mapRead(MemoryMap::iterator iter, std::uint8_t *openBus,
			std::uint8_t *buf) {
		std::size_t i = 0;
		for (MemoryMap::iterator end = iter + Size; iter != end; ++iter) {
			*iter = buf + i;
			i++;
			if (i >= BufSize) {
				i = 0;
			}
		}
	}
	/**
	 * Maps to write map
	 *
	 * @param iter Write map iterator
	 * @param dummy Dummy value
	 * @param buf Buffer pointer
	 */
	static void mapWrite(MemoryMap::iterator iter, std::uint8_t *dummy,
			std::uint8_t *buf) {
		*iter = dummy;
	}
	/**
	 * Maps to mod map
	 *
	 * @param iter Mod map iterator
	 * @param writeBuf Write buffer
	 * @param buf Buffer pointer
	 */
	static void mapMod(MemoryMap::iterator iter, std::uint8_t *writeBuf,
			std::uint8_t *buf) {
		*iter = writeBuf;
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter + (addr - Base);
	}
	/**
	 * Gives write iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrWrite(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter;
	}
	/**
	 * Gives mod iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Mod iterator
	 */
	static MemoryMap::iterator getAddrMod(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter;
	}
};

/**
 * Read only bank with conflicts (reads buf, writes nothing, bus conflict
 */
template<std::uint16_t Base, std::size_t Size, std::size_t BufSize>
struct ReadOnlyWithConflict: BaseBank {
	enum {
		ReadSize = Size //!< Requested read bytes
	};
	enum {
		WriteSize = 1 //!< Requested write bytes
	};
	enum {
		ModSize = Size //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Buffer pointer
	 */
	static void mapRead(MemoryMap::iterator iter, std::uint8_t *openBus,
			std::uint8_t *buf) {
		std::size_t i = 0;
		for (MemoryMap::iterator end = iter + Size; iter != end; ++iter) {
			*iter = buf + i;
			i++;
			if (i >= BufSize) {
				i = 0;
			}
		}
	}
	/**
	 * Maps to write map
	 *
	 * @param iter Write map iterator
	 * @param dummy Dummy value
	 * @param buf Buffer pointer
	 */
	static void mapWrite(MemoryMap::iterator iter, std::uint8_t *dummy,
			std::uint8_t *buf) {
		*iter = dummy;
	}
	/**
	 * Maps to mod map
	 *
	 * @param iter Mod map iterator
	 * @param writeBuf Write buffer
	 * @param buf Buffer pointer
	 */
	static void mapMod(MemoryMap::iterator iter, std::uint8_t *writeBuf,
			std::uint8_t *buf) {
		std::size_t i = 0;
		for (MemoryMap::iterator end = iter + Size; iter != end; ++iter) {
			*iter = buf + i;
			i++;
			if (i >= BufSize) {
				i = 0;
			}
		}
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter + (addr - Base);
	}
	/**
	 * Gives write iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Write iterator
	 */
	static MemoryMap::iterator getAddrWrite(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter;
	}
	/**
	 * Gives mod iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Mod iterator
	 */
	static MemoryMap::iterator getAddrMod(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter + (addr - Base);
	}
};

/**
 * Write only bank (reads open bus, writes buf, no bus conflict
 */
template<std::uint16_t Base, std::size_t Size, std::size_t BufSize>
struct WriteOnly: BaseBank {
	enum {
		ReadSize = 1 //!< Requested read bytes
	};
	enum {
		WriteSize = Size //!< Requested write bytes
	};
	enum {
		ModSize = 1 //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Buffer pointer
	 */
	static void mapRead(MemoryMap::iterator iter, std::uint8_t *openBus,
			std::uint8_t *buf) {
		*iter = openBus;
	}
	/**
	 * Maps to write map
	 *
	 * @param iter Write map iterator
	 * @param dummy Dummy value
	 * @param buf Buffer pointer
	 */
	static void mapWrite(MemoryMap::iterator iter, std::uint8_t *dummy,
			std::uint8_t *buf) {
		std::size_t i = 0;
		for (MemoryMap::iterator end = iter + Size; iter != end; ++iter) {
			*iter = buf + i;
			i++;
			if (i >= BufSize) {
				i = 0;
			}
		}
	}
	/**
	 * Maps to mod map
	 *
	 * @param iter Mod map iterator
	 * @param writeBuf Write buffer
	 * @param buf Buffer pointer
	 */
	static void mapMod(MemoryMap::iterator iter, std::uint8_t *writeBuf,
			std::uint8_t *buf) {
		*iter = writeBuf;
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter;
	}
	/**
	 * Gives write iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Write iterator
	 */
	static MemoryMap::iterator getAddrWrite(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter + (addr - Base);
	}
	/**
	 * Gives mod iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Mod iterator
	 */
	static MemoryMap::iterator getAddrMod(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter;
	}
};

/**
 * Read-Write bank (reads buf, writes buf, no bus conflict
 */
template<std::uint16_t Base, std::size_t Size, std::size_t BufSize>
struct ReadWrite: BaseBank {
	enum {
		ReadSize = Size //!< Requested read bytes
	};
	enum {
		WriteSize = Size //!< Requested write bytes
	};
	enum {
		ModSize = 1 //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Buffer pointer
	 */
	static void mapRead(MemoryMap::iterator iter, std::uint8_t *openBus,
			std::uint8_t *buf) {
		std::size_t i = 0;
		for (MemoryMap::iterator end = iter + Size; iter != end; ++iter) {
			*iter = buf + i;
			i++;
			if (i >= BufSize) {
				i = 0;
			}
		}
	}
	/**
	 * Maps to write map
	 *
	 * @param iter Write map iterator
	 * @param dummy Dummy value
	 * @param buf Buffer pointer
	 */
	static void mapWrite(MemoryMap::iterator iter, std::uint8_t *dummy,
			std::uint8_t *buf) {
		std::size_t i = 0;
		for (MemoryMap::iterator end = iter + Size; iter != end; ++iter) {
			*iter = buf + i;
			i++;
			if (i >= BufSize) {
				i = 0;
			}
		}
	}
	/**
	 * Maps to mod map
	 *
	 * @param iter Mod map iterator
	 * @param writeBuf Write buffer
	 * @param buf Buffer pointer
	 */
	static void mapMod(MemoryMap::iterator iter, std::uint8_t *writeBuf,
			std::uint8_t *buf) {
		*iter = writeBuf;
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter + (addr - Base);
	}
	/**
	 * Gives write iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Write iterator
	 */
	static MemoryMap::iterator getAddrWrite(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter + (addr - Base);
	}
	/**
	 * Gives mod iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Mod iterator
	 */
	static MemoryMap::iterator getAddrMod(MemoryMap::iterator iter,
			std::uint16_t addr) {
		return iter;
	}
};

}

/**
 * Bank config
 */
template<class ...>
struct BankConfig;

/**
 * Empty bank config
 */
template<>
struct BankConfig<> {
	enum {
		BanksCount = 0 //!< Total banks
	};
	enum {
		ReadSize = 0 //!< Requested read bytes
	};
	enum {
		WriteSize = 0 //!< Requested write bytes
	};
	enum {
		ModSize = 0 //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 */
	static void mapRead(MemoryMap::iterator iter, std::uint8_t *openBus) {
	}
	/**
	 * Maps to write map
	 *
	 * @param iter Write map iterator
	 * @param dummy Dummy value
	 */
	static void mapWrite(MemoryMap::iterator iter, std::uint8_t *dummy) {
	}
	/**
	 * Maps to mod map
	 *
	 * @param iter Mod map iterator
	 * @param writeBuf Write buffer
	 */
	static void mapMod(MemoryMap::iterator iter, std::uint8_t *writeBuf) {
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param bank Bank number
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(MemoryMap::iterator iter,
			std::size_t bank, std::uint16_t addr) {
		assert(false);
		return iter;
	}
	/**
	 * Gives pair of write and mod iterators
	 *
	 * @param iterWrite Base write iterator
	 * @param iterMod Base mod iterator
	 * @param bank Bank number
	 * @param addr Address
	 * @return Pair of write and mod iterator
	 */
	static std::pair<MemoryMap::iterator, MemoryMap::iterator> getAddrWrite(
			MemoryMap::iterator iterWrite, MemoryMap::iterator iterMod,
			std::size_t bank, std::uint16_t addr) {
		assert(false);
		return std::make_pair(iterWrite, iterMod);
	}
private:
	/**
	 * Forbid construction
	 */
	BankConfig();
};

/**
 * Filled bank config
 */
template<class FirstClass, class ... OtherClasses>
struct BankConfig<FirstClass, OtherClasses ...> {
	static_assert(std::is_base_of<Banks::BaseBank, FirstClass>::value,
			"Bank config contains non-bank class");
	enum {
		BanksCount = 1 + BankConfig<OtherClasses...>::BanksCount //!< Total banks
	};
	enum {
		ReadSize = FirstClass::ReadSize + BankConfig<OtherClasses...>::ReadSize //!< Requested read bytes
	};
	enum {
		WriteSize = FirstClass::WriteSize
				+ BankConfig<OtherClasses...>::WriteSize //!< Requested write bytes
	};
	enum {
		ModSize = FirstClass::ModSize + BankConfig<OtherClasses...>::ModSize //!< Requested mod bytes
	};
	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Current buffer
	 * @param args Buffers
	 */
	template<typename ... Args>
	static void mapRead(MemoryMap::iterator iter, std::uint8_t *openBus,
			std::uint8_t *buf, Args && ... args) {
		FirstClass::mapRead(iter, openBus, buf);
		BankConfig<OtherClasses...>::mapRead(iter + FirstClass::ReadSize,
				openBus, std::forward<Args>(args) ...);
	}
	/**
	 * Maps to write map
	 *
	 * @param iter Write map iterator
	 * @param dummy Dummy value
	 * @param buf Current buffer
	 * @param args Buffers
	 */
	template<typename ... Args>
	static void mapWrite(MemoryMap::iterator iter, std::uint8_t *dummy,
			std::uint8_t *buf, Args && ... args) {
		FirstClass::mapWrite(iter, dummy, buf);
		BankConfig<OtherClasses...>::mapWrite(iter + FirstClass::WriteSize,
				dummy, std::forward<Args>(args) ...);
	}
	/**
	 * Maps to mod map
	 *
	 * @param iter Mod map iterator
	 * @param writeBuf Write buffer
	 * @param buf Current buffer
	 * @param args Buffers
	 */
	template<typename ... Args>
	static void mapMod(MemoryMap::iterator iter, std::uint8_t *writeBuf,
			std::uint8_t *buf, Args && ... args) {
		FirstClass::mapMod(iter, writeBuf, buf);
		BankConfig<OtherClasses...>::mapMod(iter + FirstClass::ModSize,
				writeBuf, std::forward<Args>(args) ...);
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param bank Bank number
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(MemoryMap::iterator iter,
			std::size_t bank, std::uint16_t addr) {
		if (bank == 0)
			return FirstClass::getAddrRead(iter, addr);
		else
			return BankConfig<OtherClasses ...>::getAddrRead(
					iter + FirstClass::ReadSize, bank - 1, addr);
	}
	/**
	 * Gives pair of write and mod iterators
	 *
	 * @param iterWrite Base write iterator
	 * @param iterMod Base mod iterator
	 * @param bank Bank number
	 * @param addr Address
	 * @return Pair of write and mod iterator
	 */
	static std::pair<MemoryMap::iterator, MemoryMap::iterator> getAddrWrite(
			MemoryMap::iterator iterWrite, MemoryMap::iterator iterMod,
			std::size_t bank, std::uint16_t addr) {
		if (bank == 0)
			return std::make_pair(FirstClass::getAddrWrite(iterWrite, addr),
					FirstClass::getAddrMod(iterMod, addr));
		else
			return BankConfig<OtherClasses ...>::getAddrWrite(
					iterWrite + FirstClass::WriteSize,
					iterMod + FirstClass::ModSize, bank - 1, addr);
	}
private:
	/**
	 * Forbid construction
	 */
	BankConfig();
};

/**
 * Aggregate devices on bus
 */
template<class ...>
struct BusAggregate;

/**
 * Empty aggregated bus
 */
template<>
struct BusAggregate<> {
	enum {
		ReadSize = 0 //!< Requested read bytes
	};
	enum {
		WriteSize = 0 //!< Requested write bytes
	};
	enum {
		ModSize = 0 //!< Requested mod bytes
	};
	/**
	 * Gives read iterator
	 *
	 * @param iter Device iterator
	 * @param iterRead Base read iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(DevicePtrList::iterator iter,
			MemoryMap::iterator iterRead, std::uint16_t addr) {
		assert(false);
		return iterRead;
	}
	/**
	 * Gives pair of write and mod iterator
	 *
	 * @param iter Device iterator
	 * @param iterWrite Base write iterator
	 * @param iterMod Base mod iterator
	 * @param addr Address
	 * @return Pair of write and mod iterator
	 */
	static std::pair<MemoryMap::iterator, MemoryMap::iterator> getAddrWrite(
			DevicePtrList::iterator iter, MemoryMap::iterator iterWrite,
			MemoryMap::iterator iterMod, std::uint16_t addr) {
		assert(false);
		return std::make_pair(iterWrite, iterMod);
	}
private:
	/**
	 * Forbid construction
	 */
	BusAggregate();
};

/**
 * Filled aggregated bus
 */
template<class FirstDevice, class ... OtherDevices>
struct BusAggregate<FirstDevice, OtherDevices...> {
	static_assert(std::is_base_of<CDevice, FirstDevice>::value, "Cannot aggregate non-devices");
	enum {
		ReadSize = FirstDevice::BusConfig::BankConfig::ReadSize
				+ BusAggregate<OtherDevices...>::ReadSize //!< Requested read bytes
	};
	enum {
		WriteSize = FirstDevice::BusConfig::BankConfig::WriteSize
				+ BusAggregate<OtherDevices...>::WriteSize //!< Requested write bytes
	};
	enum {
		ModSize = FirstDevice::BusConfig::BankConfig::ModSize
				+ BusAggregate<OtherDevices...>::ModSize //!< Requested mod bytes
	};
	/**
	 * Gives read iterator
	 *
	 * @param iter Device iterator
	 * @param iterRead Base read iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(DevicePtrList::iterator iter,
			MemoryMap::iterator iterRead, std::uint16_t addr) {
		if (FirstDevice::BusConfig::isDeviceEnabled(addr)) {
			return FirstDevice::BusConfig::BankConfig::getAddrRead(iterRead,
					FirstDevice::BusConfig::getBank(addr,
							static_cast<FirstDevice &>(**iter)), addr);
		} else {
			return BusAggregate<OtherDevices...>::getAddrRead(iter + 1,
					iterRead + FirstDevice::BusConfig::BankConfig::ReadSize,
					addr);
		}
	}
	/**
	 * Gives pair of write and mod iterator
	 *
	 * @param iter Device iterator
	 * @param iterWrite Base write iterator
	 * @param iterMod Base mod iterator
	 * @param addr Address
	 * @return Pair of write and mod iterator
	 */
	static std::pair<MemoryMap::iterator, MemoryMap::iterator> getAddrWrite(
			DevicePtrList::iterator iter, MemoryMap::iterator iterWrite,
			MemoryMap::iterator iterMod, std::uint16_t addr) {
		if (FirstDevice::BusConfig::isDeviceEnabled(addr)) {
			return FirstDevice::BusConfig::BankConfig::getAddrWrite(iterWrite,
					iterMod,
					FirstDevice::BusConfig::getBank(addr,
							static_cast<FirstDevice &>(**iter)), addr);
		} else {
			return BusAggregate<OtherDevices...>::getAddrWrite(iter + 1,
					iterWrite + FirstDevice::BusConfig::BankConfig::WriteSize,
					iterMod + FirstDevice::BusConfig::BankConfig::ModSize, addr);
		}
	}
private:
	/**
	 * Forbid construction
	 */
	BusAggregate();
};

/**
 * Open bus device
 */
struct COpenBusDevice: public CDevice {
	/**
	 * Bus parameters
	 */
	struct BusConfig {
		/**
		 * Banks config
		 */
		typedef BankConfig<Banks::OpenBus> BankConfig;

		/**
		 * Maps to read map
		 *
		 * @param iter Read map iterator
		 * @param openBus Open bus
		 * @param device Device
		 */
		static void mapRead(MemoryMap::iterator iter, std::uint8_t *openBus,
				COpenBusDevice &device) {
			BankConfig::mapRead(iter, openBus, nullptr);
		}
		/**
		 * Maps to write map
		 *
		 * @param iter Write map iterator
		 * @param dummy Dummy value
		 * @param device Device
		 */
		static void mapWrite(MemoryMap::iterator iter, std::uint8_t *dummy,
				COpenBusDevice &device) {
			BankConfig::mapWrite(iter, dummy, nullptr);
		}
		/**
		 * Maps to mod map
		 *
		 * @param iter Mod map iterator
		 * @param writeBuf Write buffer
		 * @param device Device
		 */
		static void mapMod(MemoryMap::iterator iter, std::uint8_t *writeBuf,
				COpenBusDevice &device) {
			BankConfig::mapMod(iter, writeBuf, nullptr);
		}
		/**
		 * Checks if device is enabled
		 *
		 * @param addr Address
		 * @return True if enabled
		 */
		static bool isDeviceEnabled(std::uint16_t addr) {
			return true;
		}
		/**
		 * Determines active bank
		 *
		 * @param addr Address
		 * @param device Device
		 * @return Active bank
		 */
		static bool getBank(std::uint16_t addr, COpenBusDevice &device) {
			return 0;
		}
	};
};

/**
 * Configured bus
 */
template<class ...>
class CBus;

/**
 * Empty bus
 */
template<>
class CBus<> : public CBaseBus {
public:
	/**
	 * Constructor
	 */
	CBus() = default;
	/**
	 * Destructor
	 */
	~CBus() = default;

	/**
	 * Reads memory from the bus
	 *
	 * @param addr Address
	 * @return Value at the address
	 */
	std::uint8_t readMemory(std::uint16_t addr) {
		return m_OpenBus;
	}
	/**
	 * Writes memory on the bus
	 *
	 * @param s Value
	 * @param addr Address
	 */
	void writeMemory(std::uint8_t s, std::uint16_t addr) {
	}
};

/**
 * Configured bus
 */
template<class ... Devices>
class CBus: public CBaseBus {
private:
	static_assert(cond_and<std::is_base_of<CDevice, Devices>...>::value,
			"Bus can contain devices only");
	/**
	 * Open bus
	 */
	COpenBusDevice m_OpenBusDevice;
	/**
	 * Read map
	 */
	MemoryMap m_ReadArr;
	/**
	 * Write map
	 */
	MemoryMap m_WriteArr;
	/**
	 * Mod map
	 */
	MemoryMap m_ModArr;
	/**
	 * Device list
	 */
	DevicePtrList m_DeviceArr;

	/**
	 * Map read memory for device
	 *
	 * @param iter Current map iterator
	 * @param device Device
	 */
	template<class Device>
	void mapRead(MemoryMap::iterator iter, Device & device) {
		Device::BusConfig::mapRead(iter, &m_OpenBus, device);
	}
	/**
	 * Map read memory for devices
	 *
	 * @param iter Current map iterator
	 * @param device Device
	 * @param devices Other devices
	 */
	template<class FirstDevice, class ... OtherDevices>
	void mapRead(MemoryMap::iterator iter, FirstDevice & device,
			OtherDevices & ... devices) {
		FirstDevice::BusConfig::mapRead(iter, &m_OpenBus, device);
		mapRead<OtherDevices ...>(
				iter + FirstDevice::BusConfig::BankConfig::ReadSize,
				devices ...);
	}
	/**
	 * Map write memory for device
	 *
	 * @param iter Current map iterator
	 * @param device Device
	 */
	template<class Device>
	void mapWrite(MemoryMap::iterator iter, Device & device) {
		Device::BusConfig::mapWrite(iter, &m_DummyWrite, device);
	}
	/**
	 * Map write memory for devices
	 *
	 * @param iter Current map iterator
	 * @param device Device
	 * @param devices Other devices
	 */
	template<class FirstDevice, class ... OtherDevices>
	void mapWrite(MemoryMap::iterator iter, FirstDevice & device,
			OtherDevices & ... devices) {
		FirstDevice::BusConfig::mapWrite(iter, &m_DummyWrite, device);
		mapWrite<OtherDevices ...>(
				iter + FirstDevice::BusConfig::BankConfig::WriteSize,
				devices ...);
	}
	/**
	 * Map mod memory for device
	 *
	 * @param iter Current map iterator
	 * @param device Device
	 */
	template<class Device>
	void mapMod(MemoryMap::iterator iter, Device & device) {
		Device::BusConfig::mapMod(iter, &m_WriteBuf, device);
	}
	/**
	 * Map mod memory for devices
	 *
	 * @param iter Current map iterator
	 * @param device Device
	 * @param devices Other devices
	 */
	template<class FirstDevice, class ... OtherDevices>
	void mapMod(MemoryMap::iterator iter, FirstDevice & device,
			OtherDevices & ... devices) {
		FirstDevice::BusConfig::mapMod(iter, &m_WriteBuf, device);
		mapMod<OtherDevices ...>(
				iter + FirstDevice::BusConfig::BankConfig::ModSize,
				devices ...);
	}

public:
	/**
	 * Deleted default constructor
	 */
	CBus() = delete;
	/**
	 * Constructs the bus
	 *
	 * @param devices Devices
	 */
	CBus(Devices & ... devices) :
			m_OpenBusDevice(), m_ReadArr(
					BusAggregate<Devices ..., COpenBusDevice>::ReadSize,
					&m_OpenBus), m_WriteArr(
					BusAggregate<Devices ..., COpenBusDevice>::WriteSize,
					&m_DummyWrite), m_ModArr(
					BusAggregate<Devices ..., COpenBusDevice>::ModSize,
					&m_WriteBuf), m_DeviceArr(
					{ &devices..., &m_OpenBusDevice }) {
		mapRead(m_ReadArr.begin(), devices ..., m_OpenBusDevice);
		mapWrite(m_WriteArr.begin(), devices ..., m_OpenBusDevice);
		mapMod(m_ModArr.begin(), devices ..., m_OpenBusDevice);
	}
	/**
	 * Destructor
	 */
	~CBus() = default;

	/**
	 * Reads memory from the bus
	 *
	 * @param addr Address
	 * @return Value at the address
	 */
	std::uint8_t readMemory(std::uint16_t addr) {
		MemoryMap::iterator iter =
				BusAggregate<Devices..., COpenBusDevice>::getAddrRead(
						m_DeviceArr.begin(), m_ReadArr.begin(), addr);
		return **iter;
	}
	/**
	 * Writes memory on the bus
	 *
	 * @param s Value
	 * @param addr Address
	 */
	void writeMemory(std::uint8_t s, std::uint16_t addr) {
		auto iter = BusAggregate<Devices..., COpenBusDevice>::getAddrWrite(
				m_DeviceArr.begin(), m_WriteArr.begin(), m_ModArr.begin(),
				addr);
		m_WriteBuf = s;
		m_WriteBuf &= **(iter.second);
		**(iter.first) = m_WriteBuf;
	}
};

}

}

#endif /* VPNES_INCLUDE_CORE_BUS_HPP_ */
