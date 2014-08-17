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

#ifndef __GUI_H_
#define __GUI_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../nes/frontend.h"
#include "window.h"
#include "audio.h"
#include "input.h"
#include "video.h"
#if defined(VPNES_CONFIGFILE)
#include "configfile.h"
#endif

namespace vpnes_gui {

/* Основной класс приложения */
class CNESGUI: public vpnes::CNESFrontend {
private:
	/* Основное окно */
	CWindow *Window;
	/* Обработчик аудио */
	CAudio *Audio;
	/* Обработчик ввода */
	CInput *Input;
	/* Обработчик видео */
	CVideo *Video;
public:
	CNESGUI(VPNES_PATH *FileName);
	~CNESGUI();

	/* Запустить NES */
	void Start(bool ForceDendyMode);
	/* Отладочная информация ЦПУ */
	void CPUDebug(uint16 PC, uint8 A, uint8 X, uint8 Y, uint8 S, uint8 P);
	/* Отладочная информация ГПУ */
	void PPUDebug(int Frame, int Scanline, int Cycle, uint16 Reg1, uint16 Reg2, uint16 Reg1Buf);
	/* Критическая ошибка */
	void Panic();
};

}

#endif
