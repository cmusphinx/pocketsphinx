# Microsoft Developer Studio Project File - Name="pocketsphinx" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

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
!MESSAGE "pocketsphinx - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "pocketsphinx - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "pocketsphinx - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "..\..\..\lib\Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\lib\Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\include" /I "..\..\..\src\libpocketsphinx\include" /I "..\..\..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "FAST8B" /D "LM_CACHE_PROFLE" /D FBSVQ_V8=1 /D UNPADDED_CEP=1 /D "_AFXDLL" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "pocketsphinx - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\include ..\..\..\src\libpocketsphinx\include" /I "..\..\..\src\libpocketsphinx\include" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "FAST8B" /D "LM_CACHE_PROFLE" /D FBSVQ_V8=1 /D UNPADDED_CEP=1 /YX /FD /GZ /c
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

# Name "pocketsphinx - Win32 Release"
# Name "pocketsphinx - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="..\..\..\src\libpocketsphinx\add-table.c"
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\agc_emax.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\allphone.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\areadint.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\awriteshort.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\bin_mdef.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\bio.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\bisearch.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\bitvec.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\blk_cdcn_norm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\blkarray_list.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\cache_lm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\case.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\cdcn_init.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\cdcn_norm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\cdcn_update.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\cdcn_utils.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\cep_rw.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\ckd_alloc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\CM_funcs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx_audio\cont_ad_base.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\cont_mgau.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\dict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\eht_quit.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\err.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\fbs_main.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\fe_interface.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\fe_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\fe_sigproc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\fixlog.c
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

SOURCE=..\..\..\src\libpocketsphinx\get_a_word.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\glist.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\hash.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\hmm_tied_r.c
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

SOURCE=..\..\..\src\libpocketsphinx\linklist.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\list.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\live_norm.c
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

SOURCE=..\..\..\src\libpocketsphinx\logmsg.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\logs3.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\longio.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\mdef.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\norm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\nxtarg.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\pconf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\phone.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx_audio\play_win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\prime.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\r_agc_noise.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx_audio\rec_win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\resfft.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\s2_mmap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\s3hash.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\salloc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\sc_cbook_r.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\sc_vq.c
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

SOURCE=..\..\..\src\libpocketsphinx\str2words.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\strcasecmp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\time_align.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\tmat.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\unlimit.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\libpocketsphinx\util.c
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

SOURCE=..\..\..\include\ad.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\..\..\..\Program Files\Microsoft Visual Studio\VC98\Include\BASETSD.H"
# End Source File
# Begin Source File

SOURCE=..\..\..\include\basic_types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\bin_mdef.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\bio.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\bitvec.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\blkarray_list.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\byteorder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\c.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\cache_lm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\case.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\cdcn.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\cdcn_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\cepio.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\ckd_alloc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\CM_macros.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\cont_ad.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\cont_mgau.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\cviterbi4.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\dict.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\err.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\fbs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\fe.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\fixpoint.h
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

SOURCE=..\..\..\include\glist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\hash.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\hmm_tied_r.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\kb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\kdtree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\linklist.h
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

SOURCE=..\..\..\include\logmsg.h
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

SOURCE=..\..\..\include\pconf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\phone.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\posixwin32.h
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

SOURCE=..\..\..\include\s3hash.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\s3types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\sc_vq_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\scvq.h
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

SOURCE=..\..\..\include\str2words.h
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

SOURCE=..\..\..\include\vector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\word_fsg.h
# End Source File
# End Group
# End Target
# End Project
