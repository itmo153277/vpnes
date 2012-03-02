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

#include "window.h"
#include <SDL.h>

SDL_Surface *screen;
Sint32 delaytime;
Uint32 framestarttime = 0;

/* Инициализация SDL */
int InitMainWindow(void) {
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return -1;
	SDL_WM_SetCaption("VPNES 0.1", NULL);
	screen = SDL_SetVideoMode(256, 224, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (screen == NULL)
		return -1;
	return 0;
}

/* Выход */
void AppQuit(void) {
	SDL_Quit();
}

/* Callback-функция */
int WindowCallback(double Tim) {
	SDL_Event event;
	int quit = 0;
	static int cur_frame = 0;
	char buf[20];

	while (SDL_PollEvent(&event))
	switch (event.type) {
		case SDL_QUIT:
			quit = -1;
	}
	delaytime = ((Uint32) Tim) - (SDL_GetTicks() - framestarttime);
	if (delaytime > 0)
		SDL_Delay((Uint32) delaytime);
	framestarttime = SDL_GetTicks();
	itoa(cur_frame, buf, 10);
	SDL_WM_SetCaption(buf, NULL);
	cur_frame++;
	return quit;
}
