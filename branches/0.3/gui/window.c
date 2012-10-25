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
#include <SDL_syswm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if defined(VPNES_INTERACTIVE)
#include "interactive.h"
#endif

int WindowState = 0;
int SaveState = 0;
SDL_Surface *screen;
SDL_Surface *bufs;
Sint32 HideMouse = -1;
#if !defined(VPNES_DISABLE_SYNC)
Sint32 delaytime;
Uint32 framestarttime = 0;
Uint32 framecheck = 0;
double framejit = 0.0;
double delta = 0.0;
#if !defined(VPNES_DISABLE_FSKIP)
Uint32 skip = 0;
#endif
#endif
VPNES_VBUF vbuf;
VPNES_ABUF abuf;
VPNES_IBUF ibuf;
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
SDL_AudioSpec *desired = NULL;
SDL_AudioSpec *obtained = NULL;
SDL_AudioSpec *hardware_spec = NULL;
sint16 *PCMBuf[2];
int PCMindex = 1;
int PCMready = 0;
int PCMplay = 0;
int UseJoy = 0;
SDL_Joystick *joy = NULL;
#ifdef _WIN32
HICON Icon = INVALID_HANDLE_VALUE;
HWND WindowHandle = INVALID_HANDLE_VALUE;
HINSTANCE Instance = INVALID_HANDLE_VALUE;

#include "win32-res/win32-res.h"

/* Инициализация Win32 */
void InitWin32() {
	SDL_SysWMinfo WMInfo;

	SDL_VERSION(&WMInfo.version)
	SDL_GetWMInfo(&WMInfo);
	/* Получаем данные */
	WindowHandle = WMInfo.window;
	Instance = GetModuleHandle(NULL);
	/* Иконка */
	Icon = LoadIcon(Instance, MAKEINTRESOURCE(IDI_MAINICON));
	SetClassLong(WindowHandle, GCL_HICON, (LONG) Icon);
}

/* Удаление Win32 */
void DestroyWin32() {
	DestroyIcon(Icon);
}
#endif

/* Callback для SDL */
void AudioCallbackSDL(void *Data, Uint8 *Stream, int Len) {
	PCMready = 0;
	if (PCMplay)
		memcpy(Stream, PCMBuf[PCMindex], Len);
}

/* Вызывается libvpnes'ом */
void AudioCallback(int Task, void *Data) {
	SDL_LockAudio();
	switch (Task) {
		case VPNES_PCM_START:
			if (!PCMplay && PCMready) {
				SDL_PauseAudio(0);
				PCMplay = -1;
			}
			break;
		case VPNES_PCM_UPDATE:
			if (PCMready) {
				/* Противофаза */
				((VPNES_ABUF *) Data)->Pos = ((VPNES_ABUF *) Data)->Size / 2;
				memcpy(((VPNES_ABUF *) Data)->PCM, ((VPNES_ABUF *) Data)->PCM +
					((VPNES_ABUF *) Data)->Pos, (((VPNES_ABUF *) Data)->Pos) *
					sizeof(sint16));
				if (ftell(stderr) >= 0) {
					fputs("Warning: audio buffer was dropped\n", stderr);
					fflush(stderr);
				}
			} else {
				((VPNES_ABUF *) Data)->PCM = PCMBuf[PCMindex];
				PCMindex ^= 1;
				/* Буфер заполнен */
				PCMready = 1;
			}
			/* Запускаем аудио */
			if (!PCMplay) {
				SDL_PauseAudio(0);
				PCMplay = -1;
			}
			break;
		case VPNES_PCM_STOP:
			PCMplay = 0;
			SDL_PauseAudio(-1);
			break;
	}
	SDL_UnlockAudio();
}

/* Инициализация SDL */
int InitMainWindow(int Width, int Height) {
	/* Инициализация библиотеки */
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return -1;
#ifdef _WIN32
	InitWin32();
#endif
#if defined(VPNES_INTERACTIVE)
	InitInteractive();
#endif
	SDL_WM_SetCaption("VPNES " VERSION, NULL);
	screen = SDL_SetVideoMode(Width, Height, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (screen == NULL)
		return -1;
	return 0;
}

/* Очистить окно */
void ClearWindow(void) {
	if (screen == NULL)
		return;
	if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);
	SDL_FillRect(screen, NULL, screen->format->colorkey);
	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);
	SDL_Flip(screen);
}

/* Установить режим */
int SetMode(int Width, int Height, double FrameLength) {
	int i;

	screen = SDL_SetVideoMode(Width * 2, Height * 2, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (screen == NULL)
		return -1;
	/* Буфер для PPU */
	bufs = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, 32, screen->format->Rmask,
		screen->format->Gmask, screen->format->Bmask, screen->format->Amask);
	if (bufs == NULL)
		return -1;
	for (i = 0; i < 64; i++)
		Pal[i] = SDL_MapRGB(bufs->format, NES_Palette[i][0], NES_Palette[i][1], NES_Palette[i][2]);
	vbuf.Buf = bufs->pixels;
	vbuf.Pal = Pal;
	vbuf.RMask = bufs->format->Rmask;
	vbuf.GMask = bufs->format->Gmask;
	vbuf.BMask = bufs->format->Bmask;
	vbuf.AMask = bufs->format->Amask;
	vbuf.Skip = 0;
	ibuf = calloc(8, sizeof(int));
	desired = malloc(sizeof(SDL_AudioSpec));
	obtained = malloc(sizeof(SDL_AudioSpec));
	desired->freq = 44100;
	desired->format = AUDIO_S16SYS;
	desired->channels = 1;
	desired->samples = (int) (FrameLength * 44.1 * 2); /* 2 кадра */
	desired->callback = AudioCallbackSDL;
	desired->userdata = NULL;
	if (SDL_OpenAudio(desired, obtained) < 0) {
		free(desired);
		return -1;
	}
	free(desired);
	hardware_spec = obtained;
	abuf.Callback = AudioCallback;
	PCMBuf[0] = malloc(hardware_spec->size);
	memset(PCMBuf[0], hardware_spec->silence, hardware_spec->size);
	PCMBuf[1] = malloc(hardware_spec->size);
	memset(PCMBuf[1], hardware_spec->silence, hardware_spec->size);
	abuf.PCM = PCMBuf[PCMindex ^ 1];
	abuf.Size = hardware_spec->size / sizeof(sint16);
	abuf.Freq = 44.1;
	if (SDL_NumJoysticks() > 0) {
		joy = SDL_JoystickOpen(0);
		if (joy) {
			fprintf(stderr, "Joystick: %s\n", SDL_JoystickName(0));
			UseJoy = -1;
			SDL_JoystickEventState(SDL_ENABLE);
		} else
			fputs("Couldn't open joystick\n", stderr);
		fflush(stderr);
	}
	if (!UseJoy)
		SDL_JoystickEventState(SDL_IGNORE);
	Resume();
	return 0;
}

/* Выход */
void AppClose(void) {
	if (UseJoy)
		SDL_JoystickClose(joy);
	SDL_CloseAudio();
	free(obtained);
	free(PCMBuf[0]);
	free(PCMBuf[1]);
	free(ibuf);
	if (bufs != NULL) {
		SDL_FreeSurface(bufs);
	}
}

void AppQuit() {
#if defined(VPNES_INTERACTIVE)
	DestroyInteractive();
#endif
#ifdef _WIN32
	DestroyWin32();
#endif
	SDL_Quit();
}

/* Пауза */
void Pause(void) {
	SDL_PauseAudio(-1);
}

/* Проделжить */
void Resume(void) {
	SDL_LockAudio();
	if (PCMplay && PCMready)
		SDL_PauseAudio(0);
	SDL_UnlockAudio();
#if !defined(VPNES_DISABLE_SYNC)
	framestarttime = SDL_GetTicks();
	framecheck = framestarttime;
	HideMouse = framestarttime;
	framejit = 0;
#if !defined(VPNES_DISABLE_FSKIP)
	skip = 0;
#endif
#endif
}

/* Callback-функция */
int WindowCallback(uint32 VPNES_CALLBACK_EVENT, void *Data) {
	SDL_Event event;
	int quit = 0;
	double *Tim;
	VPNES_CPUHALT *HaltData;
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
#if !defined(VPNES_DISABLE_SYNC) && !defined(VPNES_DISABLE_FSKIP)
			if (vbuf.Skip) {
				framestarttime = SDL_GetTicks();
				framejit += (framestarttime - framecheck) - *Tim;
				skip += (framestarttime - framecheck);
				framecheck = framestarttime;
				if (skip >= 50)
					framejit = 0;
				if (framejit < *Tim) {
					vbuf.Skip = 0;
					skip = 0;
				}
				delta += *Tim;
				delta -= (Uint32) delta;
#if defined(VPNES_SHOW_CURFRAME) || defined(VPNES_SHOW_FPS)
				cur_frame++;
#endif
				return 0;
			}
#endif
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
				snprintf(buf, 20, "%.3lf", fps);
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
						WindowState = VPNES_QUIT;
						break;
					case SDL_MOUSEMOTION:
						SDL_ShowCursor(SDL_ENABLE);
						/* Если эмултор запущен больше ~25 дней, произойдет ошибка */
						HideMouse = SDL_GetTicks();
						break;
					case SDL_KEYDOWN:
//						if (!UseJoy)
							switch (event.key.keysym.sym) {
								case SDLK_c:
									ibuf[VPNES_INPUT_A] = 1;
									break;
								case SDLK_x:
									ibuf[VPNES_INPUT_B] = 1;
									break;
								case SDLK_a:
									ibuf[VPNES_INPUT_SELECT] = 1;
									break;
								case SDLK_s:
									ibuf[VPNES_INPUT_START] = 1;
									break;
								case SDLK_DOWN:
									ibuf[VPNES_INPUT_DOWN] = 1;
									break;
								case SDLK_UP:
									ibuf[VPNES_INPUT_UP] = 1;
									break;
								case SDLK_LEFT:
									ibuf[VPNES_INPUT_LEFT] = 1;
									break;
								case SDLK_RIGHT:
									ibuf[VPNES_INPUT_RIGHT] = 1;
								default:
									break;
							}
						break;
					case SDL_KEYUP:
//						if (!UseJoy)
							switch (event.key.keysym.sym) {
								case SDLK_c:
									ibuf[VPNES_INPUT_A] = 0;
									break;
								case SDLK_x:
									ibuf[VPNES_INPUT_B] = 0;
									break;
								case SDLK_a:
									ibuf[VPNES_INPUT_SELECT] = 0;
									break;
								case SDLK_s:
									ibuf[VPNES_INPUT_START] = 0;
									break;
								case SDLK_DOWN:
									ibuf[VPNES_INPUT_DOWN] = 0;
									break;
								case SDLK_UP:
									ibuf[VPNES_INPUT_UP] = 0;
									break;
								case SDLK_LEFT:
									ibuf[VPNES_INPUT_LEFT] = 0;
									break;
								case SDLK_RIGHT:
									ibuf[VPNES_INPUT_RIGHT] = 0;
									break;
								case SDLK_F1:
									quit = -1;
									WindowState = VPNES_RESET;
									break;
								case SDLK_F5:
									quit = -1;
									WindowState = VPNES_SAVESTATE;
									break;
								case SDLK_F7:
									quit = -1;
									WindowState = VPNES_LOADSTATE;
								default:
									break;
							}
						break;
					case SDL_JOYAXISMOTION:
						if (event.jaxis.which == 0)
							switch (event.jaxis.axis) {
								case 0:
									if (event.jaxis.value < -8192)
										ibuf[VPNES_INPUT_LEFT] = 1;
									else
										ibuf[VPNES_INPUT_LEFT] = 0;
									if (event.jaxis.value > 8192)
										ibuf[VPNES_INPUT_RIGHT] = 1;
									else
										ibuf[VPNES_INPUT_RIGHT] = 0;
									break;
								case 1:
									if (event.jaxis.value < -8192)
										ibuf[VPNES_INPUT_UP] = 1;
									else
										ibuf[VPNES_INPUT_UP] = 0;
									if (event.jaxis.value > 8192)
										ibuf[VPNES_INPUT_DOWN] = 1;
									else
										ibuf[VPNES_INPUT_DOWN] = 0;
									break;
							}
						break;
					case SDL_JOYBUTTONDOWN:
						if (event.jbutton.which == 0)
							switch (event.jbutton.button) {
								case 0:
									ibuf[VPNES_INPUT_A] = 1;
									break;
								case 1:
									ibuf[VPNES_INPUT_B] = 1;
									break;
								case 6:
									ibuf[VPNES_INPUT_SELECT] = 1;
									break;
								case 7:
									ibuf[VPNES_INPUT_START] = 1;
									break;
							}
						break;
					case SDL_JOYBUTTONUP:
						if (event.jbutton.which == 0)
							switch (event.jbutton.button) {
								case 0:
									ibuf[VPNES_INPUT_A] = 0;
									break;
								case 1:
									ibuf[VPNES_INPUT_B] = 0;
									break;
								case 6:
									ibuf[VPNES_INPUT_SELECT] = 0;
									break;
								case 7:
									ibuf[VPNES_INPUT_START] = 0;
									break;
							}
						break;
					case SDL_JOYHATMOTION:
						if (event.jhat.which == 0) {
							if (event.jhat.value & SDL_HAT_UP)
								ibuf[VPNES_INPUT_UP] = 1;
							else
								ibuf[VPNES_INPUT_UP] = 0;
							if (event.jhat.value & SDL_HAT_DOWN)
								ibuf[VPNES_INPUT_DOWN] = 1;
							else
								ibuf[VPNES_INPUT_DOWN] = 0;
							if (event.jhat.value & SDL_HAT_LEFT)
								ibuf[VPNES_INPUT_LEFT] = 1;
							else
								ibuf[VPNES_INPUT_LEFT] = 0;
							if (event.jhat.value & SDL_HAT_RIGHT)
								ibuf[VPNES_INPUT_RIGHT] = 1;
							else
								ibuf[VPNES_INPUT_RIGHT] = 0;
						}
						break;
					case SDL_USEREVENT:
						quit = -1;
						break;
#if defined(VPNES_INTERACTIVE)
					case SDL_SYSWMEVENT:
						if (InteractiveDispatcher(event.syswm.msg))
							quit = -1;
						break;
#endif
				}
#if !defined(VPNES_DISABLE_SYNC)
			/* Синхронизация */
			delta += *Tim;
			delaytime = ((Uint32) delta) - (SDL_GetTicks() - framestarttime);
			delta -= (Uint32) delta;
			if (framejit > delaytime)
				delaytime = 0;
			if (delaytime > 0)
				SDL_Delay((Uint32) (delaytime - framejit));
			framestarttime = SDL_GetTicks();
			framejit += (framestarttime - framecheck) - *Tim;
#if !defined(VPNES_DISABLE_FSKIP)
			if (framejit > *Tim) {
				vbuf.Skip = -1;
				if (ftell(stderr) >= 0) {
					fputs("Warning: frame skip\n", stderr);
					fflush(stderr);
				}
			}
#endif
			if ((HideMouse >= 0) && ((framestarttime - HideMouse) >= 2000))
				SDL_ShowCursor(SDL_DISABLE);
			framecheck = framestarttime;
#endif
			return quit;
		case VPNES_CALLBACK_CPUHALT:
			if (ftell(stderr) >= 0) {
				HaltData = (VPNES_CPUHALT *) Data;
				fprintf(stderr, "Fatal Error: CPU halted\n"
				                "Registers:\n"
				                "  PC: 0x%4.4x\n"
				                "   A: 0x%2.2x\n"
				                "   X: 0x%2.2x\n"
				                "   Y: 0x%2.2x\n"
				                "   S: 0x%2.2x\n"
				                "   P: 0x%2.2x\n",
					HaltData->PC, HaltData->A, HaltData->X, HaltData->Y, HaltData->S,
					HaltData->State);
				}
				fflush(stderr);
			return 0;
		case VPNES_CALLBACK_INPUT:
			*((VPNES_INPUT *) Data) = ibuf;
			return 0;
		case VPNES_CALLBACK_PCM:
			*((VPNES_PCM *) Data) = &abuf;
			return 0;
		case VPNES_CALLBACK_VIDEO:
			*((VPNES_VIDEO *) Data) = &vbuf;
			return 0;
	}
	return -1;
}
