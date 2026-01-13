@echo off

tcc -ml -Ic:\tc\include -Lc:\tc\lib CRUZKAN.C AUDIO.C VIDEO.C MOUSE.C
if errorlevel 1 goto build_failed

echo Build OK.
CRUZKAN
exit
goto end

:build_failed
echo Build FAILED.

:end
