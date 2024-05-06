REM create a ZIP of the iLand source code and copy the ZIP
REM to the Sync & Share Folder of iLand
REM https://syncandshare.lrz.de/getlink/fiH3yGQEfQDJZD6MMDrrP3d9/

"c:\Program Files\7-Zip\7z.exe" a src_qt6_%DATE%.zip src/*
copy /Y src_qt6_%DATE%.zip C:\Users\ge93hih\"LRZ Sync+Share"\iLand

REM Created archive and copied to sync and share. Hit any key to exit.
pause