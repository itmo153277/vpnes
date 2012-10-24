@echo off

:: ������ ��� �������� �����-������� ��������� VPNES
:: ���������������� ��� ���� ("as is"), ��������� ����� ���������

:: �������� ����������
:: �� ���������� DelayedExpansion, �.�. ������ ������ ���������� "!"
:: � ����� �����
Verify Extensions 2> nul
SetLocal EnableExtensions

If ErrorLevel 1 GoTo End

Echo VPNES bug report builder
Echo Verion: 0.2
Echo.

Set VPNES_SCRIPT_PATH=%~dp0
Set VPNES_SCRIPT_NAME=%~nx0
Set SDL_STDIO_REDIRECT=1

:: ���������, ���� �� � ��������� VPNES
Call :CheckVPNES

GoTo Step%Result%

:Step1

Echo VPNES found: %VPNES_PATH%

:: ����� ROM-������
Call :FindROM %*
Echo ROM file: %VPNES_ROM%
Echo.

:: ��������� ���������
Echo Starting emulator:
Call :Command "%VPNES_PATH%" "%VPNES_ROM%"
Set VPNES_RET=%ErrorLevel%
Echo.

:: ������� ������
Call :BuildReport

For /F "delims=" %%A In ("Report.txt") Do (
 Echo Report file: %%~fA
)

Pause>nul

:Step0

EndLocal
GoTo :EOF

:: ������� ������
:PrintLines

Set Lines=%~1

:PrintLines_l1
For /F "tokens=1* delims=," %%A In ("%Lines%") Do (
 Echo %%A
 Set Lines=%%~B
)
If "%Lines%" == "" GoTo :EOF
If "%Lines:~0,1%" == " " Set Lines=%Lines:~1%
If Not "%Lines%" == "" GoTo PrintLines_l1

GoTo :EOF

:: �������� �� ������������� VPNES
:CheckVPNES

:: ���� ���������
Call :FindVPNES

Set Result=0
If "%VPNES_PATH%" == "" GoTo :EOF

For /F "delims=" %%A In ("%VPNES_PATH%") Do (
 Set VPNES_DIR=%%~dpA
)

"%VPNES_PATH%" 2> nul

If ErrorLevel 1 GoTo :EOF
If Not Exist "%VPNES_DIR%stderr.txt" GoTo :EOF

Del "%VPNES_DIR%stderr.txt"

Set Result=1

GoTo :EOF

:: ����� VPNES
:FindVPNES

Call :EnsureNoQuotes %VPNES_PATH%
For /F "delims=" %%A In ("%Result%") Do (
 If Exist "%%~fA" (
  Set VPNES_PATH=%%~fA
  GoTo :EOF
 )
)

Set FindDirs=%VPNES_SCRIPT_PATH%;%CD%;%PATH%

For /F "delims=" %%A In ("vpnes.exe") Do (
 Set VPNES_PATH=%%~f$FindDirs:A
)

GoTo :EOF

:: ������� �����
:BuildReport

Echo == Bug report == >> Report.txt
Echo Version 0.2 >> Report.txt
Echo %date% %time% >> Report.txt

For /F "delims=" %%A In ("%VPNES_ROM%") Do (
 Echo ROM file: %%~nxA >> Report.txt
)

Echo. >> Report.txt
Echo = Program info = >> Report.txt

For /F "usebackq tokens=1* delims=: " %%A In ("%VPNES_DIR%stderr.txt") Do (
 If "%%A" == "VPNES" (
:: ������
  Echo Version: %%B >> Report.txt
 ) Else If "%%A" == "ROM" (
:: ���������� �� ������
  Echo. >> Report.txt
  Echo = ROM info = >> Report.txt
  Call :PrintLines "%%~B" >> Report.txt
 ) Else If "%%A" == "Fatal" (
  Echo. >> Report.txt
  Echo = Fatal error = >> Report.txt
  For /F "tokens=1* delims=:" %%C In ("%%B") Do (
   Echo Error: %%D >> Report.txt
  )
  Echo. >> Report.txt
 ) Else If "%%A" == "PC" (
  Echo PC reg: %%B >> Report.txt
 ) Else If "%%A" == "A" (
  Echo  A reg: %%B >> Report.txt
 ) Else If "%%A" == "X" (
  Echo  X reg: %%B >> Report.txt
 ) Else If "%%A" == "Y" (
  Echo  Y reg: %%B >> Report.txt
 ) Else If "%%A" == "S" (
  Echo  S reg: %%B >> Report.txt
 ) Else If "%%A" == "P" (
  Echo  P reg: %%B >> Report.txt
 )
)

Del "%VPNES_DIR%stderr.txt"

Echo. >> Report.txt
Echo Program exit: %VPNES_RET% >> Report.txt

Set /P VPNES_DESC="Enter your bug description: "
Call :EnsureNoQuotes %VPNES_DESC%
If Not "%Result%" == "" (
 Echo. >> Report.txt
 Echo Bug description: >> Report.txt
 Echo %VPNES_DESC% >> Report.txt
)
Echo.

GoTo :EOF

:: ����� ROM
:FindROM

If "%~1" == "" (
 Call :FindROM_l1
) Else Set VPNES_ROM=%~f1

If Exist "%VPNES_ROM%" GoTo FindROM_l2
Shift
GoTo FindROM

:FindROM_l1

Set /P VPNES_ROM="Enter path to ROM image: "
Call :EnsureNoQuotes %VPNES_ROM%
If "%Result%" == "" GoTo FindROM_l1
For /F "delims=" %%A In ("%Result%") Do (
 Set VPNES_ROM=%%~fA
)

:FindROM_l2

GoTo :EOF

:: �������� ������
:ShowError

Set PreMsg=
If Not "%~1" == "" Set PreMsg=[%~n1]: 
Echo %PreMsg%Error %ErrorLevel%

GoTo :EOF

:: ���������� ���������� �������
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

:: ��������
:Command

:: ����� ������
Copy nul nul > nul

Echo %*
%*
If ErrorLevel 1 GoTo ShowError %1

GoTo :EOF

:End
Echo Unsupported COMMAND.COM