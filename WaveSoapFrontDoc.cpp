// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveSoapFrontDoc.cpp : implementation of the CWaveSoapFrontDoc class
//

#include "stdafx.h"
#include "WaveSoapFrontDoc.h"
#include "MainFrm.h"
#include "OperationContext2.h"
#include "Resample.h"
#include "ChildFrm.h"
#include "SpectrumSectionView.h"
#include "OperationDialogs.h"
#include "OperationDialogs2.h"
#include <afxpriv.h>
#include "CustomSampleRateDlg.h"
#include "FileDialogWithHistory.h"
#include <Dlgs.h>
#include "ReopenCompressedFileDialog.h"
#include "ReopenConvertedFileDlg.h"
#include "EqualizerDialog.h"
#include "RawFileParametersDlg.h"
#include "FilterDialog.h"
#include "WaveSoapFileDialogs.h"
#include "PasteResampleModeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontDoc

BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
UINT AFXAPI AfxGetFileTitle(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);
UINT AFXAPI AfxGetFileName(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);
BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);

IMPLEMENT_DYNCREATE(CWaveSoapFrontDoc, CDocument)

BEGIN_MESSAGE_MAP(CWaveSoapFrontDoc, CDocument)
	//{{AFX_MSG_MAP(CWaveSoapFrontDoc)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_STOP, OnEditStop)
	ON_UPDATE_COMMAND_UI(ID_EDIT_STOP, OnUpdateEditStop)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	ON_COMMAND(ID_EDIT_SELECTION, OnEditSelection)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SELECTION, OnUpdateEditSelection)
	ON_UPDATE_COMMAND_UI(IDC_SOUND_PLAY, OnUpdateSoundPlay)
	ON_COMMAND(IDC_SOUND_PLAY, OnSoundPlay)
	ON_UPDATE_COMMAND_UI(IDC_SOUND_STOP, OnUpdateSoundStop)
	ON_COMMAND(IDC_SOUND_STOP, OnSoundStop)
	ON_COMMAND(ID_STOP_ALL, OnStopAll)
	ON_COMMAND(IDC_SOUND_PAUSE, OnSoundPause)
	ON_COMMAND(IDC_PLAY_AND_STOP, OnPlayAndStop)
	ON_UPDATE_COMMAND_UI(IDC_SOUND_PAUSE, OnUpdateSoundPause)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_SIZE, OnUpdateIndicatorFileSize)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CURRENT_POS, OnUpdateIndicatorCurrentPos)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SELECTION_LENGTH, OnUpdateIndicatorSelectionLength)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SAMPLE_RATE, OnUpdateIndicatorSampleRate)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SAMPLE_SIZE, OnUpdateIndicatorSampleSize)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CHANNELS, OnUpdateIndicatorChannels)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_CHANNELS_LOCK, OnEditChannelsLock)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CHANNELS_LOCK, OnUpdateEditChannelsLock)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_COMMAND(ID_SAMPLE_16BIT, OnSample16bit)
	ON_UPDATE_COMMAND_UI(ID_SAMPLE_16BIT, OnUpdateSample16bit)
	ON_COMMAND(ID_SAMPLE_8BIT, OnSample8bit)
	ON_UPDATE_COMMAND_UI(ID_SAMPLE_8BIT, OnUpdateSample8bit)
	ON_COMMAND(ID_SAMPLERATE_11025, OnSamplerate11025)
	ON_UPDATE_COMMAND_UI(ID_SAMPLERATE_11025, OnUpdateSamplerate11025)
	ON_COMMAND(ID_SAMPLERATE_16000, OnSamplerate16000)
	ON_UPDATE_COMMAND_UI(ID_SAMPLERATE_16000, OnUpdateSamplerate16000)
	ON_COMMAND(ID_SAMPLERATE_22050, OnSamplerate22050)
	ON_UPDATE_COMMAND_UI(ID_SAMPLERATE_22050, OnUpdateSamplerate22050)
	ON_COMMAND(ID_SAMPLERATE_32000, OnSamplerate32000)
	ON_UPDATE_COMMAND_UI(ID_SAMPLERATE_32000, OnUpdateSamplerate32000)
	ON_COMMAND(ID_SAMPLERATE_44100, OnSamplerate44100)
	ON_UPDATE_COMMAND_UI(ID_SAMPLERATE_44100, OnUpdateSamplerate44100)
	ON_COMMAND(ID_SAMPLERATE_48K, OnSamplerate48k)
	ON_UPDATE_COMMAND_UI(ID_SAMPLERATE_48K, OnUpdateSamplerate48k)
	ON_COMMAND(ID_SAMPLERATE_7200, OnSamplerate7200)
	ON_UPDATE_COMMAND_UI(ID_SAMPLERATE_7200, OnUpdateSamplerate7200)
	ON_COMMAND(ID_SAMPLERATE_8000, OnSamplerate8000)
	ON_UPDATE_COMMAND_UI(ID_SAMPLERATE_8000, OnUpdateSamplerate8000)
	ON_COMMAND(ID_SAMPLERATE_96K, OnSamplerate96k)
	ON_UPDATE_COMMAND_UI(ID_SAMPLERATE_96K, OnUpdateSamplerate96k)
	ON_COMMAND(ID_CHANNELS_MONO, OnChannelsMono)
	ON_UPDATE_COMMAND_UI(ID_CHANNELS_MONO, OnUpdateChannelsMono)
	ON_COMMAND(ID_CHANNELS_STEREO, OnChannelsStereo)
	ON_UPDATE_COMMAND_UI(ID_CHANNELS_STEREO, OnUpdateChannelsStereo)
	ON_COMMAND(ID_PROCESS_CHANGEVOLUME, OnProcessChangevolume)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_CHANGEVOLUME, OnUpdateProcessChangevolume)
	ON_COMMAND(IDC_VIEW_STATUS_HHMMSS, OnViewStatusHhmmss)
	ON_COMMAND(IDC_VIEW_STATUS_SAMPLES, OnViewStatusSamples)
	ON_COMMAND(IDC_VIEW_STATUS_SECONDS, OnViewStatusSeconds)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_DCOFFSET, OnUpdateProcessDcoffset)
	ON_COMMAND(ID_PROCESS_DCOFFSET, OnProcessDcoffset)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_INSERTSILENCE, OnUpdateProcessInsertsilence)
	ON_COMMAND(ID_PROCESS_INSERTSILENCE, OnProcessInsertsilence)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_MUTE, OnUpdateProcessMute)
	ON_COMMAND(ID_PROCESS_MUTE, OnProcessMute)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_NORMALIZE, OnUpdateProcessNormalize)
	ON_COMMAND(ID_PROCESS_NORMALIZE, OnProcessNormalize)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_RESAMPLE, OnUpdateProcessResample)
	ON_COMMAND(ID_PROCESS_RESAMPLE, OnProcessResample)
	ON_COMMAND(ID_FILE_STATISTICS, OnFileStatistics)
	ON_UPDATE_COMMAND_UI(ID_FILE_STATISTICS, OnUpdateFileStatistics)
	ON_UPDATE_COMMAND_UI(ID_EDIT_GOTO, OnUpdateEditGoto)
	ON_COMMAND(ID_EDIT_GOTO, OnEditGoto)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_INVERT, OnUpdateProcessInvert)
	ON_COMMAND(ID_PROCESS_INVERT, OnProcessInvert)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RESCAN_PEAKS, OnUpdateViewRescanPeaks)
	ON_COMMAND(ID_VIEW_RESCAN_PEAKS, OnViewRescanPeaks)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_SYNTHESIS_EXPRESSION_EVALUATION, OnUpdateProcessSynthesisExpressionEvaluation)
	ON_COMMAND(ID_PROCESS_SYNTHESIS_EXPRESSION_EVALUATION, OnProcessSynthesisExpressionEvaluation)
	ON_UPDATE_COMMAND_UI(IDC_VIEW_STATUS_HHMMSS, OnUpdateViewStatusHhmmss)
	ON_UPDATE_COMMAND_UI(IDC_VIEW_STATUS_SAMPLES, OnUpdateViewStatusSamples)
	ON_UPDATE_COMMAND_UI(IDC_VIEW_STATUS_SECONDS, OnUpdateViewStatusSeconds)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_COPY_AS, OnUpdateFileSaveCopyAs)
	ON_COMMAND(ID_FILE_SAVE_COPY_AS, OnFileSaveCopyAs)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_INTERPOLATE, OnUpdateToolsInterpolate)
	ON_COMMAND(ID_TOOLS_INTERPOLATE, OnToolsInterpolate)
	ON_UPDATE_COMMAND_UI(IDC_PROCESS_DO_ULF, OnUpdateProcessDoUlf)
	ON_COMMAND(IDC_PROCESS_DO_ULF, OnProcessDoUlf)
	ON_UPDATE_COMMAND_UI(IDC_PROCESS_DO_DECLICKING, OnUpdateProcessDoDeclicking)
	ON_COMMAND(IDC_PROCESS_DO_DECLICKING, OnProcessDoDeclicking)
	ON_UPDATE_COMMAND_UI(IDC_PROCESS_NOISE_REDUCTION, OnUpdateProcessNoiseReduction)
	ON_COMMAND(IDC_PROCESS_NOISE_REDUCTION, OnProcessNoiseReduction)
	ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
	ON_COMMAND(ID_PROCESS_CHANNELS, OnProcessChannels)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_CHANNELS, OnUpdateProcessChannels)
	ON_COMMAND(ID_SAMPLERATE_CUSTOM, OnSamplerateCustom)
	ON_UPDATE_COMMAND_UI(ID_SAMPLERATE_CUSTOM, OnUpdateSamplerateCustom)
	ON_COMMAND(ID_EDIT_ENABLE_UNDO, OnEditEnableUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ENABLE_UNDO, OnUpdateEditEnableUndo)
	ON_COMMAND(ID_EDIT_ENABLE_REDO, OnEditEnableRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ENABLE_REDO, OnUpdateEditEnableRedo)
	ON_COMMAND(ID_EDIT_CLEAR_UNDO, OnEditClearUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR_UNDO, OnUpdateEditClearUndo)
	ON_COMMAND(ID_EDIT_CLEAR_REDO, OnEditClearRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR_REDO, OnUpdateEditClearRedo)
	ON_COMMAND(ID_PROCESS_EQUALIZER, OnProcessEqualizer)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_EQUALIZER, OnUpdateProcessEqualizer)
	ON_COMMAND(ID_PROCESS_SWAPCHANNELS, OnProcessSwapchannels)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_SWAPCHANNELS, OnUpdateProcessSwapchannels)
	ON_COMMAND(ID_PROCESS_FILTER, OnProcessFilter)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_FILTER, OnUpdateProcessFilter)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontDoc construction/destruction

CWaveSoapFrontDoc::CWaveSoapFrontDoc()
	:m_CaretPosition(0),
	m_TimeSelectionMode(TRUE),
	m_SelectionStart(0),
	m_SelectionEnd(0),
	m_hThreadEvent(NULL),
	m_bRunThread(false),
	m_bReadOnly(true),
	m_bClosing(false),
	m_bClosePending(false),
	m_bCloseThisDocumentNow(false),
	m_bDirectMode(false),
	m_bInOnIdle(false),
	m_bChannelsLocked(true),
	m_OperationInProgress(0),
	m_PlayingSound(false),
	m_OperationNonCritical(false),
	m_StopOperation(false),
	m_Thread(ThreadProc, this),
	m_ModificationSequenceNumber(0),
	m_PrevChannelToCopy(ALL_CHANNELS),
	m_DefaultPasteMode(0),
	m_PasteResampleMode(0),
	m_FileTypeFlags(0),
	m_SelectedChannel(ALL_CHANNELS)
{
	m_bUndoEnabled = (FALSE != GetApp()->m_bUndoEnabled);
	m_bRedoEnabled = (FALSE != GetApp()->m_bRedoEnabled);

	CThisApp * pApp = GetApp();
	m_DefaultPasteMode = pApp->m_DefaultPasteMode;
	m_PasteResampleMode = pApp->m_PasteResampleMode;

	m_Thread.m_bAutoDelete = FALSE;
	m_hThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	TRACE("CWaveSoapFrontDoc::CWaveSoapFrontDoc()\n");
	m_bRunThread = TRUE;
	m_Thread.CreateThread(0, 0x10000);

}

CWaveSoapFrontDoc::~CWaveSoapFrontDoc()
{
	TRACE("CWaveSoapFrontDoc::~CWaveSoapFrontDoc()\n");
	ASSERT(0 == m_OperationInProgress);
	// stop the thread
	if (NULL != m_Thread.m_hThread)
	{
		m_bRunThread = FALSE;
#ifdef _DEBUG
		DWORD Time = timeGetTime();
#endif
		SetEvent(m_hThreadEvent);
		if (WAIT_TIMEOUT == WaitForSingleObjectAcceptSends(m_Thread.m_hThread, 20000))
		{
			TerminateThread(m_Thread.m_hThread, -1);
		}
#ifdef _DEBUG
		TRACE("Doc Thread finished in %d ms\n",
			timeGetTime() - Time);
#endif
	}
	if (m_hThreadEvent != NULL)
	{
		CloseHandle(m_hThreadEvent);
		m_hThreadEvent = NULL;
	}

	// free RetiredList and UpdateList
	while ( ! m_RetiredList.IsEmpty())
	{
		delete m_RetiredList.RemoveHeadUnsafe();
	}
	while ( ! m_UpdateList.IsEmpty())
	{
		delete m_UpdateList.RemoveHeadUnsafe();
	}

	DeleteUndo();
	DeleteRedo();

	CWaveSoapFrontDoc * pTmpDoc = NULL;
	CString str;
	CThisApp * pApp = GetApp();
	pApp->GetStatusStringAndDoc(str, & pTmpDoc);
	if (this == pTmpDoc)
	{
		pApp->SetStatusStringAndDoc(_T(""), NULL);
	}

	pApp->m_DefaultPasteMode = m_DefaultPasteMode;
	pApp->m_PasteResampleMode = m_PasteResampleMode;
}

NUMBER_OF_SAMPLES CWaveSoapFrontDoc::WaveFileSamples() const
{
	return m_WavFile.NumberOfSamples();
}
LPMMCKINFO CWaveSoapFrontDoc::WaveDataChunk() const
{
	return m_WavFile.GetDataChunk();
}
LPWAVEFORMATEX CWaveSoapFrontDoc::WaveFormat() const
{
	return m_WavFile.GetWaveFormat();
}
NUMBER_OF_CHANNELS CWaveSoapFrontDoc::WaveChannels() const
{
	return m_WavFile.Channels();
}
int CWaveSoapFrontDoc::WaveSampleRate() const
{
	return m_WavFile.SampleRate();
}

int CWaveSoapFrontDoc::WaveSampleSize() const
{
	return m_WavFile.SampleSize();
}
ULONG_PTR CWaveSoapFrontDoc::WaveFileID() const
{
	return m_WavFile.GetFileID();
}

BOOL CWaveSoapFrontDoc::OnNewDocument(NewFileParameters * pParams)
{
	if ( ! BaseClass::OnNewDocument())
		return FALSE;

	TRACE("CWaveSoapFrontDoc::OnNewDocument\n");
	// (SDI documents will reuse this document)
	WAVEFORMATEX * pWfx = pParams->pWf;
	m_CaretPosition = 0;
	m_SelectionStart = 0;
	m_SelectionEnd = 0;
	m_SelectedChannel = ALL_CHANNELS;

	m_FileTypeFlags = pParams->m_FileTypeFlags & OpenDocumentNonWavFile;

	m_TimeSelectionMode = TRUE;
	m_bReadOnly = false;

	NUMBER_OF_SAMPLES nSamples = pParams->m_InitialSamples;

	if (pParams->m_FileTypeFlags & OpenDocumentCreateNewFromCWaveFile)
	{
		m_WavFile = * pParams->m_pFile;
	}
	else
	{
		if (NULL == pWfx)
		{
			pWfx = & GetApp()->m_NewFileFormat;
		}

		ULONG flags = CreateWaveFileTempDir
					| CreateWaveFileDeleteAfterClose
					| CreateWaveFilePcmFormat
					| CreateWaveFileTemp;

		LPCTSTR FileName = NULL;
		// For PCM files, create new temporary file in the target directory
		if (NULL != pParams->m_pInitialName
			&& WAVE_FORMAT_PCM == pWfx->wFormatTag)
		{
			flags = CreateWaveFileDeleteAfterClose
					| CreateWaveFilePcmFormat
					| CreateWaveFileTemp;
			FileName = pParams->m_pInitialName;
		}

		if ( ! CanAllocateWaveFileSamplesDlg(pWfx, nSamples))
		{
			nSamples = 0;
		}

		if (FALSE == m_WavFile.CreateWaveFile(NULL, pWfx, ALL_CHANNELS,
											nSamples, flags, FileName))
		{
			AfxMessageBox(IDS_UNABLE_TO_CREATE_NEW_FILE, MB_OK | MB_ICONEXCLAMATION);
			return FALSE;
		}
	}

	m_OriginalWaveFormat = pWfx;
	if (m_WavFile.AllocatePeakData(nSamples))
	{
		// zero wave peak data
		m_WavFile.SetPeaks(0, nSamples * WaveChannels(),
							1, WavePeak(0, 0));
	}
	return TRUE;
}

void CWaveSoapFrontDoc::BuildPeakInfo(BOOL bSavePeakFile)
{
	// read the DATA chunk of wavefile
	if (0 == WaveFileSamples())
	{
		m_WavFile.AllocatePeakData(0);
		return;
	}

	CScanPeaksContext * pContext =
		new CScanPeaksContext(this, m_WavFile, m_OriginalWavFile, bSavePeakFile);

	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	pContext->Execute();
}


BOOL CWaveSoapFrontDoc::OpenWaveFile(CWaveFile & WaveFile, LPCTSTR szName, DWORD flags)
{
	CString s;
	UINT format = 0;
	if (WaveFile.Open(szName, flags))
	{
		if(WaveFile.LoadWaveformat())
		{
			if (WaveFile.FindData())
			{
				WaveFile.LoadMetadata();
				return TRUE;
			}
			else
			{
				format = IDS_UNABLE_TO_FIND_DATA_CHUNK;
			}
		}
		else
		{
			format = IDS_UNABLE_TO_LOAD_WAVEFORMAT;
		}
	}
	else
	{
		switch(GetLastError())
		{
		case ERROR_ACCESS_DENIED:
			format = IDS_FILE_OPEN_ACCESS_DENIED;
			break;
		case ERROR_SHARING_VIOLATION:
			format = IDS_FILE_OPEN_SHARING_VIOLATION;
			break;
		default:
			format = IDS_UNABLE_TO_OPEN_WAVE_FILE;
			break;
		}
	}
	s.Format(format, szName);
	AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontDoc serialization

void CWaveSoapFrontDoc::Serialize(CArchive& ar)
{
	TRACE("CWaveSoapFrontDoc::Serialize\n");
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

void _AfxAppendFilterSuffix(CString& filter, OPENFILENAME& ofn,
							CDocTemplate* pTemplate, CString* pstrDefaultExt);

// Save the document data to a file
// lpszPathName = path name where to save document file
// if lpszPathName is NULL then the user will be prompted (SaveAs)
// note: lpszPathName can be different than 'm_strPathName'
// if 'bReplace' is TRUE will change file name if successful (SaveAs)
// if 'bReplace' is FALSE will not change path name (SaveCopyAs)
BOOL CWaveSoapFrontDoc::DoSave(LPCTSTR lpszPathName, BOOL bReplace)
{
	if (m_bClosePending)
	{
		return false;   // don't continue
	}
	CThisApp * pApp = GetApp();

	CWaveFormat Wf;

	if (NULL == (WAVEFORMATEX*)m_OriginalWaveFormat)
	{
		Wf = WaveFormat();
	}
	else
	{
		Wf = m_OriginalWaveFormat;
		// sample rate and number of channels might change from the original file
		// new format may not be quite valid for some convertors!!
		Wf.SampleRate() = WaveSampleRate();
		Wf.NumChannels() = WaveChannels();
	}


	CString newName = lpszPathName;
	int SaveFlags = m_FileTypeFlags;
	if ( ! bReplace)
	{
		SaveFlags |= SaveFile_SaveCopy;
	}

	if (newName.IsEmpty())
	{
		CDocTemplate* pTemplate = GetDocTemplate();
		ASSERT(pTemplate != NULL);

		newName = m_strPathName;
		if (bReplace && newName.IsEmpty())
		{
			newName = m_strTitle;
			// check for dubious filename
			// AGr: changed from the original CDocument::
			// space and # are allowed characters, and '?*":' are illegal
			// in the file title
			int iBad = newName.FindOneOf(_T("?*%:;/\\\""));
			if (iBad != -1)
				newName.ReleaseBuffer(iBad);

			// append the default suffix if there is one
			CString strExt;
			if (pTemplate->GetDocString(strExt, CDocTemplate::filterExt) &&
				! strExt.IsEmpty())
			{
				ASSERT(strExt[0] == '.');
				newName += strExt;
			}
		}
		else
		{

			TCHAR Buf[MAX_PATH * 2] = {0};
			LPTSTR FilePart = Buf;
			GetFullPathName(newName, MAX_PATH * 2, Buf, & FilePart);
			CString path(Buf, FilePart - Buf);
			CString name(FilePart);

			CString Ext;
			int idx = name.ReverseFind('.');
			if (idx != -1
				&& idx >= name.GetLength() - 4)
			{
				Ext = LPCTSTR(name) + idx;
				name = name.Left(idx);
			}

			if (SaveFlags & SaveFile_SaveCopy)
			{
				if (name.IsEmpty())
				{
					name = GetTitle();
				}
				// add Copy Of
				// Get filename
				CString CopyOf;
				CopyOf.LoadString(IDS_COPY_OF);
				if (0 != _tcsnicmp(name, CopyOf, CopyOf.GetLength()))
				{
					name = CopyOf + name;
				}
				else
				{
					name += _T("(1)");
				}
			}

			// If there is no extension, add wav
			if (Ext.IsEmpty())
			{
				name += _T(".wav");
			}
			else
			{
				name += Ext;
			}

			newName = path + name;
		}


		CWaveSoapFileSaveDialog dlg(FALSE,
									Wf, this,
									_T("wav"),
									newName,
									OFN_HIDEREADONLY
									| OFN_PATHMUSTEXIST
									| OFN_EXPLORER
									| OFN_ENABLESIZING
									| OFN_ENABLETEMPLATE
									//| OFN_SHAREAWARE
									| OFN_OVERWRITEPROMPT
									);

		CString DlgTitle;
		if (SaveFlags & SaveFile_SaveCopy)
		{
			DlgTitle.LoadString(IDS_SAVE_COPY_AS_TITLE);
			dlg.m_ofn.lpstrTitle = DlgTitle;
		}

		CString strFilter;
		CString strDefault;
		// do for all doc template
		POSITION pos = pApp->m_pDocManager->GetFirstDocTemplatePosition();
		BOOL bFirst = TRUE;

		int nFilterIndex = 1;
		ULONG TemplateFlags[10] = {0};
		while (pos != NULL)
		{
			CDocTemplate* pTemplate = pApp->m_pDocManager->GetNextDocTemplate(pos);
			if (pTemplate == GetDocTemplate())
			{
				dlg.m_ofn.nFilterIndex = nFilterIndex;
			}
#if 0
			CWaveSoapDocTemplate * pWaveTemplate =
				dynamic_cast<CWaveSoapDocTemplate *>(pTemplate);
			if (NULL != pWaveTemplate)
			{
				TemplateFlags[nFilterIndex] = pWaveTemplate->m_OpenDocumentFlags;
			}
			else
#endif
			{
				TemplateFlags[nFilterIndex] = 0;
			}
			_AfxAppendFilterSuffix(strFilter, dlg.m_ofn, pTemplate,
									&strDefault);
			nFilterIndex++;
			bFirst = FALSE;
		}

		// append the "*.*" all files filter
		CString allFilter;
		VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));
		strFilter += allFilter;
		strFilter += (TCHAR)'\0';   // next string please
		strFilter += _T("*.*");
		strFilter += (TCHAR)'\0';   // last string
		dlg.m_ofn.nMaxCustFilter++;
		dlg.m_ofn.lpstrFilter = strFilter;

		// add the proper file type extension
		// todo: get from saved in the registry, if name is empty
		dlg.m_ofn.nFilterIndex = dlg.GetFileTypeForName(newName);

		if (IDOK != dlg.DoModal())
		{
			return FALSE;       // don't even attempt to save
		}
		newName = dlg.GetPathName();
		// convert to long name
		TCHAR LongPath[512];
		if (GetLongPathName(newName, LongPath, 512))
		{
			newName = LongPath;
		}

		Wf = dlg.GetWaveFormat();

		SaveFlags &= ~SaveFile_NonWavFile;

		switch (dlg.m_FileType)
		{
		case SoundFileWav:
			break;
		case SoundFileMp3:
			SaveFlags |= SaveFile_Mp3File;
			break;
		case SoundFileWma:
			SaveFlags |= SaveFile_WmaFile;
			break;
		case SoundFileRaw:
			SaveFlags |= SaveFile_RawFile;
			switch (dlg.m_SelectedRawFormat)
			{
			case RawSoundFilePcm16Msb:
				SaveFlags |= SaveRawFileMsbFirst;
				// fall through
			default:
			case RawSoundFilePcm16Lsb:
				Wf.FormatTag() = WAVE_FORMAT_PCM;
				Wf.BitsPerSample() = 16;
				break;
			case RawSoundFilePcm8:
				Wf.FormatTag() = WAVE_FORMAT_PCM;
				Wf.BitsPerSample() = 8;
				break;
			case RawSoundFileALaw8:
				Wf.FormatTag() = WAVE_FORMAT_ALAW;
				Wf.BitsPerSample() = 8;
				break;
			case RawSoundFileULaw8:
				Wf.FormatTag() = WAVE_FORMAT_MULAW;
				Wf.BitsPerSample() = 8;
				break;
			}
			Wf.SampleSize() = (Wf.BitsPerSample() * Wf.NumChannels()) / 8;
			Wf.BytesPerSec() = Wf.SampleSize() * Wf.SampleRate();
			break;
		}
	}

	CWaitCursor wait;
	BOOL Result = OnSaveDocument(newName, SaveFlags, Wf);

	if ( ! Result)
	{
		return FALSE;
	}

	// reset the title and change the document name
	if (SaveFlags & SaveFile_SaveCopy)
	{
		GetApp()->AddToRecentFileList(CString(TCHAR(OpenDocumentDefaultMode | 1), 1) + newName);
	}
	else
	{
		SetPathName(newName);
	}

	return TRUE;        // success
}

void CWaveSoapFrontDoc::SetSelection(SAMPLE_INDEX begin, SAMPLE_INDEX end,
									CHANNEL_MASK channel, SAMPLE_INDEX caret, int flags)
{
	CSelectionUpdateInfo ui;
	ui.OldSelBegin = m_SelectionStart;
	ui.OldSelEnd = m_SelectionEnd;
	ui.OldSelChannel = m_SelectedChannel;
	ui.OldCaretPos = m_CaretPosition;

	NUMBER_OF_SAMPLES length = WaveFileSamples();

	if (begin < 0) begin = 0;
	if (begin > length) begin = length;
	if (end < 0) end = 0;
	if (end > length) end = length;
	if (caret < 0) caret = 0;
	if (caret > length) caret = length;
	//if (channel < 0 || channel > 1) channel = ALL_CHANNELS;

	if (flags & SetSelection_SnapToMaximum)
	{
		// read from 'begin' to 'end', find the max
		NUMBER_OF_SAMPLES SamplesToRead;
		SAMPLE_INDEX BeginSample;

		if (begin <= end)
		{
			BeginSample = begin;
			SamplesToRead = end - begin;
		}
		else
		{
			BeginSample = end;
			SamplesToRead = begin - end;
		}
		if (SamplesToRead > 0x40000)
		{
			// limit to 1 MB
			SamplesToRead = 0x40000;
		}

		SAMPLE_INDEX pos = BeginSample;

		int nChannels = m_WavFile.Channels();
		ASSERT(nChannels <= 2);

		int nReadChannels = nChannels;

		if ( ! m_WavFile.AllChannels(channel))
		{
			nReadChannels = 1;
		}

		long Max = 0;

		while (SamplesToRead > 0)
		{
			WAVE_SAMPLE data[256];
			unsigned nWordsToRead = SamplesToRead * nReadChannels;
			if (nWordsToRead > countof (data))
			{
				nWordsToRead = countof (data);
			}

			long SamplesRead = m_WavFile.ReadSamples(channel,
													m_WavFile.SampleToPosition(BeginSample),
													nWordsToRead / nReadChannels, data);

			for (unsigned i = 0; i < nWordsToRead; i++)
			{
				long tmp = data[i];
				if (tmp < 0)
				{
					tmp = -tmp;
				}

				if (tmp > Max)
				{
					Max = tmp;
					pos = BeginSample + i / nReadChannels;
				}
			}
			SamplesToRead -= SamplesRead;
			BeginSample += SamplesRead;
		}
		begin = pos;
		end = pos;
		caret = pos;
	}

	if (begin <= end)
	{
		m_SelectionStart = begin;
		m_SelectionEnd = end;
	}
	else
	{
		m_SelectionStart = end;
		m_SelectionEnd = begin;
	}

	m_SelectedChannel = channel;
	m_CaretPosition = caret;

	ui.Flags = flags;
	ui.SelBegin = m_SelectionStart;
	ui.SelEnd = m_SelectionEnd;
	ui.SelChannel = m_SelectedChannel;
	ui.CaretPos = m_CaretPosition;

	UpdateAllViews(NULL, UpdateSelectionChanged, & ui);
}

void CWaveSoapFrontDoc::QueueSoundUpdate(int UpdateCode, ULONG_PTR FileID,
										SAMPLE_INDEX begin, SAMPLE_INDEX end, NUMBER_OF_SAMPLES NewLength, int flags)
{
	CSoundUpdateInfo * pui = new CSoundUpdateInfo(UpdateCode,
												FileID, begin, end, NewLength);

	{
		CSimpleCriticalSectionLock lock(m_UpdateList);

		if ((flags & QueueSoundUpdateMerge)
			&& -1 == NewLength)
		{
			for (CSoundUpdateInfo * pEntry = m_UpdateList.Last()
				;  m_UpdateList.NotEnd(pEntry); pEntry = m_UpdateList.Prev(pEntry))
			{
				// see if the ranges overlap
				if (FileID == pEntry->m_FileID
					&& pEntry->m_UpdateCode == UpdateCode
					//&& pEntry->m_NewLength == NewLength
					&& begin <= pEntry->m_End
					&& end >= pEntry->m_Begin)
				{
					if (pEntry->m_NewLength != -1)
					{
						break;
					}
					if (begin < pEntry->m_Begin)
					{
						pEntry->m_Begin = begin;
					}
					if (pEntry->m_End < end)
					{
						pEntry->m_End = end;
					}
					delete pui;
					pui = NULL;
					break;
				}
			}
		}
		else if (flags & QueueSoundUpdateReplace)
		{
			for (CSoundUpdateInfo * pEntry = m_UpdateList.First()
				;  m_UpdateList.NotEnd(pEntry); )
			{
				CSoundUpdateInfo * next = m_UpdateList.Next(pEntry);
				if (FileID == pEntry->m_FileID
					&& pEntry->m_UpdateCode == UpdateCode)
				{
					m_UpdateList.RemoveEntryUnsafe(pEntry);
					delete pEntry;
				}
				pEntry = next;
			}
		}
		if (NULL != pui)
		{
			m_UpdateList.InsertTailUnsafe(pui);
		}
	}
	::PostMessage(GetApp()->m_pMainWnd->m_hWnd, WM_KICKIDLE, 0, 0);
}

void CWaveSoapFrontDoc::FileChanged(CWaveFile & File, SAMPLE_POSITION begin,
									SAMPLE_POSITION end, NUMBER_OF_SAMPLES length, DWORD flags)
{
	if (File.GetFileID() != WaveFileID())
	{
		// changed a different (some temporary) file
		return;
	}
	SoundChanged(m_WavFile.GetFileID(),
				m_WavFile.PositionToSample(begin),
				m_WavFile.PositionToSample(end), length, flags);
}

void CWaveSoapFrontDoc::SoundChanged(DWORD FileID, SAMPLE_INDEX begin, SAMPLE_INDEX end,
									NUMBER_OF_SAMPLES FileLength, DWORD flags)
{
	// notify all views that the sound appearance changed
	if (FileID != WaveFileID())
	{
		// changed a different (some temporary) file
		return;
	}
	// update peak data
	if ((FileLength == -1
			|| m_WavFile.AllocatePeakData(FileLength))
		&& 0 == (flags & UpdateSoundDontRescanPeaks)
		&& FileID != NULL
		&& begin != end)
	{
		m_WavFile.RescanPeaks(begin, end);
	}

	int UpdateFlag = QueueSoundUpdateMerge;
	if (flags & UpdateSoundSamplingRateChanged)
	{
		QueueSoundUpdate(UpdateSampleRateChanged, FileID, 0, 0, -1);
		UpdateFlag = 0;
	}

	QueueSoundUpdate(UpdateSoundChanged, FileID, begin, end,
					FileLength, UpdateFlag);
}


/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontDoc diagnostics

#ifdef _DEBUG
void CWaveSoapFrontDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWaveSoapFrontDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontDoc commands

void CWaveSoapFrontDoc::OnEditCopy()
{
	if (CanReadSelection())
	{
		DoCopy(m_SelectionStart, m_SelectionEnd, m_SelectedChannel, NULL);
	}
}

void CWaveSoapFrontDoc::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanReadSelection());
}

void CWaveSoapFrontDoc::OnEditCut()
{
	if (CanModifySelection())
	{
		DoCut(m_SelectionStart, m_SelectionEnd, GetSelectedChannel());
	}
}

void CWaveSoapFrontDoc::OnUpdateEditCut(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanModifySelection());
}

void CWaveSoapFrontDoc::OnEditDelete()
{
	if (CanModifyFile()
		&& m_SelectionStart < WaveFileSamples())
	{
		DoDelete(m_SelectionStart, m_SelectionEnd, GetSelectedChannel());
	}
}

void CWaveSoapFrontDoc::OnUpdateEditDelete(CCmdUI* pCmdUI)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	pCmdUI->Enable(CanModifyFile()
					&& m_SelectionStart < WaveFileSamples());
}

void CWaveSoapFrontDoc::OnEditPaste()
{
	if (CanWriteFile())
	{
		DoPaste(m_SelectionStart, m_SelectionEnd, GetSelectedChannel(), NULL);
	}
}

void CWaveSoapFrontDoc::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetApp()->m_ClipboardFile.IsOpen()
					&& CanWriteFile());
}

void CWaveSoapFrontDoc::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_UndoList.IsEmpty() && CanWriteFile());
}

UINT CWaveSoapFrontDoc::_ThreadProc(void)
{
#ifdef _DEBUG
	FILETIME UserTime, EndTime, tmp;
	GetThreadTimes(GetCurrentThread(), & tmp, & tmp, & tmp, & UserTime);
#endif
	::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	bool NeedKickIdle = false;
	while (m_bRunThread)
	{
		if (NeedKickIdle)
		{
			CWinApp * pApp = GetApp();
			if (pApp->m_pMainWnd)
			{
				((CMainFrame *)pApp->m_pMainWnd)->ResetLastStatusMessage();
				if(::PostMessage(pApp->m_pMainWnd->m_hWnd, WM_KICKIDLE, 0, 0))
				{
					NeedKickIdle = false;    // otherwise keep bugging
				}
			}
		}

		m_OpList.Lock();
		if ( ! m_OpList.IsEmpty())
		{
			COperationContext * pContext = m_OpList.First();
			m_OpList.Unlock();
			// TODO: set the status message before calling Init
			if (pContext->m_Flags & OperationContextSynchronous)
			{
				pContext->ExecuteSynch();
				pContext->m_Flags |= OperationContextFinished;
			}
			else if (0 == (pContext->m_Flags & OperationContextInitialized))
			{
				if ( ! pContext->Init())
				{
					pContext->m_Flags |= OperationContextInitFailed | OperationContextStop;
				}
				SetCurrentStatusString(pContext->GetStatusString());
				pContext->m_Flags |= OperationContextInitialized;
				NeedKickIdle = true;
			}

			if (m_StopOperation)
			{
				pContext->m_Flags |= OperationContextStopRequested;
			}

			int LastPercent = pContext->m_PercentCompleted;
			// execute one step
			if (0 == (pContext->m_Flags &
					(OperationContextStop | OperationContextFinished)))
			{
				if (! pContext->OperationProc())
				{
					pContext->m_Flags |= OperationContextStop;
				}
				// signal for status update
				if (LastPercent != pContext->m_PercentCompleted)
				{
					NeedKickIdle = true;
				}
				if (NeedKickIdle)
				{
					if (pContext->m_PercentCompleted >= 0)
					{
						CString s;
						s.Format(_T("%s%d%%"),
								(LPCTSTR)pContext->GetStatusString(),
								pContext->m_PercentCompleted);
						SetCurrentStatusString(s);
					}
					else
					{
						SetCurrentStatusString(pContext->GetStatusString());
					}
				}
			}
			if (pContext->m_Flags &
				(OperationContextStop | OperationContextFinished))
			{
				if (pContext->m_PercentCompleted >= 0)
				{
					SetCurrentStatusString(pContext->GetStatusString() + _T("Completed"));
				}
				NeedKickIdle = true;

				m_OpList.RemoveEntry(pContext);

				pContext->DeInit();
				TRACE("Retire context %X\n", pContext);
				pContext->Retire();     // puts it in the document queue
			}
			continue;
		}
		else
		{
			m_OpList.Unlock();
		}
		WaitForSingleObject(m_hThreadEvent, INFINITE);
	}
#ifdef _DEBUG
	GetThreadTimes(GetCurrentThread(), & tmp, & tmp, & tmp, & EndTime);
	TRACE("Document thread used time=%d ms\n",
		(EndTime.dwLowDateTime - UserTime.dwLowDateTime) / 10000);
#endif
	return 0;
}

void CWaveSoapFrontDoc::OnEditUndo()
{
	if (IsBusy()
		|| m_UndoList.IsEmpty())
	{
		return;
	}
	// no critical section lock needed
	COperationContext * pUndo = m_UndoList.RemoveHead();

	if ( ! pUndo->PrepareUndo())
	{
		// return head back
		pUndo->UnprepareUndo();
		m_UndoList.InsertHead(pUndo);
		return;
	}

	if (RedoEnabled())
	{
		if ( ! pUndo->CreateUndo(FALSE))
		{
			// return head back
			pUndo->DeleteUndo();
			pUndo->UnprepareUndo();
			m_UndoList.InsertHead(pUndo);
			return;
		}
	}
	else
	{
		DeleteRedo();  // cannot redo anymore
	}

	DecrementModified();

	pUndo->Execute();
}

void CWaveSoapFrontDoc::OnEditRedo()
{
	if (! CanWriteFile()
		|| m_RedoList.IsEmpty())
	{
		return;
	}
	// no critical section lock needed
	COperationContext * pRedo = m_RedoList.RemoveHead();

	if ( ! pRedo->PrepareUndo())
	{
		// return head back
		pRedo->UnprepareUndo();
		m_RedoList.InsertHead(pRedo);
		return;
	}

	if (UndoEnabled())
	{
		if ( ! pRedo->CreateUndo(TRUE))
		{
			pRedo->DeleteUndo();
			pRedo->UnprepareUndo();
			m_RedoList.InsertHead(pRedo);
			return;
		}
	}
	else
	{
		DeleteUndo();   // cannot undo anymore
	}

	IncrementModified(FALSE);   // don't delete redo

	pRedo->Execute();
}

void CWaveSoapFrontDoc::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_RedoList.IsEmpty() && CanWriteFile());
}

void CWaveSoapFrontDoc::QueueOperation(COperationContext * pContext)
{
	m_StopOperation = false;
	m_OperationNonCritical = (0 != (pContext->m_Flags & OperationContextNonCritical));

	++m_OperationInProgress;

	if (pContext->m_Flags & (OperationContextDiskIntensive | OperationContextClipboard))
	{
		GetApp()->QueueOperation(pContext);
	}
	// the operation is performed by the document thread
	else
	{
		m_OpList.InsertTail(pContext);
		SetEvent(m_hThreadEvent);
	}
}

void CWaveSoapFrontDoc::DoCopy(SAMPLE_INDEX Start, SAMPLE_INDEX End,
								CHANNEL_MASK Channel, LPCTSTR FileName)
{
	// create a operation context
	ASSERT(Start < End);

	TRACE("CWaveSoapFrontDoc::DoCopy Start=%d, End=%d, Channel=%x, FileName=%s\n",
		Start, End, Channel, FileName);

	CString OpName;
	DWORD OpenFlags;
	DWORD OperationFlags;
	CWaveFile DstFile;
	if (NULL != FileName && FileName[0] != 0)
	{
		OpName.Format(_T("Copying the selection to \"%s\"..."), FileName);
		OpenFlags = 0;  // default options
		OperationFlags = 0; // default
	}
	else
	{
		OpName = _T("Copying data to clipboard...");
		OpenFlags = CreateWaveFileTempDir
					| CreateWaveFileDeleteAfterClose
					| CreateWaveFileAllowMemoryFile
					| CreateWaveFileDontCopyInfo
					| CreateWaveFilePcmFormat
					| CreateWaveFileTemp;
		OperationFlags = OperationContextClipboard;
	}

	CCopyContext::auto_ptr pContext(new CCopyContext(this, OpName, _T("Copy")));

	// create a temporary clipboard WAV file
	if (FALSE == DstFile.CreateWaveFile( & m_WavFile, NULL, Channel,
										End - Start,
										OpenFlags,
										FileName))
	{
		AfxMessageBox(IDS_STRING_UNABLE_TO_CREATE_CLIPBOARD, MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	pContext->m_Flags |= OperationFlags;

	pContext->InitCopy(DstFile, 0, ALL_CHANNELS,
						m_WavFile, Start, Channel, End - Start);

	if (OperationFlags & OperationContextClipboard)
	{
		GetApp()->m_ClipboardFile = DstFile;
	}
	// set operation context to the queue
	pContext.release()->Execute();
}

BOOL CWaveSoapFrontDoc::DoPaste(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channel, LPCTSTR FileName)
{
	TRACE("DoPaste Start=%d, End=%d, Channel=%x, FileName=%s\n",
		Start, End, Channel, FileName);
	// create a operation context
	CThisApp * pApp = GetApp();

	// todo: support copy from a file (FileName)
	CWaveFile SrcFile;

	CStagedContext::auto_ptr pStagedContext(new CStagedContext(this, _T("Paste"), 0));

	if (NULL != FileName)
	{
	}
	else
	{
		SrcFile = pApp->m_ClipboardFile;
		pStagedContext->m_Flags |= OperationContextClipboard;
	}

	if ( ! SrcFile.IsOpen())
	{
		return FALSE;
	}

	NUMBER_OF_SAMPLES NumSamplesToPasteFrom = SrcFile.NumberOfSamples();
	// check if the sampling rate matches the clipboard
	int SrcSampleRate = SrcFile.SampleRate();
	int TargetSampleRate = WaveSampleRate();

	if (SrcSampleRate != TargetSampleRate)
	{
		// sample rate is different, needs resampling
		CPasteResampleModeDlg dlg(SrcSampleRate, TargetSampleRate, m_PasteResampleMode);

		if (IDOK != dlg.DoModal())
		{
			return FALSE;
		}

		m_PasteResampleMode = dlg.GetSelectedResampleMode();

		if (0 == dlg.GetSelectedResampleMode())
		{
			LONGLONG NumSamplesToPasteFrom64;
			// resample
			NumSamplesToPasteFrom64 =
				UInt32x32To64(NumSamplesToPasteFrom, TargetSampleRate) / SrcSampleRate;
			if ( ! CanAllocateWaveFileSamplesDlg(WaveFormat(), NumSamplesToPasteFrom64))
			{
				return FALSE;
			}

			NumSamplesToPasteFrom = NUMBER_OF_SAMPLES(NumSamplesToPasteFrom64);

			// create new temporary file
			CWaveFile DstFile;

			double ResampleQuality = 40.;
			double ResampleRatio = double(TargetSampleRate) / SrcSampleRate;

			if ( ! DstFile.CreateWaveFile(& SrcFile, NULL, ALL_CHANNELS, NumSamplesToPasteFrom,
										CreateWaveFileTempDir
										| CreateWaveFileDeleteAfterClose
										| CreateWaveFilePcmFormat
										| CreateWaveFileTemp,
										NULL))
			{
				FileCreationErrorMessageBox(NULL);
				return FALSE;
			}

			DstFile.GetWaveFormat()->nSamplesPerSec = TargetSampleRate;

			CResampleContext::auto_ptr pResampleContext(
				new CResampleContext(this, _T("Changing sample rate of clipboard data..."),
									_T("Resample"),
									SrcFile, DstFile, ResampleRatio, ResampleQuality));

			pStagedContext->AddContext(pResampleContext.release());

			SrcFile = pResampleContext->m_DstFile;
		}
	}

	if (End > Start)
	{
		CPasteModeDialog dlg(m_DefaultPasteMode);

		if (dlg.DoModal() != IDOK)
		{
			return FALSE;
		}

		m_DefaultPasteMode = dlg.GetPasteMode();
		switch (m_DefaultPasteMode)
		{
		case 0:
			// selection will be replaced with clipboard
			break;
		case 1:
			// paste as much as selection length, replace extra with silence
			if (NumSamplesToPasteFrom > End - Start)
			{
				NumSamplesToPasteFrom = End - Start;
			}
			break;
		default:
			// insert at the current position
			End = Start;
			break;
		}
	}

	int nCopiedChannels = WaveChannels();
	if ( ! m_WavFile.AllChannels(Channel))
	{
		nCopiedChannels = 1;
	}

	CHANNEL_MASK ChannelToCopyFrom = ALL_CHANNELS;
	if (nCopiedChannels < SrcFile.Channels())
	{
		CCopyChannelsSelectDlg dlg(m_PrevChannelToCopy);

		if (IDOK != dlg.DoModal())
		{
			return FALSE;
		}
		ChannelToCopyFrom = dlg.GetChannelToCopy();
		m_PrevChannelToCopy = ChannelToCopyFrom;
	}

	if ( ! CanExpandWaveFileDlg(m_WavFile, NumSamplesToPasteFrom - (End - Start)))
	{
		return FALSE;
	}

	if ( ! InitInsertCopy(pStagedContext.get(), m_WavFile, Start, End - Start, Channel,
						SrcFile, 0, NumSamplesToPasteFrom, ChannelToCopyFrom))
	{
		return FALSE;
	}

	if (UndoEnabled()
		&& ! pStagedContext->CreateUndo())
	{
		return FALSE;
	}

	// set operation context to the queue
	pStagedContext.release()->Execute();
	SetModifiedFlag(TRUE);

	return TRUE;
}

void CWaveSoapFrontDoc::DoCut(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channel)
{
	// save the cut area to Undo context, then shrink the file
	// create a operation context
	CStagedContext::auto_ptr pContext(
									new CStagedContext(this, _T("Copying data to clipboard..."), 0, _T("Cut")));

	CCopyContext * pCopyContext = new CCopyContext(this, _T("Copying data to clipboard..."), _T("Cut"));

	pContext->AddContext(pCopyContext);

	CWaveFile DstFile;
	// create a temporary clipboard WAV file
	if (FALSE == DstFile.CreateWaveFile( & m_WavFile, NULL, Channel,
										End - Start,
										CreateWaveFileTempDir
										| CreateWaveFileDeleteAfterClose
										| CreateWaveFileAllowMemoryFile
										| CreateWaveFileDontCopyInfo
										| CreateWaveFilePcmFormat
										| CreateWaveFileTemp,
										NULL))
	{
		AfxMessageBox(IDS_STRING_UNABLE_TO_CREATE_CLIPBOARD, MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	pCopyContext->m_Flags |= OperationContextClipboard;

	pCopyContext->InitCopy(DstFile, 0, ALL_CHANNELS,
							m_WavFile, Start, Channel, End - Start);

	GetApp()->m_ClipboardFile = DstFile;

	// set operation context to the queue
	pContext->AddContext(pCopyContext);

	InitShrinkOperation(pContext.get(), m_WavFile, Start, End - Start, Channel);

	if (UndoEnabled())
	{
		pContext->CreateUndo();
	}

	SetSelection(Start, Start, Channel, Start);

	pContext.release()->Execute();
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::DoDelete(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Channel)
{
	if (End == Start)
	{
		End++;
	}

	CStagedContext * pContext = new CStagedContext(this,
													_T("Deleting the selection..."), OperationContextDiskIntensive,
													_T("Delete"));

	InitShrinkOperation(pContext, m_WavFile, Start, End - Start, Channel);

	if (UndoEnabled())
	{
		pContext->CreateUndo();
	}

	SetSelection(Start, Start, Channel, Start);
	pContext->Execute();
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::OnEditStop()
{
	if (IsBusy())
	{
		m_StopOperation = true;
	}
}

void CWaveSoapFrontDoc::OnUpdateEditStop(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(IsBusy() && ! m_PlayingSound);
}

BOOL CWaveSoapFrontDoc::OnOpenDocument(LPCTSTR lpszPathName, int DocOpenFlags)
{
	TRACE(_T("CWaveSoapFrontDoc::OnOpenDocument(%s)\n"), lpszPathName);
	m_WavFile.Close();
	m_OriginalWavFile.Close();

	CThisApp * pApp = GetApp();
	if ((DocOpenFlags & OpenDocumentModeFlagsMask) == OpenDocumentDefaultMode)
	{
		m_bDirectMode = pApp->m_bDirectMode != FALSE;
		m_bReadOnly = pApp->m_bReadOnly != FALSE;
	}
	else
	{
		m_bDirectMode = ((DocOpenFlags & OpenDocumentDirectMode) != 0);
		m_bReadOnly = ((DocOpenFlags & OpenDocumentReadOnly) != 0);
	}
	if (m_bReadOnly)
	{
		m_bDirectMode = TRUE;
	}

	m_szWaveFilename = lpszPathName;
	if (DocOpenFlags & OpenDocumentNonWavFile)
	{
		return OpenNonWavFileDocument(lpszPathName, DocOpenFlags);
	}

	DWORD flags = MmioFileOpenReadOnly;
	if (m_bDirectMode && ! m_bReadOnly)
	{
		flags = MmioFileOpenExisting | MmioFileAllowReadOnlyFallback;
	}

	if (FALSE == OpenWaveFile(m_WavFile, lpszPathName, flags))
	{
		// error message already shown
		return FALSE;
	}

	// check if the file can be opened in direct mode
	WAVEFORMATEX * pWf = WaveFormat();
	bool bNeedConversion = FALSE;
	if (pWf != NULL)
	{
		int nChannels = pWf->nChannels;
		int nBitsPerSample = pWf->wBitsPerSample;
		if (16 == nBitsPerSample
			&& WAVE_FORMAT_PCM == pWf->wFormatTag
			&& (nChannels == 1 || nChannels == 2))
		{
			// can open direct and readonly
		}
		else
		{
			bNeedConversion = TRUE;
		}
	}
	else
	{
		return FALSE;
	}

	// if could only open in Read-Only mode, disable DirectMode
	if ( ! bNeedConversion
		&& m_bDirectMode
		&& ! m_bReadOnly)
	{
		if (m_WavFile.IsReadOnly())
		{
			CString s;
			s.Format(IDS_FILE_OPENED_NONDIRECT, lpszPathName);
			AfxMessageBox(s, MB_OK | MB_ICONINFORMATION);
			m_bDirectMode = FALSE;
		}
	}
	if (bNeedConversion)
	{
		m_bDirectMode = FALSE;
	}
	if ( ! m_bDirectMode)
	{
		m_bReadOnly = FALSE;
	}

	// create peak file name
	// remove WAV extension and add ".wspk" extension
	// we need the full pathname.
	SetModifiedFlag(FALSE);     // start off with unmodified
	m_OriginalWavFile = m_WavFile;
	m_OriginalWaveFormat = WaveFormat();

	// if non-direct mode, create a temp file.
	if ( ! m_bDirectMode)
	{
		m_WavFile.Close();
		int nNewFileSamples = m_OriginalWavFile.NumberOfSamples();
		if (bNeedConversion)
		{
			flags = CreateWaveFileTempDir
					// don't keep the file
					| CreateWaveFileDeleteAfterClose
					| CreateWaveFilePcmFormat
					| CreateWaveFileTemp;
			// if the file contains 'fact' chunk, get number of samples
			if (m_OriginalWavFile.m_FactSamples != -1
				&& (pWf->wFormatTag != WAVE_FORMAT_PCM
					|| (pWf->wBitsPerSample != 16 && pWf->wBitsPerSample != 8)))
			{
				nNewFileSamples = m_OriginalWavFile.m_FactSamples;
			}
		}
		else
		{
			flags = // Create in the original file folder
				// don't keep the file
				CreateWaveFileDeleteAfterClose
				| CreateWaveFilePcmFormat
				| CreateWaveFileDontInitStructure
				| CreateWaveFileTemp;
		}
		if (! m_WavFile.CreateWaveFile( & m_OriginalWavFile, NULL, ALL_CHANNELS,
										nNewFileSamples, flags, NULL))
		{
			AfxMessageBox(IDS_UNABLE_TO_CREATE_TEMPORARY_FILE, MB_OK | MB_ICONEXCLAMATION);
			return FALSE;
		}

		m_WavFile.CopyMetadata(m_OriginalWavFile);
		// for compressed file, actual size of file may differ from the
		// initial size
		if (bNeedConversion)
		{
			NUMBER_OF_SAMPLES NumSamples = WaveFileSamples();
			CDecompressContext * pContext =
				new CDecompressContext(this, _T("Loading the compressed file..."),
										m_OriginalWavFile,
										m_WavFile,
										m_OriginalWavFile.SampleToPosition(0),
										m_OriginalWavFile.SampleToPosition(LAST_SAMPLE),
										NumSamples,
										m_OriginalWavFile.GetWaveFormat());

			pContext->m_Flags |= DecompressSavePeakFile;

			// peak data will be created during decompression
			m_WavFile.LoadPeaksForCompressedFile(m_OriginalWavFile, NumSamples);

			SoundChanged(WaveFileID(), 0, 0, NumSamples, UpdateSoundDontRescanPeaks);
			pContext->Execute();
		}
		else
		{
			// just link the files
			UINT ErrorFormatID = 0;
			BOOL PeaksLoaded = m_OriginalWavFile.CheckAndLoadPeakFile();

			if (m_WavFile.SetSourceFile( & m_OriginalWavFile)
				&& m_WavFile.LoadRiffChunk()
				&& m_WavFile.LoadWaveformat())
			{
				if ( ! m_WavFile.FindData())
				{
					ErrorFormatID = IDS_UNABLE_TO_FIND_DATA_CHUNK;
				}
				else
				{
					m_WavFile.LoadMetadata();
				}
			}
			else
			{
				ErrorFormatID = IDS_UNABLE_TO_LOAD_WAVEFORMAT;
			}

			if (ErrorFormatID)
			{
				CString s;
				s.Format(ErrorFormatID, lpszPathName);
				AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}

			if ( ! PeaksLoaded)
			{
				BuildPeakInfo(TRUE);
			}
			SoundChanged(WaveFileID(), 0, 0, WaveFileSamples());
		}
	}
	else
	{
		if ( ! m_WavFile.CheckAndLoadPeakFile())
		{
			BuildPeakInfo(TRUE);
		}
		SoundChanged(WaveFileID(), 0, 0, WaveFileSamples());
	}
	// if file is open in direct mode or read-only, leave it as is,
	// otherwise open
	return TRUE;
}

BOOL CWaveSoapFrontDoc::OnSaveDirectFile()
{
	//Close and reopen the file
	if ( ! m_WavFile.Commit())
	{
		m_WavFile.SavePeakInfo(m_WavFile);
		AfxMessageBox(IDS_CANT_COMMIT_FILE_DATA, MB_OK | MB_ICONEXCLAMATION);
		SetModifiedFlag(FALSE);
		m_bCloseThisDocumentNow = true;
		return FALSE;
	}
	SetModifiedFlag(FALSE);
	m_WavFile.SavePeakInfo(m_WavFile);
	// direct file saved with the same format and name
	// information committed, peak info saved, file remains open
	return TRUE;
}

BOOL CWaveSoapFrontDoc::OnSaveBufferedPcmFile(int flags, LPCTSTR FullTargetName)
{
	// non-direct PCM file, the same directory, not saving a copy,
	// the same format
	if (m_WavFile.InitializeTheRestOfFile(500))
	{
		return PostCommitFileSave(flags, FullTargetName);
	}
	// InitializeTheRestOfFile in the thread
	//prepare context and queue it
	CCommitFileSaveContext * pContext =
		new CCommitFileSaveContext(this,
									_T("Saving the file..."),
									m_WavFile, flags, FullTargetName);

	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pContext->Execute();
	return FALSE;   // not saved yet, in process
}

BOOL CWaveSoapFrontDoc::OnSaveBufferedPcmFileCopy(int flags, LPCTSTR FullTargetName)
{
	CWaveFile NewWaveFile;
	// the same file attributes, but different folder or saving a copy.
	// Create a new file and copy everything there
	if (m_bDirectMode && ! m_bReadOnly)
	{
		// file open in direct mode
		flags |= SaveFile_SaveCopy;
	}

	CString NewTempFilename(FullTargetName);
	if (0 == (flags & SaveFile_SaveCopy))
	{
		NewTempFilename += _T(".temp");
	}
	if (FALSE == NewWaveFile.CreateWaveFile(& m_OriginalWavFile,
											WaveFormat(),
											ALL_CHANNELS, WaveFileSamples(),
											0, NewTempFilename))
	{
		FileCreationErrorMessageBox(NewTempFilename);
		return FALSE;
	}

	LPCTSTR sOp = _T("Saving the file...");
	if (flags & SaveFile_SaveCopy)
	{
		sOp = _T("Saving the file copy...");
	}

	CFileSaveContext::auto_ptr pSaveContext
	(new CFileSaveContext(this, sOp, _T("File Save")));

	if (NULL == pSaveContext.get())
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pSaveContext->m_NewName = FullTargetName;
	pSaveContext->m_SrcFile = m_WavFile;
	pSaveContext->m_DstFile = NewWaveFile;

	CCopyContext * pCopyContext = new CCopyContext(this, sOp, _T("File Save"));
	if (NULL == pCopyContext)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}
	pSaveContext->AddContext(pCopyContext);

	pCopyContext->m_SrcFile = m_WavFile;
	pCopyContext->m_DstFile = NewWaveFile;
	pCopyContext->m_SrcStart = 0;
	pCopyContext->m_DstStart = 0;
	pCopyContext->m_SrcPos = 0;
	pCopyContext->m_DstPos = 0;
	pCopyContext->m_SrcEnd = m_WavFile.GetLength();
	pCopyContext->m_DstEnd = pCopyContext->m_SrcEnd;
	// copy 1:1
	pCopyContext->m_SrcChan = ALL_CHANNELS;
	pCopyContext->m_DstChan = ALL_CHANNELS;

	if (flags & SaveFile_SaveCopy)
	{
		pSaveContext->m_Flags |= FileSaveContext_SavingCopy;
	}
	if (flags & SaveFile_SameName)
	{
		pSaveContext->m_Flags |= FileSaveContext_SameName;
	}

	pSaveContext.release()->Execute();
	return FALSE;   // not saved yet
}

BOOL CWaveSoapFrontDoc::OnSaveConvertedFile(int flags, LPCTSTR FullTargetName, WAVEFORMATEX * pWf)
{
	CWaveFile NewWaveFile;
	// conversion or compression required
	CString NewTempFilename = FullTargetName;
	if (0 == (flags & SaveFile_SaveCopy))
	{
		NewTempFilename += _T(".temp");
	}
	if (WAVE_FORMAT_PCM == pWf->wFormatTag
		&& 16 == pWf->wBitsPerSample)
	{
		LONGLONG nNewSamples = MulDiv(WaveFileSamples(), pWf->nSamplesPerSec, WaveSampleRate());
		if ( ! CanAllocateWaveFileSamplesDlg(pWf, nNewSamples))
		{
			return FALSE;
		}

		if (FALSE == NewWaveFile.CreateWaveFile(& m_OriginalWavFile, pWf, ALL_CHANNELS, ULONG(nNewSamples),
												CreateWaveFilePcmFormat | CreateWaveFileDontCopyInfo,
												NewTempFilename))
		{
			FileCreationErrorMessageBox(NewTempFilename);
			return FALSE;
		}
	}
	else
	{
		// saving compressed file
		// if number of channels changes, ask about it
		// sample rate change may be also required
		MEDIA_FILE_SIZE NewSize = MEDIA_FILE_SIZE(WaveFileSamples()) * pWf->wBitsPerSample * pWf->nChannels / 8;

		if (NewSize < 0x100000) NewSize = 0x100000; // 1 meg
		if (NewSize > 0x100 * 0x100000) NewSize = 0x100 * 0x100000; // 256 meg

		if ( ! NewWaveFile.CreateWaveFile(& m_OriginalWavFile, pWf, ALL_CHANNELS, NewSize,
										CreateWaveFileDontCopyInfo
										| CreateWaveFileCreateFact
										| CreateWaveFileSizeSpecified,
										NewTempFilename))
		{
			FileCreationErrorMessageBox(NewTempFilename);
			return FALSE;
		}
	}
	LPCTSTR sOp = _T("Converting and saving the file...");
	if (flags & SaveFile_SaveCopy)
	{
		sOp = _T("Saving the file copy...");
	}

	CFileSaveContext::auto_ptr pSaveContext
	(new CFileSaveContext(this, sOp, _T("File Convert and Save")));

	if (NULL == pSaveContext.get())
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pSaveContext->m_NewName = FullTargetName;

	CConversionContext * pConvert =
		new CConversionContext(this, _T(""), _T(""), m_WavFile, NewWaveFile);

	if (NULL == pConvert)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pSaveContext->AddContext(pConvert);

	pSaveContext->m_SrcFile = pConvert->m_SrcFile;
	pSaveContext->m_DstFile = pConvert->m_DstFile;
	// fill unused data members:

	if (flags & SaveFile_SaveCopy)
	{
		pSaveContext->m_Flags |= FileSaveContext_SavingCopy;
	}

	NUMBER_OF_CHANNELS const OldChannels = WaveChannels();
	NUMBER_OF_CHANNELS const NewChannels = pWf->nChannels;
	// if target channels is less than source, convert it before resampling,
	if (NewChannels < OldChannels)
	{
		// ask which channels to save
		CCopyChannelsSelectDlg dlg(m_PrevChannelToCopy);

		if (IDOK != dlg.DoModal())
		{
			return FALSE;
		}
		CChannelConvertor * pChConvertor =
			new CChannelConvertor(OldChannels, NewChannels, dlg.GetChannelToCopy());

		if (NULL == pChConvertor)
		{
			NotEnoughMemoryMessageBox();
			return FALSE;
		}
		m_PrevChannelToCopy = dlg.GetChannelToCopy();

		pConvert->AddWaveProc(pChConvertor);
	}

	if (pWf->nSamplesPerSec != WaveSampleRate())
	{
		CResampleFilter * pFilter = new CResampleFilter;
		if (NULL == pFilter)
		{
			NotEnoughMemoryMessageBox();
			return FALSE;
		}
		// FIX: Wrong conversion if resampling and increasing channels
		NUMBER_OF_CHANNELS nChannels = NewChannels;
		if (nChannels > OldChannels)
		{
			nChannels = OldChannels;
		}
		pFilter->InitResample(double(pWf->nSamplesPerSec)
							/ WaveSampleRate(), 40., nChannels);
		pConvert->AddWaveProc(pFilter);
	}

	// if target channels is more than source, convert it after resampling,
	if (NewChannels > OldChannels)
	{
		CChannelConvertor * pChConvertor =
			new CChannelConvertor(OldChannels, NewChannels, ALL_CHANNELS);

		if (NULL == pChConvertor)
		{
			NotEnoughMemoryMessageBox();
			return FALSE;
		}
		pConvert->AddWaveProc(pChConvertor);
	}

	if (pWf->wFormatTag != WAVE_FORMAT_PCM
		|| pWf->wBitsPerSample != 16)
	{
		CAudioConvertor * pAcmConvertor = new CAudioConvertor;
		if (NULL == pAcmConvertor)
		{
			NotEnoughMemoryMessageBox();
			return FALSE;
		}

		CWaveFormat SrcFormat;
		SrcFormat.InitFormat(WAVE_FORMAT_PCM, pWf->nSamplesPerSec,
							pWf->nChannels);

		pConvert->AddWaveProc(pAcmConvertor);
		//todo: init conversion in Context->Init, error dialog in PostRetire.
		if (! pAcmConvertor->InitConversion(SrcFormat, pWf))
		{
			AfxMessageBox(IDS_STRING_UNABLE_TO_CONVERT);
			return FALSE;
		}
	}

	pSaveContext.release()->Execute();
	return FALSE;   // not saved yet
}

BOOL CWaveSoapFrontDoc::OnSaveMp3File(int flags, LPCTSTR FullTargetName, WAVEFORMATEX * pWf)
{
	// create output file
	CWaveFile NewWaveFile;
	CString NewTempFilename = FullTargetName;
	if (0 == (flags & SaveFile_SaveCopy))
	{
		NewTempFilename += _T(".temp");
	}
	DWORD FileSize = MulDiv(pWf->nAvgBytesPerSec, m_WavFile.GetDataChunk()->cksize,
							WaveFormat()->nAvgBytesPerSec);
	if (FALSE == NewWaveFile.CreateWaveFile(& m_OriginalWavFile, pWf, ALL_CHANNELS,
											FileSize,
											CreateWaveFileDontInitStructure | CreateWaveFileSizeSpecified,
											NewTempFilename))
	{
		FileCreationErrorMessageBox(NewTempFilename);
		return FALSE;
	}

	WAVEFORMATEX * pNewFormat = NewWaveFile.AllocateWaveformat(sizeof (WAVEFORMATEX) + pWf->cbSize);
	if (NULL != pNewFormat)
	{
		memcpy(pNewFormat, pWf, sizeof (WAVEFORMATEX) + pWf->cbSize);
	}

	CFileSaveContext::auto_ptr pContext(new CFileSaveContext(this,
											_T("Compressing and saving Mp3 file..."), _T("Mp3 File Compress and Save")));

	if (NULL == pContext.get())
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pContext->m_NewName = FullTargetName;
	pContext->m_NewFileTypeFlags = OpenDocumentMp3File;

	CConversionContext * pConvert = new CConversionContext(this, _T(""), _T(""));

	if (NULL == pConvert)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pContext->AddContext(pConvert);

	pConvert->m_SrcFile = m_WavFile;
	pConvert->m_DstFile = NewWaveFile;
	pConvert->m_SrcStart = m_WavFile.SampleToPosition(0);
	pConvert->m_DstStart = 0;
	pConvert->m_SrcPos = pConvert->m_SrcStart;
	pConvert->m_DstPos = pConvert->m_DstStart;
	pConvert->m_SrcEnd = m_WavFile.SampleToPosition(LAST_SAMPLE);
	pConvert->m_DstEnd = pConvert->m_DstStart;

	pConvert->m_SrcChan = ALL_CHANNELS;
	pConvert->m_DstChan = ALL_CHANNELS;

	pContext->m_SrcFile = pConvert->m_SrcFile;
	pContext->m_DstFile = pConvert->m_DstFile;

#if 0
	// fill unused data members:
	pContext->m_SrcStart = pConvert->m_SrcStart;
	pContext->m_DstStart = pConvert->m_DstStart;
	pContext->m_SrcPos = pContext->m_SrcStart;
	pContext->m_DstPos = pContext->m_DstPos;
	pContext->m_SrcEnd = pConvert->m_SrcEnd;
	pContext->m_DstEnd = pContext->m_DstStart;

	pContext->m_SrcChan = ALL_CHANNELS;
	pContext->m_DstChan = ALL_CHANNELS;
#endif

	if (WAVE_FORMAT_MPEGLAYER3 == pWf->wFormatTag)
	{
		CAudioConvertor * pAcmConvertor = new CAudioConvertor;
		if (NULL == pAcmConvertor)
		{
			NotEnoughMemoryMessageBox();
			return FALSE;
		}
		pConvert->AddWaveProc(pAcmConvertor);

		WAVEFORMATEX SrcFormat;
		SrcFormat.nChannels = pWf->nChannels;
		SrcFormat.nSamplesPerSec = pWf->nSamplesPerSec;
		SrcFormat.nBlockAlign = SrcFormat.nChannels * sizeof (WAVE_SAMPLE);
		SrcFormat.nAvgBytesPerSec = SrcFormat.nBlockAlign * SrcFormat.nSamplesPerSec;
		SrcFormat.cbSize = 0;
		SrcFormat.wFormatTag = WAVE_FORMAT_PCM;
		SrcFormat.wBitsPerSample = 16;
		if (! pAcmConvertor->InitConversion( & SrcFormat, pWf))
		{
			//todo: unable to convert dialog
			return FALSE;
		}
	}
	else
	{
		CLameEncConvertor * pMp3Convertor = new CLameEncConvertor;
		pConvert->AddWaveProc(pMp3Convertor);

		if ( ! pMp3Convertor->Open(pWf))
		{
			return FALSE;
		}
	}

	if (flags & SaveFile_SaveCopy)
	{
		pContext->m_Flags |= FileSaveContext_SavingCopy;
	}
	pContext.release()->Execute();
	return FALSE;   // not saved yet
}

BOOL CWaveSoapFrontDoc::OnSaveWmaFile(int flags, LPCTSTR FullTargetName, WAVEFORMATEX * pWf)
{
	// create output file
	CString NewTempFilename = FullTargetName;
	if (0 == (flags & SaveFile_SaveCopy))
	{
		NewTempFilename += _T(".temp");
	}

	CWaveFile NewWaveFile;

	DWORD FileSize = MulDiv(pWf->nAvgBytesPerSec, m_WavFile.GetDataChunk()->cksize,
							WaveFormat()->nAvgBytesPerSec);
	if (FALSE == NewWaveFile.CreateWaveFile(& m_OriginalWavFile, pWf, ALL_CHANNELS,
											FileSize,
											CreateWaveFileDontInitStructure | CreateWaveFileSizeSpecified,
											NewTempFilename))
	{
		FileCreationErrorMessageBox(NewTempFilename);
		return FALSE;
	}

	WAVEFORMATEX * pNewFormat = NewWaveFile.AllocateWaveformat(sizeof (WAVEFORMATEX) + pWf->cbSize);
	if (NULL != pNewFormat)
	{
		memcpy(pNewFormat, pWf, sizeof (WAVEFORMATEX) + pWf->cbSize);
	}

	CFileSaveContext::auto_ptr pContext(new CFileSaveContext(this,
															_T("Compressing and saving Windows Media file..."), _T("")));

	if (NULL == pContext.get())
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pContext->m_NewName = FullTargetName;
	pContext->m_DstFile = NewWaveFile;
	pContext->m_NewFileTypeFlags = OpenDocumentWmaFile;
	CWmaSaveContext * pConvert = new CWmaSaveContext(this, _T(""), _T(""));

	if (NULL == pConvert)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pContext->AddContext(pConvert);

	pConvert->m_SrcFile = m_WavFile;
	pConvert->m_DstFile = NewWaveFile;
	pConvert->m_SrcStart = m_WavFile.SampleToPosition(0);
	pConvert->m_DstStart = 0;
	pConvert->m_SrcPos = pConvert->m_SrcStart;
	pConvert->m_DstPos = pConvert->m_DstStart;
	pConvert->m_SrcEnd = m_WavFile.SampleToPosition(LAST_SAMPLE);
	pConvert->m_DstEnd = pConvert->m_DstStart;

	pConvert->m_SrcChan = ALL_CHANNELS;
	pConvert->m_DstChan = ALL_CHANNELS;

	pContext->m_SrcFile = pConvert->m_SrcFile;
	pContext->m_DstFile = pConvert->m_DstFile;

	if (flags & SaveFile_SaveCopy)
	{
		pContext->m_Flags |= FileSaveContext_SavingCopy;
	}

	pContext.release()->Execute();
	return FALSE;   // not saved yet
}

BOOL CWaveSoapFrontDoc::OnSaveRawFile(int flags, LPCTSTR FullTargetName, WAVEFORMATEX * pWf)
{
	// create output file
	CWaveFile NewWaveFile;
	CString NewTempFilename = FullTargetName;
	if (0 == (flags & SaveFile_SaveCopy))
	{
		NewTempFilename += _T(".temp");
	}
	// TODO
	DWORD FileSize = MulDiv(pWf->nAvgBytesPerSec, m_WavFile.GetDataChunk()->cksize,
							WaveFormat()->nAvgBytesPerSec);
	if (FALSE == NewWaveFile.CreateWaveFile(& m_OriginalWavFile, pWf, ALL_CHANNELS,
											FileSize,
											CreateWaveFileDontInitStructure | CreateWaveFileSizeSpecified,
											NewTempFilename))
	{
		FileCreationErrorMessageBox(NewTempFilename);
		return FALSE;
	}

	WAVEFORMATEX * pNewFormat = NewWaveFile.AllocateWaveformat();
	if (NULL != pNewFormat)
	{
		* pNewFormat = * pWf;
	}

	CFileSaveContext::auto_ptr pContext(new CFileSaveContext(this,
											_T("Saving the file in raw binary format..."), _T("Raw File Save")));

	if (NULL == pContext.get())
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pContext->m_NewName = FullTargetName;
	pContext->m_NewFileTypeFlags = OpenDocumentMp3File;

	CConversionContext * pConvert = new CConversionContext(this, _T(""), _T(""));

	if (NULL == pConvert)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pContext->AddContext(pConvert);

	pConvert->m_SrcFile = m_WavFile;
	pConvert->m_DstFile = NewWaveFile;
	pConvert->m_SrcStart = m_WavFile.SampleToPosition(0);
	pConvert->m_DstStart = 0;
	pConvert->m_SrcPos = pConvert->m_SrcStart;
	pConvert->m_DstPos = pConvert->m_DstStart;
	pConvert->m_SrcEnd = m_WavFile.SampleToPosition(LAST_SAMPLE);
	pConvert->m_DstEnd = pConvert->m_DstStart;

	pConvert->m_SrcChan = ALL_CHANNELS;
	pConvert->m_DstChan = ALL_CHANNELS;

	pContext->m_SrcFile = pConvert->m_SrcFile;
	pContext->m_DstFile = pConvert->m_DstFile;

	if (WAVE_FORMAT_PCM != pWf->wFormatTag
		|| 16 != pWf->wBitsPerSample)
	{
		CAudioConvertor * pAcmConvertor = new CAudioConvertor;
		if (NULL == pAcmConvertor)
		{
			NotEnoughMemoryMessageBox();
			return FALSE;
		}

		pConvert->AddWaveProc(pAcmConvertor);

		WAVEFORMATEX SrcFormat;
		SrcFormat.nChannels = pWf->nChannels;
		SrcFormat.nSamplesPerSec = pWf->nSamplesPerSec;
		SrcFormat.nBlockAlign = SrcFormat.nChannels * sizeof (WAVE_SAMPLE);
		SrcFormat.nAvgBytesPerSec = SrcFormat.nBlockAlign * SrcFormat.nSamplesPerSec;
		SrcFormat.cbSize = 0;
		SrcFormat.wFormatTag = WAVE_FORMAT_PCM;
		SrcFormat.wBitsPerSample = 16;

		if (! pAcmConvertor->InitConversion( & SrcFormat, pWf))
		{
			//todo: unable to convert dialog
			return FALSE;
		}
	}
	else if (flags & SaveRawFileMsbFirst)
	{
		pConvert->AddWaveProc(new CByteSwapConvertor);
	}

	if (flags & SaveFile_SaveCopy)
	{
		pContext->m_Flags |= FileSaveContext_SavingCopy;
	}

	pContext->Execute();
	return FALSE;   // not saved yet
}

BOOL CWaveSoapFrontDoc::OnSaveDocument(LPCTSTR lpszPathName, DWORD flags, WAVEFORMATEX * pWf)
{

	m_bClosePending = m_bClosing;

	// file where the data is currently kept
	TCHAR SourceDir[MAX_PATH];
	TCHAR FullSourceName[MAX_PATH];
	// Original file where the data is currently kept
	TCHAR OriginalDir[MAX_PATH] = {0};
	TCHAR FullOriginalName[MAX_PATH] = {0};
	// file where to copy the data
	TCHAR TargetDir[MAX_PATH];
	TCHAR FullTargetName[MAX_PATH];
	if ( ! AfxFullPath(FullTargetName, lpszPathName)
		|| ! AfxFullPath(FullSourceName, m_WavFile.GetName())
		|| (m_OriginalWavFile.IsOpen()
			&& ! AfxFullPath(FullOriginalName, m_OriginalWavFile.GetName())))
	{
		// incorrect name????
		return FALSE;
	}
	LPTSTR pTargetFilePart = _tcsrchr(FullTargetName, '\\');
	if (pTargetFilePart)
	{
		int TargetDirChars = pTargetFilePart - FullTargetName;
		_tcsncpy(TargetDir, FullTargetName, TargetDirChars);
		TargetDir[TargetDirChars] = 0;
	}
	else
	{
		_tcsncpy(TargetDir, FullTargetName, MAX_PATH);
		TargetDir[MAX_PATH - 1] = 0;
	}

	LPTSTR pOriginalFilePart = _tcsrchr(FullOriginalName, '\\');
	if (pOriginalFilePart)
	{
		int DirChars = pOriginalFilePart - FullOriginalName;
		_tcsncpy(OriginalDir, FullOriginalName, DirChars);
		OriginalDir[DirChars] = 0;
	}
	else
	{
		_tcsncpy(OriginalDir, FullOriginalName, MAX_PATH);
		OriginalDir[MAX_PATH - 1] = 0;
	}

	LPTSTR pSourceFilePart = _tcsrchr(FullSourceName, '\\');
	if (pSourceFilePart)
	{
		int SourceDirChars = pSourceFilePart - FullSourceName;
		_tcsncpy(SourceDir, FullSourceName, SourceDirChars);
		SourceDir[SourceDirChars] = 0;
	}
	else
	{
		_tcsncpy(SourceDir, FullSourceName, MAX_PATH);
		SourceDir[MAX_PATH - 1] = 0;
	}

	if (AfxComparePath(FullTargetName, FullOriginalName))
	{
		flags |= SaveFile_SameName;
	}

	m_WavFile.CommitChanges();

	DeletePermanentUndoRedo();

	if (flags & SaveFile_Mp3File)
	{
		return OnSaveMp3File(flags, FullTargetName, pWf);
	}
	else if (flags & SaveFile_WmaFile)
	{
		return OnSaveWmaFile(flags, FullTargetName, pWf);
	}
	else if (flags & SaveFile_RawFile)
	{
		return OnSaveRawFile(flags, FullTargetName, pWf);
	}

	if (WAVE_FORMAT_PCM == pWf->wFormatTag
		// the same format
		&& 16 == pWf->wBitsPerSample
		&& pWf->nSamplesPerSec == WaveSampleRate()
		&& pWf->nChannels == WaveChannels())
	{
// if direct file: commit all changes (including format change)
		if (m_bDirectMode
			&& (flags & SaveFile_SameName)
			&& 0 == (flags & SaveFile_SaveCopy))
		{
			return OnSaveDirectFile();
		}
// non-direct PCM file:
// commit all changes. If the file is saved to the same directory as the temporary file,
// clear Temp flag and DeleteAfterClose, rename the old file,
// rename the new file, delete the old file, open new temp file.
// otherwise create a new file and copy old one to the new.
// if directory is different,
// create temp file, copy the data to it, rename old file if exists,
		else if (0 == (flags & SaveFile_SaveCopy)
				&& ! m_bDirectMode
				&& 0 == _tcsicmp(SourceDir, TargetDir))
		{
			return OnSaveBufferedPcmFile(flags, FullTargetName);
		}
		else
		{
			return OnSaveBufferedPcmFileCopy(flags, FullTargetName);
		}
	}
	else
	{
		return OnSaveConvertedFile(flags, FullTargetName, pWf);
	}
}

void CWaveSoapFrontDoc::PostFileSave(CWaveFile & DstFile,
									LPCTSTR NewName, BOOL SameName)
{
	WAVEFORMATEX OldFormat = * WaveFormat();
	WAVEFORMATEX NewFormat = * DstFile.GetWaveFormat();

	// copy security descriptor
	if (m_OriginalWavFile.IsOpen()
		&& SameName)
	{
		// it isn't a new file
		// TODO: use ReplaceFile
		DWORD dwLength = 0;
		PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
		if ( ! GetFileSecurity(NewName, DACL_SECURITY_INFORMATION,
								NULL, 0, &dwLength)
			&& ERROR_INSUFFICIENT_BUFFER == GetLastError())
		{
			pSecurityDescriptor = (PSECURITY_DESCRIPTOR) new BYTE[dwLength];
			if (NULL != pSecurityDescriptor
				&& ::GetFileSecurity(NewName,
									DACL_SECURITY_INFORMATION,
									pSecurityDescriptor, dwLength, &dwLength))
			{
				SetFileSecurity(DstFile.GetName(), DACL_SECURITY_INFORMATION, pSecurityDescriptor);
			}
			delete[] (BYTE*)pSecurityDescriptor;
		}
	}

	m_OriginalWavFile.Close();  // close to allow delete and rename
	if (m_bDirectMode || m_bReadOnly)
	{
		m_WavFile.Close();   // close to allow delete and rename
	}
	else
	{
		// commit all source file data and detach
		m_WavFile.InitializeTheRestOfFile();
		m_WavFile.DetachSourceFile();
	}
	// delete the old file
	DeleteFile(NewName);
	DstFile.CommitChanges();

	int ReopenDocumentFlags = 0;
	// rename the new file
	if ( ! DstFile.Rename(NewName, 0))
	{
		m_bClosePending = false;
		CString s;
		s.Format(IDS_UNABLE_TO_RENAME_TEMPORARY_FILE, DstFile.GetName(), NewName);
		AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	else
	{
		SetModifiedFlag(FALSE);
		// need to commit file to save its peak info with the correct timestamp
		DstFile.Commit();
		// we always reopen the document
		if (WAVE_FORMAT_PCM == NewFormat.wFormatTag
			&& 16 == NewFormat.wBitsPerSample)
		{
			m_WavFile.SavePeakInfo(DstFile);

			if (m_bClosePending)
			{
				DstFile.Close();
				SetModifiedFlag(FALSE);
				m_bCloseThisDocumentNow = true; // the document will be deleted
				return;
			}
			if (! m_bDirectMode || m_bReadOnly)
			{
				// if PCM format and read-only or non-direct, ask about reopening as direct
				CReopenDialog ReopenDlg(IDS_REOPEN_IN_DIRECT_MODE, NewName);

				int result = ReopenDlg.DoModalPopDocument(this);
				if (IDOK == result)
				{
					ReopenDocumentFlags = OpenDocumentDirectMode;
				}
				else if (IDCANCEL == result)
				{
					DstFile.Close();
					//SetModifiedFlag(FALSE);
					m_bCloseThisDocumentNow = true; // the document will be deleted
					return;
				}
			}
		}
		else
		{
			if (m_bClosePending)
			{
				if (OldFormat.nSamplesPerSec == NewFormat.nSamplesPerSec
					&& OldFormat.nChannels == NewFormat.nChannels)
				{
					m_WavFile.SavePeakInfo(DstFile);
				}
				DstFile.Close();
				SetModifiedFlag(FALSE);
				m_bCloseThisDocumentNow = true; // the document will be deleted
				return;
			}
			// if non-PCM format, ask about reloading the file
			// if format changed, always reload
			if (OldFormat.nSamplesPerSec == NewFormat.nSamplesPerSec
				&& OldFormat.nChannels == NewFormat.nChannels)
			{
				m_WavFile.SavePeakInfo(DstFile);
				CReopenCompressedFileDialog dlg;
				dlg.m_Text.Format(IDS_RELOAD_COMPRESSED_FILE, LPCTSTR(NewName));

				CDocumentPopup pop(this);
				int result = dlg.DoModal();

				if (IDOK == result)
				{
					// set new title
					SetPathName(NewName);
					m_OriginalWavFile = DstFile;
					m_OriginalWaveFormat = DstFile.GetWaveFormat();
					DstFile.Close();
					ASSERT(m_WavFile.IsOpen());
					//todo
					return; //??
				}
				else if (IDCANCEL == result)
				{
					DstFile.Close();
					//SetModifiedFlag(FALSE);
					m_bCloseThisDocumentNow = true; // the document will be deleted
					return;
				}
			}
			else
			{
				// samples or channels changed
				CReopenConvertedFileDlg dlg;
				dlg.m_Text.Format(IDS_SHOULD_RELOAD_COMPRESSED_FILE, NewName);

				CDocumentPopup pop(this);

				int result = dlg.DoModal();
				if (IDCANCEL == result)
				{
					DstFile.Close();
					//SetModifiedFlag(FALSE);
					m_bCloseThisDocumentNow = true; // the document will be deleted
					return;
				}
				// else reload
			}
			// TODO: reconsider keeping undo/redo,
			DeleteUndo();
			DeleteRedo();
		}
		// todo
	}

	DstFile.Close();
	// file will be reopened
	if (OldFormat.nSamplesPerSec != NewFormat.nSamplesPerSec)
	{
		DeleteUndo();
		DeleteRedo();
		// recalculate selection
		SAMPLE_INDEX SelStart = MulDiv(m_SelectionStart, NewFormat.nSamplesPerSec,
										OldFormat.nSamplesPerSec);
		SAMPLE_INDEX SelEnd = MulDiv(m_SelectionEnd, NewFormat.nSamplesPerSec,
									OldFormat.nSamplesPerSec);
		SAMPLE_INDEX Caret = MulDiv(m_CaretPosition, NewFormat.nSamplesPerSec,
									OldFormat.nSamplesPerSec);

		CHANNEL_MASK Chan = m_SelectedChannel;
		if (OldFormat.nChannels != NewFormat.nChannels)
		{
			Chan = ALL_CHANNELS;
		}

		SetSelection(SelStart, SelEnd, Chan, Caret, SetSelection_MakeCaretVisible);
		// view length will be adjusted in OpenDocument?
	}
	else if (OldFormat.nChannels != NewFormat.nChannels)
	{
		DeleteUndo();
		DeleteRedo();
		SetSelection(m_SelectionStart, m_SelectionEnd,
					ALL_CHANNELS, m_CaretPosition, 0);
	}
	if ( ! OnOpenDocument(NewName, ReopenDocumentFlags | m_FileTypeFlags))
	{
		// message box, then close the document
		CString s;
		s.Format(IDS_UNABLE_TO_REOPEN_AS, NewName);
		AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
		m_bCloseThisDocumentNow = true; // the document will be deleted
		return;
	}
	SetPathName(NewName);
}

BOOL CWaveSoapFrontDoc::PostCommitFileSave(int flags, LPCTSTR FullTargetName)
{
	m_WavFile.DetachSourceFile();
	m_WavFile.DeleteOnClose(false);
	if (flags & SaveFile_SameName)
	{
		// copy security descriptor
		if (m_OriginalWavFile.IsOpen())
		{
			// it isn't a new file
			DWORD dwLength = 0;
			PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
			if ( ! GetFileSecurity(m_OriginalWavFile.GetName(), DACL_SECURITY_INFORMATION,
									NULL, 0, &dwLength)
				&& ERROR_INSUFFICIENT_BUFFER == GetLastError())
			{
				pSecurityDescriptor = (PSECURITY_DESCRIPTOR) new BYTE[dwLength];
				if (NULL != pSecurityDescriptor
					&& ::GetFileSecurity(m_OriginalWavFile.GetName(),
										DACL_SECURITY_INFORMATION,
										pSecurityDescriptor, dwLength, &dwLength))
				{
					SetFileSecurity(m_WavFile.GetName(), DACL_SECURITY_INFORMATION, pSecurityDescriptor);
				}
				delete[] (BYTE*)pSecurityDescriptor;
			}
			m_OriginalWavFile.DeleteOnClose(true);
		}
	}
	else
	{
		// the user was asked about replacing the file in the save file dialog

		DeleteFile(FullTargetName);
	}
	m_OriginalWavFile.Close();
	// delete the original file
	DeleteFile(FullTargetName);

	if (m_WavFile.Rename(FullTargetName, 0))
	{
		// if PCM format and non-direct, ask about reopening as direct
		m_WavFile.SavePeakInfo(m_WavFile);
		SetModifiedFlag(FALSE);

		if (m_bClosePending)
		{
			m_WavFile.Close();
			m_bCloseThisDocumentNow = true;
			return TRUE;
		}

		CReopenDialog ReopenDlg(IDS_REOPEN_IN_DIRECT_MODE, m_WavFile.GetName());

		int result = ReopenDlg.DoModalPopDocument(this);

		if (IDOK == result)
		{
			// keep undo/redo
			OnOpenDocument(FullTargetName, OpenDocumentDirectMode);
			return TRUE;
		}
		else if (IDCANCEL == result)
		{
			m_bCloseThisDocumentNow = true;
			return TRUE;
		}
		else
		{
			m_WavFile.Close();
			// keep undo/redo
			if (OnOpenDocument(FullTargetName, 0))
			{
				return TRUE;
			}
			else
			{
				OnOpenDocument(FullTargetName, OpenDocumentReadOnly);
				return TRUE;
			}
		}
	}
	else
	{
		// error
		CString s;
		s.Format(IDS_UNABLE_TO_RENAME_TEMPORARY_FILE, m_WavFile.GetName(), FullTargetName);
		AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
		// todo: query new name
		return FALSE;
	}
}

void CWaveSoapFrontDoc::OnIdle()
{
	if (m_bInOnIdle)
	{
		return;
	}
	m_bInOnIdle = true;
	CSoundUpdateInfo * pInfo;
	while (NULL != (pInfo = m_UpdateList.RemoveHead()))
	{
		UpdateAllViews(NULL, pInfo->m_UpdateCode, pInfo);
		if (pInfo->m_NewLength != -1)
		{
			// check if the selection fits in the new length
			SetSelection(std::min(pInfo->m_NewLength, m_SelectionStart),
						std::min(pInfo->m_NewLength, m_SelectionEnd), m_SelectedChannel,
						std::min(pInfo->m_NewLength, m_CaretPosition), SetSelection_MakeFileVisible);
		}
		delete pInfo;
	}
	while ( ! m_RetiredList.IsEmpty())
	{
		COperationContext * pContext = m_RetiredList.RemoveHead();
		pContext->PostRetire(FALSE);     // deletes it usually

		--m_OperationInProgress;
	}
	m_bInOnIdle = false;
}


void CWaveSoapFrontDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(IsModified() && CanWriteFile());
}

void CWaveSoapFrontDoc::OnUpdateFileSaveAs(CCmdUI* pCmdUI)
{
	// in direct or readonly mode only Save Copy command is allowed
	pCmdUI->Enable(CanWriteFile() && ! m_bDirectMode );
}

void CWaveSoapFrontDoc::OnUpdateFileSaveCopyAs(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(! IsBusy());
}

void CWaveSoapFrontDoc::OnEditSelectAll()
{
	NUMBER_OF_SAMPLES len = WaveFileSamples();
	SetSelection(0, len, ALL_CHANNELS, len, SetSelection_MakeCaretVisible);
}

void CWaveSoapFrontDoc::OnEditSelection()
{
	CSelectionDialog dlg(m_SelectionStart, m_SelectionEnd, m_CaretPosition,
						m_SelectedChannel, m_WavFile, GetApp()->m_SoundTimeFormat);

	if (IDOK == dlg.DoModal())
	{
		SetSelection(dlg.GetStart(), dlg.GetEnd(),
					dlg.GetChannel(), dlg.GetEnd(), SetSelection_MakeCaretVisible);
	}
}

void CWaveSoapFrontDoc::OnUpdateEditSelection(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! IsBusy());
}

static void LimitUndoRedo(ListHead<COperationContext> * pEntry, int MaxNum, size_t MaxSize)
{
	LONGLONG Size = 0;
	int Num = 0;
	for (COperationContext * pContext = pEntry->First()
		;  pEntry->NotEnd(pContext); pContext = pEntry->Next(pContext))
	{
		Num++;
		CUndoRedoContext * pUndoRedo = dynamic_cast<CUndoRedoContext *>(pContext);
		if (NULL != pUndoRedo
			//&& pUndoRedo->m_SrcFile.IsOpen()
			)
		{
			Size += pUndoRedo->GetTempDataSize();
		}
		if ((MaxNum != 0 && Num > MaxNum)
			|| (MaxSize != 0 && Size > MaxSize))
		{
			while (pContext != pEntry->Last())
			{
				delete pEntry->RemoveTail();
			}
			break;
		}
	}
}

void CWaveSoapFrontDoc::AddUndoRedo(CUndoRedoContext * pContext)
{
	CThisApp * pApp = GetApp();
	if (pContext->IsUndoOperation())
	{
		m_UndoList.InsertHead(pContext);

		// free extra undo, if count or size limit is exceeded
		int MaxUndoDepth = pApp->m_MaxUndoDepth;
		if ( ! pApp->m_bEnableUndoDepthLimit)
		{
			MaxUndoDepth = 0;
		}
		int MaxUndoSize = pApp->m_MaxUndoSize;
		if ( ! pApp->m_bEnableUndoLimit)
		{
			MaxUndoSize = 0;
		}

		LimitUndoRedo( & m_UndoList, MaxUndoDepth, MaxUndoSize);
	}
	else
	{
		m_RedoList.InsertHead(pContext);

		// free extra redo, if count or size limit is exceeded
		int MaxRedoDepth = pApp->m_MaxRedoDepth;
		if ( ! pApp->m_bEnableRedoDepthLimit)
		{
			MaxRedoDepth = 0;
		}
		int MaxRedoSize = pApp->m_MaxRedoSize;
		if ( ! pApp->m_bEnableRedoLimit)
		{
			MaxRedoSize = 0;
		}

		LimitUndoRedo( & m_RedoList, MaxRedoDepth, MaxRedoSize);
	}
}

void CWaveSoapFrontDoc::OnUpdateSoundPlay(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanReadFile());
}

void CWaveSoapFrontDoc::OnSoundPlay()
{
	if (CanReadFile())
	{
		CThisApp * pApp = GetApp();

		CSoundPlayContext * pContext = new CSoundPlayContext(this,
															m_WavFile,
															m_SelectionStart, m_SelectionEnd, m_SelectedChannel,
															pApp->m_DefaultPlaybackDevice,
															pApp->m_NumPlaybackBuffers,
															pApp->m_SizePlaybackBuffers);

		if (NULL != pContext)
		{
			m_PlayingSound = true;
			pContext->Execute();
		}
	}
}

void CWaveSoapFrontDoc::OnUpdateSoundStop(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_PlayingSound);
}

void CWaveSoapFrontDoc::OnUpdateIndicatorFileSize(CCmdUI* pCmdUI)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	SetStatusString(pCmdUI,
					SampleToString(WaveFileSamples(), WaveSampleRate(),
									GetApp()->m_SoundTimeFormat));
}

void CWaveSoapFrontDoc::OnUpdateIndicatorSelectionLength(CCmdUI* pCmdUI)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	if (m_SelectionStart != m_SelectionEnd)
	{
		SetStatusString(pCmdUI,
						SampleToString(m_SelectionEnd - m_SelectionStart, WaveSampleRate(),
										GetApp()->m_SoundTimeFormat));
	}
	else
	{
		SetStatusString(pCmdUI, _T("            "));
	}
}

void CWaveSoapFrontDoc::OnUpdateIndicatorCurrentPos(CCmdUI* pCmdUI)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}

	CSoundPlayContext * pCx = NULL;

	m_OpList.Lock();
	if (m_PlayingSound && ! m_OpList.IsEmpty())
	{
		pCx = dynamic_cast<CSoundPlayContext *>(m_OpList.First());
	}
	m_OpList.Unlock();

	CString s;
	int TimeFormat = GetApp()->m_SoundTimeFormat;
	if (m_PlayingSound && NULL != pCx)
	{
		s = pCx->GetPlaybackTimeString(TimeFormat);
	}
	else
	{
		s = SampleToString(m_CaretPosition, WaveSampleRate(), TimeFormat);
	}
	SetStatusString(pCmdUI, s);
}

void CWaveSoapFrontDoc::OnUpdateIndicatorSampleRate(CCmdUI* pCmdUI)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	SetStatusString(pCmdUI, LtoaCS(WaveSampleRate()));
}

void CWaveSoapFrontDoc::OnUpdateIndicatorSampleSize(CCmdUI* pCmdUI)
{
	SetStatusString(pCmdUI, _T("16-bit"));
}

void CWaveSoapFrontDoc::OnUpdateIndicatorChannels(CCmdUI* pCmdUI)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	SetStatusString(pCmdUI,
					(WaveChannels() == 1) ? _T("Mono") : _T("Stereo"));
}

void CWaveSoapFrontDoc::OnSoundStop()
{
	if (m_PlayingSound)
	{
		m_StopOperation = true;
	}
}

void CWaveSoapFrontDoc::OnPlayAndStop()
{
	if ( ! IsBusy())
	{
		OnSoundPlay();
	}
	else if (m_PlayingSound)
	{
		m_StopOperation = true;
	}
}

void CWaveSoapFrontDoc::OnStopAll()
{
	m_StopOperation = true;
}

void CWaveSoapFrontDoc::OnSoundPause()
{
	if (m_PlayingSound)
	{
		CSoundPlayContext * pCx = NULL;
		m_OpList.Lock();

		if ( ! m_OpList.IsEmpty())
		{
			pCx = dynamic_cast<CSoundPlayContext *>(m_OpList.First());
			if (pCx)
			{
				// TODO: Make sure to protect m_OpList
				pCx->Pause();
			}
		}
		m_OpList.Unlock();
		m_StopOperation = true;
	}
}

void CWaveSoapFrontDoc::OnUpdateSoundPause(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_PlayingSound);
}

BOOL CWaveSoapFrontDoc::SaveModified()
{
	if (IsBusy())
	{
		if (m_OperationNonCritical)
		{
			m_StopOperation = TRUE;
			DWORD BeginTime = GetTickCount();
			do {
				WaitForSingleObjectAcceptSends(m_Thread.m_hThread, 50);
				OnIdle();
			}
			while (IsBusy() && GetTickCount() - BeginTime < 5000);
			if (IsBusy())
			{
				return false;
			}
		}
		else
		{
			return FALSE;
		}
	}
	return CDocument::SaveModified();
}

void CWaveSoapFrontDoc::DeleteUndo()
{
	while ( ! m_UndoList.IsEmpty())
	{
		delete m_UndoList.RemoveHead();
	}
}

void CWaveSoapFrontDoc::DeleteRedo()
{
	while ( ! m_RedoList.IsEmpty())
	{
		delete m_RedoList.RemoveHead();
	}
}

BOOL CWaveSoapFrontDoc::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	OnIdle();   // retire all contexts
	if (m_bCloseThisDocumentNow)
	{
		return FALSE;   // we are dead, don't accept anything
	}
	try
	{
		return BaseClass::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	}
	catch(std::bad_alloc)
	{
		NotEnoughMemoryMessageBox();
		return TRUE;    // eat the command
	}
}

inline BOOL CWaveSoapFrontDoc::IsModified()
{
	return m_ModificationSequenceNumber != 0;
}

void CWaveSoapFrontDoc::IncrementModified(BOOL bDeleteRedo, int KeepPreviousUndo)
{
	// bDeleteRedo is FALSE for Redo command ONLY
	BOOL OldModified = IsModified();
	// Modification sequence number may become zero only after Redo operation
	// if bDeleteRedo, don't allow it to become 0 from -1.
	if (bDeleteRedo)
	{
		// make sure it will never become 0, as there is no way anymore
		// to restore the original
		if (m_ModificationSequenceNumber < 0)
		{
			m_ModificationSequenceNumber = 0x8000;
		}
		else
		{
			m_ModificationSequenceNumber++;
		}
		DeleteRedo();
	}
	else
	{
		m_ModificationSequenceNumber++;
	}
	if (-1 == KeepPreviousUndo)
	{
		KeepPreviousUndo = UndoEnabled();
	}
	if (! KeepPreviousUndo)
	{
		// since it is unable to restore undo, delete all previous undo
		DeleteUndo();
	}

	if (IsModified() != OldModified)
	{
		UpdateFrameTitles();        // will cause name change in views
	}
}

void CWaveSoapFrontDoc::DecrementModified()   // called at UNDO
{
	BOOL OldModified = IsModified();
	m_ModificationSequenceNumber--;

	if (! RedoEnabled())
	{
		// since it is unable to redo, delete all previous redo
		DeleteRedo();
	}

	if (IsModified() != OldModified)
	{
		UpdateFrameTitles();        // will cause name change in views
	}
}

void CWaveSoapFrontDoc::SetModifiedFlag(BOOL bModified, int KeepPreviousUndo)
{
	if (bModified)
	{
		IncrementModified(TRUE, KeepPreviousUndo);  // don't keep redo
	}
	else
	{
		BOOL OldModified = IsModified();
		m_ModificationSequenceNumber = 0;
		if (OldModified)
		{
			UpdateFrameTitles();        // will cause name change in views
		}
	}
}

void CWaveSoapFrontDoc::PlaybackPositionNotify(SAMPLE_INDEX position, CHANNEL_MASK channel)
{
	QueueSoundUpdate(UpdatePlaybackPositionChanged, WaveFileID(),
					position, channel,
					-1, QueueSoundUpdateReplace);
}

// return TRUE if there was UpdatePlaybackPositionChanged request queued
// return FALSE otherwise
BOOL CWaveSoapFrontDoc::ProcessUpdatePlaybackPosition()
{
	BOOL result = FALSE;
	m_UpdateList.Lock();

	for (CSoundUpdateInfo * pEntry = m_UpdateList.First()
		; m_UpdateList.NotEnd(pEntry); )
	{
		CSoundUpdateInfo * next = m_UpdateList.Next(pEntry);

		if (pEntry->m_UpdateCode == UpdatePlaybackPositionChanged)
		{
			m_UpdateList.RemoveEntryUnsafe(pEntry);
			m_UpdateList.Unlock();

			UpdateAllViews(NULL, pEntry->m_UpdateCode, pEntry);

			delete pEntry;
			result = TRUE;
			m_UpdateList.Lock();
		}

		pEntry = next;
	}

	m_UpdateList.Unlock();
	return result;
}
void CWaveSoapFrontDoc::OnEditChannelsLock()
{
	if (WaveChannels() < 2)
	{
		return;
	}
	m_bChannelsLocked = ! m_bChannelsLocked;
}

void CWaveSoapFrontDoc::OnUpdateEditChannelsLock(CCmdUI* pCmdUI)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	if (WaveChannels() < 2)
	{
		pCmdUI->Enable(FALSE);
		pCmdUI->SetCheck(FALSE);
	}
	else
	{
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bChannelsLocked);
	}
}

void CWaveSoapFrontDoc::EnableUndo(BOOL bEnable)
{
	m_bUndoEnabled = (FALSE != bEnable);
}

void CWaveSoapFrontDoc::EnableRedo(BOOL bEnable)
{
	m_bRedoEnabled = (FALSE != bEnable);
}


void CWaveSoapFrontDoc::OnSample16bit()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateSample16bit(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here

}

void CWaveSoapFrontDoc::OnSample8bit()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateSample8bit(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here

}

void CWaveSoapFrontDoc::SetSampleRate(unsigned SampleRate)
{
	if (m_bReadOnly)
	{
		return;
	}
	WAVEFORMATEX * pWf = WaveFormat();
	if (WaveSampleRate() == SampleRate)
	{
		return;
	}
	CWaveFormat wf;
	wf.InitFormat(WAVE_FORMAT_PCM, SampleRate, WaveChannels());

	CReplaceFormatContext * pContext = new CReplaceFormatContext(this, _T("Sample Rate Change"), pWf);
	if (UndoEnabled())
	{
		pContext->CreateUndo();
	}

	pContext->Execute();
	SetModifiedFlag();
}

void CWaveSoapFrontDoc::OnUpdateSampleRate(CCmdUI* pCmdUI, unsigned SampleRate)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	pCmdUI->SetRadio(WaveSampleRate() == SampleRate);
}

void CWaveSoapFrontDoc::OnSamplerate11025()
{
	SetSampleRate(11025);
}

void CWaveSoapFrontDoc::OnUpdateSamplerate11025(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 11025);
}

void CWaveSoapFrontDoc::OnSamplerate16000()
{
	SetSampleRate(16000);
}

void CWaveSoapFrontDoc::OnUpdateSamplerate16000(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 16000);
}

void CWaveSoapFrontDoc::OnSamplerate22050()
{
	SetSampleRate(22050);
}

void CWaveSoapFrontDoc::OnUpdateSamplerate22050(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 22050);
}

void CWaveSoapFrontDoc::OnSamplerate32000()
{
	SetSampleRate(32000);
}

void CWaveSoapFrontDoc::OnUpdateSamplerate32000(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 32000);
}

void CWaveSoapFrontDoc::OnSamplerate44100()
{
	SetSampleRate(44100);
}

void CWaveSoapFrontDoc::OnUpdateSamplerate44100(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 44100);
}

void CWaveSoapFrontDoc::OnSamplerate48k()
{
	SetSampleRate(48000);
}

void CWaveSoapFrontDoc::OnUpdateSamplerate48k(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 48000);
}

void CWaveSoapFrontDoc::OnSamplerate7200()
{
	SetSampleRate(7200);
}

void CWaveSoapFrontDoc::OnUpdateSamplerate7200(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 7200);
}

void CWaveSoapFrontDoc::OnSamplerate8000()
{
	SetSampleRate(8000);
}

void CWaveSoapFrontDoc::OnUpdateSamplerate8000(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 8000);
}

void CWaveSoapFrontDoc::OnSamplerate96k()
{
	SetSampleRate(96000);
}

void CWaveSoapFrontDoc::OnUpdateSamplerate96k(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 96000);
}

void CWaveSoapFrontDoc::ChangeChannels(NUMBER_OF_CHANNELS nChannels)
{
	if (IsReadOnly()
		|| ! m_WavFile.IsOpen()
		|| nChannels == WaveChannels())
	{
		return;
	}
	WAVEFORMATEX * pWf = WaveFormat();
	NUMBER_OF_SAMPLES SampleCount = WaveFileSamples();

	CWaveFormat NewFormat;
	NewFormat.InitFormat(WAVE_FORMAT_PCM, WaveSampleRate(), nChannels);
	if (0 == SampleCount)
	{
		// easy change

		CReplaceFormatContext * pContext = new CReplaceFormatContext(this,
												_T("Number Of Channels Change"), NewFormat);
		if (NULL == pContext)
		{
			NotEnoughMemoryMessageBox();
			return;
		}
		if (UndoEnabled())
		{
			pContext->CreateUndo();
		}

		pContext->Execute();

		SetModifiedFlag();
		return;
	}

	CHANNEL_MASK nSrcChan = ALL_CHANNELS;

	if ( ! CanAllocateWaveFileSamplesDlg(NewFormat, SampleCount))
	{
		return;
	}

	if (nChannels < WaveChannels())
	{
		CCopyChannelsSelectDlg dlg(m_PrevChannelToCopy);

		if (IDOK != dlg.DoModal())
		{
			return;
		}
		nSrcChan = dlg.GetChannelToCopy();
		m_PrevChannelToCopy = nSrcChan;
	}
	else
	{
		// if mono->Stereo, duplicate the channels
		if (IDYES != AfxMessageBox(IDS_MONO_TO_STEREO, MB_YESNO | MB_ICONQUESTION))
		{
			return;
		}
	}

	CStagedContext::auto_ptr pContext(new CStagedContext(this,
														_T("Changing number of channels..."), 0,
														_T("Number Of Channels Change")));

	if (NULL == pContext.get())
	{
		NotEnoughMemoryMessageBox();
		return;
	}
	CConversionContext * pConversionContext = new CConversionContext(this, _T("Changing number of channels..."),
												_T("Number Of Channels Change"));

	if (NULL == pConversionContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	pContext->AddContext(pConversionContext);

	CChannelConvertor * pConvert =
		new CChannelConvertor(WaveChannels(), nChannels, nSrcChan);
	if (NULL == pConvert)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	pConversionContext->AddWaveProc(pConvert);

	CReplaceFileContext * pReplaceFile =
		new CReplaceFileContext(this, _T(""), m_WavFile, false);
	if (NULL == pReplaceFile)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	pContext->AddContext(pReplaceFile);

	// create new temporary file
	CWaveFile DstFile;
	if ( ! DstFile.CreateWaveFile( & m_WavFile, NewFormat, ALL_CHANNELS, SampleCount,
									CreateWaveFileDeleteAfterClose
									| CreateWaveFilePcmFormat
									| CreateWaveFileTemp,
									NULL))
	{
		FileCreationErrorMessageBox(NULL);
		return;
	}

	pConversionContext->InitDestination(DstFile, 0, SampleCount, ALL_CHANNELS, FALSE);
	pConversionContext->InitSource(m_WavFile, 0, SampleCount, ALL_CHANNELS);

	if (UndoEnabled())
	{
		pContext->CreateUndo();
	}
	pContext.release()->Execute();
}

void CWaveSoapFrontDoc::OnChannelsMono()
{
	ChangeChannels(1);
}

void CWaveSoapFrontDoc::OnUpdateChannelsMono(CCmdUI* pCmdUI)
{
	if (m_WavFile.IsOpen())
	{
		pCmdUI->SetRadio(WaveChannels() == 1);
	}
}

void CWaveSoapFrontDoc::OnChannelsStereo()
{
	ChangeChannels(2);
}

void CWaveSoapFrontDoc::OnUpdateChannelsStereo(CCmdUI* pCmdUI)
{
	if (m_WavFile.IsOpen())
	{
		pCmdUI->SetRadio(WaveChannels() == 2);
	}
}

void CWaveSoapFrontDoc::OnProcessChangevolume()
{
	if ( ! CanModifyFile())
	{
		return;
	}

	SAMPLE_INDEX start = m_SelectionStart;
	SAMPLE_INDEX end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}

	CVolumeChangeDialog dlg(start, end, m_CaretPosition, GetSelectedChannel(),
							m_WavFile,
							m_bChannelsLocked, UndoEnabled(), GetApp()->m_SoundTimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	double const Volume[MAX_NUMBER_OF_CHANNELS] =
	{
		dlg.GetLeftVolume(),
		dlg.GetRightVolume(),
	};

	if (1. == Volume[0]
		&& 1. == Volume[1])
	{
		return;
	}

	CVolumeChangeContext::auto_ptr pContext(new CVolumeChangeContext(this, _T("Changing volume..."),
												_T("Volume Change"), Volume, m_WavFile.Channels()));

	if (NULL == pContext.get())
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	// check if the values can exceed the maximum
	WavePeak LeftPeak, RightPeak;

	GetSoundMinMax(LeftPeak, RightPeak, dlg.GetStart(), dlg.GetEnd());

	long MinL = LeftPeak.low;
	long MaxL = LeftPeak.high;
	long MinR = RightPeak.low;
	long MaxR = RightPeak.high;

	if (MinL < 0)
	{
		MinL = - MinL;
	}
	if (MaxL < 0)
	{
		MaxL = - MaxL;
	}
	if (MaxL < MinL) MaxL = MinL;

	if (MinR < 0)
	{
		MinR = - MinR;
	}
	if (MaxR < 0)
	{
		MaxR = - MaxR;
	}
	if (MaxR < MinR) MaxR = MinR;

	if (((dlg.GetChannel() & SPEAKER_FRONT_LEFT) && Volume[0] * MaxL > 32767.)
		|| (WaveChannels() > 1 && (dlg.GetChannel() & SPEAKER_FRONT_RIGHT)
			&& Volume[1] * MaxR > 32767.))
	{
		CString s;
		s.Format(IDS_SOUND_MAY_BE_CLIPPED, GetTitle());
		if (IDYES != AfxMessageBox(s, MB_YESNO | MB_ICONQUESTION))
		{
			return;
		}
	}

	if ( ! pContext->InitDestination(m_WavFile, dlg.GetStart(),
									dlg.GetEnd(), dlg.GetChannel(), dlg.UndoEnabled()))
	{
		return;
	}

	pContext.release()->Execute();
	SetModifiedFlag(TRUE, dlg.UndoEnabled());
}

void CWaveSoapFrontDoc::GetSoundMinMax(WavePeak & Left,
										WavePeak & Right,
										long begin, long end)
{
	unsigned Granularity = m_WavFile.GetPeakGranularity();
	unsigned PeakBegin = (begin & ~Granularity) / Granularity;
	unsigned PeakEnd = ((end + Granularity - 1) & ~Granularity) / Granularity;

	if (0 == m_WavFile.GetPeaksSize())
	{
		Left.low = 0;
		Left.high = 0;
		Right.low = 0;
		Right.high = 0;
		return;
	}

	NUMBER_OF_CHANNELS Channels = WaveChannels();

	if (Channels == 1)
	{
		Left = m_WavFile.GetPeakMinMax(PeakBegin, PeakEnd);
	}
	else if (Channels == 2)
	{
		Left = m_WavFile.GetPeakMinMax(PeakBegin * 2, PeakEnd * 2, 2);
		Right = m_WavFile.GetPeakMinMax(1 + PeakBegin * 2, 1 + PeakEnd * 2, 2);
	}
	else
	{
		ASSERT(0);  // TODO
	}
}

void CWaveSoapFrontDoc::OnUpdateProcessChangevolume(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanModifyFile());
}

void CWaveSoapFrontDoc::OnViewStatusHhmmss()
{
	GetApp()->m_SoundTimeFormat =
		SampleToString_HhMmSs | TimeToHhMmSs_NeedsMs | TimeToHhMmSs_NeedsHhMm;
}

void CWaveSoapFrontDoc::OnViewStatusSamples()
{
	GetApp()->m_SoundTimeFormat = SampleToString_Sample;
}

void CWaveSoapFrontDoc::OnViewStatusSeconds()
{
	GetApp()->m_SoundTimeFormat =
		SampleToString_Seconds | TimeToHhMmSs_NeedsMs;
}

void CWaveSoapFrontDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU)
{
	// store the path fully qualified
	TCHAR szFullPath[_MAX_PATH];
	AfxFullPath(szFullPath, lpszPathName);

	m_strPathName = szFullPath;
	ASSERT(!m_strPathName.IsEmpty());       // must be set to something

	m_bEmbedded = FALSE;
	ASSERT_VALID(this);

	// set the document title based on path name
	TCHAR szTitle[_MAX_FNAME];
	if (AfxGetFileTitle(szFullPath, szTitle, _MAX_FNAME) == 0)
		SetTitle(szTitle);

	if (bAddToMRU)
	{
		// add it to the file MRU list
		// first byte is a flag byte
		TCHAR flag = 1;
		if (m_bReadOnly)
		{
			flag |= OpenDocumentReadOnly;
		}
		else if (m_bDirectMode)
		{
			flag |= OpenDocumentDirectMode;
		}

		GetApp()->AddToRecentFileList(CString(flag, 1) + m_strPathName);
	}

	ASSERT_VALID(this);
}

void CWaveSoapFrontDoc::OnUpdateProcessDcoffset(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanModifyFile());
}

void CWaveSoapFrontDoc::OnProcessDcoffset()
{
	if ( ! CanModifyFile())
	{
		return;
	}

	SAMPLE_INDEX start = m_SelectionStart;
	SAMPLE_INDEX end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}

	CThisApp * pApp = GetApp();
	CDcOffsetDialog dlg(start, end, m_CaretPosition, GetSelectedChannel(),
						m_WavFile, ChannelsLocked(), UndoEnabled(), GetApp()->m_SoundTimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	COperationContext * pContext = NULL;
	CDcOffsetContext::auto_ptr pDcContext;

	if (dlg.NeedToCalculateDcOffset())
	{
		CStagedContext::auto_ptr pStagedContext
		(new CStagedContext(this, _T("Adjusting DC..."), 0, _T("DC Adjust")));

		CDcScanContext * pScanContext =
			new CDcScanContext(this, _T("Scanning for DC offset..."), _T(""));

		pStagedContext->AddContext(pScanContext);

		SAMPLE_INDEX EndScanSample = dlg.GetEnd();
		if (dlg.ScanOnly5Seconds())
		{
			// 5.4 seconds is actually scanned, to compensate for turntable rotation
			EndScanSample = dlg.GetStart()
							+ 54 * WaveSampleRate() / 10;
			if (EndScanSample > dlg.GetEnd())
			{
				EndScanSample = dlg.GetEnd();
			}
		}

		pScanContext->InitDestination(m_WavFile, dlg.GetStart(),
									EndScanSample, dlg.GetChannel(), FALSE);

		pDcContext.reset(new CDcOffsetContext(this, _T("Adjusting DC..."),
											_T("DC Adjust"), pScanContext));

		if (NULL == pDcContext.get())
		{
			NotEnoughMemoryMessageBox();
			return;
		}

		if ( ! pDcContext->InitDestination(m_WavFile, dlg.GetStart(),
											dlg.GetEnd(), dlg.GetChannel(), dlg.UndoEnabled()))
		{
			return;
		}

		pStagedContext->AddContext(pDcContext.release());
		// staged operation becomes main context
		pContext = pStagedContext.release();
	}
	else // Use specified DC offset
	{
		int DcOffset = dlg.GetDcOffset();
		if (0 == DcOffset)
		{
			// nothing to do
			return;
		}

		// check if the values can exceed the maximum

		WavePeak LeftPeak, RightPeak;
		GetSoundMinMax(LeftPeak, RightPeak, dlg.GetStart(), dlg.GetEnd());

		long MinL = LeftPeak.low + DcOffset;
		long MaxL = LeftPeak.high + DcOffset;
		long MinR = RightPeak.low + DcOffset;
		long MaxR = RightPeak.high + DcOffset;

		if (((dlg.GetChannel() & SPEAKER_FRONT_LEFT) && (MaxL > 0x7FFF || MinL < -0x8000))
			|| (WaveChannels() > 1 && (dlg.GetChannel() & SPEAKER_FRONT_RIGHT)
				&& (MaxR > 0x7FFF || MinR < -0x8000)))
		{
			CString s;
			s.Format(IDS_SOUND_MAY_BE_CLIPPED, GetTitle());
			if (IDYES != AfxMessageBox(s, MB_YESNO | MB_ICONQUESTION))
			{
				return;
			}
		}

		int offset[2] = { DcOffset, DcOffset, };

		pDcContext.reset(new CDcOffsetContext(this, _T("Adjusting DC..."),
											_T("DC Adjust"), offset));

		if (NULL == pDcContext.get())
		{
			NotEnoughMemoryMessageBox();
			return;
		}

		if ( ! pDcContext->InitDestination(m_WavFile, dlg.GetStart(),
											dlg.GetEnd(), dlg.GetChannel(), dlg.UndoEnabled()))
		{
			return;
		}

		pContext = pDcContext.release();
	}

	pContext->Execute();
	SetModifiedFlag(TRUE, dlg.UndoEnabled());
}

void CWaveSoapFrontDoc::OnUpdateProcessInsertsilence(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanWriteFile());
}

void CWaveSoapFrontDoc::OnProcessInsertsilence()
{
	if ( ! CanWriteFile())
	{
		// don't do anything
		return;
	}
	if (m_SelectionStart != m_SelectionEnd)
	{
		CSilenceOptionDialog dlg1;
		switch (dlg1.DoModal())
		{
		case IDOK:
			OnProcessMute();
			return;
			break;
		default:
		case IDCANCEL:
			// do nothing
			return;
			break;
		case IDC_BUTTON_SILENCE:
			// go on
			break;
		}
	}

	CInsertSilenceDialog dlg(m_SelectionStart,
							m_SelectionEnd - m_SelectionStart,
							GetSelectedChannel(),
							m_WavFile, GetApp()->m_SoundTimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	//GetApp()->m_SoundTimeFormat = dlg.m_TimeFormat;

	if (dlg.GetLength() <= 0)
	{
		// nothing to change
		return;
	}

	if ( ! CanExpandWaveFileDlg(m_WavFile, dlg.GetLength()))
	{
		return;
	}

	CStagedContext::auto_ptr pContext(new CStagedContext(this,
														_T("Inserting silence..."), OperationContextDiskIntensive,
														_T("Insert Silence")));

	if ( ! InitExpandOperation(pContext.get(), m_WavFile, dlg.GetStart(),
								dlg.GetLength(), dlg.GetChannel()))
	{
		return;
	}

	pContext->AddContext(new CInitChannelsUndo(this, dlg.GetStart(),
												dlg.GetLength(), dlg.GetChannel()));

	pContext->CreateUndo();

	pContext.release()->Execute();
	SetModifiedFlag(TRUE, TRUE);    // keep previous undo
}

void CWaveSoapFrontDoc::OnUpdateProcessMute(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanModifySelection());
}

void CWaveSoapFrontDoc::OnProcessMute()
{
	if ( ! CanModifySelection())
	{
		// don't do anything
		return;
	}

	CVolumeChangeContext::auto_ptr pContext
	(new CVolumeChangeContext(this, _T("Muting the selection..."),
							_T("Mute"), 0.));

	if ( ! pContext->InitDestination(m_WavFile, m_SelectionStart,
									m_SelectionEnd, GetSelectedChannel(), UndoEnabled()))
	{
		return;
	}

	pContext.release()->Execute();
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::OnUpdateProcessNormalize(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanModifyFile());
}

void CWaveSoapFrontDoc::OnProcessNormalize()
{
	if ( ! CanModifyFile())
	{
		return;
	}

	SAMPLE_INDEX start = m_SelectionStart;
	SAMPLE_INDEX end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}

	CNormalizeSoundDialog dlg(start, end, m_CaretPosition, GetSelectedChannel(),
							m_WavFile, ChannelsLocked(), UndoEnabled(), GetApp()->m_SoundTimeFormat);

	//if (1 == WaveChannels())
	//{
	//dlg.SetTemplate(IDD_DIALOG_VOLUME_CHANGE_MONO);
	//}

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	CStagedContext::auto_ptr pContext(new CStagedContext(this,
														_T("Normalizing sound level..."), 0, _T("Normalize")));
	if (NULL == pContext.get())
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	CMaxScanContext * pStatContext =
		new CMaxScanContext(this, _T("Scanning for maximum amplitude..."), _T(""));

	pStatContext->InitDestination(m_WavFile, dlg.GetStart(),
								dlg.GetEnd(), dlg.GetChannel(), FALSE);

	if (NULL == pStatContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}
	pContext->AddContext(pStatContext);

	CNormalizeContext * pNormContext =
		new CNormalizeContext(this,
							_T("Normalizing sound level..."),
							_T("Normalize"), dlg.GetLimitLevel(),
							dlg.ChannelsLocked(), pStatContext);

	if (NULL == pNormContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	pContext->AddContext(pNormContext);

	if ( ! pNormContext->InitDestination(m_WavFile, dlg.GetStart(),
										dlg.GetEnd(), dlg.GetChannel(), dlg.UndoEnabled()))
	{
		return;
	}

	pContext.release()->Execute();
	SetModifiedFlag(TRUE, dlg.UndoEnabled());
}

void CWaveSoapFrontDoc::OnUpdateProcessResample(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanWriteFile());
}

void CWaveSoapFrontDoc::OnProcessResample()
{
	if ( ! CanWriteFile())
	{
		return;
	}

	CResampleDialog dlg(UndoEnabled(), WaveSampleRate(), WaveFileSamples() == 0);

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	double ResampleQuality = 40.;

	long OldSamplingRate = WaveSampleRate();
	long NewSamplingRate;
	double ResampleRatio;

	NewSamplingRate = dlg.NewSampleRate();
	ResampleRatio = dlg.ResampleRatio();

	if (dlg.ChangeRateOnly())
	{
		SetSampleRate(NewSamplingRate);
		return;
	}

	LONGLONG NewSampleCount =
		UInt32x32To64(WaveFileSamples(), NewSamplingRate) / OldSamplingRate;
	if ( ! CanAllocateWaveFileSamplesDlg(WaveFormat(), NewSampleCount))
	{
		return;
	}

	if (! dlg.ChangeSampleRate())
	{
		NewSamplingRate = WaveSampleRate();
	}
	// create new temporary file
	CWaveFile DstFile;
	CWaveFormat NewFormat;
	NewFormat.InitFormat(WAVE_FORMAT_PCM, NewSamplingRate, WaveChannels());

	if ( ! DstFile.CreateWaveFile( & m_WavFile, NewFormat, ALL_CHANNELS, ULONG(NewSampleCount),
									//CreateWaveFileTempDir |
									CreateWaveFileDeleteAfterClose
									| CreateWaveFilePcmFormat
									| CreateWaveFileTemp,
									NULL))
	{
		FileCreationErrorMessageBox(NULL);
		return;
	}

	CStagedContext::auto_ptr pContext
	(new CStagedContext(this, _T("Changing sample rate..."), 0, _T("Resample")));

	if (NULL == pContext.get())
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	CResampleContext * pResampleContext = new CResampleContext(this,
																_T("Changing sample rate..."), _T("Resample"),
																m_WavFile, DstFile, ResampleRatio, ResampleQuality);

	if (NULL == pResampleContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	pContext->AddContext(pResampleContext);

	CReplaceFileContext * pReplace = new CReplaceFileContext(this, _T("Resample"), m_WavFile, false);
	if (NULL == pReplace)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	pContext->AddContext(pReplace);
	if (dlg.UndoEnabled())
	{
		pReplace->CreateUndo();
	}

	pContext.release()->Execute();
	// don't mark the file as modified yet
	//SetModifiedFlag();
}

void CWaveSoapFrontDoc::OnFileStatistics()
{
	// disable for zero length file
	if ( ! CanReadFile())
	{
		return;
	}
	CStatisticsContext * pContext = new CStatisticsContext(this, _T("Scanning the file for statistics..."), _T(""));
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}
	SAMPLE_INDEX begin = m_SelectionStart;
	SAMPLE_INDEX end = m_SelectionEnd;
	if (begin >= end)
	{
		begin = 0;
		end = WaveFileSamples();
	}
	pContext->InitDestination(m_WavFile, begin, end, ALL_CHANNELS, FALSE);
	pContext->Execute();
}

void CWaveSoapFrontDoc::OnUpdateFileStatistics(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanReadFile());
}

void CWaveSoapFrontDoc::OnUpdateEditGoto(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanReadFile());
}

void CWaveSoapFrontDoc::OnEditGoto()
{
	if ( ! CanReadFile())
	{
		return;
	}

	CGotoDialog dlg(m_CaretPosition, m_WavFile, GetApp()->m_SoundTimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	//GetApp()->m_SoundTimeFormat = dlg.m_TimeFormat;
	SetSelection(dlg.GetSelectedPosition(), dlg.GetSelectedPosition(),
				m_SelectedChannel, dlg.GetSelectedPosition(), SetSelection_MoveCaretToCenter);
}

void CWaveSoapFrontDoc::OnUpdateProcessInvert(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanModifyFile());
}

void CWaveSoapFrontDoc::OnProcessInvert()
{
	if ( ! CanModifyFile())
	{
		return;
	}

	CVolumeChangeContext::auto_ptr pContext
	(new CVolumeChangeContext(this, _T("Inverting the waveform..."),
							_T("Inversion"), -1.));

	SAMPLE_INDEX start = m_SelectionStart;
	SAMPLE_INDEX end = m_SelectionEnd;

	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}

	if ( ! pContext->InitDestination(m_WavFile, start,
									end, GetSelectedChannel(), UndoEnabled()))
	{
		return;
	}

	pContext.release()->Execute();
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::OnUpdateViewRescanPeaks(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanReadFile());
}

void CWaveSoapFrontDoc::OnViewRescanPeaks()
{
	if ( ! CanReadFile())
	{
		return;
	}
	BuildPeakInfo(FALSE);   // don't save it right now
}

void CWaveSoapFrontDoc::OnUpdateProcessSynthesisExpressionEvaluation(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanWriteFile());
}

void CWaveSoapFrontDoc::OnProcessSynthesisExpressionEvaluation()
{
	if ( ! CanWriteFile())
	{
		return;
	}
	SAMPLE_INDEX start = m_SelectionStart;
	SAMPLE_INDEX end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}

	CExpressionEvaluationContext * pContext =
		new CExpressionEvaluationContext(this, _T("Calculating the waveform..."),
										_T("Expression evaluation"));

	CExpressionEvaluationDialog dlg(start, end, m_CaretPosition, GetSelectedChannel(),
									m_WavFile, ChannelsLocked(), UndoEnabled(), GetApp()->m_SoundTimeFormat,
									pContext);

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	pContext = dlg.GetExpressionContext();

	if (NULL != pContext)
	{
		pContext->Execute();
		SetModifiedFlag(TRUE, dlg.UndoEnabled());
	}
}

void CWaveSoapFrontDoc::OnUpdateViewStatusHhmmss(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio((GetApp()->m_SoundTimeFormat & SampleToString_Mask)
					== SampleToString_HhMmSs);
}

void CWaveSoapFrontDoc::OnUpdateViewStatusSamples(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio((GetApp()->m_SoundTimeFormat & SampleToString_Mask)
					== SampleToString_Sample);
}

void CWaveSoapFrontDoc::OnUpdateViewStatusSeconds(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio((GetApp()->m_SoundTimeFormat & SampleToString_Mask)
					== SampleToString_Seconds);
}

void CWaveSoapFrontDoc::OnFileSaveCopyAs()
{
	DoSave(NULL, FALSE);
}

BOOL CWaveSoapFrontDoc::OpenNonWavFileDocument(LPCTSTR lpszPathName, int flags)
{
	if (flags & OpenDocumentWmaFile)
	{
		m_FileTypeFlags = OpenDocumentWmaFile;
		return OpenWmaFileDocument(lpszPathName);
	}
	if (flags & OpenDocumentMp3File)
	{
		m_FileTypeFlags = OpenDocumentMp3File;
		return OpenMp3FileDocument(lpszPathName);
	}
	if (flags & OpenDocumentRawFile)
	{
		m_FileTypeFlags = OpenDocumentRawFile;
		return OpenRawFileDocument(lpszPathName);
	}
	if (flags & OpenDocumentAviFile)
	{
		m_FileTypeFlags = OpenDocumentAviFile;
		return OpenAviFileDocument(lpszPathName);
	}
	return FALSE;
}

BOOL CWaveSoapFrontDoc::OpenRawFileDocument(LPCTSTR lpszPathName)
{
	// open source file,
	if ( ! m_OriginalWavFile.Open(lpszPathName,
								MmioFileOpenExisting
								| MmioFileOpenReadOnly
								| MmioFileOpenDontLoadRiff))
	{
		CString s;
		UINT format;
		switch(GetLastError())
		{
		case ERROR_ACCESS_DENIED:
			format = IDS_FILE_OPEN_ACCESS_DENIED;
			break;
		case ERROR_SHARING_VIOLATION:
			format = IDS_FILE_OPEN_SHARING_VIOLATION;
			break;
		default:
			format = IDS_UNABLE_TO_OPEN_WMA_FILE;
			break;
		}
		s.Format(format, lpszPathName);
		AfxMessageBox(s, MB_ICONEXCLAMATION | MB_OK);
		return FALSE;
	}

	MEDIA_FILE_SIZE RawFileSize = m_OriginalWavFile.GetLength();
	CRawFileParametersDlg dlg(RawFileSize);

	if (IDOK != dlg.DoModal())
	{
		return FALSE;
	}

	WAVEFORMATEX wf;
	wf.cbSize = 0;
	wf.nChannels = dlg.NumberOfChannels();

	wf.wBitsPerSample = dlg.NumberOfBits();

	wf.nSamplesPerSec = dlg.SamplingRate();

	wf.wFormatTag = dlg.GetFormatTag();

	wf.nBlockAlign = wf.wBitsPerSample * wf.nChannels / 8;
	wf.nAvgBytesPerSec = wf.nBlockAlign * wf.nSamplesPerSec;

	LONG nNewFileSamples = (DWORD(RawFileSize) - dlg.HeaderLength() - dlg.TrailerLength())
							/ wf.nBlockAlign;

	if (! m_WavFile.CreateWaveFile( NULL, & wf, ALL_CHANNELS,
									nNewFileSamples,
									CreateWaveFileDeleteAfterClose
									| CreateWaveFileTempDir
									| CreateWaveFileTemp
									| CreateWaveFilePcmFormat,
									NULL))
	{
		AfxMessageBox(IDS_UNABLE_TO_CREATE_TEMPORARY_FILE, MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (16 == wf.wBitsPerSample)
	{
		if (dlg.MsbFirst())
		{
			m_FileTypeFlags |= OpenRawFileMsbFirst;
		}
	}

	m_OriginalWaveFormat = & wf;

	m_WavFile.AllocatePeakData(nNewFileSamples);

	CDecompressContext * pContext =
		new CDecompressContext(this, _T("Loading the raw sound file..."),
								m_OriginalWavFile,
								m_WavFile,
								dlg.HeaderLength(),
								DWORD(RawFileSize) - dlg.TrailerLength(),
								nNewFileSamples,
								& wf,
								0 != (m_FileTypeFlags & OpenRawFileMsbFirst));

	// peak data will be created during decompression
	pContext->Execute();
	return TRUE;
}

BOOL CWaveSoapFrontDoc::OpenAviFileDocument(LPCTSTR lpszPathName)
{
	return FALSE;
}

BOOL CWaveSoapFrontDoc::OpenWmaFileDocument(LPCTSTR lpszPathName)
{

	TRACE(_T("CWaveSoapFrontDoc::OpenWmaFileDocument(%s)\n"), lpszPathName);

	CThisApp * pApp = GetApp();
	if ( ! pApp->CanOpenWindowsMedia())
	{
		CString s;
		s.Format(IDS_CANNOT_OPEN_WMA, lpszPathName);
		AfxMessageBox(s, MB_OK);
		return FALSE;
	}
	m_bDirectMode = FALSE;
	m_bReadOnly = FALSE;

	m_szWaveFilename = lpszPathName;
	if ( ! m_OriginalWavFile.Open(lpszPathName,
								MmioFileOpenExisting
								| MmioFileOpenReadOnly
								| MmioFileOpenDontLoadRiff))
	{
		CString s;
		UINT format;
		switch(GetLastError())
		{
		case ERROR_ACCESS_DENIED:
			format = IDS_FILE_OPEN_ACCESS_DENIED;
			break;
		case ERROR_SHARING_VIOLATION:
			format = IDS_FILE_OPEN_SHARING_VIOLATION;
			break;
		default:
			format = IDS_UNABLE_TO_OPEN_WMA_FILE;
			break;
		}
		s.Format(format, lpszPathName);
		AfxMessageBox(s, MB_ICONEXCLAMATION | MB_OK);
		return FALSE;
	}

	CWmaDecodeContext * pWmaContext = new CWmaDecodeContext(this,
															_T("Loading the compressed file..."), m_OriginalWavFile);

	if (NULL == pWmaContext)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}
	pWmaContext->m_Flags |= DecompressSavePeakFile;

	pWmaContext->Execute();
	return TRUE;
}

void CWaveSoapFrontDoc::OnUpdateToolsInterpolate(CCmdUI* pCmdUI)
{
	// the area must be at least 5* length away from the file beginning and from the end
	int InterpolateSamples = m_SelectionEnd - m_SelectionStart;
	int PreInterpolateSamples = 0;
	int PostInterpolateSamples = 0;
	int InterpolationOverlap;
	bool BigGap = (InterpolateSamples > 32);
	if (BigGap)
	{
		InterpolationOverlap = 2048 + InterpolateSamples + InterpolateSamples / 2;
		PostInterpolateSamples = InterpolateSamples / 2;
		PreInterpolateSamples = InterpolateSamples - InterpolateSamples / 2;
	}
	else
	{
		InterpolationOverlap = 5 * InterpolateSamples;
	}
	pCmdUI->Enable(CanModifyFile()
					&& 0 != InterpolateSamples
					&& InterpolateSamples <= MaxInterpolatedLength   // 128
					&& m_SelectionStart >= InterpolationOverlap
					&& m_SelectionEnd + InterpolationOverlap < WaveFileSamples());
}

void CWaveSoapFrontDoc::OnToolsInterpolate()
{
	int InterpolateSamples = m_SelectionEnd - m_SelectionStart;
	int PreInterpolateSamples = 0;
	int PostInterpolateSamples = 0;
	int InterpolationOverlap;

	bool BigGap = (InterpolateSamples > 32);
	if (BigGap)
	{
		InterpolationOverlap = 2048 + InterpolateSamples + InterpolateSamples / 2;
		PostInterpolateSamples = InterpolateSamples / 2;
		PreInterpolateSamples = InterpolateSamples - InterpolateSamples / 2;
	}
	else
	{
		InterpolationOverlap = 5 * InterpolateSamples;
	}

	if ( ! CanModifyFile()
		|| 0 == InterpolateSamples
		|| InterpolateSamples > MaxInterpolatedLength   // 128
		|| m_SelectionStart < InterpolationOverlap
		|| m_SelectionEnd + InterpolationOverlap >= WaveFileSamples())
	{
		return;
	}
	// interpolate the selected area.
	int SampleSize = WaveSampleSize();
	NUMBER_OF_CHANNELS nChannels = WaveChannels();

	SAMPLE_POSITION ReadStartOffset = m_WavFile.SampleToPosition(m_SelectionStart - InterpolationOverlap);
	SAMPLE_POSITION WriteStartOffset = m_WavFile.SampleToPosition(m_SelectionStart - PreInterpolateSamples);
	// allocate the buffer
	int BufferSamples = InterpolateSamples + 2 * InterpolationOverlap;
	int InterpolateOffset = InterpolationOverlap;
	int WriteBufferOffset = InterpolationOverlap  - PreInterpolateSamples;

	ATL::CHeapPtr<WAVE_SAMPLE> pBuf;
	pBuf.Allocate(BufferSamples * nChannels);

	if (NULL == (WAVE_SAMPLE*)pBuf)
	{
		return;
	}

	int const ReadBytes = BufferSamples * SampleSize;
	int const WrittenSamples = InterpolateSamples + PreInterpolateSamples + PostInterpolateSamples;
	int const WriteBytes = WrittenSamples * SampleSize;

	if (BufferSamples != m_WavFile.ReadSamples(ALL_CHANNELS, ReadStartOffset, BufferSamples, pBuf))
	{
		return;
	}
	// create undo
	if (UndoEnabled())
	{
		CUndoRedoContext::auto_ptr pUndo(new CUndoRedoContext(this, _T("Interpolate")));

		CCopyUndoContext * pCopy = new CCopyUndoContext(this, _T("Interpolate"), _T(""));
		pUndo->AddContext(pCopy);

		if ( ! pCopy->InitUndoCopy(m_WavFile,
									WriteStartOffset,
									WriteStartOffset + WriteBytes,
									m_SelectedChannel))
		{
			return;
		}

		pCopy->SaveUndoData(pBuf + WriteBufferOffset * nChannels, WriteBytes,
							WriteStartOffset, nChannels);
		AddUndoRedo(pUndo.release());
	}

	// now, do the interpolation
	CClickRemoval crm;
	if (m_SelectedChannel & SPEAKER_FRONT_LEFT) // mono or not right channel only
	{
		crm.InterpolateGap(pBuf, InterpolateOffset, InterpolateSamples, nChannels, BigGap);
	}
	if (nChannels == 2
		&& (m_SelectedChannel & SPEAKER_FRONT_RIGHT) != 0) // mono or not right channel only
	{
		crm.InterpolateGap(pBuf + 1, InterpolateOffset, InterpolateSamples, nChannels, BigGap);
	}

	// write the data back
	m_WavFile.WriteSamples(m_SelectedChannel, WriteStartOffset, WrittenSamples,
							pBuf + WriteBufferOffset * nChannels, m_SelectedChannel, nChannels);

	SetModifiedFlag(TRUE);
	SoundChanged(WaveFileID(), m_SelectionStart - PreInterpolateSamples,
				m_SelectionEnd + PostInterpolateSamples);

	// check for clipping
	if (crm.WasClipped())
	{
		OnIdle();   // update views
		CString s;
		s.Format(IDS_SOUND_CLIPPED, GetTitle(), int(crm.GetMaxClipped() * (100. / 32678)));
		AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
	}
}

void CWaveSoapFrontDoc::OnUpdateProcessDoUlf(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanStartOperation(17, false, true)
					&& (m_SelectionEnd - m_SelectionStart > 16
						|| m_SelectionEnd == m_SelectionStart));

}

void CWaveSoapFrontDoc::OnProcessDoUlf()
{
	if ( ! CanStartOperation(17, false, true))
	{
		return;
	}
	SAMPLE_INDEX start = m_SelectionStart;
	SAMPLE_INDEX end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}

	CLowFrequencySuppressDialog dlg(start, end, m_CaretPosition, GetSelectedChannel(),
									m_WavFile, ChannelsLocked(), UndoEnabled(), GetApp()->m_SoundTimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	CConversionContext::auto_ptr pContext
	(new CConversionContext(this, _T("Reducing low frequency static..."),
							_T("Low Frequency Suppression")));

	if ( ! pContext->InitDestination(m_WavFile, dlg.GetStart(),
									dlg.GetEnd(), dlg.GetChannel(), dlg.UndoEnabled()))
	{
		return;
	}

	CHumRemoval::auto_ptr pUlfProc(new CHumRemoval);

	pUlfProc->SetAndValidateWaveformat(WaveFormat());
	pUlfProc->m_ChannelsToProcess = dlg.GetChannel();

	pUlfProc->EnableDifferentialSuppression(dlg.m_DifferentialModeSuppress);
	pUlfProc->EnableLowFrequencySuppression(dlg.m_LowFrequencySuppress);
	pUlfProc->SetDifferentialCutoff(dlg.m_dDiffNoiseRange);
	pUlfProc->SetHighpassCutoff(dlg.m_dLfNoiseRange);

	pContext->AddWaveProc(pUlfProc.release());

	pContext.release()->Execute();
	SetModifiedFlag(TRUE, dlg.UndoEnabled());
}

void CWaveSoapFrontDoc::OnUpdateProcessDoDeclicking(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanStartOperation(64, false, true));
}

void CWaveSoapFrontDoc::OnProcessDoDeclicking()
{
	if ( ! CanStartOperation(64, false, true))
	{
		return;
	}
	SAMPLE_INDEX start = m_SelectionStart;
	SAMPLE_INDEX end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}

	CDeclickDialog dlg(start, end, m_CaretPosition, GetSelectedChannel(),
						m_WavFile, ChannelsLocked(), UndoEnabled(), GetApp()->m_SoundTimeFormat);

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	CConversionContext::auto_ptr pContext
	(new CConversionContext(this, _T("Removing vinyl disk clicks..."),
							_T("Declicking")));

	CClickRemoval * pDeclick = new CClickRemoval;
	pContext->AddWaveProc(pDeclick);

	pDeclick->SetAndValidateWaveformat(WaveFormat());
	dlg.SetDeclickData(pDeclick);

	if ( ! pContext->InitDestination(m_WavFile, dlg.GetStart(),
									dlg.GetEnd(), dlg.GetChannel(), dlg.UndoEnabled()))
	{
		return;
	}

	pContext.release()->Execute();
	SetModifiedFlag(TRUE, dlg.UndoEnabled());

}

void CWaveSoapFrontDoc::OnUpdateProcessNoiseReduction(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanStartOperation(1024, false, true));
}

void CWaveSoapFrontDoc::OnProcessNoiseReduction()
{
	if ( ! CanStartOperation(1024, false, true))
	{
		return;
	}
	SAMPLE_INDEX start = m_SelectionStart;
	SAMPLE_INDEX end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}

	CNoiseReductionDialog dlg(start, end, m_CaretPosition, GetSelectedChannel(),
							m_WavFile, ChannelsLocked(), UndoEnabled(), GetApp()->m_SoundTimeFormat);

	ULONG result = dlg.DoModal();

	if (IDC_BUTTON_SET_THRESHOLD == result)
	{
		// get active frame, enable spectrum section, store threshold
		CChildFrame * pFrame =
			dynamic_cast<CChildFrame*>(((CFrameWnd*)GetApp()->GetMainWnd())->GetActiveFrame());
		if (NULL != pFrame)
		{
			CSpectrumSectionView * pSectionView =
				dynamic_cast<CSpectrumSectionView *>
				(pFrame->m_wClient.GetDlgItem(CWaveMDIChildClient::SpectrumSectionViewID));
			if (NULL != pSectionView)
			{
				pSectionView->m_bShowNoiseThreshold = TRUE;
				pSectionView->m_dNoiseThresholdLow = dlg.GetNoiseThresholdLow();
				pSectionView->m_dNoiseThresholdHigh = dlg.GetNoiseThresholdHigh();
				pSectionView->nBeginFrequency = int(dlg.GetLowerFrequency());

				if (! pFrame->m_wClient.m_bShowSpectrumSection)
				{
					pFrame->m_wClient.m_bShowSpectrumSection = TRUE;
					pFrame->m_wClient.RecalcLayout();   // would invalidate
				}
				else
				{
					pSectionView->Invalidate();
				}
			}
		}
		return;
	}
	else if (IDOK != result)
	{
		return;
	}

	CConversionContext::auto_ptr pContext
	(new CConversionContext(this, _T("Removing background noise..."),
							_T("Noise Reduction")));

	CNoiseReduction * pNoiseReduction = new CNoiseReduction;

	pContext->AddWaveProc(pNoiseReduction);

	pNoiseReduction->SetAndValidateWaveformat(WaveFormat());
	dlg.SetNoiseReductionData(pNoiseReduction);

	if ( ! pContext->InitDestination(m_WavFile, dlg.GetStart(),
									dlg.GetEnd(), dlg.GetChannel(), dlg.UndoEnabled()))
	{
		return;
	}

	pContext.release()->Execute();
	SetModifiedFlag(TRUE, dlg.UndoEnabled());
}

void CWaveSoapFrontDoc::OnFileClose()
{
	if (m_bClosePending)
	{
		return;
	}
	m_bClosing = true;
	// close may be pending
	if (!SaveModified())
	{
		// m_bClosePEnding may be set to eventually destroy the document
		m_bClosing = false;
		return;
	}
	m_bClosing = false;

	// shut it down
	OnCloseDocument();
	// this should destroy the document
}

BOOL CWaveSoapFrontDoc::CanCloseFrame(CFrameWnd* pFrameArg)
	// permission to close all views using this frame
	//  (at least one of our views must be in this frame)
{
	ASSERT_VALID(pFrameArg);
	UNUSED(pFrameArg);   // unused in release builds

	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView* pView = GetNextView(pos);
		ASSERT_VALID(pView);
		CFrameWnd* pFrame = pView->GetParentFrame();
		// assume frameless views are ok to close
		if (pFrame != NULL)
		{
			// assumes 1 document per frame
			ASSERT_VALID(pFrame);
			if (pFrame->m_nWindow > 0)
				return TRUE;        // more than one frame refering to us
		}
	}

	// otherwise only one frame that we know about
	if (m_bClosePending)
	{
		return FALSE;
	}
	m_bClosing = true;
	BOOL res = SaveModified();
	m_bClosing = false;
	return res;
}

void CWaveSoapFrontDoc::OnActivateDocument(BOOL bActivate)
{
	if (bActivate)
	{
		if ( ! m_PlayingSound)
		{
			SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);
		}
	}
	else
	{
		if ( ! m_PlayingSound)
		{
			SetThreadPriority(THREAD_PRIORITY_LOWEST);
		}
	}

}

void CWaveSoapFrontDoc::GetCurrentStatusString(CString & str)
{
	m_StatusStringLock.Lock();
	str = m_CurrentStatusString;
	m_StatusStringLock.Unlock();
}

void CWaveSoapFrontDoc::SetCurrentStatusString(const CString & str)
{
	m_StatusStringLock.Lock();
	m_CurrentStatusString = str;
	m_StatusStringLock.Unlock();
}

void CWaveSoapFrontDoc::OnProcessChannels()
{
	if ( ! CanWriteFile())
	{
		return;
	}

	if (1 == WaveChannels())
	{
		ChangeChannels(2);
	}
	else if (2 == WaveChannels())
	{
		ChangeChannels(1);
	}
}

void CWaveSoapFrontDoc::OnUpdateProcessChannels(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanWriteFile());
}

void CWaveSoapFrontDoc::OnSamplerateCustom()
{
	if ( ! CanWriteFile())
	{
		return;
	}

	CCustomSampleRateDlg dlg(WaveSampleRate());

	if (IDOK == dlg.DoModal()
		&& dlg.GetSampleRate() > 0)
	{
		SetSampleRate(dlg.GetSampleRate());
	}
}

void CWaveSoapFrontDoc::OnUpdateSamplerateCustom(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanWriteFile());
}

void CWaveSoapFrontDoc::DeletePermanentUndoRedo()
{
	COperationContext * pContext;
	for (pContext = m_UndoList.First()
		; m_UndoList.NotEnd(pContext); pContext = m_UndoList.Next(pContext))
	{
		if (pContext->KeepsPermanentFileReference())
		{
			while (pContext != m_UndoList.Last())
			{
				delete m_UndoList.RemoveTail();
			}
			delete m_UndoList.RemoveTail();
			break;
		}
	}

	for (pContext = m_RedoList.First()
		; m_RedoList.NotEnd(pContext); pContext = m_RedoList.Next(pContext))
	{
		if (pContext->KeepsPermanentFileReference())
		{
			while (pContext != m_RedoList.Last())
			{
				delete m_RedoList.RemoveTail();
			}
			delete m_RedoList.RemoveTail();
			break;
		}
	}
}


void CWaveSoapFrontDoc::OnEditEnableUndo()
{
	if ( ! m_bReadOnly)
	{
		m_bUndoEnabled = ! m_bUndoEnabled;
	}
}

void CWaveSoapFrontDoc::OnUpdateEditEnableUndo(CCmdUI* pCmdUI)
{
	if (m_bReadOnly)
	{
		pCmdUI->Enable(FALSE);
	}
	else
	{
		pCmdUI->SetCheck(UndoEnabled());
		pCmdUI->Enable(! IsBusy());
	}
}

void CWaveSoapFrontDoc::OnEditEnableRedo()
{
	if ( ! m_bReadOnly)
	{
		m_bRedoEnabled = ! m_bRedoEnabled;
	}
}

void CWaveSoapFrontDoc::OnUpdateEditEnableRedo(CCmdUI* pCmdUI)
{
	if (m_bReadOnly)
	{
		pCmdUI->Enable(FALSE);
	}
	else
	{
		pCmdUI->SetCheck(RedoEnabled());
		pCmdUI->Enable(! IsBusy());
	}
}

void CWaveSoapFrontDoc::OnEditClearUndo()
{
	if ( ! IsBusy())
	{
		DeleteUndo();
	}
}

void CWaveSoapFrontDoc::OnUpdateEditClearUndo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(! IsBusy() && ! m_UndoList.IsEmpty());
}

void CWaveSoapFrontDoc::OnEditClearRedo()
{
	if ( ! IsBusy())
	{
		DeleteRedo();
	}
}

void CWaveSoapFrontDoc::OnUpdateEditClearRedo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(! IsBusy() && ! m_RedoList.IsEmpty());
}

void CWaveSoapFrontDoc::OnProcessEqualizer()
{
	if ( ! CanStartOperation(2, false, true))
	{
		return;
	}

	SAMPLE_INDEX start = m_SelectionStart;
	SAMPLE_INDEX end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}

	CEqualizerDialog dlg(start, end, m_CaretPosition,
						GetSelectedChannel(), m_WavFile,
						GetApp()->m_SoundTimeFormat, m_bChannelsLocked, UndoEnabled());

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	CEqualizerContext::auto_ptr pContext
	(new CEqualizerContext(this, _T("Applying equalizer..."), _T("Equalizer")));

	for (int i = 0; i < dlg.m_nBands; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			pContext->m_BandCoefficients[i][j] = dlg.m_wGraph.m_BandCoefficients[i][j];
		}
	}

	pContext->m_NumOfBands = dlg.m_nBands;
	pContext->m_bZeroPhase = dlg.m_wGraph.m_bZeroPhase;

	if ( ! pContext->InitDestination(m_WavFile, dlg.GetStart(),
									dlg.GetEnd(), dlg.GetChannel(), dlg.UndoEnabled()))
	{
		return;
	}

	pContext.release()->Execute();
	SetModifiedFlag(TRUE, dlg.UndoEnabled());
}

void CWaveSoapFrontDoc::OnUpdateProcessEqualizer(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanStartOperation(3, false, true));
}

void CWaveSoapFrontDoc::OnProcessSwapchannels()
{
	if (! CanModifyFile()
		|| WaveChannels() != 2)
	{
		// don't do anything
		return;
	}

	SAMPLE_INDEX start = m_SelectionStart;
	SAMPLE_INDEX end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}
	CSwapChannelsContext * pContext =
		new CSwapChannelsContext(this, _T("Swapping the channels..."), _T("Swap channels"));
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	if ( ! pContext->InitDestination(m_WavFile, start,
									end, ALL_CHANNELS, UndoEnabled()))
	{
		delete pContext;
		return;
	}
	pContext->Execute();
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::OnUpdateProcessSwapchannels(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanModifyFile()
					&& WaveChannels() == 2);
}

void CWaveSoapFrontDoc::OnProcessFilter()
{
	if (! CanStartOperation(3, false, true))
	{
		return;
	}

	SAMPLE_INDEX start = m_SelectionStart;
	SAMPLE_INDEX end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}

	CThisApp * pApp = GetApp();
	CFilterDialog dlg(start, end, m_CaretPosition,
					GetSelectedChannel(), m_WavFile,
					pApp->m_SoundTimeFormat, m_bChannelsLocked, UndoEnabled());

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	CFilterContext::auto_ptr pContext
	(new CFilterContext(this, _T("Applying filter..."), _T("Filter")));

	dlg.GetLpfCoefficients(pContext->m_LpfCoeffs);
	dlg.GetHpfCoefficients(pContext->m_HpfCoeffs);
	dlg.GetNotchCoefficients(pContext->m_NotchCoeffs);

	pContext->m_bZeroPhase = dlg.IsZeroPhase();

	pContext->m_nLpfOrder = dlg.GetLowpassFilterOrder();
	pContext->m_nHpfOrder = dlg.GetHighpassFilterOrder();
	pContext->m_nNotchOrder = dlg.GetNotchFilterOrder();

	if ( ! pContext->InitDestination(m_WavFile, dlg.GetStart(),
									dlg.GetEnd(), dlg.GetChannel(), dlg.UndoEnabled()))
	{
		return;
	}

	pContext.release()->Execute();
	SetModifiedFlag(TRUE, dlg.UndoEnabled());
}

void CWaveSoapFrontDoc::OnUpdateProcessFilter(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanStartOperation(3, false, true));
}

BOOL CWaveSoapFrontDoc::DoFileSave()
{
	DWORD dwAttrib;
	if (m_strPathName.IsEmpty()
		|| (INVALID_FILE_ATTRIBUTES != (dwAttrib = GetFileAttributes(m_strPathName))
			&& dwAttrib & FILE_ATTRIBUTE_READONLY))
	{
		// we do not have read-write access or the file does not (now) exist
		if (!DoSave(NULL))
		{
			TRACE0("Warning: File save with new name failed.\n");
			return FALSE;
		}
	}
	else
	{
		if (!DoSave(m_strPathName))
		{
			TRACE0("Warning: File save failed.\n");
			return FALSE;
		}
	}
	return TRUE;
}

bool CWaveSoapFrontDoc::CanStartOperation(NUMBER_OF_SAMPLES SamplesNecessary,
										bool SelectionNecessary, bool Modify) const
{
	return m_OperationInProgress == 0
			&& WaveFileSamples() >= SamplesNecessary
			&& ( ! SelectionNecessary || m_SelectionEnd > m_SelectionStart)
			&& ! (Modify && IsReadOnly());
}

CHANNEL_MASK CWaveSoapFrontDoc::GetSelectedChannel() const
{
	CHANNEL_MASK Channels = m_SelectedChannel;
	if ( ! ChannelsLocked())
	{
		Channels = ALL_CHANNELS;
	}

	return Channels & m_WavFile.ChannelsMask();
}
