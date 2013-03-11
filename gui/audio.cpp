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

#include "audio.h"

#include <cstring>

namespace vpnes_gui {

/* CAudio */

CAudio::CAudio() {
	ObtainedAudio = NULL;
	memset(PCMBuffers, 0, sizeof(sint16 *) * 4);
	PCMPlay = false;
	Stop = false;
	WriteWAV = false;
	memcpy(WAVHeader.ChunkID, "RIFF", 4 * sizeof(char));
	memcpy(WAVHeader.RIFFType, "WAVE", 4 * sizeof(char));
	memcpy(WAVHeader.fmtID, "fmt ", 4 * sizeof(char));
	memcpy(WAVHeader.dataID, "data", 4 * sizeof(char));
	WAVHeader.fmtSize = 18;
	WAVHeader.Compression = 0x0001;
	WAVHeader.Channels = 1;
	WAVHeader.SampleRate = 44100;
	WAVHeader.BytesPerSec = 44100 * 2;
	WAVHeader.BlockAlign = 2;
	WAVHeader.BitsPerSample = 16;
	WAVHeader.ExtraBytes = 0;
}

CAudio::~CAudio() {
	int i;

	if (WriteWAV)
		StopWAVRecord();
	if (ObtainedAudio != NULL) {
		::SDL_CloseAudio();
		delete ObtainedAudio;
	}
	for (i = 0; i < 4; i++)
		delete [] PCMBuffers[i];
}

/* Callback */
void CAudio::AudioCallbackSDL(void *Data, Uint8 *Stream, int Len) {
	CAudio *Audio = (CAudio *) Data;

	if (Audio->PCMPlay && Audio->BuffersFull[Audio->PCMIndex]) {
		Audio->BuffersFull[Audio->PCMIndex] = false;
		memcpy(Stream, Audio->PCMBuffers[Audio->PCMIndex], Len);
		memcpy(Audio->PCMBuffers[3], Audio->PCMBuffers[Audio->PCMIndex], Audio->Size *
			sizeof(sint16));
		Audio->PCMIndex++;
		if (Audio->PCMIndex > 2)
			Audio->PCMIndex = 0;
	} else
		memcpy(Stream, Audio->PCMBuffers[3], Len);
}

/* Обновить буфер */
void CAudio::UpdateBuffer() {
	int NextIndex;

	if (WriteWAV) {
		WAVHeader.ChunkDataSize += Size * sizeof(sint16);
		WAVHeader.Size += Size * sizeof(sint16);
		WAVStream.write((char *) Buf, Size * sizeof(sint16));
	}
	::SDL_LockAudio();
	NextIndex = CurIndex + 1;
	if (NextIndex > 2)
		NextIndex = 0;
	if (!BuffersFull[NextIndex]) {
		BuffersFull[CurIndex] = true;
		CurIndex = NextIndex;
		Buf = PCMBuffers[CurIndex];
	}
	if (!PCMPlay && !Stop) {
		::SDL_PauseAudio(0);
		PCMPlay = true;
	}
	::SDL_UnlockAudio();
}

/* Остановить воспроизведение */
void CAudio::StopAudio() {
	::SDL_LockAudio();
	PCMPlay = false;
	::SDL_PauseAudio(-1);
	::SDL_UnlockAudio();
}

/* Продолжить воспроизведение */
void CAudio::ResumeAudio() {
	if (Stop)
		return;
	::SDL_LockAudio();
	if (!PCMPlay && BuffersFull[PCMIndex]) {
		PCMPlay = true;
		::SDL_PauseAudio(0);
	}
	::SDL_UnlockAudio();
}

/* Остановить устройство */
void CAudio::StopDevice() {
	int i;

	if (Stop)
		return;
	Stop = true;
	StopAudio();
	for (i = 0; i < 3; i++)
		BuffersFull[i] = false; /* DROP SAMPLES !!! */
	if (WriteWAV) {
		WAVHeader.ChunkDataSize += Pos * sizeof(sint16);
		WAVHeader.Size += Pos * sizeof(sint16);
		WAVStream.write((char *) Buf, Pos * sizeof(sint16));
	}
	PCMIndex = CurIndex;
	Pos = 0;
}

/* Возобновить устройство */
void CAudio::ResumeDevice() {
	if (!Stop)
		return;
	Stop = false;
	ResumeAudio();
}

/* Начать запись WAV */
bool CAudio::StartWAVRecord(const char *FileName) {
	if (WriteWAV)
		StopWAVRecord();
	WAVStream.clear();
	WAVStream.open(FileName,  std::ios_base::out | std::ios_base::binary |
		std::ios_base::trunc);
	WAVHeader.ChunkDataSize = sizeof(SWAVHeader) - 8;
	WAVHeader.Size = 0;
	WAVStream.write((char *) &WAVHeader, sizeof(SWAVHeader));
	WriteWAV = !WAVStream.fail();
	Pos = 0; /* DROP SAMPLES !!! */
	return WriteWAV;
}

/* Остановить запись WAV */
bool CAudio::StopWAVRecord() {
	if (!WriteWAV)
		return false;
	WriteWAV = false;
	WAVStream.seekp(0);
	WAVStream.write((char *) &WAVHeader, sizeof(SWAVHeader));
	WAVStream.close();
	return !WAVStream.fail();
}

/* Обновить устройство */
void CAudio::UpdateDevice(double FrameLength) {
	SDL_AudioSpec DesiredAudio;
	int i;

	if (WriteWAV)
		StopWAVRecord();
	if (ObtainedAudio != NULL) {
		::SDL_CloseAudio();
		delete ObtainedAudio;
	}
	ObtainedAudio = new SDL_AudioSpec;
	DesiredAudio.freq = 44100;
	DesiredAudio.format = AUDIO_S16SYS;
	DesiredAudio.channels = 1;
	DesiredAudio.samples = (int) (FrameLength * 44.1 * 2); /* 2 кадра */
	DesiredAudio.callback = AudioCallbackSDL;
	DesiredAudio.userdata = (void *) this;
	if (::SDL_OpenAudio(&DesiredAudio, ObtainedAudio) < 0) {
		delete ObtainedAudio;
		throw CGenericException("Audio initializtion failure");
	}
	Size = ObtainedAudio->size / sizeof(sint16);
	Pos = 0;
	for (i = 0; i < 4; i++) {
		delete [] PCMBuffers[i];
		PCMBuffers[i] = new sint16[Size];
		if (i < 3)
			BuffersFull[i] = false;
		memset(PCMBuffers[i], ObtainedAudio->silence, ObtainedAudio->size);
	}
	PCMIndex = 0;
	CurIndex = 0;
	PCMPlay = false;
	Stop = false;
	Buf = PCMBuffers[0];
	ChangeSpeed(1.0);
}

}
