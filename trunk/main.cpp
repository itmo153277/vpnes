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

#include <fstream>
#include <SDL.h>
#include "nes/nes.h"
#include "nes/mapper.h"

/* Точка входа в программу */
int main(int argc, char *argv[]) {
	vpnes::CBasicNES *NES;
	std::fstream ROM;

	/* Открываем образ */
	if (argc != 2)
		return 0;
	ROM.open(argv[1], std::ios_base::in | std::ios_base::binary);
	if (ROM.fail())
		return 0;
	NES = OpenROM(ROM);
	if (NES == NULL)
		return -1;
	/* Инициализация SDL */
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return -1;
	/* Запуск */
	NES->PowerOn();
	delete NES;
	SDL_Quit();
	return 0;
}
