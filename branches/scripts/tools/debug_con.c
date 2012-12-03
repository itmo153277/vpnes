/* Редирект stderr в консоль */
/* Работает даже с программами с /subsystem:windows */

#include <windows.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 4096

#define USAGE \
	"Usage: debug_con command_line\n" \
	" Execute windows app with stderr redirected to console\n\n" \
	"Report bugs to <viktprog@gmail.com>\n"

/* Pipe */
HANDLE hRead, hWrite;

/* Поток чтения */
DWORD WINAPI ReadThread(void *lpParameter);

int main(int argc, char *argv[]) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa;
	HANDLE hThread = INVALID_HANDLE_VALUE;
	char *cmd;
	char w = ' ';

	if (argc != 2) {
		fputs(USAGE, stderr);
		return 0;
	}
	/* Определяем команду для запуска */
	cmd = GetCommandLine();
	if (*cmd == '"') { /* Пробелы в имени файла */
		w = '"';
		cmd++;
	}
	do { cmd++; } while (*cmd != w);
	if (*(++cmd) == ' ')
		cmd++;
	/* Создаем канал достпный новому процессу */
	memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = 1;
	if (!CreatePipe(&hRead, &hWrite, &sa, 0))
		return 0;
	/* Стандартные файлы */
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = INVALID_HANDLE_VALUE;
	si.hStdOutput = INVALID_HANDLE_VALUE;
	si.hStdError = hWrite;
	/* Создаем процесс */
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	if (CreateProcess(NULL, cmd, NULL, NULL, 1, 0, NULL, NULL, &si, &pi)) {
		/* Запускаем поток чтения */
		hThread = CreateThread(NULL, 0, ReadThread, NULL, 0, NULL);
		/* Ожидаем завершения процесса */
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		/* Закрываем дескриптор канала. hRead теперь недействителен */
		CloseHandle(hWrite);
		/* Ждем завершения потока */
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
	} else
		CloseHandle(hWrite);
	CloseHandle(hRead);
	return 0;
}

DWORD WINAPI ReadThread(void *lpParameter) {
	DWORD s = 0, l = 0, r;
	char Buf[BUF_SIZE];
	HANDLE hOut;

	/* Вывод в stderr */
	hOut = GetStdHandle(STD_ERROR_HANDLE);
	while (PeekNamedPipe(hRead, NULL, 0, NULL, &l, NULL)) {
		if (l > 0) { /* Есть данные для чтения */
			do {
				r = l - s;
				if (r > BUF_SIZE)
					r = BUF_SIZE;
				/* Читаем данные (в буфере уже что-то может быть */
				if (ReadFile(hRead, Buf + s, r + s, &r, NULL)) {
					/* Выводим */
					WriteFile(hOut, Buf, r + s, &s, NULL);
					l -= r;
					s = 0;
				} else
					break;
			} while (l > 0);
		} else {
			if (s > 0) /* Если в буфере есть остаток данных — выводим */
				WriteFile(hOut, Buf, s, &s, NULL);
			/* Ожидаем данные */
			if (!ReadFile(hRead, Buf, 1, &s, NULL) || (s == 0))
				break;
		}
	}
}
