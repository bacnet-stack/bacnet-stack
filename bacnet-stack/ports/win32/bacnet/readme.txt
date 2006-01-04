BACnet Stack - SourceForge.net
Build for Visual C++ 6.0

When building the BACnet stack using Visual C++ compiler, 
there are some settings that are important.

Q. error LNK2001: unresolved external symbol _WinMain@16

A. The demo ports/win32/main.c was designed as a Win32 Console 
Application.  If you want to change it to a Windows GUI application, 
you will have to add all the Windows GUI code, including WinMain().
I recommend that you use a framework, such as WxWidgets/WxWindows,
but this has not been done yet.

Q. error C1083: Cannot open include file: 'stdint.h': No such file

A. The BACnet stack uses some header files, and Visual C++ needs to know
where they are:
1. Select "Project" menu
2. Select "Settings..."
3. Select the "C/C++" tab (3rd Tab)
4. Select the Category: Preprocessor
5. You can see the "Additional include directories:" box
6. Type the path to stdint.h in that edit box (using a comma if necessary)
7. Type the path to bacdcode.h in that edit box (using a comma if necessary)
In my system, the paths look like:
c:\code\bacnet-stack\,c:\code\bacnet-stack\ports\win32\
8. Press OK
9. Compile the project again...

Q. error C2065: 'MAX_MPDU' : undeclared identifier

A. The BACnet stack uses a preprocessor define to configure
its datalink layer. In Visual C++, add a Preprocessor Definition by: 
1. Select "Project" menu
2. Select "Settings..."
3. Select the "C/C++" tab (3rd Tab)
4. Select the Category: General
5. You can see the "Preprocessor Definitions:" box
6. Type BACDL_BIP in that edit box (using a comma if necessary)
7. Press OK
8. Compile the project again...

Q. error LNK2001: unresolved external symbol __imp__closesocket@4

A. Visual C++ needs to have the Winsock library to be happy:
1. Select "Project" menu
2. Select "Settings..."
3. Select the "Link" tab (4th Tab)
4. You can see "Object/library modules:" edit box
5. Type Wsock32.LIB in that edit box
6. Press OK
7. Compile the project again...

