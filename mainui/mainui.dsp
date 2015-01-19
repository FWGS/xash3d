# Microsoft Developer Studio Project File - Name="mainui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=mainui - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mainui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mainui.mak" CFG="mainui - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mainui - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mainui - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mainui - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\temp\mainui\!release"
# PROP Intermediate_Dir "..\temp\mainui\!release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PLATFORM_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "./" /I "../common" /I "../pm_shared" /I "../engine" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /opt:nowin98
# ADD LINK32 msvcrt.lib user32.lib /nologo /dll /pdb:none /machine:I386 /nodefaultlib:"libc.lib" /def:".\mainui.def" /out:"..\temp\mainui\!release/menu.dll" /opt:nowin98
# SUBTRACT LINK32 /profile /nodefaultlib
# Begin Custom Build
TargetDir=\Xash3D\src_main\temp\mainui\!release
InputPath=\Xash3D\src_main\temp\mainui\!release\menu.dll
SOURCE="$(InputPath)"

"D:\Xash3D\menu.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetDir)\menu.dll "D:\Xash3D\menu.dll"

# End Custom Build

!ELSEIF  "$(CFG)" == "mainui - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\temp\mainui\!debug"
# PROP Intermediate_Dir "..\temp\mainui\!debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PLATFORM_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "./" /I "../common" /I "../pm_shared" /I "../engine" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /def:".\mainui.def" /pdbtype:sept
# ADD LINK32 msvcrtd.lib user32.lib /nologo /dll /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /def:".\mainui.def" /out:"..\temp\mainui\!debug/menu.dll" /pdbtype:sept
# SUBTRACT LINK32 /incremental:no /nodefaultlib
# Begin Custom Build
TargetDir=\Xash3D\src_main\temp\mainui\!debug
InputPath=\Xash3D\src_main\temp\mainui\!debug\menu.dll
SOURCE="$(InputPath)"

"D:\Xash3D\menu.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetDir)\menu.dll "D:\Xash3D\menu.dll"

# End Custom Build

!ENDIF 

# Begin Target

# Name "mainui - Win32 Release"
# Name "mainui - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\basemenu.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_advcontrols.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_audio.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_btns.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_configuration.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_controls.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_creategame.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_credits.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_customgame.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_gameoptions.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_internetgames.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_langame.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_loadgame.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_main.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_multiplayer.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_newgame.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_playersetup.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_savegame.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_saveload.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_strings.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_video.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_vidmodes.cpp
# End Source File
# Begin Source File

SOURCE=.\menu_vidoptions.cpp
# End Source File
# Begin Source File

SOURCE=.\udll_int.cpp
# End Source File
# Begin Source File

SOURCE=.\ui_title_anim.cpp
# End Source File
# Begin Source File

SOURCE=.\utils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\basemenu.h
# End Source File
# Begin Source File

SOURCE=.\enginecallback.h
# End Source File
# Begin Source File

SOURCE=.\extdll.h
# End Source File
# Begin Source File

SOURCE=.\menu_btnsbmp_table.h
# End Source File
# Begin Source File

SOURCE=.\menu_strings.h
# End Source File
# Begin Source File

SOURCE=.\menufont.H
# End Source File
# Begin Source File

SOURCE=.\ui_title_anim.h
# End Source File
# Begin Source File

SOURCE=.\utils.h
# End Source File
# End Group
# End Target
# End Project
