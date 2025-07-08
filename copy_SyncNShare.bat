REM Copy iland executable to the Sync & Share Folder of iLand
REM https://syncandshare.lrz.de/getlink/fiH3yGQEfQDJZD6MMDrrP3d9/
@echo off
REM Get the current date in YYYYMMDD format
FOR /F "tokens=1-3 delims=. " %%A IN ("%DATE%") DO SET YYYYMMDD=%%C%%B%%A
echo "Date: %YYYYMMDD%"
REM Create the destination folder with the date
SET DEST_FOLDER=C:\Users\ge93hih\"LRZ Sync+Share"\iLand\dev\%YYYYMMDD%
mkdir %DEST_FOLDER% 


REM Copy the files into the created folder
rem Qt 6.5: C:\dev\iland-model\src\build-iland-Desktop_Qt_6_5_0_MSVC2019_64bit-Release
rem Qt 6.7 (llvm): C:\dev\iland-model\src\iland\build\Desktop_Qt_6_7_2_MSVC2019_64bit-Release\release
rem Qt 6.7: copy /Y C:\dev\iland-model\src\iland\build\Desktop_Qt_6_7_2_llvm_mingw_64_bit-Release\release\iland.exe %DEST_FOLDER%
rem Qt 6.7 copy /Y C:\dev\iland-model\src\ilandc\build\Desktop_Qt_6_7_2_llvm_mingw_64_bit-Release\release\ilandc.exe %DEST_FOLDER%
copy /Y C:\dev\iland-model\src\iland\build\Desktop_Qt_6_8_0_llvm_mingw_64_bit-Release\release\iland.exe %DEST_FOLDER%


echo "copied to %DEST_FOLDER%"

pause

