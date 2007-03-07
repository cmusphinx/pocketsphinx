# Microsoft Developer Studio Project File - Name="pocketsphinx" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=pocketsphinx - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pocketsphinx.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pocketsphinx.mak" CFG="pocketsphinx - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pocketsphinx - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "pocketsphinx - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "pocketsphinx - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\bin\Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "POCKETSPHINX_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../../include" /I "../../../../sphinxbase/include" /I "../../../../sphinxbase/include/win32" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "POCKETSPHINX_EXPORTS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib libsphinxutil.lib libsphinxfe.lib libsphinxfeat.lib libsphinxad.lib winmm.lib /nologo /dll /machine:I386 /libpath:"..\..\..\..\sphinxbase\lib\Release"

!ELSEIF  "$(CFG)" == "pocketsphinx - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\bin\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "POCKETSPHINX_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../../include" /I "../../../../sphinxbase/include" /I "../../../../sphinxbase/include/win32" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "POCKETSPHINX_EXPORTS" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib libsphinxutil.lib libsphinxfe.lib libsphinxfeat.lib libsphinxad.lib winmm.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\sphinxbase\lib\Debug"

!ENDIF 

# Begin Target

# Name "pocketsphinx - Win32 Release"
# Name "pocketsphinx - Win32 Debug"
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\allphone.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\basic_types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\bin_mdef.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\blkarray_list.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\c.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\cache_lm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\cepio.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\cmdln_macro.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\dict.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\fbs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\fsg_history.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\fsg_lextree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\fsg_psubtree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\fsg_search.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\kb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\kdtree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\list.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lm_3g.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\lmclass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\log.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\logs3.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\magic.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\mdef.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\msd.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\mulaw.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\phone.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\posixwin32.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\s2_semi_mgau.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\s2io.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\s2params.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\s2types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\s3types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\search.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\search_const.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\senscr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\smmap4f.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\strfuncs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\time_align.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\tmat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\uttproc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\word_fsg.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\allphone.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\awriteshort.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\bin_mdef.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\blkarray_list.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\cache_lm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\dict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\fbs_main.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\fsg_history.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\fsg_lextree.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\fsg_psubtree.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\fsg_search.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\kb_main.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\kdtree.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\lab.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\list.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\lm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\lm_3g.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\lmclass.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\log_add.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\mdef.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\nxtarg.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\phone.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\s2_mmap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\s2_semi_mgau.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\search.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\searchlat.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\senscr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\skipto.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\strcasecmp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\subvq_mgau.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\time_align.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\tmat.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\uttproc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\vector.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\word_fsg.c
# End Source File
# End Group
# End Target
# End Project
