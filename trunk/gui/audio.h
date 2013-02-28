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

#ifndef __AUDIO_H_
#define __AUDIO_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../types.h"

#include <SDL.h>
#include <cmath>
#include "../nes/frontend.h"
#if defined(VPNES_CONFIGFILE)
#include "configfile.h"
#endif

namespace vpnes_gui {

/* Зависимости */
class CAudioDependencies:
	public vpnes::CAudioFrontend
#if defined(VPNES_CONFIGFILE)
	, public CConfigProcessor
#endif
	{};

/* Обработчик аудио */
class CAudio: public CAudioDependencies {
private:
	/* Параметры аудио */
	SDL_AudioSpec *ObtainedAudio;
	/* Массив буферов */
	sint16 *PCMBuffers[4];
	/* Флаги заполненности буферов */
	bool BuffersFull[3];
	/* Текущий индекс */
	int PCMIndex;
	/* Заполняемый индекс */
	int CurIndex;
	/* Флаг проигрывания */
	bool PCMPlay;
	/* Флаг работы */
	bool Stop;

	/* Callback */
	static void AudioCallbackSDL(void *Data, Uint8 *Stream, int Len);
protected:
	/* Обновить буфер */
	void UpdateBuffer();
public:
	CAudio();
	~CAudio();

	/* Остановить воспроизведение */
	void StopAudio();
	/* Продолжить воспроизведение */
	void ResumeAudio();
	/* Остановить устройство */
	void StopDevice();
	/* Возобновить устройство */
	void ResumeDevice();
	/* Обновить устройство */
	void UpdateDevice(double FrameLength);
	/* Изменить внутреннюю скорость */
	inline void ChangeSpeed(double Rate) {
		Frequency = 44.1 / Rate;
		Decay = 220 * exp(1 / 44.1 - 1 / Frequency);
	}
};

}

#endif
