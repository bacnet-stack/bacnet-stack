@echo off

rem Run clang-format over the source tree to conform to project requirements

rem Open a Tools->Command Line->Developer Command Prompt from within Visual studio
rem run clang-format-win.bat

echo running clang-format

cd ..\..\..

for /R %%f in (*.c) do clang-format --style=file -i  "%%f"
for /R %%f in (*.h) do clang-format --style=file -i  "%%f"

cd "ports\win32\microsoft visual studio"
