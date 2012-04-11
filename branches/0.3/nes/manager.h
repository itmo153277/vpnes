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

#ifndef __MANAGER_H_
#define __MANAGER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace vpnes {

/* Менеджер памяти */
class CMemoryManager {
private:
	/* Запись о блоке памяти */
	struct SMemory {
		char *ID; /* ID */
		void *p; /* Pointer */
		size_t Size; /* Size */
	};

	/* Вектор */
	typedef std::vector<SMemory *> MemoryVec;
	MemoryVec MemoryBlocks;

	/* Поиск по ID */
	template <class _ID>
	inline static bool MemoryCompare(SMemory *Memory) {
		return !strncmp(Memory->ID + _ID::Pos, _ID::ID, _ID::Length);
	}
public:
	inline explicit CMemoryManager() {}
	inline virtual ~CMemoryManager() {
		MemoryVec::iterator iter;

		/* Удаляем записи */
		for (iter = MemoryBlocks.begin(); iter != MemoryBlocks.end(); iter++)
			delete *iter;
	}

	/* Получить указатель */
	template <class _ID>
	inline void *GetPointer(size_t Size = 0);
	/* Установить указатель */
	template <class _ID>
	inline void SetPointer(void *Pointer, size_t Size);
	/* Загрузить память из потока */
	template <class _ID>
	inline void LoadMemory(std::istream &State);
	/* Сохранить память в поток */
	template <class _ID>
	inline void SaveMemory(std::ostream &State);
	/* Освободить память */
	template <class _ID>
	inline void FreeMemory();
};

/* Идентификаторы */

/* Обший идентификатор */
template <int _Length = 0, char c0 = 0, char c1 = 0, char c2 = 0, char c3 = 0,
	char c4 = 0, char c5 = 0>
struct UniversalIDTemplate {
	/* ID */
	static const char ID[6];
	/* Length */
	static const int Length;
	/* Position */
	static const int Pos;
};

template <int _Length, char c0, char c1, char c2, char c3, char c4, char c5>
const char UniversalIDTemplate<_Length, c0, c1, c2, c3, c4, c5>::ID[6] = {c0, c1,
	c2, c3, c4, c5};

template <int _Length, char c0, char c1, char c2, char c3, char c4, char c5>
const int UniversalIDTemplate<_Length, c0, c1, c2, c3, c4, c5>::Length = _Length;

template <int _Length, char c0, char c1, char c2, char c3, char c4, char c5>
const int UniversalIDTemplate<_Length, c0, c1, c2, c3, c4, c5>::Pos = 0;

/* Общий идетнификатор группы */
template <int _Pos, char c>
struct UniversalGroupIDTemplate {
	/* ID */
	static const char ID[1];
	/* Length */
	static const int Length;
	/* Position */
	static const int Pos;
};

template <int _Pos, char c>
const char UniversalGroupIDTemplate<_Pos, c>::ID[1] = {c};

template <int _Pos, char c>
const int UniversalGroupIDTemplate<_Pos, c>::Length = 1;

template <int _Pos, char c>
const int UniversalGroupIDTemplate<_Pos, c>::Pos = _Pos;

/* Определение основных групп */

/* Static Group */
typedef UniversalGroupIDTemplate<0, 'S'> StaticGroupID;
template <int Length = 0, char c0 = 0, char c1 = 0, char c2 = 0, char c3 = 0>
struct StaticGroupTemplate {
	typedef UniversalIDTemplate<Length + 2, 'S', 'N', c0, c1, c2, c3> ID;
};

/* Dynamic Group */
typedef UniversalGroupIDTemplate<0, 'D'> DynamicGroupID;
template <int Length = 0, char c0 = 0, char c1 = 0, char c2 = 0, char c3 = 0,
	char c4 = 0>
struct DynamicGroupTemplate {
	typedef UniversalIDTemplate<Length + 1, 'D', c0, c1, c2, c3, c4> ID;
};

/* Battery Group */
typedef UniversalGroupIDTemplate<1, 'B'> BatteryGroupID;
template <int Length = 0, char c0 = 0, char c1 = 0, char c2 = 0, char c3 = 0>
struct BatteryGroupTemplate {
	typedef typename DynamicGroupTemplate<Length + 1, 'B', c0, c1, c2, c3>::ID ID;
};

/* NoBattery Group */
typedef UniversalGroupIDTemplate<1, 'N'> NoBatteryGroupID;
template <int Length = 0, char c0 = 0, char c1 = 0, char c2 = 0, char c3 = 0>
struct NoBatteryGroupTemplate {
	typedef typename DynamicGroupTemplate<Length + 1, 'N', c0, c1, c2, c3>::ID ID;
};

/* Шаблон для групп */
template <char g, char c0 = 0, char c1 = 0, char c2 = 0>
struct GroupTemplate {
	typedef typename StaticGroupTemplate<c2 != 0 ? 4 : c1 != 0 ? 3 : c0 != 0 ? 2 : 1,
		g, c0, c1, c2>::ID StaticID;
	typedef typename BatteryGroupTemplate<c2 != 0 ? 4 : c1 != 0 ? 3 : c0 != 0 ? 2 : 1,
		g, c0, c1, c2>::ID BatteryID;
	typedef typename NoBatteryGroupTemplate<c2 != 0 ? 4 : c1 != 0 ? 3 : c0 != 0 ? 2 : 1,
		g, c0, c1, c2>::UD NoBatteryID;
};

/* CPU Group */
typedef UniversalGroupIDTemplate<2, 'C'> CPUGroupID;
template <int _ID>
struct CPUGroup {
	typedef GroupTemplate<'C', _ID, (_ID >> 8), (_ID >> 16)> ID;
};

/* APU Group */
typedef UniversalGroupIDTemplate<2, 'A'> APUGroupID;
template <int _ID>
struct APUGroup {
	typedef GroupTemplate<'A', _ID, (_ID >> 8), (_ID >> 16)> ID;
};

/* PPU Group */
typedef UniversalGroupIDTemplate<2, 'P'> PPUGroupID;
template <int _ID>
struct PPUGroup {
	typedef GroupTemplate<'P', _ID, (_ID >> 8), (_ID >> 16)> ID;
};

/* Mapper Group */
typedef UniversalGroupIDTemplate<2, 'M'> MapperGroupID;
template <char m, int _ID>
struct MapperGroup {
	struct Group {
		/* ID */
		static const char ID[2];
		/* Length */
		static const int Length;
		/* Position */
		static const int Pos;
	};
	typedef GroupTemplate<'M', m, _ID, (_ID >> 8)> ID;
};

template <char m, int _ID>
const char MapperGroup<m, _ID>::Group::ID[2] = {'M', m};

template <char m, int _ID>
const int MapperGroup<m, _ID>::Group::Length = 2;

template <char m, int _ID>
const int MapperGroup<m, _ID>::Group::Pos = 2;

/* CMemoryManager */

/* Получить указатель */
template <class _ID>
inline void *CMemoryManager::GetPointer(size_t Size) {
	MemoryVec::iterator iter;
	void *p;

	iter = find_if(MemoryBlocks.begin(), MemoryBlocks.end(), MemoryCompare<_ID>);
	if (iter != MemoryBlocks.end()) { /* Нашли с нужным ID */
		if (((*iter)->p == NULL) & (Size > 0)) /* Память была освобождена */
			(*iter)->p = malloc(Size);
		return (*iter)->p;
	}
	if (Size > 0) { /* Создаем новый */
		p = malloc(Size);
		if (p == NULL)
			return NULL;
		SetPointer<_ID>(p, Size);
		return p;
	}
	return NULL;
}

/* Установить указатель */
template <class _ID>
inline void CMemoryManager::SetPointer(void *Pointer, size_t Size) {
	/* Предполагается что блоков с таким ID нет */
	SMemory *Block;

	Block = new SMemory;
	Block->ID = _ID::ID;
	Block->Size = Size;
	Block->p = Pointer;
	MemoryBlocks.push_back(Block);
}

/* Загрузить память из потока */
template <class _ID>
inline void CMemoryManager::LoadMemory(std::istream &State) {
}

/* Сохранить память в поток */
template <class _ID>
inline void CMemoryManager::SaveMemory(std::ostream &State) {
}

/* Освободить память */
template <class _ID>
inline void CMemoryManager::FreeMemory() {
	MemoryVec::iterator iter;

	for (iter = MemoryBlocks.begin();;) {
		iter = find_if(iter, MemoryBlocks.end(), MemoryCompare<_ID>);
		if (iter == MemoryBlocks.end())
			break;
		free((*iter)->p);
		(*iter)->p = NULL;
	}
}

}

#endif
