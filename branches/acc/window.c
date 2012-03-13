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
SDL_Surface *bufs;
Sint32 delaytime;
Uint32 framestarttime = 0;
double delta = 0.0;
Uint32 Pal[64];
const void *pal = Pal;
const Uint8 NES_Palette[64][3] = {
	{124, 124, 124}, {0,   0,   252}, {0,   0,   188}, {68,  40,  188}, {148, 0,   132}, 
	{168, 0,  32  }, {168, 16,  0  }, {136, 20,  0  }, {80,  48,  0  }, {0,   120, 0  }, 
	{0,   104, 0  }, {0,   88,  0  }, {0,   64,  88 }, {0,   0,   0  }, {0,   0,   0  }, 
	{0,   0,   0  }, {188, 188, 188}, {0,   120, 248}, {0,   88,  248}, {104, 68,  252}, 
	{216, 0,   204}, {228, 0,   88 }, {248, 56,  0  }, {228, 92,  16 }, {172, 124, 0  }, 
	{0,   184, 0  }, {0,   168, 0  }, {0,   168, 68 }, {0,   136, 136}, {0,   0,   0  }, 
	{0,   0,   0  }, {0,   0,   0  }, {248, 248, 248}, {60,  188, 252}, {104, 136, 252}, 
	{152, 120, 248}, {248, 120, 248}, {248, 88,  152}, {248, 120, 88 }, {252, 160, 68 }, 
	{248, 184, 0  }, {184, 248, 24 }, {88,  216, 84 }, {88,  248, 152}, {0,   232, 216}, 
	{120, 120, 120}, {0,   0,   0  }, {0,   0,   0  }, {252, 252, 252}, {164, 228, 252}, 
	{184, 184, 248}, {216, 184, 248}, {248, 184, 248}, {248, 164, 192}, {240, 208, 176}, 
	{252, 224, 168}, {248, 216, 120}, {216, 248, 120}, {184, 248, 184}, {184, 248, 216}, 
	{0,   252, 252}, {216, 216, 216}, {0,   0,   0  }, {0,   0,   0  }
};

/* Инициализация SDL */
void *InitMainWindow(int Width, int Height) {
	int i;

	/* Инициализация библиотеки */
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return NULL;
	SDL_WM_SetCaption("VPNES 0.1", NULL);
	screen = SDL_SetVideoMode(Width * 2, Height * 2, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (screen == NULL)
		return NULL;
	/* Буфер для PPU */
	bufs = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, 32, screen->format->Rmask,
		screen->format->Gmask, screen->format->Bmask, screen->format->Amask);
	if (bufs == NULL)
		return NULL;
	for (i = 0; i < 64; i++)
		Pal[i] = SDL_MapRGB(bufs->format, NES_Palette[i][0], NES_Palette[i][1], NES_Palette[i][2]);
	return bufs->pixels;
}

/* Выход */
void AppQuit(void) {
	if (bufs != NULL) {
		SDL_FreeSurface(bufs);
	}
	SDL_Quit();
}

/* Callback-функция */
int WindowCallback(double Tim) {
	SDL_Event event;
	int quit = 0;
#if 0
	static int cur_frame = 0;
	static Uint32 fpst = 0;
	char buf[20];
	Sint32 passed;
	double fps;
#endif

	/* Обновляем экран */
	if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);
	SDL_SoftStretch(bufs, NULL, screen, NULL);
	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);
	SDL_Flip(screen);
	/* Текущий фрейм */
#if 0
	itoa(cur_frame, buf, 10);
	SDL_WM_SetCaption(buf, NULL);
	passed = SDL_GetTicks() - fpst;
	if (passed > 1000) {
		fps = cur_frame * 1000.0 / passed;
		itoa((int) fps, buf, 10);
		SDL_WM_SetCaption(buf, NULL);
		fpst = SDL_GetTicks();
		cur_frame = 0;
	}
	cur_frame++;
#endif
	/* Обрабатываем сообщения */
	while (SDL_PollEvent(&event))
		switch (event.type) {
			case SDL_QUIT:
				quit = -1;
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_SPACE)
					quit = 1;
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
