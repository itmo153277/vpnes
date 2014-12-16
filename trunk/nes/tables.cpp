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

#include "tables.h"

namespace vpnes {

namespace apu {

/* Таблица выхода прямоугольных сигналов */
const double SquareTable[31] = {
	0.0000000, 0.0117721, 0.0238033, 0.0330029, 0.0455114, 0.0550786,
	0.0648099, 0.0747103, 0.0847831, 0.0950345, 0.1054680, 0.1125270,
	0.1232750, 0.1305480, 0.1416260, 0.1491250, 0.1567160, 0.1644010,
	0.1721840, 0.1800640, 0.1880450, 0.1961270, 0.2043120, 0.2126040,
	0.2167910, 0.2252440, 0.2295140, 0.2381360, 0.2424900, 0.2512840,
	0.2557250
};

/* Таблица выхода остальных каналов */
const double TNDTable[203] = {
	0.00000000, 0.00671614, 0.01337750, 0.01998480, 0.02653860, 0.03303970,
	0.03948860, 0.04588590, 0.05223240, 0.05852850, 0.06477490, 0.07097220,
	0.07712100, 0.08322180, 0.08927510, 0.09528150, 0.10124200, 0.10715600,
	0.11302500, 0.11884900, 0.12462900, 0.13036600, 0.13605900, 0.14170900,
	0.14731700, 0.15288300, 0.15840800, 0.16389300, 0.16933600, 0.17473900,
	0.18010400, 0.18542900, 0.19071500, 0.19596300, 0.20117300, 0.20634600,
	0.21148200, 0.21658100, 0.22164300, 0.22667000, 0.23166200, 0.23661800,
	0.24153900, 0.24642600, 0.25128000, 0.25609900, 0.26088600, 0.26563800,
	0.27035900, 0.27504700, 0.27970400, 0.28432900, 0.28892100, 0.29348300,
	0.29801500, 0.30251700, 0.30698800, 0.31142900, 0.31584100, 0.32022400,
	0.32457700, 0.32890300, 0.33319900, 0.33746900, 0.34170900, 0.34592300,
	0.35010900, 0.35426800, 0.35840100, 0.36250700, 0.36658600, 0.37064100,
	0.37466800, 0.37867200, 0.38264900, 0.38660100, 0.39052900, 0.39443300,
	0.39831200, 0.40216600, 0.40599700, 0.40980400, 0.41358800, 0.41734900,
	0.42108700, 0.42480100, 0.42849500, 0.43216500, 0.43581200, 0.43943800,
	0.44304300, 0.44662500, 0.45018600, 0.45372700, 0.45724500, 0.46074300,
	0.46422100, 0.46767900, 0.47111600, 0.47453200, 0.47793000, 0.48130700,
	0.48466500, 0.48800300, 0.49132200, 0.49462200, 0.49790300, 0.50116500,
	0.50440900, 0.50763400, 0.51084200, 0.51403100, 0.51720200, 0.52035500,
	0.52349000, 0.52660900, 0.52971000, 0.53279300, 0.53586000, 0.53891000,
	0.54194200, 0.54495800, 0.54795800, 0.55094100, 0.55390800, 0.55685900,
	0.55979400, 0.56271300, 0.56561700, 0.56850400, 0.57137600, 0.57423400,
	0.57707400, 0.57990200, 0.58271200, 0.58551000, 0.58829200, 0.59105900,
	0.59381200, 0.59655100, 0.59927500, 0.60198400, 0.60468100, 0.60736200,
	0.61003000, 0.61268400, 0.61532500, 0.61795300, 0.62056600, 0.62316700,
	0.62575400, 0.62832800, 0.63088900, 0.63343700, 0.63597300, 0.63849500,
	0.64100500, 0.64350200, 0.64598700, 0.64845900, 0.65092000, 0.65336800,
	0.65580400, 0.65822800, 0.66064100, 0.66304100, 0.66543000, 0.66780700,
	0.67017200, 0.67252600, 0.67486800, 0.67720000, 0.67951900, 0.68182900,
	0.68412700, 0.68641300, 0.68868900, 0.69095400, 0.69320800, 0.69545300,
	0.69768600, 0.69990800, 0.70212000, 0.70432100, 0.70651400, 0.70869400,
	0.71086500, 0.71302600, 0.71517700, 0.71731900, 0.71944900, 0.72157100,
	0.72368100, 0.72578400, 0.72787600, 0.72995900, 0.73203200, 0.73409600,
	0.73615000, 0.73819500, 0.74023100, 0.74225700, 0.74427500
};

/* NTSC */

/* Число тактов для каждого шага */
const int NTSC_Tables::StepCycles[6] = {89484, 89472, 89496, 89496, 89424, 214260};

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
	0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x005f, 0x007f, 0x009f,
	0x00c9, 0x00fd, 0x017b, 0x01fb, 0x02f9, 0x03f7, 0x07f1, 0x0fe3
};

/* Таблица длин для ДМ-канала */
const int NTSC_Tables::DMTable[16] = {
	427, 379, 339, 319, 285, 253, 225, 213,
	189, 159, 141, 127, 105,  83,  71,  53
};

/* PAL */

/* Число тактов для каждого шага */
const int PAL_Tables::StepCycles[6] = {8312, 16626, 24938, 33252, 41564, 0};

/* Таблица длин для канала шума */
const int PAL_Tables::NoiseTable[16] = {
	0x0003, 0x0006, 0x000d, 0x001d, 0x003b, 0x0057, 0x0075, 0x0093,
	0x00bb, 0x00eb, 0x0161, 0x01d7, 0x02c3, 0x03af, 0x0761, 0x0ec1
};

/* Таблица длин для ДМ-канала */
const int PAL_Tables::DMTable[16] = {
	397, 353, 315, 297, 275, 235, 209, 197,
	175, 147, 131, 117,  97,  77,  65,  49
};

/* Buggy clones */

/* Таблица формы */
const bool Buggy_Tables::DutyTable[32] = {
	false, true,  false, false, false, false, false, false, false, true,  true,  true,
	true,  false, false, false, false, true,  true,  false, false, false, false, false,
	true,  false, false, true,  true,  true,  true,  true
};

}

namespace ppu {

/* Таблица для преобразования тайлов */
const uint16 ColourTable[256] = {
	0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015, 0x0040, 0x0041,
	0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055, 0x0100, 0x0101, 0x0104, 0x0105,
	0x0110, 0x0111, 0x0114, 0x0115, 0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151,
	0x0154, 0x0155, 0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415,
	0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455, 0x0500, 0x0501,
	0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515, 0x0540, 0x0541, 0x0544, 0x0545,
	0x0550, 0x0551, 0x0554, 0x0555, 0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011,
	0x1014, 0x1015, 0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055,
	0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115, 0x1140, 0x1141,
	0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155, 0x1400, 0x1401, 0x1404, 0x1405,
	0x1410, 0x1411, 0x1414, 0x1415, 0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451,
	0x1454, 0x1455, 0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515,
	0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555, 0x4000, 0x4001,
	0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015, 0x4040, 0x4041, 0x4044, 0x4045,
	0x4050, 0x4051, 0x4054, 0x4055, 0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111,
	0x4114, 0x4115, 0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155,
	0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415, 0x4440, 0x4441,
	0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455, 0x4500, 0x4501, 0x4504, 0x4505,
	0x4510, 0x4511, 0x4514, 0x4515, 0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551,
	0x4554, 0x4555, 0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015,
	0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055, 0x5100, 0x5101,
	0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115, 0x5140, 0x5141, 0x5144, 0x5145,
	0x5150, 0x5151, 0x5154, 0x5155, 0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411,
	0x5414, 0x5415, 0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455,
	0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515, 0x5540, 0x5541,
	0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
};

/* Таблица для преобразования атрибутов */
const uint16 AttributeTable[4] = {
	0x0000, 0x5555, 0xaaaa, 0xffff
};

/* Таблица для быстрого разворота */
const uint8 FlipTable[256] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30,
	0xb0, 0x70, 0xf0, 0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98,
	0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64,
	0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 0x0c, 0x8c, 0x4c, 0xcc,
	0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 0x02,
	0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2,
	0x72, 0xf2, 0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a,
	0xda, 0x3a, 0xba, 0x7a, 0xfa, 0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 0x0e, 0x8e, 0x4e, 0xce, 0x2e,
	0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 0x01, 0x81,
	0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71,
	0xf1, 0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9,
	0x39, 0xb9, 0x79, 0xf9, 0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15,
	0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad,
	0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 0x03, 0x83, 0x43,
	0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b,
	0xbb, 0x7b, 0xfb, 0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97,
	0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f,
	0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

}

}
