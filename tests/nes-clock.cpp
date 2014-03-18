/****************************************************************************\

	NES Emulator
	Copyright (C) 2012-2014  Ivanov Viktor

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

#include <SDL.h>
#include <check.h>
#include <string.h>
#include "../nes/clock.h"

/* Общая функция для проверки */
template <int EmulationTime, int EventTime1, int EventTime2, int EventPeriod1, int EventPeriod2, int Reset = 0>
int NesClock_Test() {
	int Flag = 0;
	int EmulatedClocks = 0;
	vpnes::CClock TestClock;

	struct EmulateClass {
		enum {MAX_EVENTS = 2};
		enum {EVENT_FUNC0 = 0};
		enum {EVENT_FUNC1 = 1};

		vpnes::SEventData LocalEvents[MAX_EVENTS];
		vpnes::SEvent *EventChain[MAX_EVENTS];
		vpnes::CClock *Clock;
		int CurT, *Flag;
		EmulateClass(vpnes::CClock *pClock, int *pFlag) {
			vpnes::SEvent *NewEvent;
			CurT = 0;
			Flag = pFlag;
			Clock = pClock;
			memset(LocalEvents, 0, sizeof(vpnes::SEventData) * MAX_EVENTS);
			NewEvent = new vpnes::SEvent;
			NewEvent->Data = &LocalEvents[EVENT_FUNC0];
			NewEvent->Execute = [this]{this->Func0();};
			EventChain[EVENT_FUNC0] = NewEvent;
			Clock->RegisterEvent(NewEvent);
			NewEvent = new vpnes::SEvent;
			NewEvent->Data = &LocalEvents[EVENT_FUNC1];
			NewEvent->Execute = [this]{this->Func1();};
			Clock->RegisterEvent(NewEvent);
			EventChain[EVENT_FUNC1] = NewEvent;
			Clock->EnableEvent(EventChain[EVENT_FUNC0]);
			Clock->EnableEvent(EventChain[EVENT_FUNC1]);
			Clock->SetEventTime(EventChain[EVENT_FUNC0], EventTime1);
			Clock->SetEventTime(EventChain[EVENT_FUNC1], EventTime2);
		}
		~EmulateClass() {}

		inline void Func0() {
			Clock->SetEventTime(EventChain[EVENT_FUNC0], Clock->GetTime() + EventPeriod1);
			if (CurT == 0) {
				*Flag = 1;
				CurT++;
			} else {
				*Flag = 3;
				Clock->Terminate();
			}
		}
		inline void Func1() {
			Clock->SetEventTime(EventChain[EVENT_FUNC1], Clock->GetTime() + EventPeriod2);
			if (CurT == 1) {
				*Flag = 2;
				CurT++;
				if (Reset != 0)
					Clock->Reset(Clock->GetTime());
			} else
				*Flag = 4;
		}
	} Emulation(&TestClock, &Flag);
	TestClock.Start([&TestClock, &EmulatedClocks](int Clocks) -> int {
		if ((EmulatedClocks += Clocks) >= EmulationTime) {
			TestClock.Terminate();
			return 0;
		}
		return Clocks;
	});
	return Flag;
}

/* Обычная работа */
START_TEST (nes_clock_basic1) {
	ck_assert_int_eq((NesClock_Test<30, 10, 15, 10, 10>()), 3);
}
END_TEST

/* Преждевременное завершение */
START_TEST (nes_clock_basic2) {
	ck_assert_int_eq((NesClock_Test<20, 10, 15, 10, 10>()), 2);
}
END_TEST

/* Разный период */
START_TEST (nes_clock_basic3) {
	ck_assert_int_eq((NesClock_Test<25, 10, 15, 10, 5>()), 4);
}
END_TEST

/* Выход сразу после vpnes::CClock::Terminated */
START_TEST (nes_clock_basic4) {
	ck_assert_int_eq((NesClock_Test<0, 10, 15, 10, 10>()), 0);
}
END_TEST

/* Вызов vpnes::CClock::Reset не влияет на работу */
START_TEST (nes_clock_basic5) {
	ck_assert_int_eq((NesClock_Test<30, 10, 15, 10, 10, 1>()), 3);
}
END_TEST

Suite *NesClockSuite() {
	Suite *s = suite_create("vpnes::CClock");
	TCase *tc_core = tcase_create("Core");

	tcase_add_test(tc_core, nes_clock_basic1);
	tcase_add_test(tc_core, nes_clock_basic2);
	tcase_add_test(tc_core, nes_clock_basic3);
	tcase_add_test(tc_core, nes_clock_basic4);
	tcase_add_test(tc_core, nes_clock_basic5);
	suite_add_tcase(s, tc_core);
	return s;
}

int main(int argc, char *argv[]) {
	int number_failed;
	Suite *s = NesClockSuite();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
