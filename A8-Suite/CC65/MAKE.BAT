@echo off

rem Set flags

 set af=0
 if "%2"=="a" set af=1
 if "%3"=="a" set af=1
 if "%4"=="a" set af=1
 if "%5"=="a" set af=1
 if "%6"=="a" set af=1

 set bf=0
 if "%2"=="b" set bf=1
 if "%3"=="b" set bf=1
 if "%4"=="b" set bf=1
 if "%5"=="b" set bf=1
 if "%6"=="b" set bf=1

 set if=0
 if "%2"=="i" set if=1
 if "%3"=="i" set if=1
 if "%4"=="i" set if=1
 if "%5"=="i" set if=1
 if "%6"=="i" set if=1

rem Build asm file
 echo Building: %1
 echo .include "%1.inc" >game.inc

rem Conditional assemble game
 echo.
 echo Assemble: game.asm
 ca65 -l game.lst -o %1.o game.asm -D iflag=%if% -D aflag=%af% -D bflag=%bf% 
 if not "%errorlevel%" == "0" goto failed

rem Linking
 echo.
 echo Linking: %1
 ld65  %1.o -C a8.cfg -o %1.bin 
 if not "%errorlevel%" == "0" goto failed
 goto end

:failed
echo.
echo Error %errorlevel%
 echo.
 echo Usage: build [agd file] [condition0] [condition1] [condition2]
 echo.
 echo Conditional assembly:
 echo a = Adventuremode
 echo b = Big Sprites (16x24)
 echo i = Invertmode
 echo.

:end
del *.o


