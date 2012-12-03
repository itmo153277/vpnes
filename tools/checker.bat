@echo off
if not exist vpnes-roms mkdir vpnes-roms
for %%a in (*.nes) do call :DoIt "%%~a"
pause
goto :EOF
:DoIt
echo %~nx1
verify rom 2> nul
rominfo "%~f1"
if not "%errorlevel%" == "0" (
 echo No match
) else (
 echo Success
 copy "%~f1" "vpnes-roms\%~nx1" > nul
)
echo.
