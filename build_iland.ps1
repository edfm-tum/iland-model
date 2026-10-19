# build iLand on Windows
# release mode

# for testing locally:
# start native MSVC shell (from startmenu): x64 Native Tools Command Prompt for VS 2022
# set PATH=C:\Qt\6.8.0\msvc2022_64\bin;%PATH%
# powershell
# you may need to run: Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
# navigate to root folder and start: .\build_iland.cmd
# use variables for readability 
$cores = $env:NUMBER_OF_PROCESSORS

# Funktion zum Bauen eines Teilprojekts (reduziert Redundanz)
function Build-QtProject {
    param($path, $proFile)
    
    Write-Host "Building $proFile in $path..." -ForegroundColor Cyan
    
    if (!(Test-Path $path)) { New-Item -ItemType Directory -Path $path -Force }
    Push-Location $path
    
    # qmake aufrufen
    qmake ../../$proFile
    
    # nmake für MSVC nutzen (Standard auf GitHub Windows Runnern)
    # -j wird von nmake nicht unterstützt, daher rufen wir es normal auf
    nmake release
    
    Pop-Location
}

# build sub projects
Build-QtProject "src/plugins/build/release" "plugins.pro"
Build-QtProject "src/iland/build/release" "iland.pro"
Build-QtProject "src/ilandc/build/release" "ilandc.pro"
Build-QtProject "src/fonstudio/build/release" "fonstudio.pro"

Write-Host "===================================================================" -ForegroundColor Green
Write-Host "Executables in src/ilandc/build/release and src/iland/build/release" -ForegroundColor Green
Write-Host "===================================================================" -ForegroundColor Green