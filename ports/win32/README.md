# Makefile build under MinGW32

2023-09-12 EKH: I plan on deprecating this ports/win32 project. I have created a new ports/win that supports the latest 
                Microsoft Visual Studio 2022 Community Edition and I plan on keeping that up to date going forward
                
                Please see the README.md in ports/win for further information.
                
                I understand that others have contributed to this win32 edition, and may depend on it for their projects
                so let us leave this in-place for a year, and delete it any time after 2024-09-12 
                
                And comments/requests/discussions, either to me directly edward@bac-test.com or via the github forum
                
                I hope I am not stepping on any toes. Let me know if there are factors outside my awareness.
                
                Thanks all, and thank you Steve!!

* MSYS2 installation is:

    c:\> winget install --id=MSYS2.MSYS2  -e

* Start MSYS, and install MinGW32.

    $ pacman -Syu mingw-w64-i686-toolchain

* Edit ~/.bashrc file and add:

    alias make=mingw32-make.exe

* Exit MSYS.

* Start MSYS profile for MinGW32.

* Verify MSYS profile:

    $ uname

    MINGW32_NT-10.0-19045

* Verify GCC is targeting i686:

    $ gcc -dumpmachine

    i686-w64-mingw32

* Verify make is built for Windows32:

     make --version
     
     Built for Windows32
