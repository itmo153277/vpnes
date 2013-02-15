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

#include "gui.h"

#include "../types.h"

#include "../nes/ines.h"
#include "../nes/nes.h"
#include "../nes/libvpnes.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

namespace vpnes_gui {

/* CNESGUI */

CNESGUI::CNESGUI(const char *FileName) {
	if (::SDL_Init(SDL_INIT_EVERYTHING) < 0)
		throw CGenericException("SDL initilaization failure");
	Audio = new CAudio();
	Input = new CInput();
	Window = new CWindow(FileName, Audio, Input);
	Video = new CVideo(Window);
#if !defined(VPNES_DISABLE_SYNC)
	Window->AppendSyncManager(Video);
#endif
	AudioFrontend = Audio;
	VideoFrontend = Video;
	Input1Frontend = Input->GetInput1Frontend();
	Input2Frontend = Input->GetInput2Frontend();
}

CNESGUI::~CNESGUI() {
	delete Video;
	delete Window;
	delete Input;
	delete Audio;
	::SDL_Quit();
}

/* Запустить NES */
void CNESGUI::Start() {
	vpnes::ines::NES_ROM_Data Data;
	vpnes::CNESConfig *NESConfig;
	vpnes::CBasicNES *NES;
	std::ifstream ROM;
	std::fstream State;
	std::basic_string<char> Name;
	std::basic_string<char> RomName;

	NESConfig = NULL;
	NES = NULL;
	memset(&Data, 0, sizeof(vpnes::ines::NES_ROM_Data));
	try {
		/* Открываем файл */
		RomName = Window->GetFileName();
		ROM.open(RomName.c_str(), std::ios_base::in | std::ios_base::binary);
		if (ROM.fail())
			throw CGenericException("Couldn't open ROM file");
		NESConfig = vpnes::OpenROM(ROM, &Data);
		ROM.close();
		if (NESConfig == NULL)
			throw CGenericException("Couldn't find any compatible NES configuration "
				"(unsupported file)");
		Window->UpdateSizes(NESConfig->GetWidth(), NESConfig->GetHeight());
		Video->UpdateSizes(NESConfig->GetWidth(), NESConfig->GetHeight());
		Audio->UpdateDevice(NESConfig->GetFrameLength());
		NES = NESConfig->GetNES(this);
		if (Data.Header.HaveBattery) {
			Name = RomName;
			Name += ".vpram";
			State.open(Name.c_str(), std::ios_base::in | std::ios_base::binary);
			if (!State.fail()) {
				NES->LoadState(State);
				State.close();
			}
		}
		Window->SetText("Hard Reset");
		NES->Reset();
		do {
			NES->PowerOn();
		} while (Window->GetWindowState() != CWindow::wsQuit);
		if (Data.Header.HaveBattery) {
			Name = RomName;
			Name += ".vpram";
			State.open(Name.c_str(), std::ios_base::out |
				std::ios_base::binary | std::ios_base::trunc);
			if (!State.fail()) {
				NES->PowerOff(State);
				State.close();
			}
		}
	} catch (CGenericException &e) {
#ifdef _WIN32
		MessageBox(Window->GetWindowHandle(), e.what(), "Error", MB_ICONHAND);
#else
		std::clog << e.what() << std::endl;
#endif
	}
	delete NES;
	delete NESConfig;
	vpnes::FreeROMData(&Data);
}

/* Отладочная информация ЦПУ */
void CNESGUI::CPUDebug(uint16 PC, uint8 A, uint8 X, uint8 Y, uint8 S, uint8 P) {
}

/* Отладочная информация ГПУ */
void CNESGUI::PPUDebug(int Frame, int Scanline, int Cycle, uint16 Reg1, uint16 Reg2,
	uint16 Reg1Buf) {
}

/* Критическая ошибка */
void CNESGUI::Panic() {
}

}
