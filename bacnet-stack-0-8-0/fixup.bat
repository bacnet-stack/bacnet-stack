@echo off
rem fix DOS/Unix names and Subversion EOL-Style
rem unix2dos.exe from MSYS: mingw.sourceforge.net
rem svn.exe from Subversion Tools
call :treeProcess
goto :eof

:treeProcess
rem fix all the specific files of this subdirectory:
for %%f in (*.c) do (
    unix2dos.exe "%%f"
    svn.exe propset svn:eol-style native "%%f"
    svn.exe propset svn:mime-type text/plain "%%f"
)
for %%f in (*.h) do (
    unix2dos.exe "%%f"
    svn.exe propset svn:eol-style native "%%f"
    svn.exe propset svn:mime-type text/plain "%%f"
)
for %%f in (*.bat) do (
    unix2dos.exe "%%f"
    svn.exe propset svn:eol-style native "%%f"
    svn.exe propset svn:mime-type text/plain "%%f"
)
rem loop over all directories and sub directories
for /D %%d in (*) do (
    cd %%d
    call :treeProcess
    cd ..
)
exit /b
