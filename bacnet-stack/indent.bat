rem Indent the C and H files with specific coding standard
rem requires 'indent.exe' from MSYS (MinGW).
echo -kr -nut -nlp -ip4 -cli4 -bfda -nbc -nbbo -c0 -cd0 -cp0 -di0 -l79 -nhnl > ".indent.pro"

call :treeProcess
goto :eof

:treeProcess
rem perform the indent on all the files of this subdirectory:
for %%f in (*.c) do (
    indent.exe "%%f" -o "%%f"
)
for %%f in (*.h) do (
    indent.exe "%%f" -o "%%f"
)
rem loop over all directories and sub directories
for /D %%d in (*) do (
    cd %%d
    call :treeProcess
    cd ..
)
exit /b

