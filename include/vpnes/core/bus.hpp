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
#include <memory>
#include <type_traits>
#include <unordered_map>
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
 * Hook with address
 */
class CAddrHook {
protected:
	/**
	 * Default constructor
	 */
	CAddrHook() = default;
	/**
	 * Executes the hook
	 *
	 * @param addr Address
	 */
	virtual void execute(std::uint16_t addr) = 0;

public:
	/**
	 * Default destructor
	 */
	virtual ~CAddrHook() = default;

	/**
	 * Executes the hook
	 *
	 * @param addr Address
	 */
	void operator()(std::uint16_t addr) {
		execute(addr);
	}
};

/**
 * Hook with address mapped to a device
 */
template <class T>
class CAddrHookMapped : public CAddrHook {
	static_assert(
	    std::is_base_of<CDevice, T>::value, "Can hook from devices only");

public:
	/**
	 * Hook in device
	 *
	 * @param addr Address
	 */
	typedef void (T::*addrHook_t)(std::uint16_t addr);

private:
	/**
	 * Device
	 */
	T *m_Device;
	/**
	 * Device hook
	 */
	addrHook_t m_Hook;

protected:
	/**
	 * Executes the hook
	 *
	 * @param addr Address
	 */
	void execute(std::uint16_t addr) {
		(m_Device->*m_Hook)(addr);
	}

public:
	/**
	 * Deleted default constructor
	 */
	CAddrHookMapped() = delete;
	/**
	 * Constructs the object
	 *
	 * @param device Device
	 * @param hook Hook
	 */
	CAddrHookMapped(T &device, addrHook_t hook)
	    : m_Device(&device), m_Hook(hook) {
	}
};

/**
 * Hook with address and value
 */
class CAddrValHook {
protected:
	/**
	 * Default constructor
	 */
	CAddrValHook() = default;

protected:
	/**
	 * Executes the hook
	 *
	 * @param s Value
	 * @param addr Address
	 */
	virtual void execute(std::uint8_t s, std::uint16_t addr) = 0;

public:
	/**
	 * Default destructor
	 */
	virtual ~CAddrValHook() = default;

	/**
	 * Executes the hook
	 *
	 * @param s Value
	 * @param addr Address
	 */
	void operator()(std::uint8_t s, std::uint16_t addr) {
		execute(s, addr);
	}
};

/**
 * Hook with address and value mapped to a device
 */
template <class T>
class CAddrValHookMapped : public CAddrValHook {
	static_assert(
	    std::is_base_of<CDevice, T>::value, "Can hook from devices only");

public:
	/**
	 * Hook in device
	 *
	 * @param s Value
	 * @param addr Address
	 */
	typedef void (T::*addrHook_t)(std::uint8_t s, std::uint16_t addr);

private:
	/**
	 * Device
	 */
	T *m_Device;
	/**
	 * Device hook
	 */
	addrHook_t m_Hook;

protected:
	/**
	 * Executes the hook
	 *
	 * @param s Value
	 * @param addr Address
	 */
	void execute(std::uint8_t s, std::uint16_t addr) {
		(m_Device->*m_Hook)(s, addr);
	}

public:
	/**
	 * Deleted default constructor
	 */
	CAddrValHookMapped() = delete;
	/**
	 * Constructs the object
	 *
	 * @param device Device
	 * @param hook Hook
	 */
	CAddrValHookMapped(T &device, addrHook_t hook)
	    : m_Device(&device), m_Hook(hook) {
	}
};

/**
 * Defines basic bus
 */
class CBus {
protected:
	/**
	 * Pre read hooks mapped to address
	 */
	typedef std::unordered_multimap<std::uint16_t, std::unique_ptr<CAddrHook>>
	    ReadHooksPre;
	/**
	 * Post read hooks mapped to address
	 */
	typedef std::unordered_multimap<std::uint16_t,
	    std::unique_ptr<CAddrValHook>>
	    ReadHooksPost;
	/**
	 * Write hooks mapped to address
	 */
	typedef std::unordered_multimap<std::uint16_t,
	    std::unique_ptr<CAddrValHook>>
	    WriteHooks;
	/**
	 * Pre read hooks mapped to address
	 */
	ReadHooksPre m_ReadHooksPre;
	/**
	 * Post read hooks mapped to address
	 */
	ReadHooksPost m_ReadHooksPost;
	/**
	 * Write hooks mapped to address
	 */
	WriteHooks m_WriteHooks;
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
	 * Executes all pre read hooks for an address
	 *
	 * @param addr Address
	 */
	void processPreReadHooks(std::uint16_t addr) {
		auto range = m_ReadHooksPre.equal_range(addr);
		for (auto hook = range.first; hook != range.second; ++hook) {
			(*hook->second)(addr);
		}
	}
	/**
	 * Executes all post read hooks for an address
	 *
	 * @param s Value
	 * @param addr Address
	 */
	void processPostReadHooks(std::uint8_t s, std::uint16_t addr) {
		auto range = m_ReadHooksPost.equal_range(addr);
		for (auto hook = range.first; hook != range.second; ++hook) {
			(*hook->second)(s, addr);
		}
	}
	/**
	 * Executes all write hooks for an address
	 *
	 * @param s Value
	 * @param addr Address
	 */
	void processWriteHooks(std::uint8_t s, std::uint16_t addr) {
		auto range = m_WriteHooks.equal_range(addr);
		for (auto hook = range.first; hook != range.second; ++hook) {
			(*hook->second)(s, addr);
		}
	}

	/**
	 * Deleted default constructor
	 */
	CBus() = delete;
	/**
	 * Constructs the object
	 *
	 * @param openBus Open bus value
	 */
	CBus(std::uint8_t openBus)
	    : m_ReadHooksPre()
	    , m_ReadHooksPost()
	    , m_WriteHooks()
	    , m_OpenBus(openBus)
	    , m_WriteBuf()
	    , m_DummyWrite() {
	}
	/**
	 * Deleted copy constructor
	 *
	 * @param s Copied value
	 */
	CBus(const CBus &s) = delete;

public:
	/**
	 * Destroys the object
	 */
	virtual ~CBus() = default;

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

	/**
	 * Adds new pre read hook
	 *
	 * @param addr Address
	 * @param device Device
	 * @param hook Hook in device
	 */
	template <class T>
	void addPreReadHook(std::uint16_t addr, T &device,
	    typename CAddrHookMapped<T>::addrHook_t hook) {
		m_ReadHooksPre.emplace(addr, new CAddrHookMapped<T>(device, hook));
	}
	/**
	 * Adds new post read hook
	 *
	 * @param addr Address
	 * @param device Device
	 * @param hook Hook in device
	 */
	template <class T>
	void addPostReadHook(std::uint16_t addr, T &device,
	    typename CAddrValHookMapped<T>::addrHook_t hook) {
		m_ReadHooksPost.emplace(addr, new CAddrValHookMapped<T>(device, hook));
	}
	/**
	 * Adds new write hook
	 *
	 * @param addr Address
	 * @param device Device
	 * @param hook Hook in device
	 */
	template <class T>
	void addWriteHook(std::uint16_t addr, T &device,
	    typename CAddrValHookMapped<T>::addrHook_t hook) {
		m_WriteHooks.emplace(addr, new CAddrValHookMapped<T>(device, hook));
	}
};

namespace banks {

/**
 * Base bank meta class
 */
struct BaseBank {
	enum {
		ReadSize = 0  //!< Requested read bytes
	};
	enum {
		WriteSize = 0  //!< Requested write bytes
	};
	enum {
		ModSize = 0  //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Buffer pointer
	 */
	static void mapRead(
	    MemoryMap::iterator iter, std::uint8_t *openBus, std::uint8_t *buf) {
		assert(false);
	}
	/**
	 * Maps to write map
	 *
	 * @param iter Write map iterator
	 * @param dummy Dummy value
	 * @param buf Buffer pointer
	 */
	static void mapWrite(
	    MemoryMap::iterator iter, std::uint8_t *dummy, std::uint8_t *buf) {
		assert(false);
	}
	/**
	 * Maps to mod map
	 *
	 * @param iter Mod map iterator
	 * @param writeBuf Write buffer
	 * @param buf Buffer pointer
	 */
	static void mapMod(
	    MemoryMap::iterator iter, std::uint8_t *writeBuf, std::uint8_t *buf) {
		assert(false);
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(
	    MemoryMap::iterator iter, std::uint16_t addr) {
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
	static MemoryMap::iterator getAddrWrite(
	    MemoryMap::iterator iter, std::uint16_t addr) {
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
	static MemoryMap::iterator getAddrMod(
	    MemoryMap::iterator iter, std::uint16_t addr) {
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
struct OpenBus : BaseBank {
	enum {
		ReadSize = 1  //!< Requested read bytes
	};
	enum {
		WriteSize = 1  //!< Requested write bytes
	};
	enum {
		ModSize = 1  //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Buffer pointer
	 */
	static void mapRead(
	    MemoryMap::iterator iter, std::uint8_t *openBus, std::uint8_t *buf) {
		*iter = openBus;
	}
	/**
	 * Maps to write map
	 *
	 * @param iter Write map iterator
	 * @param dummy Dummy value
	 * @param buf Buffer pointer
	 */
	static void mapWrite(
	    MemoryMap::iterator iter, std::uint8_t *dummy, std::uint8_t *buf) {
		*iter = dummy;
	}
	/**
	 * Maps to mod map
	 *
	 * @param iter Mod map iterator
	 * @param writeBuf Write buffer
	 * @param buf Buffer pointer
	 */
	static void mapMod(
	    MemoryMap::iterator iter, std::uint8_t *writeBuf, std::uint8_t *buf) {
		*iter = writeBuf;
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter;
	}
	/**
	 * Gives write iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Write iterator
	 */
	static MemoryMap::iterator getAddrWrite(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter;
	}
	/**
	 * Gives mod iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Mod iterator
	 */
	static MemoryMap::iterator getAddrMod(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter;
	}
};

/**
 * Read only bank (reads buf, writes nothing, no bus conflict
 */
template <std::uint16_t Base, std::size_t Size, std::size_t BufSize>
struct ReadOnly : BaseBank {
	enum {
		ReadSize = Size  //!< Requested read bytes
	};
	enum {
		WriteSize = 1  //!< Requested write bytes
	};
	enum {
		ModSize = 1  //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Buffer pointer
	 */
	static void mapRead(
	    MemoryMap::iterator iter, std::uint8_t *openBus, std::uint8_t *buf) {
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
	static void mapWrite(
	    MemoryMap::iterator iter, std::uint8_t *dummy, std::uint8_t *buf) {
		*iter = dummy;
	}
	/**
	 * Maps to mod map
	 *
	 * @param iter Mod map iterator
	 * @param writeBuf Write buffer
	 * @param buf Buffer pointer
	 */
	static void mapMod(
	    MemoryMap::iterator iter, std::uint8_t *writeBuf, std::uint8_t *buf) {
		*iter = writeBuf;
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter + (addr - Base);
	}
	/**
	 * Gives write iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrWrite(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter;
	}
	/**
	 * Gives mod iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Mod iterator
	 */
	static MemoryMap::iterator getAddrMod(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter;
	}
};

/**
 * Read only bank with conflicts (reads buf, writes nothing, bus conflict
 */
template <std::uint16_t Base, std::size_t Size, std::size_t BufSize>
struct ReadOnlyWithConflict : BaseBank {
	enum {
		ReadSize = Size  //!< Requested read bytes
	};
	enum {
		WriteSize = 1  //!< Requested write bytes
	};
	enum {
		ModSize = Size  //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Buffer pointer
	 */
	static void mapRead(
	    MemoryMap::iterator iter, std::uint8_t *openBus, std::uint8_t *buf) {
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
	static void mapWrite(
	    MemoryMap::iterator iter, std::uint8_t *dummy, std::uint8_t *buf) {
		*iter = dummy;
	}
	/**
	 * Maps to mod map
	 *
	 * @param iter Mod map iterator
	 * @param writeBuf Write buffer
	 * @param buf Buffer pointer
	 */
	static void mapMod(
	    MemoryMap::iterator iter, std::uint8_t *writeBuf, std::uint8_t *buf) {
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
	static MemoryMap::iterator getAddrRead(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter + (addr - Base);
	}
	/**
	 * Gives write iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Write iterator
	 */
	static MemoryMap::iterator getAddrWrite(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter;
	}
	/**
	 * Gives mod iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Mod iterator
	 */
	static MemoryMap::iterator getAddrMod(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter + (addr - Base);
	}
};

/**
 * Write only bank (reads open bus, writes buf, no bus conflict
 */
template <std::uint16_t Base, std::size_t Size, std::size_t BufSize>
struct WriteOnly : BaseBank {
	enum {
		ReadSize = 1  //!< Requested read bytes
	};
	enum {
		WriteSize = Size  //!< Requested write bytes
	};
	enum {
		ModSize = 1  //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Buffer pointer
	 */
	static void mapRead(
	    MemoryMap::iterator iter, std::uint8_t *openBus, std::uint8_t *buf) {
		*iter = openBus;
	}
	/**
	 * Maps to write map
	 *
	 * @param iter Write map iterator
	 * @param dummy Dummy value
	 * @param buf Buffer pointer
	 */
	static void mapWrite(
	    MemoryMap::iterator iter, std::uint8_t *dummy, std::uint8_t *buf) {
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
	static void mapMod(
	    MemoryMap::iterator iter, std::uint8_t *writeBuf, std::uint8_t *buf) {
		*iter = writeBuf;
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter;
	}
	/**
	 * Gives write iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Write iterator
	 */
	static MemoryMap::iterator getAddrWrite(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter + (addr - Base);
	}
	/**
	 * Gives mod iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Mod iterator
	 */
	static MemoryMap::iterator getAddrMod(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter;
	}
};

/**
 * Read-Write bank (reads buf, writes buf, no bus conflict
 */
template <std::uint16_t Base, std::size_t Size, std::size_t BufSize>
struct ReadWrite : BaseBank {
	enum {
		ReadSize = Size  //!< Requested read bytes
	};
	enum {
		WriteSize = Size  //!< Requested write bytes
	};
	enum {
		ModSize = 1  //!< Requested mod bytes
	};

	/**
	 * Maps to read map
	 *
	 * @param iter Read map iterator
	 * @param openBus Open bus value
	 * @param buf Buffer pointer
	 */
	static void mapRead(
	    MemoryMap::iterator iter, std::uint8_t *openBus, std::uint8_t *buf) {
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
	static void mapWrite(
	    MemoryMap::iterator iter, std::uint8_t *dummy, std::uint8_t *buf) {
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
	static void mapMod(
	    MemoryMap::iterator iter, std::uint8_t *writeBuf, std::uint8_t *buf) {
		*iter = writeBuf;
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter + (addr - Base);
	}
	/**
	 * Gives write iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Write iterator
	 */
	static MemoryMap::iterator getAddrWrite(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter + (addr - Base);
	}
	/**
	 * Gives mod iterator
	 *
	 * @param iter Base iterator
	 * @param addr Address
	 * @return Mod iterator
	 */
	static MemoryMap::iterator getAddrMod(
	    MemoryMap::iterator iter, std::uint16_t addr) {
		return iter;
	}
};

/**
 * Bank offset by index
 */
template <std::size_t Index, class... Banks>
struct BankOffsetValue;

/**
 * Bank offset for index 0
 */
template <class FirstBank, class... OtherBanks>
struct BankOffsetValue<0, FirstBank, OtherBanks...> {
	enum {
		ReadSize = FirstBank::ReadSize  //!< Requested read bytes
	};
	enum {
		ReadOffset = 0  //!< Read offset
	};
	enum {
		WriteSize = FirstBank::WriteSize  //!< Requested write bytes
	};
	enum {
		WriteOffset = 0  //!< Write Offset
	};
	enum {
		ModSize = FirstBank::ModSize  //!< Requested mod bytes
	};
	enum {
		ModOffset = 0  //!< Mod offset
	};
};

/**
 * Bank offset for general index
 */
template <std::size_t Index, class FirstBank, class... OtherBanks>
struct BankOffsetValue<Index, FirstBank, OtherBanks...> {
	enum {
		ReadSize = FirstBank::ReadSize +
		           BankOffsetValue<Index - 1,
		               OtherBanks...>::ReadSize  //!< Requested read bytes
	};
	enum {
		ReadOffset = BankOffsetValue<Index - 1,
		    OtherBanks...>::ReadSize  //!< Read offset
	};
	enum {
		WriteSize = FirstBank::WriteSize +
		            BankOffsetValue<Index - 1,
		                OtherBanks...>::WriteSize  //!< Requested write bytes
	};
	enum {
		WriteOffset = BankOffsetValue<Index - 1,
		    OtherBanks...>::WriteSize  //!< Write Offset
	};
	enum {
		ModSize = FirstBank::ModSize +
		          BankOffsetValue<Index - 1,
		              OtherBanks...>::ModSize  //!< Requested mod bytes
	};
	enum {
		ModOffset =
		    BankOffsetValue<Index - 1, OtherBanks...>::ModSize  //!< Mod offset
	};
};

/**
 * Expands bank offset values
 */
template <bool Expanded, class BankPack, class IndexPack>
struct BankOffsetExpand;

/**
 * Expands bank offset values by 1
 */
template <std::size_t Index, std::size_t... Indices, class... Banks>
struct BankOffsetExpand<false, class_pack<Banks...>,
    index_pack<Index, Indices...>>
    : public BankOffsetExpand<Index == 1, class_pack<Banks...>,
          index_pack<Index - 1, Index, Indices...>> {};

/**
 * Expanded bank offset values
 */
template <std::size_t... Indices, class... Banks>
struct BankOffsetExpand<true, class_pack<Banks...>, index_pack<Indices...>> {
	/**
	 * Gets read offset
	 *
	 * @param bank Bank number
	 * @return Read offset
	 */
	static std::size_t getOffsetRead(std::size_t bank) {
		static const int val[sizeof...(Indices)] = {
		    BankOffsetValue<Indices, Banks...>::ReadOffset...};
		return val[bank];
	}
	/**
	 * Gets write offset
	 *
	 * @param bank Bank number
	 * @return Read offset
	 */
	static std::size_t getOffsetWrite(std::size_t bank) {
		static const int val[sizeof...(Indices)] = {
		    BankOffsetValue<Indices, Banks...>::WriteOffset...};
		return val[bank];
	}
	/**
	 * Gets mod offset
	 *
	 * @param bank Bank number
	 * @return Read offset
	 */
	static std::size_t getOffsetMod(std::size_t bank) {
		static const int val[sizeof...(Indices)] = {
		    BankOffsetValue<Indices, Banks...>::ModOffset...};
		return val[bank];
	}
};

/**
 * Bank offsets
 */
template <class... Banks>
struct BankOffset
    : public BankOffsetExpand<sizeof...(Banks) == 1, class_pack<Banks...>,
          index_pack<sizeof...(Banks) - 1>> {};

/**
 * Bank config
 */
template <class...>
struct BankConfig;

/**
 * Empty bank config
 */
template <>
struct BankConfig<> {
	enum {
		ReadSize = 0  //!< Requested read bytes
	};
	enum {
		WriteSize = 0  //!< Requested write bytes
	};
	enum {
		ModSize = 0  //!< Requested mod bytes
	};

	/**
	 * Maps IO
	 *
	 * @param iterRead Read iterator
	 * @param iterWrite Write iterator
	 * @param iterMod Mod iterator
	 * @param openBus Open bus
	 * @param dummy Dummy write
	 * @param writeBuf Write buffer
	 */
	static void mapIO(MemoryMap::iterator iterRead,
	    MemoryMap::iterator iterWrite, MemoryMap::iterator iterMod,
	    std::uint8_t *openBus, std::uint8_t *dummy, std::uint8_t *writeBuf) {
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param bank Bank number
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(
	    MemoryMap::iterator iter, std::size_t bank, std::uint16_t addr) {
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
template <class FirstClass, class... OtherClasses>
struct BankConfig<FirstClass, OtherClasses...> {
	static_assert(std::is_base_of<banks::BaseBank, FirstClass>::value,
	    "Bank config contains non-bank class");
	enum {
		ReadSize =
		    FirstClass::ReadSize +
		    BankConfig<OtherClasses...>::ReadSize  //!< Requested read bytes
	};
	enum {
		WriteSize =
		    FirstClass::WriteSize +
		    BankConfig<OtherClasses...>::WriteSize  //!< Requested write bytes
	};
	enum {
		ModSize = FirstClass::ModSize +
		          BankConfig<OtherClasses...>::ModSize  //!< Requested mod bytes
	};
	/**
	 * Maps to read map
	 *
	 * @param iterRead Read iterator
	 * @param iterWrite Write iterator
	 * @param iterMod Mod iterator
	 * @param openBus Open bus
	 * @param dummy Dummy write
	 * @param writeBuf Write buffer
	 * @param buf Current buffer
	 * @param args Buffers
	 */
	template <typename... Args>
	static void mapIO(MemoryMap::iterator iterRead,
	    MemoryMap::iterator iterWrite, MemoryMap::iterator iterMod,
	    std::uint8_t *openBus, std::uint8_t *dummy, std::uint8_t *writeBuf,
	    std::uint8_t *buf, Args &&... args) {
		FirstClass::mapRead(iterRead, openBus, buf);
		FirstClass::mapWrite(iterWrite, dummy, buf);
		FirstClass::mapMod(iterMod, writeBuf, buf);
		BankConfig<OtherClasses...>::mapIO(iterRead + FirstClass::ReadSize,
		    iterWrite + FirstClass::WriteSize, iterMod + FirstClass::ModSize,
		    openBus, dummy, writeBuf, std::forward<Args>(args)...);
	}
	/**
	 * Gives read iterator
	 *
	 * @param iter Base iterator
	 * @param bank Bank number
	 * @param addr Address
	 * @return Read iterator
	 */
	static MemoryMap::iterator getAddrRead(
	    MemoryMap::iterator iter, std::size_t bank, std::uint16_t addr) {
		typedef MemoryMap::iterator (*readAddr)(
		    MemoryMap::iterator iter, std::uint16_t addr);

		static const readAddr readAddrs[1 + sizeof...(OtherClasses)] = {
		    &FirstClass::getAddrRead, &OtherClasses::getAddrRead...};

		return (*readAddrs[bank])(
		    iter + BankOffset<FirstClass, OtherClasses...>::getOffsetRead(bank),
		    addr);
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
		typedef MemoryMap::iterator (*writeAddr)(
		    MemoryMap::iterator iter, std::uint16_t addr);
		typedef MemoryMap::iterator (*modAddr)(
		    MemoryMap::iterator iter, std::uint16_t addr);

		static const writeAddr writeAddrs[1 + sizeof...(OtherClasses)] = {
		    &FirstClass::getAddrWrite, &OtherClasses::getAddrWrite...};
		static const modAddr modAddrs[1 + sizeof...(OtherClasses)] = {
		    &FirstClass::getAddrMod, &OtherClasses::getAddrMod...};

		return std::make_pair(
		    (*writeAddrs[bank])(
		        iterWrite +
		            BankOffset<FirstClass, OtherClasses...>::getOffsetWrite(
		                bank),
		        addr),
		    (*modAddrs[bank])(
		        iterMod +
		            BankOffset<FirstClass, OtherClasses...>::getOffsetMod(bank),
		        addr));
	}

private:
	/**
	 * Forbid construction
	 */
	BankConfig();
};

} /* banks */

/**
 * Aggregate devices on bus
 */
template <class...>
struct BusAggregate;

/**
 * Empty aggregated bus
 */
template <>
struct BusAggregate<> {
	enum {
		ReadSize = 0  //!< Requested read bytes
	};
	enum {
		WriteSize = 0  //!< Requested write bytes
	};
	enum {
		ModSize = 0  //!< Requested mod bytes
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
	/**
	 * Maps IO
	 *
	 * @param iter Device iterator
	 * @param iterRead Read iterator
	 * @param iterWrite Write iterator
	 * @param iterMod Mod iterator
	 * @param openBus Open bus
	 * @param dummy Dummy write
	 * @param writeBuf Write buffer
	 */
	static void mapIO(DevicePtrList::iterator iter,
	    MemoryMap::iterator iterRead, MemoryMap::iterator iterWrite,
	    MemoryMap::iterator iterMod, std::uint8_t *openBus, std::uint8_t *dummy,
	    std::uint8_t *writeBuf) {
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
template <class FirstDeviceConfig, class... OtherDevicesConfig>
struct BusAggregate<FirstDeviceConfig, OtherDevicesConfig...> {
	static_assert(
	    std::is_base_of<CDevice, typename FirstDeviceConfig::Device>::value,
	    "Cannot aggregate non-devices");
	enum {
		ReadSize = FirstDeviceConfig::BankConfig::ReadSize +
		           BusAggregate<OtherDevicesConfig...>::ReadSize  //!< Requested
		                                                          //! read bytes
	};
	enum {
		WriteSize =
		    FirstDeviceConfig::BankConfig::WriteSize +
		    BusAggregate<OtherDevicesConfig...>::WriteSize  //!< Requested write
		                                                    //! bytes
	};
	enum {
		ModSize =
		    FirstDeviceConfig::BankConfig::ModSize +
		    BusAggregate<OtherDevicesConfig...>::ModSize  //!< Requested mod
		                                                  //! bytes
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
		if (FirstDeviceConfig::isDeviceEnabled(addr)) {
			return FirstDeviceConfig::BankConfig::getAddrRead(iterRead,
			    FirstDeviceConfig::getBank(addr,
			        static_cast<typename FirstDeviceConfig::Device &>(**iter)),
			    addr);
		} else {
			return BusAggregate<OtherDevicesConfig...>::getAddrRead(iter + 1,
			    iterRead + FirstDeviceConfig::BankConfig::ReadSize, addr);
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
		if (FirstDeviceConfig::isDeviceEnabled(addr)) {
			return FirstDeviceConfig::BankConfig::getAddrWrite(iterWrite,
			    iterMod,
			    FirstDeviceConfig::getBank(addr,
			        static_cast<typename FirstDeviceConfig::Device &>(**iter)),
			    addr);
		} else {
			return BusAggregate<OtherDevicesConfig...>::getAddrWrite(iter + 1,
			    iterWrite + FirstDeviceConfig::BankConfig::WriteSize,
			    iterMod + FirstDeviceConfig::BankConfig::ModSize, addr);
		}
	}
	/**
	 * Maps IO
	 *
	 * @param iter Device iterator
	 * @param iterRead Read iterator
	 * @param iterWrite Write iterator
	 * @param iterMod Mod iterator
	 * @param openBus Open bus
	 * @param dummy Dummy write
	 * @param writeBuf Write buffer
	 */
	static void mapIO(DevicePtrList::iterator iter,
	    MemoryMap::iterator iterRead, MemoryMap::iterator iterWrite,
	    MemoryMap::iterator iterMod, std::uint8_t *openBus, std::uint8_t *dummy,
	    std::uint8_t *writeBuf) {
		FirstDeviceConfig::mapIO(iterRead, iterWrite, iterMod, openBus, dummy,
		    writeBuf,
		    static_cast<typename FirstDeviceConfig::Device &>(**iter));
		BusAggregate<OtherDevicesConfig...>::mapIO(iter + 1,
		    iterRead + FirstDeviceConfig::BankConfig::ReadSize,
		    iterWrite + FirstDeviceConfig::BankConfig::WriteSize,
		    iterMod + FirstDeviceConfig::BankConfig::ModSize, openBus, dummy,
		    writeBuf);
	}

private:
	/**
	 * Forbid construction
	 */
	BusAggregate();
};

/**
 * Base for bus config
 */
template <class T>
struct BusConfigBase {
	/**
	 * Device
	 */
	typedef T Device;
	/**
	 * Banks config
	 */
	typedef banks::BankConfig<> BankConfig;

	/**
	 * Maps IO
	 *
	 * @param iterRead Read iterator
	 * @param iterWrite Write iterator
	 * @param iterMod Mod iterator
	 * @param openBus Open bus
	 * @param dummy Dummy write
	 * @param writeBuf Write bus
	 * @param device Device
	 */
	static void mapIO(MemoryMap::iterator iterRead,
	    MemoryMap::iterator iterWrite, MemoryMap::iterator iterMod,
	    std::uint8_t *openBus, std::uint8_t *dummy, std::uint8_t *writeBuf,
	    T &device) {
	}
	/**
	 * Checks if device is enabled
	 *
	 * @param addr Address
	 * @return True if enabled
	 */
	static bool isDeviceEnabled(std::uint16_t addr) {
		return false;
	}
	/**
	 * Determines active bank
	 *
	 * @param addr Address
	 * @param device Device
	 * @return Active bank
	 */
	static std::size_t getBank(std::uint16_t addr, T &device) {
		return 0;
	}

private:
	/**
	 * Forbid construction
	 */
	BusConfigBase();
};

/**
 * Open bus device
 */
struct COpenBusDevice : public CDevice {
	/**
	 * Bus parameters
	 */
	struct BusConfig : public BusConfigBase<COpenBusDevice> {
		/**
		 * Banks config
		 */
		typedef banks::BankConfig<banks::OpenBus> BankConfig;

		/**
		 * Maps IO
		 *
		 * @param iterRead Read iterator
		 * @param iterWrite Write iterator
		 * @param iterMod Mod iterator
		 * @param openBus Open bus
		 * @param dummy Dummy write
		 * @param writeBuf Write bus
		 * @param device Device
		 */
		static void mapIO(MemoryMap::iterator iterRead,
		    MemoryMap::iterator iterWrite, MemoryMap::iterator iterMod,
		    std::uint8_t *openBus, std::uint8_t *dummy, std::uint8_t *writeBuf,
		    COpenBusDevice &device) {
			BankConfig::mapIO(iterRead, iterWrite, iterMod, openBus, dummy,
			    writeBuf, nullptr);
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
		static std::size_t getBank(std::uint16_t addr, COpenBusDevice &device) {
			return 0;
		}
	};
};

/**
 * Configured bus
 */
template <class...>
class CBusConfig;

/**
 * Empty bus
 */
template <>
class CBusConfig<> : public CBus {
public:
	/**
	 * Constructor
	 *
	 * @param openBus Open bus value
	 */
	CBusConfig(std::uint8_t openBus) : CBus(openBus) {
	}
	/**
	 * Destructor
	 */
	~CBusConfig() = default;

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
template <class... DeviceConfigs>
class CBusConfig : public CBus {
private:
	static_assert(
	    cond_and<
	        std::is_base_of<CDevice, typename DeviceConfigs::Device>...>::value,
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

public:
	/**
	 * Deleted default constructor
	 */
	CBusConfig() = delete;
	/**
	 * Constructs the bus
	 *
	 * @param openBus Open bus value
	 * @param devices Devices
	 */
	CBusConfig(
	    std::uint8_t openBus, typename DeviceConfigs::Device &... devices)
	    : CBus(openBus)
	    , m_OpenBusDevice()
	    , m_ReadArr(BusAggregate<DeviceConfigs...,
	                    COpenBusDevice::BusConfig>::ReadSize,
	          &m_OpenBus)
	    , m_WriteArr(BusAggregate<DeviceConfigs...,
	                     COpenBusDevice::BusConfig>::WriteSize,
	          &m_DummyWrite)
	    , m_ModArr(BusAggregate<DeviceConfigs...,
	                   COpenBusDevice::BusConfig>::ModSize,
	          &m_WriteBuf)
	    , m_DeviceArr({&devices..., &m_OpenBusDevice}) {
		BusAggregate<DeviceConfigs..., COpenBusDevice::BusConfig>::mapIO(
		    m_DeviceArr.begin(), m_ReadArr.begin(), m_WriteArr.begin(),
		    m_ModArr.begin(), &m_OpenBus, &m_DummyWrite, &m_WriteBuf);
	}
	/**
	 * Destructor
	 */
	~CBusConfig() = default;

	/**
	 * Reads memory from the bus
	 *
	 * @param addr Address
	 * @return Value at the address
	 */
	std::uint8_t readMemory(std::uint16_t addr) {
		processPreReadHooks(addr);
		MemoryMap::iterator iter = BusAggregate<DeviceConfigs...,
		    COpenBusDevice::BusConfig>::getAddrRead(m_DeviceArr.begin(),
		    m_ReadArr.begin(), addr);
		std::uint8_t res = **iter;
		processPostReadHooks(res, addr);
		return res;
	}
	/**
	 * Writes memory on the bus
	 *
	 * @param s Value
	 * @param addr Address
	 */
	void writeMemory(std::uint8_t s, std::uint16_t addr) {
		auto iter = BusAggregate<DeviceConfigs...,
		    COpenBusDevice::BusConfig>::getAddrWrite(m_DeviceArr.begin(),
		    m_WriteArr.begin(), m_ModArr.begin(), addr);
#if defined(VPNES_BUSCONFLICT_CYCLED)
		m_WriteBuf = s;
		std::uint8_t val = m_WriteBuf & **(iter.second);
		do {
			m_WriteBuf = val;
			processWriteHooks(m_WriteBuf, addr);
			val &= **(iter.second);
		} while (val != m_WriteBuf);
		**(iter.first) = m_WriteBuf;
#else
		m_WriteBuf = s;
		m_WriteBuf &= **(iter.second);
		processWriteHooks(m_WriteBuf, addr);
		**(iter.first) = m_WriteBuf;
#endif
	}
};

} /* core */

} /* vpnes */

#endif /* VPNES_INCLUDE_CORE_BUS_HPP_ */
