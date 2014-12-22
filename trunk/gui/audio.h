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

#include <fstream>
#include <SDL.h>
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
	enum { PCMBufs = 5 };
	sint16 *PCMBuffers[PCMBufs + 1];
	/* Флаги заполненности буферов */
	bool BuffersFull[PCMBufs];
	/* Текущий индекс */
	int PCMIndex;
	/* Заполняемый индекс */
	int CurIndex;
	/* Флаг проигрывания */
	bool PCMPlay;
	/* Флаг работы */
	bool Stop;
	/* Флаг записи в WAV */
	bool WriteWAV;
	/* Таймеры */
	Uint32 LoadTimer;
	Uint32 ReadTimer;
	/* Нужный буфер */
	int TargetBuf;
	/* Начальная позиция */
	int StartPos;
	/* Количество семплов для выброса */
	int SamplesToDrop;
	/* Количество семплов для выброса */
	int DelaySamples;
	/* Количество семплов для выброшено */
	int SamplesDropped;
	/* Файл записи */
	VPNES_PATH_OSTREAM WAVStream;

	/* Формат WAV */
#pragma pack(push, 1)
	struct SWAVHeader {
		char ChunkID[4]; /* RIFF */
		uint32 ChunkDataSize;
		char RIFFType[4]; /* WAVE */
		char fmtID[4]; /* fmt  */
		uint32 fmtSize; /* 16 */
		uint16 Compression; /* 1 - PCM */
		uint16 Channels; /* 1 - mono */
		uint32 SampleRate; /* 44100 */
		uint32 BytesPerSec; /* 44100 * 2 */
		uint16 BlockAlign; /* 2 — sint16 */
		uint16 BitsPerSample; /* 16 — sint16 */
		uint16 ExtraBytes; /* 0 — PCM */
		char dataID[4]; /* data */
		uint32 Size; /* Data size */
	} WAVHeader;
#pragma pack(pop)

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
	/* Начать запись WAV */
	bool StartWAVRecord(VPNES_PATH *FileName);
	/* Остановить запись WAV */
	bool StopWAVRecord();
	/* Обновить устройство */
	void UpdateDevice(double FrameLength);
	/* Изменить внутреннюю скорость */
	inline void ChangeSpeed(double Rate) {
		Frequency = 44.1 / Rate;
		RecalculateDecay();
	}

	/* Флаг записи WAV */
	inline const bool &IsWritingWAV() const { return WriteWAV; }
	/* Увеличить задержку */
	inline void ChangeDelay(int Time) { DelaySamples += Time; }
};

}

#endif
