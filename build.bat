@echo off

cd /d "%~dp0"

if not exist build mkdir build

cd build

cmake .. ^
-G Ninja ^
-Wno-dev ^
-DANDROID_ABI=arm64-v8a ^
-DANDROID_PLATFORM=android-26 ^
-DCMAKE_TOOLCHAIN_FILE=C:/android-ndk-r27d/build/cmake/android.toolchain.cmake ^
-DOBBY_GENERATE_SHARED=OFF

if errorlevel 1 pause & exit /b

ninja

pause