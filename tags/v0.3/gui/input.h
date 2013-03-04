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

#ifndef __INPUT_H_
#define __INPUT_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#include "../nes/frontend.h"
#if defined(VPNES_CONFIGFILE)
#include "configfile.h"
#endif

namespace vpnes_gui {

/* Обработчик ввода */
#if defined(VPNES_CONFIGFILE)
class CInputConfigProcessor: public CConfigProcessor {
#else
class CInputConfigProcessor {
#endif
protected:
	/* Обработать сообщение ввода */
	int GetKeyboardAction(SDL_keysym *Event) { return 0; }
	int GetGamepadAxisAction(SDL_JoyAxisEvent *Event);
	int GetGamepadHatAction(SDL_JoyHatEvent *Event);
	int GetGamepadButtonAction(SDL_JoyButtonEvent *Event);
public:
	CInputConfigProcessor() {}
	~CInputConfigProcessor() {}
};

/* Обработчик аудио */
class CInput: public CInputConfigProcessor {
private:
	/* Контроллеры */
	vpnes::CStdController Input1;
	vpnes::CStdController Input2;
public:
	CInput() {}
	~CInput() {}

	/* Обработать сообщение ввода */
	void ProcessKeyboard(SDL_KeyboardEvent *Event) {
		vpnes::CStdController::ButtonName Res;

		switch (Event->keysym.sym) {
			case SDLK_c:
				Res = vpnes::CStdController::ButtonA;
				break;
			case SDLK_x:
				Res = vpnes::CStdController::ButtonB;
				break;
			case SDLK_a:
				Res = vpnes::CStdController::ButtonSelect;
				break;
			case SDLK_s:
				Res = vpnes::CStdController::ButtonStart;
				break;
			case SDLK_DOWN:
				Res = vpnes::CStdController::ButtonDown;
				break;
			case SDLK_UP:
				Res = vpnes::CStdController::ButtonUp;
				break;
			case SDLK_LEFT:
				Res = vpnes::CStdController::ButtonLeft;
				break;
			case SDLK_RIGHT:
				Res = vpnes::CStdController::ButtonRight;
				break;
			default:
				return;
		}
		if (Event->type == SDL_KEYDOWN)
			Input1.PushButton(Res);
		else
			Input1.PopButton(Res);
	}
	void ProcessGamepadAxis(SDL_JoyAxisEvent *Event) {}
	void ProcessGamepadHat(SDL_JoyHatEvent *Event) {}
	void ProcessGamepadButton(SDL_JoyButtonEvent *Event) {}
	/* Интерфейсы к контроллерам */
	inline vpnes::CInputFrontend *GetInput1Frontend() { return &Input1; }
	inline vpnes::CInputFrontend *GetInput2Frontend() { return &Input2; }
};

}

#endif
