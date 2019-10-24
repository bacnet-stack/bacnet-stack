# How to configure clang‐format

## Overview
clang-format is an utility to format source code in several languages 
according to predefined settings. Settings for auto-formatting of C source 
files are summarized below, and should be saved in .clang-format file in root 
folder. These settings are chosen to closely match the formatting style used.

## Configuring Editors
### Visual Studio Code
Install the C/C++ extension and customize settings.json to include:

    "C_Cpp.clang_format_path": "/path/to/<clang-format-executable>",
    "C_Cpp.clang_format_style": "file",
    "C_Cpp.clang_format_fallbackStyle": "none"

### Emacs
Add the line

    (load "/path/to/clang-format.el")

to `~/.emacs.d/init.el`. Format a source file with `M-x clang-format-region`.

### CLion
* Add the .clang-format file to the Kratos root directory as 
  explained aboveGo to File->Settings->Tools->External Tools 
  and click on the plus sign. A window should pop up. 
  Choose a name, for example "clang-format"
* For the Tool settings tab I'm using this configuration:
  - Program: clang-format (you should use the name of your executable here)
  - Parameters: --style=file -i $FileName$
  - Working directory: $FileDir$
Now, with your file open, you can go to Tools->External tools and run the config above. It basically calls clang-format and does inplace formatting using the style define in the first .clang-format file found in a parent directory.

### .clang-format file

    ---
    BasedOnStyle: Google
    AllowShortFunctionsOnASingleLine: None
    AllowShortIfStatementsOnASingleLine: false
    AllowShortLoopsOnASingleLine: false
    AlwaysBreakAfterReturnType: None
    BreakBeforeBraces: Linux
    IndentWidth: 4
    SortIncludes: false
    ...
