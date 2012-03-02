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

/* Инициализация SDL */
void *InitMainWindow(int Width, int Height) {
	/* Инициализация библиотеки */
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return NULL;
	SDL_WM_SetCaption("VPNES 0.1", NULL);
	screen = SDL_SetVideoMode(Width * 2, Height * 2, 0, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (screen == NULL)
		return NULL;
	/* Буфер для PPU */
	bufs = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, 32, rmask, gmask, bmask, amask);
	if (bufs == NULL)
		return NULL;
	/* Блокировка */
	if (SDL_MUSTLOCK(bufs))
		SDL_LockSurface(bufs);
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
	char buf[20];

	/* Обновляем экран */
	if (SDL_MUSTLOCK(bufs))
		SDL_UnlockSurface(bufs);
	SDL_BlitSurface(bufs, &bufs->clip_rect, screen, &screen->clip_rect);
	SDL_Flip(screen);
	if (SDL_MUSTLOCK(bufs))
		SDL_LockSurface(bufs);
	/* Обрабатываем сообщения */
	while (SDL_PollEvent(&event))
		switch (event.type) {
			case SDL_QUIT:
				quit = -1;
		}
	/* Синхронизация */
	delaytime = ((Uint32) Tim) - (SDL_GetTicks() - framestarttime);
	if (delaytime > 0)
		SDL_Delay((Uint32) delaytime);
	framestarttime = SDL_GetTicks();
	/* Текущий фрейм */
	itoa(cur_frame, buf, 10);
	SDL_WM_SetCaption(buf, NULL);
	cur_frame++;
	return quit;
}