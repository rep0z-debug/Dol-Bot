@echo off
setlocal

:: CONFIGURATION CHECK
if defined CMAKE_PREFIX_PATH goto :skip_qt_msg
echo [INFO] CMAKE_PREFIX_PATH not set. Attempting to find Qt...
:skip_qt_msg

REM DETECT MINGW
where g++ >nul 2>nul
if %ERRORLEVEL% EQU 0 goto :found_mingw

echo [INFO] g++ not in PATH. Checking common locations...
if exist "C:\Qt\Tools\mingw1310_64\bin" goto :set_mingw_1310
if exist "C:\Qt\Tools\mingw1120_64\bin" goto :set_mingw_1120
goto :found_mingw

:set_mingw_1310
echo [INFO] Found C:\Qt\Tools\mingw1310_64\bin
set "PATH=C:\Qt\Tools\mingw1310_64\bin;%PATH%"
goto :found_mingw

:set_mingw_1120
echo [INFO] Found C:\Qt\Tools\mingw1120_64\bin
set "PATH=C:\Qt\Tools\mingw1120_64\bin;%PATH%"
goto :found_mingw

:found_mingw

REM DETECT QT
if defined CMAKE_PREFIX_PATH goto :found_qt

if exist "C:\Qt\6.10.1\mingw_64" goto :set_qt_6101
if exist "C:\Qt\6.8.0\mingw_64" goto :set_qt_680
if exist "C:\Qt\6.5.0\mingw_64" goto :set_qt_650
goto :found_qt

:set_qt_6101
echo [INFO] Found C:\Qt\6.10.1\mingw_64
set "CMAKE_PREFIX_PATH=C:\Qt\6.10.1\mingw_64"
goto :found_qt

:set_qt_680
echo [INFO] Found C:\Qt\6.8.0\mingw_64
set "CMAKE_PREFIX_PATH=C:\Qt\6.8.0\mingw_64"
goto :found_qt

:set_qt_650
echo [INFO] Found C:\Qt\6.5.0\mingw_64
set "CMAKE_PREFIX_PATH=C:\Qt\6.5.0\mingw_64"
goto :found_qt

:found_qt

REM VALIDATION CHECKS 
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 goto :err_cmake

where g++ >nul 2>nul
if %ERRORLEVEL% NEQ 0 goto :err_gpp

where mingw32-make >nul 2>nul
if %ERRORLEVEL% NEQ 0 goto :check_make_fallback
goto :start_build

:check_make_fallback
where make >nul 2>nul
if %ERRORLEVEL% NEQ 0 goto :err_make
goto :start_build

REM ERROR HANDLERS 
:err_cmake
echo [ERROR] CMake not found. Install from https://cmake.org/download/
pause
exit /b 1

:err_gpp
echo [ERROR] g++ (MinGW) not found. Install Qt with MinGW or standalone MinGW.
pause
exit /b 1

:err_make
echo [ERROR] 'mingw32-make' or 'make' not found.
echo Check your MinGW installation / PATH.
pause
exit /b 1

:start_build
REM CLEANUP 
echo [1/4] Cleaning...
if exist build rmdir /s /q build
if exist dist rmdir /s /q dist

REM CONFIG 
echo.
echo [2/4] Configuring...
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 goto :err_config

REM COMPILE 
echo.
echo [3/4] Compiling...
cmake --build build --config Release
if %ERRORLEVEL% NEQ 0 goto :err_compile

REM DEPLOY
echo.
echo [4/4] Deploying...
mkdir dist
copy build\DolBot.exe dist\DolBot.exe >nul

set "WINDEPLOYQT=windeployqt.exe"

if not defined CMAKE_PREFIX_PATH goto :run_deploy
if exist "%CMAKE_PREFIX_PATH%\bin\windeployqt.exe" set "WINDEPLOYQT=%CMAKE_PREFIX_PATH%\bin\windeployqt.exe"

:run_deploy
echo Running windeployqt...
"%WINDEPLOYQT%" dist\DolBot.exe --compiler-runtime --no-translations --no-opengl-sw
if %ERRORLEVEL% NEQ 0 goto :err_deploy

echo.
echo [SUCCESS] Build complete in 'dist/'.
echo ZIP the contents of 'dist/' and upload to GitHub Releases!
pause
exit /b 0

:err_config
echo [ERROR] CMake Configuration failed.
pause
exit /b 1

:err_compile
echo [ERROR] Compilation failed.
pause
exit /b 1

:err_deploy
echo [WARNING] windeployqt failed. You might need to copy DLLs manually.
pause
exit /b 0