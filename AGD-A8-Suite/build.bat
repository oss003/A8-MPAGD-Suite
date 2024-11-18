@echo off

rem Compile AGD file
 cls
 copy AGDsources\%1.agd agd >nul
 if errorlevel 1 goto error
 cd AGD

 AGD %1
 copy %1.inc ..\cc65\ >nul
 copy game.cfg ..\cc65\ >nul
 del %1.*
 del game.cfg

rem Assemble file
 cd ..\cc65
 call make %1 %2 %3 %4 %5 %6 %7 %8 %9
 copy %1.bin ..\altirra\tapes >nul
rem del %1.*

rem Start emulator
 echo Starting emulator with %1.bin
 cd ..\altirra
 altirra64 /defprofile:800 /pal /memsize:64K /nobasic /run tapes\%1.bin
 echo Quiting emulator
 cd ..
 goto end

:error
 echo %1.agd not found .....

:end
