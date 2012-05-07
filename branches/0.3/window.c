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
#include <stdlib.h>

SDL_Surface *screen;
SDL_Surface *bufs;
Sint32 delaytime;
Uint32 framestarttime = 0;
double delta = 0.0;
VPNES_VBUF vbuf;
Uint32 Pal[64];
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
VPNES_VBUF *InitMainWindow(int Width, int Height) {
	int i;

	/* Инициализация библиотеки */
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return NULL;
	SDL_WM_SetCaption("VPNES 0.2", NULL);
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
	vbuf.Buf = bufs->pixels;
	vbuf.Pal = Pal;
	vbuf.RMask = bufs->format->Rmask;
	vbuf.GMask = bufs->format->Gmask;
	vbuf.BMask = bufs->format->Bmask;
	vbuf.AMask = bufs->format->Amask;
	return &vbuf;
}

/* Выход */
void AppQuit(void) {
	if (bufs != NULL) {
		SDL_FreeSurface(bufs);
	}
	SDL_Quit();
}

/* Callback-функция */
int WindowCallback(uint32 VPNES_CALLBACK_EVENT, void *Data) {
	SDL_Event event;
	int quit = 0;
	double *Tim;
#if defined(VPNES_SHOW_CURFRAME) || defined(VPNES_SHOW_FPS)
	static int cur_frame = 0;
	char buf[20];
#endif
#if defined(VPNES_SHOW_FPS)
	static Uint32 fpst = 0;
	Sint32 passed;
	double fps;
#endif

	switch (VPNES_CALLBACK_EVENT) {
		case VPNES_CALLBACK_FRAME:
			Tim = (VPNES_FRAME *) Data;
			/* Обновляем экран */
			if (SDL_MUSTLOCK(screen))
				SDL_LockSurface(screen);
			SDL_SoftStretch(bufs, NULL, screen, NULL);
			if (SDL_MUSTLOCK(screen))
				SDL_UnlockSurface(screen);
			SDL_Flip(screen);
#if defined(VPNES_SHOW_CURFRAME)
			/* Текущий фрейм */
			itoa(cur_frame, buf, 10);
			SDL_WM_SetCaption(buf, NULL);
			cur_frame++;
#elif defined(VPNES_SHOW_FPS)
			/* FPS */
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
#if !defined(VPNES_DISABLE_SYNC)
			/* Синхронизация */
			delta += *Tim;
			delaytime = ((Uint32) delta) - (SDL_GetTicks() - framestarttime);
			delta -= (Uint32) delta;
			if (delaytime > 0)
				SDL_Delay((Uint32) delaytime);
			framestarttime = SDL_GetTicks();
#endif
			return quit;
	}
	return -1;
}
