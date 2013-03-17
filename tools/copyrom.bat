@echo off

:: Скрипт для копирования образов iNES 1.0
:: Распространяется как есть ("as is"), запрещены любые изменения

:: Включаем расширения
:: Не используем DelayedExpansion, т.к. многие образа используют "!"
:: в имени файла
Verify Extensions 2> nul
SetLocal EnableExtensions

If ErrorLevel 1 GoTo End

Echo Copy ROM 1>&2
Echo Version: 0.1 1>&2
Echo. 1>&2

Set VPNES_SCRIPT_PATH=%~dp0
Set VPNES_SCRIPT_NAME=%~n0
Set VPNES_TARGET_PATH=%CD%

Path %VPNES_SCRIPT_PATH%;%PATH%

:: Необходима программа 7z для распаковки архивов
Call :Tool 7z.exe

Set VPNES_DIR_LEVEL=0
Call :NewScriptPrefix

If "%~1" == "" GoTo Usage
If "%~2" == "" GoTo Usage

If Not Exist "%~2" Call :Command MkDir "%~2"

Set CleanOnExit=

For /F "usebackq delims=" %%A In ("%~f1") Do (
 Call :CopyRom "%~f2" %%A 
)

For %%A In (%CleanOnExit%) Do (
 Call :Command RD /S /Q "%%~fA"
)

GoTo ExitScript

:Usage

Echo %VPNES_PREFIX% Usage: %VPNES_SCRIPT_NAME% listfile folder 1>&2

:ExitScript

Pause>nul

EndLocal
GoTo :EOF

:: Копировать образ
:CopyROM

Set CurFileName=""
Set LastPath=""
For /F "tokens=1* delims=\" %%A In ("%~2") Do (
 Set CurFileName="%%~A"
 Set LastPath="%%~B"
)

If %LastPath% == "" (
 For /F "delims=" %%A In (%CurFileName%) Do Call :Command Copy "%%~fA" %1
 GoTo :EOF
)

Set /A VPNES_DIR_LEVEL=%VPNES_DIR_LEVEL%+1
Call :NewScriptPrefix

Echo %VPNES_PREFIX% Entering directory: %CurFileName% 1>&2

If "%CurFileName:~1,1%" == "<" (
 Call :ExtractArchive %1 "%CurFileName:~2,-2%" %LastPath%
 Set CurFileName=%CurFileName%
 GoTo CopyROM_l
)

PushD %CurFileName%
Call :CopyROM %1 %LastPath%
PopD

:CopyROM_l

Echo %VPNES_PREFIX% Leaving directory: %CurFileName% 1>&2

Set /A VPNES_DIR_LEVEL=%VPNES_DIR_LEVEL%-1
Call :NewScriptPrefix

GoTo :EOF

:: Распаковать архив
:ExtractArchive

If Not Exist "%~f2-temp" (
 MkDir "%~f2-temp"
 Call :Command 7z e "%~f2" -o"%~f2-temp" -r
 Set CleanOnExit="%~f2-temp" %CleanOnExit%
)
PushD "%~f2-temp"
Call :CopyROM %1 %3
PopD

GoTo :EOF

:: Префикс
:NewScriptPrefix

If "%VPNES_DIR_LEVEL%" == "0" (
 Set VPNES_PREFIX=%VPNES_SCRIPT_NAME%:
) Else (
 Set VPNES_PREFIX=%VPNES_SCRIPT_NAME%[%VPNES_DIR_LEVEL%]:
)

GoTo :EOF

:: Поиск программы в переменных окружения
:Tool

If "%~$PATH:1" == "" (
 GoTo Tool_e
)

Echo tool: %~$PATH:1 1>&2

GoTo :EOF

:Tool_e

Echo.
:Tool_l
Set /p ToolP="Enter path to %~1: "
Call :EnsureNoQuotes %ToolP%
For /F "delims=" %%A In ("%Result%") Do (
 Set ToolP=%%~fA
)
If Not Exist "%ToolP%\%~1" (
 GoTo Tool_l
)
Echo.
Path %ToolP%;%PATH%
Set ToolP=
Call :Tool %*

GoTo :EOF

:: Показать ошибку
:ShowError

Set PreMsg=
If "%~1" == "" (
 Set PreMsg=%VPNES_PREFIX%
) Else (
 Set PreMsg=%~n1:
)
Echo %PreMsg% Error %ErrorLevel% 1>&2

GoTo :EOF

:: Обеспечить отсутствие кавычек
:EnsureNoQuotes

Set Result=

:EnsureNoQuotes_l1

If "%~1" == "" GoTo EnsureNoQuotes_l2

Set Result=%Result% %~1
Shift
GoTo EnsureNoQuotes_l1

:EnsureNoQuotes_l2

If Not "%Result%" == "" Set Result=%Result:~1%

GoTo :EOF

:: Вполнить
:Command

:: Сброс ошибки
Copy nul nul > nul

Echo %* 1>&2
%*
If ErrorLevel 1 GoTo ShowError %1

GoTo :EOF

:End

Echo Unsupported COMMAND.COM