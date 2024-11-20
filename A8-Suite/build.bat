@echo off

rem Compile AGD file
 cls
 copy AGDsources\%1.agd agd >nul
 if errorlevel 1 goto copyerror
 cd AGD
 AGD %1 %2 %3 %4
 if errorlevel 1 goto agderror
 copy %1.inc ..\cc65\ >nul
 if errorlevel 1 goto copyerror
 copy game.cfg ..\cc65\ >nul
 if errorlevel 1 goto copyerror
 del %1.*
 del game.cfg

rem Assemble file
 cd ..\cc65
 call make %1 %2 %3 %4 %5 %6 %7 %8 %9
 copy %1.bin ..\altirra\tapes\*.xex >nul
 if errorlevel 1 goto copyerror1
rem del %1.*

rem Start emulator
 echo Starting emulator with %1.bin
 cd ..\altirra
 altirra64 /defprofile:xl /pal /memsize:64K /nobasic /run tapes\%1.xex
 echo Quiting emulator
 cd ..
 goto end

:copyerror1
 cd ..
:copyerror
 echo *** COPY ERROR ***, %1.agd not found .....
 goto end

:agderror
 del %1.*
 del game.cfg
 cd ..
 echo.
 echo *** AGD COMPILER ERROR ***, errors found in %1.AGD
 goto end

:end
