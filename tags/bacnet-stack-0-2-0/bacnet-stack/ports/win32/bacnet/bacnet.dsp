# Microsoft Developer Studio Project File - Name="bacnet" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=bacnet - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "bacnet.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "bacnet.mak" CFG="bacnet - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "bacnet - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "bacnet - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "bacnet - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "c:\code\bacnet-stack\\" /I "c:\code\bacnet-stack\ports\win32\\" /I "c:\code\bacnet-stack\demo\object\\" /I "c:\code\bacnet-stack\demo\handler\\" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "BACDL_BIP" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.dll /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "bacnet - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "c:\code\bacnet-stack\demo\object" /I "c:\code\bacnet-stack\\" /I "c:\code\bacnet-stack\ports\win32\\" /I "c:\code\bacnet-stack\demo\object\\" /I "c:\code\bacnet-stack\demo\handler\\" /D "_DEBUG" /D "BACDL_BIP" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "bacnet - Win32 Release"
# Name "bacnet - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\abort.c
# End Source File
# Begin Source File

SOURCE=..\..\..\address.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\object\ai.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\object\ao.c
# End Source File
# Begin Source File

SOURCE=..\..\..\apdu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\arf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bacapp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bacdcode.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bacerror.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\object\bacfile.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bacstr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bactext.c
# End Source File
# Begin Source File

SOURCE=..\..\..\bigend.c
# End Source File
# Begin Source File

SOURCE="..\bip-init.c"
# End Source File
# Begin Source File

SOURCE=..\..\..\bip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\crc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\datalink.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\object\device.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\handler\h_arf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\handler\h_arf_a.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\handler\h_iam.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\handler\h_rp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\handler\h_rp_a.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\handler\h_whois.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\handler\h_wp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\iam.c
# End Source File
# Begin Source File

SOURCE=..\..\..\indtext.c
# End Source File
# Begin Source File

SOURCE=..\main.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\handler\noserv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\npdu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\reject.c
# End Source File
# Begin Source File

SOURCE=..\..\..\ringbuf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\rp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\handler\s_rp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\handler\s_whois.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\handler\s_wp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\tsm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\handler\txbuf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\whois.c
# End Source File
# Begin Source File

SOURCE=..\..\..\wp.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\abort.h
# End Source File
# Begin Source File

SOURCE=..\..\..\address.h
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\object\ai.h
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\object\ao.h
# End Source File
# Begin Source File

SOURCE=..\..\..\apdu.h
# End Source File
# Begin Source File

SOURCE=..\..\..\arcnet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bacapp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bacdcode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bacdef.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bacenum.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bacerror.h
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\object\bacfile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bacstr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bigend.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bits.h
# End Source File
# Begin Source File

SOURCE=..\..\..\bytes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\config.h
# End Source File
# Begin Source File

SOURCE=..\..\..\crc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\datalink.h
# End Source File
# Begin Source File

SOURCE=..\..\..\demo\object\device.h
# End Source File
# Begin Source File

SOURCE=..\..\..\ethernet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\handlers.h
# End Source File
# Begin Source File

SOURCE=..\..\..\iam.h
# End Source File
# Begin Source File

SOURCE=..\..\..\mstp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\npdu.h
# End Source File
# Begin Source File

SOURCE=..\..\..\reject.h
# End Source File
# Begin Source File

SOURCE=..\..\..\ringbuf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\rp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\rs485.h
# End Source File
# Begin Source File

SOURCE=..\stdbool.h
# End Source File
# Begin Source File

SOURCE=..\stdint.h
# End Source File
# Begin Source File

SOURCE=..\..\..\tsm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\whois.h
# End Source File
# Begin Source File

SOURCE=..\..\..\wp.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
