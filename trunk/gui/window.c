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
#if defined(VPNES_USE_TTF)
#include <SDL_ttf.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if defined(VPNES_INTERACTIVE)
#include "interactive.h"
#endif

int WindowState = 0;
int SaveState = 0;
int CanLog = 0;
SDL_Surface *screen;
SDL_Surface *bufs;
Sint32 HideMouse = -1;
int Active = 0;
int Stop = 0;
int Run = 0;
#if !defined(VPNES_DISABLE_SYNC)
double PlayRate = 1.0;
Sint32 delaytime;
Uint32 framestarttime = 0;
Uint32 framecheck = 0;
double framejit = 0.0;
double delta = 0.0;
#if !defined(VPNES_DISABLE_FSKIP)
Uint32 skip = 0;
int skipped = 0;
#endif
#endif
VPNES_VBUF vbuf;
VPNES_ABUF abuf;
VPNES_IBUF ibuf;
Uint32 Pal[64 * 8]; /* + tinted */
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
int skipped_samples = 0;
SDL_Joystick *joy = NULL;
#if defined(VPNES_USE_TTF)
TTF_Font *font = NULL;
SDL_Surface *text_surface = NULL;
char *text_string = NULL;
const SDL_Color text_color = {224, 224, 224};
const SDL_Color border_color = {56, 56, 56};
const SDL_Color bg_color = {40, 40, 40};
SDL_Rect text_rect = {10, 10};
SDL_Rect text_in_rect = {7, 7};
SDL_Rect border_rect = {1, 1};
Uint32 text_timer = 0;
Sint32 text_timer_dif = 0;
int draw_text = 0;
char spstr[20];
#ifdef _WIN32
HGLOBAL ResourceHandle = INVALID_HANDLE_VALUE;
HRSRC ResourceInfo = INVALID_HANDLE_VALUE;
void *FontData = NULL;
SDL_RWops *FontRW = NULL;
#endif
#endif
#ifdef _WIN32
HICON Icon = INVALID_HANDLE_VALUE;
HWND WindowHandle = INVALID_HANDLE_VALUE;
HINSTANCE Instance = INVALID_HANDLE_VALUE;

#include "win32-res/win32-res.h"

/* Инициализация Win32 */
void InitWin32() {
	SDL_SysWMinfo WMInfo;

	/* Получаем данные */
	SDL_VERSION(&WMInfo.version)
	SDL_GetWMInfo(&WMInfo);
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

/* Возобновление */
void Resume(void);

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
				if (!Stop)
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
				if (CanLog && (skipped_samples == 0)) {
					fputs("Warning: audio buffer was dropped\n", stderr);
					fflush(stderr);
				}
				skipped_samples += ((VPNES_ABUF *) Data)->Pos;
			} else {
				if (CanLog && (skipped_samples > 0)) {
					fprintf(stderr, "Info: Skipped %d samples\n", skipped_samples);
					fflush(stderr);
					skipped_samples = 0;
				}
				((VPNES_ABUF *) Data)->PCM = PCMBuf[PCMindex];
				PCMindex ^= 1;
				/* Буфер заполнен */
				PCMready = 1;
			}
			/* Запускаем аудио */
			if ((!PCMplay || (SDL_GetAudioStatus() != SDL_AUDIO_PLAYING)) && !Stop) {
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

/* Инициализация лога */
int InitLog(void) {
	if (ftell(stderr) >= 0)
		CanLog = -1;
	else {
		fputc('\r', stderr);
		CanLog = !ferror(stderr);
	}
	return CanLog;
}

/* Инициализация SDL */
int InitMainWindow(int Width, int Height) {
	/* Инициализация библиотеки */
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		return -1;
	/* Установка параметров окна */
#ifdef _WIN32
	InitWin32();
#endif
#if defined(VPNES_INTERACTIVE)
	InitInteractive();
#endif
#if defined(VPNES_USE_TTF)
	if (TTF_Init() < 0)
		return -1;
#ifdef _WIN32
	ResourceInfo = FindResource(Instance, MAKEINTRESOURCE(IDR_MAINFONT), RT_RCDATA);
	ResourceHandle = LoadResource(Instance, ResourceInfo);
	FontData = LockResource(ResourceHandle);
	FontRW = SDL_RWFromConstMem(FontData, SizeofResource(Instance, ResourceInfo));
	if (FontRW != NULL)
		font = TTF_OpenFontRW(FontRW, -1, 22);
#else
	font = TTF_OpenFont("text.otf", 22);
#endif
	if (font == NULL)
		return -1;
#endif
	SDL_WM_SetCaption("VPNES " VERSION, NULL);
	screen = SDL_SetVideoMode(Width, Height, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (screen == NULL)
		return -1;
	/* Джойстик */
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
	int i, j;

	screen = SDL_SetVideoMode(Width * 2, Height * 2, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (screen == NULL)
		return -1;
	/* Буфер для PPU */
	bufs = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, 32, screen->format->Rmask,
		screen->format->Gmask, screen->format->Bmask, screen->format->Amask);
	if (bufs == NULL)
		return -1;
#if defined(VPNES_USE_TTF)
	draw_text = -1;
	text_string = "Hardware reset";
	text_timer_dif = 0;
#endif
#define C_R(comp) ((comp) & 1)
#define C_G(comp) (((comp) & 2) >> 1)
#define C_B(comp) (((comp) & 4) >> 2)
	for (i = 0; i < 64; i++)
		for (j = 0; j < 8; j++)
			Pal[i + 64 * j] = SDL_MapRGB(bufs->format,
				(int) (NES_Palette[i][0] * (6 - C_G(j) - C_B(j)) / 6.0),
				(int) (NES_Palette[i][1] * (6 - C_R(j) - C_B(j)) / 6.0),
				(int) (NES_Palette[i][2] * (6 - C_R(j) - C_G(j)) / 6.0));
#undef C_R
#undef C_G
#undef C_B
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
	PCMindex = 1;
	PCMready = 0;
	PCMplay = 0;
	abuf.PCM = PCMBuf[PCMindex ^ 1];
	abuf.Size = hardware_spec->size / sizeof(sint16);
	abuf.Freq = 44.1;
	skipped_samples = 0;
#if !defined(VPNES_DISABLE_SYNC)
	delta = 0.0;
	PlayRate = 1.0;
#endif
	Active = 0;
	Run = -1;
	Stop = 0;
#if defined(VPNES_INTERACTIVE)
	ChangeRenderState(0);
#endif
	Resume();
	return 0;
}

/* Выход */
void AppClose(void) {
	SDL_CloseAudio();
	free(obtained);
	free(PCMBuf[0]);
	free(PCMBuf[1]);
	free(ibuf);
#if defined(VPNES_USE_TTF)
	if (text_surface != NULL) {
		SDL_FreeSurface(text_surface);
		text_surface = NULL;
	}
#endif
	if (bufs != NULL) {
		SDL_FreeSurface(bufs);
		bufs = NULL;
	}
	Run = 0;
}

void AppQuit() {
#if defined(VPNES_USE_TTF)
	if (font != NULL)
		TTF_CloseFont(font);
#ifdef _WIN32
	UnlockResource(ResourceHandle);
	FreeResource(ResourceHandle);
#endif
	TTF_Quit();
#endif
#if defined(VPNES_INTERACTIVE)
	DestroyInteractive();
#endif
#ifdef _WIN32
	DestroyWin32();
#endif
	if (UseJoy)
		SDL_JoystickClose(joy);
	SDL_Quit();
}

/* Пауза */
void Pause(void) {
	if (!Run || !Active)
		return;
#if defined(VPNES_USE_TTF)
	text_timer_dif = text_timer - SDL_GetTicks();
#endif
	SDL_PauseAudio(-1);
	Active = 0;
}

/* Остановка */
void StopRender(void) {
	Stop = -1;
	SDL_PauseAudio(-1);
#if defined(VPNES_INTERACTIVE)
	ChangeRenderState(-1);
#endif
}

/* Возобновить рендер */
void ResumeRender(void) {
	if (!Stop)
		return;
	Stop = 0;
	SDL_LockAudio();
	if (PCMplay && PCMready)
		SDL_PauseAudio(0);
	SDL_UnlockAudio();
#if defined(VPNES_INTERACTIVE)
	ChangeRenderState(0);
#endif
}

/* Продолжить */
void Resume(void) {
	if (!Run || Active)
		return;
	if (!Stop) {
		SDL_LockAudio();
		if (PCMplay && PCMready)
			SDL_PauseAudio(0);
		SDL_UnlockAudio();
	}
#if !defined(VPNES_DISABLE_SYNC)
	framestarttime = SDL_GetTicks();
	framecheck = framestarttime;
	HideMouse = framestarttime;
#if defined(VPNES_USE_TTF)
	text_timer = framestarttime + text_timer_dif;
#endif
	framejit = 0;
#if !defined(VPNES_DISABLE_FSKIP)
	skip = 0;
	skipped = 0;
#endif
#elif defined(VPNES_USE_TTF)
	text_timer = SDL_GetTicks() + text_timer_dif;
#endif
	Active = -1;
}

/* Callback-функция */
int WindowCallback(uint32 VPNES_CALLBACK_EVENT, void *Data) {
	SDL_Event event;
	int quit = 0;
	double Tim;
	VPNES_CPUHALT *HaltData;
#if defined(VPNES_SHOW_CURFRAME) || defined(VPNES_SHOW_FPS)
	static int cur_frame = 0;
	int first = 1;
	char buf[20];
#endif
#if defined(VPNES_SHOW_FPS)
	static Uint32 fpst = 0;
	Sint32 passed;
	double fps;
#endif

	switch (VPNES_CALLBACK_EVENT) {
		case VPNES_CALLBACK_FRAME:
#if !defined(VPNES_DISABLE_SYNC)
			Tim = *((VPNES_FRAME *) Data) / PlayRate;
#endif
			do {
#if !defined(VPNES_DISABLE_SYNC) && !defined(VPNES_DISABLE_FSKIP)
				if (!vbuf.Skip) {
#endif
				/* Обновляем экран */
				if (SDL_MUSTLOCK(screen))
					SDL_LockSurface(screen);
				SDL_SoftStretch(bufs, NULL, screen, NULL);
#if defined(VPNES_USE_TTF)
				if (text_surface != NULL)
					SDL_BlitSurface(text_surface, NULL, screen, &text_rect);
#endif
				if (SDL_MUSTLOCK(screen))
					SDL_UnlockSurface(screen);
				SDL_Flip(screen);
#if !defined(VPNES_DISABLE_SYNC) && !defined(VPNES_DISABLE_FSKIP)
				}
#endif
#if defined(VPNES_SHOW_CURFRAME)
				if (first) {
					/* Текущий фрейм */
					itoa(cur_frame, buf, 10);
					SDL_WM_SetCaption(buf, NULL);
					cur_frame++;
					first = 0;
				}
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
				while (SDL_PollEvent(&event)) {
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
										break;
									case SDLK_PAUSE:
										quit = -1;
										if (Stop) {
											WindowState = VPNES_RESUME;
											ResumeRender();
										} else {
											WindowState = VPNES_PAUSE;
											StopRender();
										}
										break;
									case SDLK_SPACE:
										quit = -1;
										WindowState = VPNES_STEP;
										StopRender();
										break;
#if !defined(VPNES_DISABLE_SYNC)
									case SDLK_TAB:
										quit = -1;
										WindowState = VPNES_RATE;
										PlayRate = PlayRate * 2;
										if (PlayRate > 4)
											PlayRate = 0.5;
										abuf.Freq = 44.1 / PlayRate;
#endif
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
				}
#if !defined(VPNES_DISABLE_SYNC)
				/* Синхронизация */
				delta += Tim;
				delaytime = ((Uint32) delta) - (SDL_GetTicks() - framestarttime);
				delta -= (Uint32) delta;
#if !defined(VPNES_DISABLE_FSKIP)
				if (!vbuf.Skip) {
#endif
					if (framejit > delaytime)
						delaytime = 0;
					else
						delaytime = (Uint32) (delaytime - framejit);
					if (delaytime > 0)
						SDL_Delay(delaytime);
#if !defined(VPNES_DISABLE_FSKIP)
				}
#endif
				framestarttime = SDL_GetTicks();
				framejit += (framestarttime - framecheck) - Tim;
#if !defined(VPNES_DISABLE_FSKIP)
				if (vbuf.Skip) {
					skip += (framestarttime - framecheck);
					if (skip >= 50) {
						framejit = 0;
						if (CanLog) {
							fputs("Warning: resetting sync\n", stderr);
							fflush(stderr);
						}
					}
					skipped++;
					if (framejit < Tim) {
						vbuf.Skip = 0;
						if (CanLog) {
							fprintf(stderr, "Info: Skipped %d frames\n", skipped);
							fflush(stderr);
						}
					}
				} else if (framejit > Tim) {
					vbuf.Skip = -1;
					skip = 0;
					skipped = 0;
					if (CanLog) {
						fputs("Warning: frame skip\n", stderr);
						fflush(stderr);
					}
				}
#endif
				if ((HideMouse >= 0) && ((framestarttime - HideMouse) >= 2000))
					SDL_ShowCursor(SDL_DISABLE);
				framecheck = framestarttime;
#endif
#if defined(VPNES_USE_TTF)
				if ((quit < 0) && !draw_text && (WindowState != VPNES_QUIT)) {
					draw_text = -1;
					switch (WindowState) {
						case VPNES_SAVESTATE:
							text_string = "Save state";
							break;
						case VPNES_LOADSTATE:
							text_string = "Load state";
							break;
						case VPNES_RESET:
							text_string = "Software reset";
							break;
						case VPNES_PAUSE:
							text_string = "Pause";
							quit = 0;
							break;
						case VPNES_RESUME:
							text_string = "Resume";
							quit = 0;
							break;
						case VPNES_STEP:
							text_string = "Step";
							break;
#if !defined(VPNES_DISABLE_SYNC)
						case VPNES_RATE:
							sprintf(spstr, "Speed: x%1.1f", PlayRate);
							text_string = spstr;
							quit = 0;
							break;
#endif
						default:
							draw_text = 0;
					}
				}
				/* Тут уже должно быть отключено ожидание */
				if (!Active)
					Resume();
				if ((text_surface != NULL) && (draw_text ||
					((SDL_GetTicks() - text_timer) >= 4000))) {
					SDL_FreeSurface(text_surface);
					text_surface = NULL;
				}
				if (draw_text) {
					SDL_Surface *temp_surface;

					temp_surface = TTF_RenderUTF8_Shaded(font, text_string, text_color, bg_color);
					text_surface = SDL_CreateRGBSurface(SDL_HWSURFACE,
						temp_surface->w + text_in_rect.x * 2,
						temp_surface->h + text_in_rect.y * 2,
						screen->format->BitsPerPixel, screen->format->Rmask,
						screen->format->Gmask, screen->format->Bmask, screen->format->Amask);
					SDL_FillRect(text_surface, NULL, SDL_MapRGB(text_surface->format,
						border_color.r, border_color.g, border_color.b));
					border_rect.w = text_surface->w - border_rect.x * 2;
					border_rect.h = text_surface->h - border_rect.y * 2;
					SDL_FillRect(text_surface, &border_rect, SDL_MapRGB(text_surface->format,
						bg_color.r, bg_color.g, bg_color.b));
					SDL_BlitSurface(temp_surface, NULL, text_surface, &text_in_rect);
					SDL_FreeSurface(temp_surface);
					text_timer = SDL_GetTicks();
					draw_text = 0;
				}
#endif
			} while (!quit && Stop);
			return quit;
		case VPNES_CALLBACK_CPUHALT:
#if defined(VPNES_USE_TTF)
			draw_text = -1;
			text_string = "CPU halt";
#endif
			if (CanLog) {
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
