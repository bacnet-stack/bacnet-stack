# Makefile build under MinGW32

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
