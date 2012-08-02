@Echo Off

Verify Extensions 2> nul
SetLocal EnableExtensions EnableDelayedExpansion
If ErrorLevel 1 GoTo End

Title Creating zip file

If Not Exist vpnes-0.3 (
 Echo Source is not ready
 GoTo Error
)
If Exist vpnes-0.3-win32.zip Call :Command Del vpnes-0.3-win32.zip
Call :Command C:\Progra~1\7-zip\7z a -tzip -mx9 vpnes-0.3-win32.zip vpnes-0.3
If ErrorLevel 1 (
 Echo Failed to create zip file
 GoTo Error
)
Call :Command RmDir /S /Q vpnes-0.3

Set BuildNumber=0

If Exist win32-builds\BUILD (
 For /F "delims=" %%A In (win32-builds\BUILD) Do (
  Set /A BuildNumber=%%A
 )
)

Set /A NextBuildNumber=%BuildNumber% + 1
Echo %NextBuildNumber% 1> win32-builds\BUILD 2> nul

Call :Command Copy vpnes-0.3-win32.zip vpnes-0.3-win32-%BuildNumber%.zip

:Error
Pause > nul
EndLocal
Exit

GoTo :EOF

:Command

Echo %*
%*
GoTo :EOF

:End
Echo Unsupported COMMAND.COM
