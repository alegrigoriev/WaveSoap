# Microsoft Developer Studio Project File - Name="WaveSoapFront" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=WaveSoapFront - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WaveSoapFront.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WaveSoapFront.mak" CFG="WaveSoapFront - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WaveSoapFront - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "WaveSoapFront - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WaveSoapFront", XGAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WaveSoapFront - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /GR /GX /Zi /O2 /I "c:\ntddk\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0400 /D "OEMRESOURCE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib msacm32.lib wmstub.lib wmvcore.lib delayimp.lib /nologo /subsystem:windows /map /machine:I386 /out:"Release/WaveSoap.exe" /libpath:"G:\WMSDK\WMFSDK\lib" /delayload:wmvcore.dll
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "WaveSoapFront - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "c:\ntddk\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0400 /D "OEMRESOURCE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib msacm32.lib wmstub.lib wmvcore.lib delayimp.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"Debug/WaveSoap.exe" /pdbtype:sept /libpath:"G:\WMSDK\WMFSDK\lib" /delayload:wmvcore.dll
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "WaveSoapFront - Win32 Release"
# Name "WaveSoapFront - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AmplitudeRuler.cpp
# End Source File
# Begin Source File

SOURCE=.\ApplicationProfile.cpp
# End Source File
# Begin Source File

SOURCE=.\CdDrive.cpp
# End Source File
# Begin Source File

SOURCE=.\ChildDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\CustomSampleRateDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\DirectFile.cpp
# End Source File
# Begin Source File

SOURCE=.\FftRulerView.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\NumEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\OperationContext.cpp
# End Source File
# Begin Source File

SOURCE=.\OperationContext2.cpp
# End Source File
# Begin Source File

SOURCE=.\OperationDialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\OperationDialogs2.cpp
# End Source File
# Begin Source File

SOURCE=.\Resample.cpp
# End Source File
# Begin Source File

SOURCE=.\Ruler.cpp
# End Source File
# Begin Source File

SOURCE=.\SaveExpressionDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ScaledScrollView.cpp
# End Source File
# Begin Source File

SOURCE=.\shellink.cpp
# End Source File
# Begin Source File

SOURCE=.\SpectrumSectionView.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TimeEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\TimeRulerView.cpp
# End Source File
# Begin Source File

SOURCE=.\WaveFftView.cpp
# End Source File
# Begin Source File

SOURCE=.\WaveFile.cpp

!IF  "$(CFG)" == "WaveSoapFront - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "WaveSoapFront - Win32 Debug"

# ADD CPP /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\WaveOutlineView.cpp
# End Source File
# Begin Source File

SOURCE=.\waveproc.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\WaveSoapFront.cpp
# End Source File
# Begin Source File

SOURCE=.\hlp\WaveSoapFront.hpj

!IF  "$(CFG)" == "WaveSoapFront - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__WAVES="hlp\AfxCore.rtf"	"hlp\AfxPrint.rtf"	"hlp\$(TargetName).hm"	
# Begin Custom Build - Making help file...
OutDir=.\Release
TargetName=WaveSoap
InputPath=.\hlp\WaveSoapFront.hpj
InputName=WaveSoapFront

"$(OutDir)\WaveSoap.hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	start /wait hcw /C /E /M "hlp\$(InputName).hpj" 
	if errorlevel 1 goto :Error 
	if not exist "hlp\WaveSoap.hlp" goto :Error 
	copy "hlp\WaveSoap.hlp" $(OutDir) 
	goto :done 
	:Error 
	echo hlp\$(InputName).hpj(1) : error: 
	type "hlp\$(InputName).log" 
	:done 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WaveSoapFront - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__WAVES="hlp\AfxCore.rtf"	"hlp\AfxPrint.rtf"	"hlp\$(TargetName).hm"	
# Begin Custom Build - Making help file...
OutDir=.\Debug
TargetName=WaveSoap
InputPath=.\hlp\WaveSoapFront.hpj
InputName=WaveSoapFront

"$(OutDir)\WaveSoap.hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	start /wait hcw /C /E /M "hlp\$(InputName).hpj" 
	if errorlevel 1 goto :Error 
	if not exist "hlp\WaveSoap.hlp" goto :Error 
	copy "hlp\WaveSoap.hlp" $(OutDir) 
	goto :done 
	:Error 
	echo hlp\$(InputName).hpj(1) : error: 
	type "hlp\$(InputName).log" 
	:done 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\WaveSoapFront.rc
# End Source File
# Begin Source File

SOURCE=.\WaveSoapFrontDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\WaveSoapFrontView.cpp
# End Source File
# Begin Source File

SOURCE=.\WaveSupport.cpp
# End Source File
# Begin Source File

SOURCE=.\WmaFile.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AmplitudeRuler.h
# End Source File
# Begin Source File

SOURCE=.\ApplicationProfile.h
# End Source File
# Begin Source File

SOURCE=.\CdDrive.h
# End Source File
# Begin Source File

SOURCE=.\ChildDialog.h
# End Source File
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\Complex.h
# End Source File
# Begin Source File

SOURCE=.\CustomSampleRateDlg.h
# End Source File
# Begin Source File

SOURCE=.\DirectFile.h
# End Source File
# Begin Source File

SOURCE=.\fft.h
# End Source File
# Begin Source File

SOURCE=.\FFT.inl
# End Source File
# Begin Source File

SOURCE=.\FftRulerView.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\NumEdit.h
# End Source File
# Begin Source File

SOURCE=.\OperationContext.h
# End Source File
# Begin Source File

SOURCE=.\OperationContext2.h
# End Source File
# Begin Source File

SOURCE=.\OperationDialogs.h
# End Source File
# Begin Source File

SOURCE=.\OperationDialogs2.h
# End Source File
# Begin Source File

SOURCE=.\Resample.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h

!IF  "$(CFG)" == "WaveSoapFront - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Making help include file...
TargetName=WaveSoap
InputPath=.\Resource.h

"hlp\$(TargetName).hm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo. >"hlp\$(TargetName).hm" 
	echo // Commands (ID_* and IDM_*) >>"hlp\$(TargetName).hm" 
	makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Prompts (IDP_*) >>"hlp\$(TargetName).hm" 
	makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Resources (IDR_*) >>"hlp\$(TargetName).hm" 
	makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Dialogs (IDD_*) >>"hlp\$(TargetName).hm" 
	makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Frame Controls (IDW_*) >>"hlp\$(TargetName).hm" 
	makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\$(TargetName).hm" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "WaveSoapFront - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Making help include file...
TargetName=WaveSoap
InputPath=.\Resource.h

"hlp\$(TargetName).hm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	echo. >"hlp\$(TargetName).hm" 
	echo // Commands (ID_* and IDM_*) >>"hlp\$(TargetName).hm" 
	makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Prompts (IDP_*) >>"hlp\$(TargetName).hm" 
	makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Resources (IDR_*) >>"hlp\$(TargetName).hm" 
	makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Dialogs (IDD_*) >>"hlp\$(TargetName).hm" 
	makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\$(TargetName).hm" 
	echo. >>"hlp\$(TargetName).hm" 
	echo // Frame Controls (IDW_*) >>"hlp\$(TargetName).hm" 
	makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\$(TargetName).hm" 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Ruler.h
# End Source File
# Begin Source File

SOURCE=.\SaveExpressionDialog.h
# End Source File
# Begin Source File

SOURCE=.\ScaledScrollView.h
# End Source File
# Begin Source File

SOURCE=.\shellink.h
# End Source File
# Begin Source File

SOURCE=.\SpectrumSectionView.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TimeEdit.h
# End Source File
# Begin Source File

SOURCE=.\TimeRulerView.h
# End Source File
# Begin Source File

SOURCE=.\WaveFftView.h
# End Source File
# Begin Source File

SOURCE=.\WaveFile.h
# End Source File
# Begin Source File

SOURCE=.\WaveOutlineView.h
# End Source File
# Begin Source File

SOURCE=.\waveproc.h
# End Source File
# Begin Source File

SOURCE=.\WaveSoapFront.h
# End Source File
# Begin Source File

SOURCE=.\WaveSoapFrontDoc.h
# End Source File
# Begin Source File

SOURCE=.\WaveSoapFrontView.h
# End Source File
# Begin Source File

SOURCE=.\WaveSupport.h
# End Source File
# Begin Source File

SOURCE=.\WmaFile.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\cur00001.cur
# End Source File
# Begin Source File

SOURCE=.\res\cur00002.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursor_b.cur
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\WaveSoap.ico
# End Source File
# Begin Source File

SOURCE=.\res\WaveSoapFront.ico
# End Source File
# Begin Source File

SOURCE=.\res\WaveSoapFront.rc2
# End Source File
# Begin Source File

SOURCE=.\res\WaveSoapFrontDoc.ico
# End Source File
# End Group
# Begin Group "Help Files"

# PROP Default_Filter "cnt;rtf"
# Begin Source File

SOURCE=.\hlp\AfxCore.rtf
# End Source File
# Begin Source File

SOURCE=.\hlp\AfxPrint.rtf
# End Source File
# Begin Source File

SOURCE=.\hlp\AppExit.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Bullet.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurArw2.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurArw4.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurHelp.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditCopy.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditCut.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditPast.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditUndo.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileNew.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileOpen.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FilePrnt.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileSave.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileSaveAll.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileSaveAs.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileSaveCopy.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HlpSBar.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HlpTBar.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecFirst.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecLast.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecNext.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecPrev.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Scmax.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\ScMenu.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Scmin.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\WaveSoapFront.cnt

!IF  "$(CFG)" == "WaveSoapFront - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Copying contents file...
OutDir=.\Release
InputPath=.\hlp\WaveSoapFront.cnt
InputName=WaveSoapFront

"$(OutDir)\$(InputName).cnt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "hlp\$(InputName).cnt" $(OutDir)

# End Custom Build

!ELSEIF  "$(CFG)" == "WaveSoapFront - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Copying contents file...
OutDir=.\Debug
InputPath=.\hlp\WaveSoapFront.cnt
InputName=WaveSoapFront

"$(OutDir)\$(InputName).cnt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "hlp\$(InputName).cnt" $(OutDir)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\aspi_w32.txt
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
