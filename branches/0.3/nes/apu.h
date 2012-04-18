/****************************************************************************\

	NES Emulator
	Copyright (C) 2012  Ivanov Viktor

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

\****************************************************************************/

#ifndef __APU_H_
#define __APU_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include "manager.h"
#include "bus.h"

namespace vpnes {

/* APU */
template <class _Bus>
class CAPU {
public:
	inline explicit CAPU(_Bus *pBus) {}
	inline ~CAPU() {}

	/* Обработать такты */
	inline int DoClocks(int Clocks) {
		return 6;
	}

	/* Сброс */
	inline void Reset() {
	}
};

struct APU_rebind {
	template <class _Bus>
	struct rebind {
		typedef CAPU<_Bus> rebinded;
	};
};

}

#endif
