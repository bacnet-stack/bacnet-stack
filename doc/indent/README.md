# How to configure clang-format

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
* Add the .clang-format file to the root directory as
  explained above.  Go to File->Settings->Tools->External Tools
  and click on the plus sign. A window should pop up.
  Choose a name, for example "clang-format"
* For the Tool settings tab use this configuration:
  - Program: `clang-format` (use the name of your executable here)
  - Parameters: `--style=file -i $FileName$`
  - Working directory: `$FileDir$`
With your file open, go to `Tools->External tools` and run the config above.
This calls `clang-format` and does in-place formatting using the style
defined in the first `.clang-format` file found in a parent directory.

### .clang-format file

    ---
    BasedOnStyle: WebKit
    BinPackParameters: false
    AlignEscapedNewlines: Left
    PointerAlignment: Right
    AllowShortFunctionsOnASingleLine: None
    AllowShortIfStatementsOnASingleLine: false
    AllowShortLoopsOnASingleLine: false
    BreakBeforeBraces: Linux
    BreakBeforeBinaryOperators: None
    KeepEmptyLinesAtTheStartOfBlocks: false
    PenaltyBreakBeforeFirstCallParameter: 1
    IndentCaseLabels: true
    IndentWidth: 4
    UseTab: Never
    SortIncludes: false
    ColumnLimit: 80
    ...
