/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2014  Ivanov Viktor

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
#include <exception>
#include <iostream>
#include <clocale>
#include "types.h"

#include "gui/gui.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace vpnes {

/* Операции с wchar_t */
wchar_t *ConvertToWChar(const char *Text) {
	size_t TextLen = 1, CharLen, MaxLen = strlen(Text);
	const char *CurChar = Text;
	wchar_t *OutText;

	while (MaxLen > 0) {
		CharLen = mblen(CurChar, MaxLen);
		CurChar += CharLen;
		MaxLen -= CharLen;
		TextLen++;
	}
	OutText = new wchar_t[TextLen];
	mbstowcs(OutText, Text, TextLen);
	return OutText;
}

char *ConvertFromWChar(const wchar_t *Text) {
	size_t TextLen = wcslen(Text) + 1;
	char *OutText = new char[TextLen];

	wcstombs(OutText, Text, TextLen);
	return OutText;
}

}

/* Точка входа в программу */
int main(int argc, char *argv[]) {
	vpnes_gui::CNESGUI *GUI = NULL;
	VPNES_PATH *FileName = NULL;
#ifdef _WIN32
	LPWSTR * wargv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
#endif

	setlocale(LC_ALL, "");
#ifdef BUILDNUM
	std::clog << "VPNES " VERSION " Build " BUILDNUM
#ifdef SVNREV
		" (" SVNREV ")"
#endif
		<< std::endl;
	std::clog << "License: GPL v2" << std::endl;
#endif
	std::clog << std::internal << std::hex << std::showbase;
	std::cout << std::internal << std::hex << std::showbase;
	if (argc >= 2) {
#ifdef _WIN32
		FileName = wargv[1];
#else
		FileName = argv[1];
#endif
	}
	try {
		GUI = new vpnes_gui::CNESGUI(FileName);
		GUI->Start(argc == 3);
	} catch (std::exception &e) {
		std::clog << "Fatal error: " << e.what() << std::endl;
		return -1;
	}
	delete GUI;
#ifdef _WIN32
	::LocalFree(wargv);
#endif
	return 0;
}
