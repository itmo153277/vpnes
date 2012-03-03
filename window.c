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


#define rmask 0xff000000
#define gmask 0x00ff0000
#define bmask 0x0000ff00
#define amask 0x000000ff

SDL_Surface *screen;
SDL_Surface *bufs;
Sint32 delaytime;
Uint32 framestarttime = 0;
double delta = 0.0;
Uint32 Pal[4];
const void *pal = Pal;

/* Инициализация SDL */
void *InitMainWindow(int Width, int Height) {
	/* Инициализация библиотеки */
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return NULL;
	SDL_WM_SetCaption("VPNES 0.1", NULL);
	screen = SDL_SetVideoMode(Width, Height, 0, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (screen == NULL)
		return NULL;
	/* Буфер для PPU */
	bufs = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, 32, rmask, gmask, bmask, amask);
	bufs=screen;
	if (bufs == NULL)
		return NULL;
	/* Блокировка */
	if (SDL_MUSTLOCK(bufs))
		SDL_LockSurface(bufs);
	Pal[0] = SDL_MapRGB(bufs->format, 0, 0, 0);
	Pal[1] = SDL_MapRGB(bufs->format, 255, 255, 255);
	Pal[2] = SDL_MapRGB(bufs->format, 0, 0, 255);
	Pal[3] = SDL_MapRGB(bufs->format, 255, 0, 0);
	return bufs->pixels;
}

/* Выход */
void AppQuit(void) {
	if (bufs != NULL) {
		if (SDL_MUSTLOCK(bufs))
			SDL_UnlockSurface(bufs);
		SDL_FreeSurface(bufs);
	}
	SDL_Quit();
}

/* Callback-функция */
int WindowCallback(double Tim) {
	SDL_Event event;
	int quit = 0;
	static int cur_frame = 0;
	static Uint32 fpst = 0;
	char buf[20];
	Sint32 passed;
	double fps;

	/* Обновляем экран */
	if (SDL_MUSTLOCK(bufs))
		SDL_UnlockSurface(bufs);
	SDL_BlitSurface(bufs, NULL, screen, NULL);
	SDL_Flip(screen);
	if (SDL_MUSTLOCK(bufs))
		SDL_LockSurface(bufs);
	/* Текущий фрейм */
#if 0
	itoa(cur_frame, buf, 10);
	SDL_WM_SetCaption(buf, NULL);
#endif
	passed = SDL_GetTicks() - fpst;
	if (passed > 1000) {
		fps = cur_frame * 1000.0 / passed;
		itoa((int) fps, buf, 10);
		SDL_WM_SetCaption(buf, NULL);
		fpst = SDL_GetTicks();
		cur_frame = 0;
	}
	cur_frame++;
	/* Обрабатываем сообщения */
	while (SDL_PollEvent(&event))
		switch (event.type) {
			case SDL_QUIT:
				quit = -1;
		}
#if 1
	/* Синхронизация */
	delta += Tim - (Uint32) Tim;
	delaytime = ((Uint32) Tim) - (SDL_GetTicks() - framestarttime) + ((Uint32) delta);
	delta -= (Uint32) delta;
	if (delaytime > 0)
		SDL_Delay((Uint32) delaytime);
	framestarttime = SDL_GetTicks();
#endif
	return quit;
}
