# Microsoft Developer Studio Project File - Name="agistudio" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=agistudio - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "agistudio.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "agistudio.mak" CFG="agistudio - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "agistudio - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "agistudio - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "agistudio - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".."
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FD /c
# ADD CPP /nologo /MDd /W3 /O1 /I "$(QTDIR)\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "QT_DLL" /D "QT_THREAD_SUPPORT" /D "UNICODE" /D "NO_DEBUG" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib winmm.lib wsock32.lib imm32.lib wsock32.lib winmm.lib $(QTDIR)\lib\qt-mt230nc.lib $(QTDIR)\lib\qtmain.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(QTDIR)\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "QT_DLL" /D "QT_THREAD_SUPPORT" /D "UNICODE" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib winmm.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib winmm.lib wsock32.lib imm32.lib wsock32.lib winmm.lib $(QTDIR)\lib\qt-mt230nc.lib $(QTDIR)\lib\qtmain.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc" /nodefaultlib:"msvcrt" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "agistudio - Win32 Release"
# Name "agistudio - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\agicommands.cpp
# End Source File
# Begin Source File

SOURCE=.\agiplay.cpp
# End Source File
# Begin Source File

SOURCE=.\bpicture.cpp
# End Source File
# Begin Source File

SOURCE=.\dir.cpp
# End Source File
# Begin Source File

SOURCE=.\game.cpp
# End Source File
# Begin Source File

SOURCE=.\helpwindow.cpp
# End Source File
# Begin Source File

SOURCE=.\linklist.cpp
# End Source File
# Begin Source File

SOURCE=.\logcompile.cpp
# End Source File
# Begin Source File

SOURCE=.\logdecode.cpp
# End Source File
# Begin Source File

SOURCE=.\logedit.cpp
# End Source File
# Begin Source File

SOURCE=.\logic.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\menu.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_dir.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_helpwindow.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_logedit.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_menu.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_objedit.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_options.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_picedit.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_preview.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_resources.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_roomgen.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_viewedit.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_wordsedit.cpp
# End Source File
# Begin Source File

SOURCE=.\moc_wutil.cpp
# End Source File
# Begin Source File

SOURCE=.\object.cpp
# End Source File
# Begin Source File

SOURCE=.\objedit.cpp
# End Source File
# Begin Source File

SOURCE=.\options.cpp
# End Source File
# Begin Source File

SOURCE=.\picedit.cpp
# End Source File
# Begin Source File

SOURCE=.\picture.cpp
# End Source File
# Begin Source File

SOURCE=.\preview.cpp
# End Source File
# Begin Source File

SOURCE=.\resources.cpp
# End Source File
# Begin Source File

SOURCE=.\roomgen.cpp
# End Source File
# Begin Source File

SOURCE=.\util.cpp
# End Source File
# Begin Source File

SOURCE=.\view.cpp
# End Source File
# Begin Source File

SOURCE=.\viewedit.cpp
# End Source File
# Begin Source File

SOURCE=.\words.cpp
# End Source File
# Begin Source File

SOURCE=.\wordsedit.cpp
# End Source File
# Begin Source File

SOURCE=.\wutil.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\agicommands.h
# End Source File
# Begin Source File

SOURCE=.\dir.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing dir.h...
InputPath=.\dir.h

"moc_dir.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe dir.h -o moc_dir.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing dir.h...
InputPath=.\dir.h

"moc_dir.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe dir.h -o moc_dir.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\game.h
# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=.\helpwindow.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing helpwindow.h...
InputPath=.\helpwindow.h

"moc_helpwindow.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe helpwindow.h -o moc_helpwindow.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing helpwindow.h...
InputPath=.\helpwindow.h

"moc_helpwindow.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe helpwindow.h -o moc_helpwindow.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\linklist.h
# End Source File
# Begin Source File

SOURCE=.\logedit.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing logedit.h...
InputPath=.\logedit.h

"moc_logedit.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe logedit.h -o moc_logedit.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing logedit.h...
InputPath=.\logedit.h

"moc_logedit.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe logedit.h -o moc_logedit.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\logic.h
# End Source File
# Begin Source File

SOURCE=.\menu.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing menu.h...
InputPath=.\menu.h

"moc_menu.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe menu.h -o moc_menu.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing menu.h...
InputPath=.\menu.h

"moc_menu.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe menu.h -o moc_menu.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\object.h
# End Source File
# Begin Source File

SOURCE=.\objedit.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing objedit.h...
InputPath=.\objedit.h

"moc_objedit.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe objedit.h -o moc_objedit.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing objedit.h...
InputPath=.\objedit.h

"moc_objedit.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe objedit.h -o moc_objedit.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\options.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing options.h...
InputPath=.\options.h

"moc_options.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe options.h -o moc_options.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing options.h...
InputPath=.\options.h

"moc_options.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe options.h -o moc_options.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\picedit.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing picedit.h...
InputPath=.\picedit.h

"moc_picedit.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe picedit.h -o moc_picedit.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing picedit.h...
InputPath=.\picedit.h

"moc_picedit.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe picedit.h -o moc_picedit.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\picture.h
# End Source File
# Begin Source File

SOURCE=.\preview.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing preview.h...
InputPath=.\preview.h

"moc_preview.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe preview.h -o moc_preview.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing preview.h...
InputPath=.\preview.h

"moc_preview.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe preview.h -o moc_preview.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\resources.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing resources.h...
InputPath=.\resources.h

"moc_resources.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe resources.h -o moc_resources.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing resources.h...
InputPath=.\resources.h

"moc_resources.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe resources.h -o moc_resources.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\roomgen.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing roomgen.h...
InputPath=.\roomgen.h

"moc_roomgen.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe roomgen.h -o moc_roomgen.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing roomgen.h...
InputPath=.\roomgen.h

"moc_roomgen.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe roomgen.h -o moc_roomgen.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\util.h
# End Source File
# Begin Source File

SOURCE=.\view.h
# End Source File
# Begin Source File

SOURCE=.\viewedit.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing viewedit.h...
InputPath=.\viewedit.h

"moc_viewedit.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe viewedit.h -o moc_viewedit.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing viewedit.h...
InputPath=.\viewedit.h

"moc_viewedit.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe viewedit.h -o moc_viewedit.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\words.h
# End Source File
# Begin Source File

SOURCE=.\wordsedit.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing wordsedit.h...
InputPath=.\wordsedit.h

"moc_wordsedit.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe wordsedit.h -o moc_wordsedit.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing wordsedit.h...
InputPath=.\wordsedit.h

"moc_wordsedit.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe wordsedit.h -o moc_wordsedit.cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wutil.h

!IF  "$(CFG)" == "agistudio - Win32 Release"

# Begin Custom Build - Moc'ing wutil.h...
InputPath=.\wutil.h

"moc_wutil.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe wutil.h -o moc_wutil.cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "agistudio - Win32 Debug"

# Begin Custom Build - Moc'ing wutil.h...
InputPath=.\wutil.h

"moc_wutil.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%QTDIR%\bin\moc.exe wutil.h -o moc_wutil.cpp

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\agistudio.rc
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# End Group
# Begin Group "Interfaces"

# PROP Default_Filter "ui"
# End Group
# Begin Source File

SOURCE=.\license.txt
# End Source File
# End Target
# End Project
