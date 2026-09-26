@echo off
setlocal EnableExtensions
cd /d "%~dp0..\app\kaigate"

REM Pick up a user PATH that was updated after this terminal opened.
for /f "tokens=2*" %%A in ('reg query "HKCU\Environment" /v Path 2^>nul') do set "PATH=%%B;%PATH%"
if exist "%USERPROFILE%\flutter\bin\flutter.bat" set "PATH=%USERPROFILE%\flutter\bin;%PATH%"

where flutter >nul 2>nul
if errorlevel 1 (
  echo Flutter not found on PATH in this terminal.
  echo Close this window, open a new PowerShell, and run this script again.
  exit /b 1
)

flutter create --org com.kaigate --project-name kaigate --platforms=android,ios .
if errorlevel 1 exit /b 1
flutter pub get
if errorlevel 1 exit /b 1
echo Done. From app\kaigate run: flutter build apk
endlocal
