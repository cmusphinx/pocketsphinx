# Microsoft Developer Studio Project File - Name="libpocketsphinx" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libpocketsphinx - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libpocketsphinx.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libpocketsphinx.mak" CFG="libpocketsphinx - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libpocketsphinx - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libpocketsphinx - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libpocketsphinx - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\..\lib\Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\lib\Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\..\src\libpocketsphinx\include" /I "../../../include" /I "../../../../sphinxbase/include" /I "../../../../sphinxbase/include/win32" /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D "_MBCS" /D "HAVE_CONFIG_H" /D "LIBPOCKETSPHINX" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libpocketsphinx - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "..\..\..\lib\Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\lib\Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\include ..\..\..\src\libpocketsphinx\include" /I "../../../include" /I "../../../../sphinxbase/include" /I "../../../../sphinxbase/include/win32" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "_MBCS" /D "HAVE_CONFIG_H" /D "LIBPOCKETSPHINX" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libpocketsphinx - Win32 Release"
# Name "libpocketsphinx - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
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

SOURCE=..\..\..\src\libpocketsphinx\hmm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\kb_main.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\kdtree.c
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

SOURCE=..\..\..\src\libpocketsphinx\ms_gauden.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\ms_mgau.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\ms_senone.c
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

SOURCE=..\..\..\src\libpocketsphinx\subvq_mgau.c
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
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
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
# End Target
# End Project
