@echo off
for /f "delims=" %%a in ('git -C "%~dp0" rev-parse --short HEAD') do set GIT_HASH=%%a
echo %GIT_HASH%