@echo off
path %~dp0;%PATH%
if "%~1" == "" goto end
vpnes %1 1
:end
