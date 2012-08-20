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

#include <SDL.h>
#include "types.h"

#include <iostream>
#include <fstream>
#include "window.h"
#include "nes/ines.h"
#include "nes/nes.h"
#include "nes/libvpnes.h"

/* Точка входа в программу */
int main(int argc, char *argv[]) {
	vpnes::ines::NES_ROM_Data Data;
	vpnes::CNESConfig *NESConfig;
	vpnes::CBasicNES *NES;
	std::ifstream ROM;

#ifdef BUILDNUM
	std::cerr << "VPNES " VERSION " Build " << BUILDNUM <<
#ifdef SVNREV
		" (" SVNREV ")" <<
#endif
		std::endl;
	std::cerr << "License: GPL v2" << std::endl;
#endif
	/* Открываем образ */
	if (argc != 2)
		return 0;
	ROM.open(argv[1], std::ios_base::in | std::ios_base::binary);
	if (ROM.fail())
		return 0;
	NESConfig = vpnes::OpenROM(ROM, &Data);
	ROM.close();
	if (NESConfig == NULL)
		return 0;
	std::cerr << "ROM: " << Data.Header.Mapper << " mapper, PRG " << Data.Header.PRGSize <<
		", W-RAM " << Data.Header.RAMSize << (Data.Header.HaveBattery ? " (battery "
		"backed), CHR " : " (no battery), CHR " ) << Data.Header.CHRSize <<
		((Data.CHR == NULL) ? " RAM, System " : ", System ") << Data.Header.TVSystem <<
		", Mirroring " << Data.Header.Mirroring << ((Data.Trainer == NULL) ?
		", no trainer" : ", have trainer 512") << std::endl;
	/* Инициализация окна */
	if (InitMainWindow(NESConfig->GetWidth(), NESConfig->GetHeight()) == 0) {
		/* Включаем приставку */
		NES = NESConfig->GetNES(&WindowCallback);
		NES->UpdateBuf();
		NES->Reset();
		do {
			WindowState = 0;
			NES->PowerOn();
			switch (WindowState) {
				case VPNES_UPDATEBUF:
					NES->UpdateBuf();
					break;
				case VPNES_SAVESTATE:
				case VPNES_LOADSTATE:
					break;
				case VPNES_RESET:
					NES->Reset();
					break;
			}
		} while (WindowState != VPNES_QUIT);
		delete NES;
	}
	delete NESConfig;
	vpnes::FreeROMData(&Data);
	AppQuit();
	return 0;
}
