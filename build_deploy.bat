# step 1: build environment: please adapt
# 1.1. Qt:
C:\Qt\6.3.1\msvc2019_64\bin\qtenv2.bat
 
# 1.1. init VC build system
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64

windeployqt release\iland\release\iland.exe --dir deploy --qmldir ..\src\iland\qml

