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
# ADD CPP /nologo /G6 /MT /W3 /GR /GX /Zi /O2 /I "." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0500 /D "OEMRESOURCE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib msacm32.lib wmstub.lib wmvcore.lib delayimp.lib /nologo /subsystem:windows /pdb:none /map /machine:I386 /out:"Release/WaveSoap.exe" /delayload:wmvcore.dll
# SUBTRACT LINK32 /debug

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
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /I "." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0500 /D "OEMRESOURCE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib msacm32.lib wmstub.lib wmvcore.lib delayimp.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"Debug/WaveSoap.exe" /pdbtype:sept /delayload:wmvcore.dll
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

SOURCE=.\CStringW.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\CustomSampleRateDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\DirectFile.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgFileNew.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\EqualizerDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\FftRulerView.cpp
# End Source File
# Begin Source File

SOURCE=.\FileDialogWithHistory.cpp
# End Source File
# Begin Source File

SOURCE=.\FilterDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\FolderDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\NewFilePropertiesDlg.cpp
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

SOURCE=.\PreferencesPropertySheet.cpp
# End Source File
# Begin Source File

SOURCE=.\RawFileParametersDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ReopenCompressedFileDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ReopenConvertedFileDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ReopenSavedFileCopyDlg.cpp
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
# ADD CPP /Yu
# End Source File
# Begin Source File

SOURCE=.\hlp\WaveSoap.hhp
# End Source File
# Begin Source File

SOURCE=.\WaveSoapFileDialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\WaveSoapFront.cpp
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

SOURCE=.\AFXMSG_.H
# End Source File
# Begin Source File

SOURCE=.\AmplitudeRuler.h
# End Source File
# Begin Source File

SOURCE=.\ApplicationProfile.h
# End Source File
# Begin Source File

SOURCE=.\BladeMP3EncDLL.h
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

SOURCE=.\CStringW.h
# End Source File
# Begin Source File

SOURCE=.\CustomSampleRateDlg.h
# End Source File
# Begin Source File

SOURCE=.\DirectFile.h
# End Source File
# Begin Source File

SOURCE=.\EqualizerDialog.h
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

SOURCE=.\FileDialogWithHistory.h
# End Source File
# Begin Source File

SOURCE=.\FilterDialog.h
# End Source File
# Begin Source File

SOURCE=.\FolderDialog.h
# End Source File
# Begin Source File

SOURCE=.\KListEntry.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\MfcStdAfx.h
# End Source File
# Begin Source File

SOURCE=.\NewFilePropertiesDlg.h
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

SOURCE=.\PreferencesPropertySheet.h
# End Source File
# Begin Source File

SOURCE=.\RawFileParametersDlg.h
# End Source File
# Begin Source File

SOURCE=.\ReopenCompressedFileDialog.h
# End Source File
# Begin Source File

SOURCE=.\ReopenConvertedFileDlg.h
# End Source File
# Begin Source File

SOURCE=.\ReopenSavedFileCopyDlg.h
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

SOURCE=.\SimpleCriticalSection.h
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

SOURCE=.\WaveSoapFileDialogs.h
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

SOURCE=.\res\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00002.bmp
# End Source File
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

SOURCE=.\res\id_bitma.bmp
# End Source File
# Begin Source File

SOURCE=.\res\idc_zoom.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\toolbar1.bmp
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

SOURCE=.\hlp\html\afxc0085.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc0853.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc0c4l.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc0l7p.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc0wc8.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc0xpz.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc1304.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc181c.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc1cof.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc1g4z.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc1tk7.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc1vqd.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc214i.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc2fsc.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc30hf.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc31id.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc31ym.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc33uf.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc3at6.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc3ewk.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc3zz9.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc45r9.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc48h1.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc48o5.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc49x0.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc4mn9.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc4r72.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc56lw.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc57j9.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc590x.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc5tr9.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc5zol.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc67ad.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc6t2v.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc6tv6.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc6ztx.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc71o2.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc72d3.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc72ya.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc7d4k.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc7yt5.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc85dk.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc8ble.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc8eed.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc8rhq.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc8zxr.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc9b3o.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc9ctz.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc9jci.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc9sz0.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc9u5h.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc9vp0.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc9y05.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxc9ysw.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxp0434.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxp0vjk.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxp225w.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxp3i9e.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxp42b4.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxp5hf8.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxp7q2f.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\afxp86av.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\amplitude_ruler.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\images\AppExit.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\Bullet.png
# End Source File
# Begin Source File

SOURCE=.\hlp\html\creating_a_new_sound_file.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\images\CurArw4.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\CurHelp.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\DlgResizeHandle.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\EditCopy.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\EditCut.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\EditPast.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\EditUndo.png
# End Source File
# Begin Source File

SOURCE=.\hlp\html\fft_spectrum_command.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\fft_spectrum_view.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\images\FileCloseButton.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\FileCloseIcon.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\FileNew.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\FileOpen.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\FileOpenDlg.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\FilePrnt.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\FileSave.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\HlpSBar.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\HlpTBar.png
# End Source File
# Begin Source File

SOURCE=".\hlp\html\Menu Item Template.htm"
# End Source File
# Begin Source File

SOURCE=.\hlp\images\NewFileDlg.png
# End Source File
# Begin Source File

SOURCE=.\hlp\html\opening_an_existing_sound_file.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\outline_command.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\process_menu_commands.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\images\Scmax.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\ScMenu.png
# End Source File
# Begin Source File

SOURCE=.\hlp\images\Scmin.png
# End Source File
# Begin Source File

SOURCE=.\hlp\html\spectrum_section_command.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\spectrum_section_pane.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\time_ruler.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\tools_menu_commands.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\images\ViewMenu.png
# End Source File
# Begin Source File

SOURCE=.\hlp\html\ViewWaveform.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\wave_outline_view.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\html\waveform_view.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\wavesoap.hhc
# End Source File
# Begin Source File

SOURCE=.\hlp\html\wavesoap_main_window.htm
# End Source File
# Begin Source File

SOURCE=.\hlp\WaveSoapFront.hhk
# End Source File
# Begin Source File

SOURCE=.\hlp\html\zoom_in_time_command.htm
# End Source File
# End Group
# Begin Group "FilterMath"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FilterMath.cpp
# End Source File
# Begin Source File

SOURCE=.\FilterMath.h
# End Source File
# Begin Source File

SOURCE=.\PolyMath.cpp
# End Source File
# Begin Source File

SOURCE=.\PolyMath.h
# End Source File
# Begin Source File

SOURCE=.\PolyRatio.cpp
# End Source File
# Begin Source File

SOURCE=.\PolyRatio.h
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
