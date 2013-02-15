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
	PCMBuffers[0] = NULL;
	PCMBuffers[1] = NULL;
	PCMPlay = false;
	PCMReady = false;
}

CAudio::~CAudio() {
	if (ObtainedAudio != NULL) {
		::SDL_CloseAudio();
		delete ObtainedAudio;
	}
	delete [] PCMBuffers[0];
	delete [] PCMBuffers[1];
}

/* Callback */
void CAudio::AudioCallbackSDL(void *Data, Uint8 *Stream, int Len) {
	CAudio *Audio = (CAudio *) Data;

	Audio->PCMReady = false;
	if (Audio->PCMPlay)
		memcpy(Stream, Audio->PCMBuffers[Audio->PCMIndex], Len);
}

/* Обновить буфер */
void CAudio::UpdateBuffer() {
	::SDL_LockAudio();
	if (!PCMReady) {
		Buf = PCMBuffers[PCMIndex];
		PCMIndex ^= 1;
		PCMReady = true;
	}
	if (!PCMPlay) {
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
	::SDL_LockAudio();
	if (!PCMPlay && PCMReady) {
		PCMPlay = true;
		::SDL_PauseAudio(0);
	}
	::SDL_UnlockAudio();
}

/* Обновить устройство */
void CAudio::UpdateDevice(double FrameLength) {
	SDL_AudioSpec DesiredAudio;

	if (ObtainedAudio != NULL) {
		::SDL_CloseAudio();
		delete ObtainedAudio;
	}
	ObtainedAudio = new SDL_AudioSpec;
	DesiredAudio.freq = 44100;
	DesiredAudio.format = AUDIO_S16SYS;
	DesiredAudio.channels = 1;
	DesiredAudio.samples = (int) (FrameLength * 44.1 * 4); /* 4 кадра */
	DesiredAudio.callback = AudioCallbackSDL;
	DesiredAudio.userdata = (void *) this;
	if (::SDL_OpenAudio(&DesiredAudio, ObtainedAudio) < 0) {
		delete ObtainedAudio;
		throw CGenericException("Audio initializtion failure");
	}
	delete [] PCMBuffers[0];
	delete [] PCMBuffers[1];
	Length = ObtainedAudio->size / sizeof(sint16);
	Pos = 0;
	PCMBuffers[0] = new sint16[Length];
	memset(PCMBuffers[0], ObtainedAudio->silence, ObtainedAudio->size);
	PCMBuffers[1] = new sint16[Length];
	memset(PCMBuffers[1], ObtainedAudio->silence, ObtainedAudio->size);
	PCMIndex = 1;
	PCMReady = false;
	PCMPlay = false;
	Frequency = 44.1;
	Buf = PCMBuffers[0];
}

}
