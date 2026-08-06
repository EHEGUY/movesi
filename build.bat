@echo off
echo Compiling Resources...
windres src\resources.rc -O coff -o src\resources.o

echo Compiling Application...
g++ -O2 -mwindows src\main.cpp src\resources.o -o Movesi.exe -luser32 -lgdi32 -lshell32 -lcomctl32

if %errorlevel% equ 0 (
    echo.
    echo Build Succeeded! Created Movesi.exe
    del src\resources.o
) else (
    echo.
    echo Build Failed with code %errorlevel%!
)
