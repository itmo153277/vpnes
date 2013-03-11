/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2013  Ivanov Viktor

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

#include "aputables.h"

namespace vpnes {

namespace apu {

/* Таблица выхода прямоугольных сигналов */
const double SquareTable[31] = {
	0.0000000, 0.0129492, 0.0255874, 0.0379257, 0.0499746, 0.0617442,
	0.0732442, 0.0844836, 0.0954712, 0.1062150, 0.1167240, 0.1270050,
	0.1370660, 0.1469130, 0.1565540, 0.1659940, 0.1752400, 0.1842980,
	0.1931730, 0.2018720, 0.2103990, 0.2187590, 0.2269570, 0.2349980,
	0.2428860, 0.2506250, 0.2582210, 0.2656760, 0.2729950, 0.2801800,
	0.2872370
};

/* Таблица выхода остальных каналов */
const double TNDTable[203] = {
	0.00000000, 0.00643024, 0.01280800, 0.01913410, 0.02540890, 0.03163320,
	0.03780760, 0.04393260, 0.05000890, 0.05603700, 0.06201750, 0.06795100,
	0.07383800, 0.07967910, 0.08547470, 0.09122550, 0.09693190, 0.10259400,
	0.10821400, 0.11379000, 0.11932400, 0.12481600, 0.13026700, 0.13567700,
	0.14104600, 0.14637500, 0.15166500, 0.15691600, 0.16212800, 0.16730100,
	0.17243700, 0.17753500, 0.18259600, 0.18762100, 0.19260900, 0.19756200,
	0.20247900, 0.20736100, 0.21220800, 0.21702100, 0.22180000, 0.22654500,
	0.23125700, 0.23593600, 0.24058300, 0.24519700, 0.24978000, 0.25433000,
	0.25885000, 0.26333900, 0.26779700, 0.27222500, 0.27662200, 0.28099000,
	0.28532900, 0.28963900, 0.29392000, 0.29817200, 0.30239600, 0.30659200,
	0.31076000, 0.31490200, 0.31901500, 0.32310300, 0.32716300, 0.33119700,
	0.33520500, 0.33918700, 0.34314400, 0.34707500, 0.35098100, 0.35486300,
	0.35871900, 0.36255200, 0.36636000, 0.37014400, 0.37390500, 0.37764200,
	0.38135600, 0.38504600, 0.38871400, 0.39235900, 0.39598200, 0.39958300,
	0.40316200, 0.40671800, 0.41025400, 0.41376800, 0.41726000, 0.42073200,
	0.42418300, 0.42761300, 0.43102200, 0.43441200, 0.43778100, 0.44113000,
	0.44446000, 0.44777000, 0.45106100, 0.45433200, 0.45758500, 0.46081800,
	0.46403300, 0.46722900, 0.47040700, 0.47356600, 0.47670800, 0.47983100,
	0.48293700, 0.48602500, 0.48909600, 0.49214900, 0.49518500, 0.49820400,
	0.50120600, 0.50419200, 0.50716100, 0.51011300, 0.51304900, 0.51596900,
	0.51887200, 0.52176000, 0.52463200, 0.52748800, 0.53032900, 0.53315400,
	0.53596400, 0.53875900, 0.54153900, 0.54430300, 0.54705300, 0.54978900,
	0.55250900, 0.55521600, 0.55790700, 0.56058500, 0.56324900, 0.56589800,
	0.56853400, 0.57115600, 0.57376400, 0.57635800, 0.57894000, 0.58150700,
	0.58406200, 0.58660300, 0.58913100, 0.59164700, 0.59414900, 0.59663900,
	0.59911600, 0.60158100, 0.60403300, 0.60647200, 0.60890000, 0.61131500,
	0.61371800, 0.61610900, 0.61848800, 0.62085500, 0.62321100, 0.62555500,
	0.62788700, 0.63020800, 0.63251800, 0.63481600, 0.63710300, 0.63937900,
	0.64164300, 0.64389700, 0.64614000, 0.64837200, 0.65059300, 0.65280400,
	0.65500400, 0.65719300, 0.65937200, 0.66154100, 0.66369900, 0.66584800,
	0.66798600, 0.67011400, 0.67223100, 0.67433900, 0.67643800, 0.67852600,
	0.68060400, 0.68267300, 0.68473300, 0.68678300, 0.68882300, 0.69085400,
	0.69287500, 0.69488800, 0.69689100, 0.69888500, 0.70087000, 0.70284600,
	0.70481300, 0.70677100, 0.70872000, 0.71066000, 0.71259200
};

/* NTSC */

/* Число тактов для каждого шага */
const int NTSC_Tables::StepCycles[6] = {7456, 14912, 22370, 29828, 37280, 0};

/* Таблица счетчика */
const int NTSC_Tables::LengthCounterTable[32] = {
	0x0a, 0xfe, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xa0, 0x08, 0x3c, 0x0a,
	0x0e, 0x0c, 0x1a, 0x0e, 0x0c, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
	0xc0, 0x18, 0x48, 0x1a, 0x10, 0x1c, 0x20, 0x1e
};

/* Таблица формы */
const bool NTSC_Tables::DutyTable[32] = {
	false, true,  false, false, false, false, false, false, false, true,  true,  false,
	false, false, false, false, false, true,  true,  true,  true,  false, false, false,
	true,  false, false, true,  true,  true,  true,  true
};

/* Таблица пилообразной формы */
const int NTSC_Tables::SeqTable[32] = {
	15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
};

/* Таблица длин для канала шума */
const int NTSC_Tables::NoiseTable[16] = {
	0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0060, 0x0080, 0x00a0,
	0x00ca, 0x00fe, 0x017c, 0x01fc, 0x02fa, 0x03f8, 0x07f2, 0x0fe4
};

/* Таблица длин для ДМ-канала */
const int NTSC_Tables::DMTable[16] = {
	428, 380, 340, 320, 286, 254, 226, 214,
	190, 160, 142, 128, 106,  84,  72,  54
};

/* PAL */

/* Число тактов для каждого шага */
const int PAL_Tables::StepCycles[6] = {8312, 16626, 24938, 33252, 41564, 0};

/* Таблица длин для канала шума */
const int PAL_Tables::NoiseTable[16] = {
	0x0004, 0x0007, 0x000e, 0x001e, 0x003c, 0x0058, 0x0076, 0x0094,
	0x00bc, 0x00ec, 0x0162, 0x01d8, 0x02c4, 0x03b0, 0x0762, 0x0ec2
};

/* Таблица длин для ДМ-канала */
const int PAL_Tables::DMTable[16] = {
	398, 354, 316, 298, 276, 236, 210, 198,
	176, 148, 132, 118,  98,  78,  66,  50
};

}

}