// WaveSoapFrontDoc.cpp : implementation of the CWaveSoapFrontDoc class
//

#include "stdafx.h"
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
	:m_pPeaks(NULL),
	m_WavePeakSize(0),
	m_AllocatedWavePeakSize(0),
	m_PeakDataGranularity(512),
	m_CaretPosition(0),
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
	m_FileTypeFlags(0),
	m_SelectedChannel(ALL_CHANNELS)
{
	m_bUndoEnabled = (FALSE != GetApp()->m_bUndoEnabled);
	m_bRedoEnabled = (FALSE != GetApp()->m_bRedoEnabled);
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
	// free RetiredList and UpdateList
	while ( ! m_RetiredList.IsEmpty())
	{
		delete m_RetiredList.RemoveHead();
	}
	while ( ! m_UpdateList.IsEmpty())
	{
		delete m_UpdateList.RemoveHead();
	}

	DeleteUndo();
	DeleteRedo();
	// stop the thread
	if (NULL != m_Thread.m_hThread)
	{
		m_bRunThread = FALSE;
#ifdef _DEBUG
		DWORD Time = timeGetTime();
#endif
		SetEvent(m_hThreadEvent);
		if (WAIT_TIMEOUT == WaitForSingleObject(m_Thread.m_hThread, 5000))
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
	if (m_pPeaks != NULL)
	{
		delete[] m_pPeaks;
		m_pPeaks = NULL;
	}
	CWaveSoapFrontDoc * pTmpDoc = NULL;
	CString str;
	CThisApp * pApp = GetApp();
	pApp->GetStatusStringAndDoc(str, & pTmpDoc);
	if (this == pTmpDoc)
	{
		pApp->SetStatusStringAndDoc("", NULL);
	}
}

int CWaveSoapFrontDoc::WaveFileSamples() const
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
int CWaveSoapFrontDoc::WaveChannels() const
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
DWORD CWaveSoapFrontDoc::WaveFileID() const
{
	return m_WavFile.GetFileID();
}

BOOL CWaveSoapFrontDoc::OnNewDocument(NewFileParameters * pParams)
{
	if (!CDocument::OnNewDocument())
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

	LONG nSamples = pParams->InitialSamples;
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
	AllocatePeakData(nSamples);
	// zero wave peak data
	if (NULL != m_pPeaks)
	{
		memset(m_pPeaks, 0, m_AllocatedWavePeakSize * sizeof (WavePeak));
	}
	return TRUE;
}

void CWaveSoapFrontDoc::CheckAndLoadPeakFile(CWaveFile & WaveFile)
{
	// if peak file exists and the wav file length/date/time matches the stored
	// length/date/time, then use this peak file.
	// otherwise scan the wav file and build the new peak file
	CFile PeakFile;
	PeakFileHeader pfh;
	CString szPeakFilename(WaveFile.GetName());

	if (0 == szPeakFilename.Right(4).CompareNoCase(_T(".WAV")))
	{
		szPeakFilename.Delete(szPeakFilename.GetLength() - 4, 4);
	}
	szPeakFilename += _T(".wspk");
	if (PeakFile.Open(szPeakFilename,
					CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary))
	{
		if (offsetof(PeakFileHeader, WaveFileTime)
			== PeakFile.Read( & pfh, offsetof(PeakFileHeader, WaveFileTime))
			&& PeakFileHeader::pfhSignature == pfh.dwSignature
			&& pfh.dwVersion == PeakFileHeader::pfhMaxVersion
			&& pfh.wSize == sizeof (PeakFileHeader)
			// read the rest of the header
			&& pfh.wSize - offsetof(PeakFileHeader, WaveFileTime)
			== PeakFile.Read( & pfh.WaveFileTime, pfh.wSize - offsetof(PeakFileHeader, WaveFileTime))
			&& pfh.WaveFileTime.dwHighDateTime == WaveFile.GetFileInformation().ftLastWriteTime.dwHighDateTime
			&& pfh.WaveFileTime.dwLowDateTime == WaveFile.GetFileInformation().ftLastWriteTime.dwLowDateTime
			&& pfh.dwWaveFileSize == WaveFile.GetFileInformation().nFileSizeLow
			&& 0 == memcmp(& pfh.wfFormat, WaveFile.GetWaveFormat(), sizeof pfh.wfFormat)
			&& m_PeakDataGranularity == pfh.Granularity
			&& pfh.PeakInfoSize
			== CalculatePeakInfoSize() * sizeof (WavePeak)
			)
		{
			// allocate data and read it
			m_WavePeakSize = 0;
			if ( ! AllocatePeakData(WaveFile.NumberOfSamples()))
			{
				TRACE("Unable to allocate peak info buffer\n");
				return;
			}

			if (pfh.PeakInfoSize <= m_WavePeakSize * sizeof (WavePeak)
				&& pfh.PeakInfoSize == PeakFile.Read(m_pPeaks, pfh.PeakInfoSize))
			{
				return;
			}
			TRACE("Unable to read peak data\n");
			// rebuild the info from the WAV file
		}
		else
		{
			TRACE("Peak Info modification time = 0x%08X%08X, open file time=0x%08X%08X\n",
				pfh.WaveFileTime.dwHighDateTime, pfh.WaveFileTime.dwLowDateTime,
				WaveFile.GetFileInformation().ftLastWriteTime.dwHighDateTime,
				WaveFile.GetFileInformation().ftLastWriteTime.dwLowDateTime);
		}
		PeakFile.Close();
	}
	BuildPeakInfo(TRUE);
}

// the function is called to load peak info for a compressed file
// WaveFile argument - temporary wave file
// OriginalWaveFile - compressed file
void CWaveSoapFrontDoc::LoadPeaksForCompressedFile(CWaveFile & WaveFile, CWaveFile & OriginalWaveFile)
{

	// don't check peak file data size, just make sure source file parameters match
	// if peak file exists and the wav file length/date/time matches the stored
	// length/date/time, then use this peak file.
	// otherwise don't use it.
	// the peak info will be rebuilt in any case during file load
	CFile PeakFile;
	PeakFileHeader pfh;
	CString szPeakFilename(OriginalWaveFile.GetName());

	if (0 == szPeakFilename.Right(4).CompareNoCase(_T(".WAV")))
	{
		szPeakFilename.Delete(szPeakFilename.GetLength() - 4, 4);
	}
	szPeakFilename += _T(".wspk");
	if (PeakFile.Open(szPeakFilename,
					CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary))
	{
		if (offsetof(PeakFileHeader, WaveFileTime)
			== PeakFile.Read( & pfh, offsetof(PeakFileHeader, WaveFileTime))
			&& PeakFileHeader::pfhSignature == pfh.dwSignature
			&& pfh.dwVersion == PeakFileHeader::pfhMaxVersion
			&& pfh.wSize == sizeof (PeakFileHeader)
			// read the rest of the header
			&& pfh.wSize - offsetof(PeakFileHeader, WaveFileTime)
			== PeakFile.Read( & pfh.WaveFileTime, pfh.wSize - offsetof(PeakFileHeader, WaveFileTime))
			// check date and time
			&& pfh.WaveFileTime.dwHighDateTime == OriginalWaveFile.GetFileInformation().ftLastWriteTime.dwHighDateTime
			&& pfh.WaveFileTime.dwLowDateTime == OriginalWaveFile.GetFileInformation().ftLastWriteTime.dwLowDateTime
			// check source file size
			&& pfh.dwWaveFileSize == OriginalWaveFile.GetFileInformation().nFileSizeLow
			// check PCM number of channels and sampling rate
			&& 0 == memcmp(& pfh.wfFormat, WaveFile.GetWaveFormat(), sizeof pfh.wfFormat)
			&& m_PeakDataGranularity == pfh.Granularity
			)
		{
			// allocate data and read it
			m_WavePeakSize = 0;
			if ( ! AllocatePeakData(pfh.NumOfSamples))
			{
				TRACE("Unable to allocate peak info buffer\n");
				return;
			}

			if (pfh.PeakInfoSize <= m_WavePeakSize * sizeof (WavePeak)
				&& pfh.PeakInfoSize == PeakFile.Read(m_pPeaks, pfh.PeakInfoSize))
			{
				return;
			}
			TRACE("Unable to read peak data\n");
			// rebuild the info from the WAV file
		}
		else
		{
			TRACE("Peak Info modification time = 0x%08X%08X, open file time=0x%08X%08X\n",
				pfh.WaveFileTime.dwHighDateTime, pfh.WaveFileTime.dwLowDateTime,
				WaveFile.GetFileInformation().ftLastWriteTime.dwHighDateTime,
				WaveFile.GetFileInformation().ftLastWriteTime.dwLowDateTime);
		}
		PeakFile.Close();
	}
}

void CWaveSoapFrontDoc::BuildPeakInfo(BOOL bSavePeakFile)
{
	// read the DATA chunk of wavefile
	if ( ! AllocatePeakData(m_WavFile.NumberOfSamples()))
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	for (int i = 0; i < m_WavePeakSize; i++)
	{
		m_pPeaks[i].high = -0x8000;
		m_pPeaks[i].low = 0x7FFF;
	}
	CScanPeaksContext * pContext = new CScanPeaksContext(this);
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}
	if (bSavePeakFile)
	{
		pContext->m_Flags |= ScanPeaksSavePeakFile;
	}
	pContext->m_Start = WaveDataChunk()->dwDataOffset;
	pContext->m_Position = pContext->m_Start;
	pContext->m_End = pContext->m_Start + WaveDataChunk()->cksize;
	pContext->m_GranuleSize =
		WaveChannels() * m_PeakDataGranularity * sizeof(__int16);
	if (pContext->m_End <= pContext->m_Start)
	{
		delete pContext;
		return;
	}
	pContext->Execute();
}

void CWaveSoapFrontDoc::SavePeakInfo(CWaveFile & WaveFile, CWaveFile & SavedWaveFile)
{
	CFile PeakFile;
	PeakFileHeader pfh;
	CString szPeakFilename(SavedWaveFile.GetName());

	if (0 == szPeakFilename.Right(4).CompareNoCase(_T(".WAV")))
	{
		szPeakFilename.Delete(szPeakFilename.GetLength() - 4, 4);
	}
	szPeakFilename += _T(".wspk");

	if (PeakFile.Open(szPeakFilename,
					CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeBinary))
	{
		pfh.wSize = sizeof PeakFileHeader;
		pfh.dwSignature = PeakFileHeader::pfhSignature;
		pfh.dwVersion = PeakFileHeader::pfhMaxVersion;
		pfh.dwWaveFileSize = SavedWaveFile.GetFileSize(NULL);
		pfh.Granularity = m_PeakDataGranularity;
		pfh.PeakInfoSize = CalculatePeakInfoSize() * sizeof (WavePeak);
		pfh.WaveFileTime = SavedWaveFile.GetFileInformation().ftLastWriteTime;
		pfh.NumOfSamples = WaveFile.NumberOfSamples();
		pfh.wfFormat = * WaveFile.GetWaveFormat();

		PeakFile.Write( & pfh, sizeof pfh);
		PeakFile.Write(m_pPeaks, pfh.PeakInfoSize);

		PeakFile.Close();
	}

}


BOOL CWaveSoapFrontDoc::OpenWaveFile(CWaveFile & WaveFile, LPCTSTR szName, DWORD flags)
{
	CString s;
	UINT format = 0;
	if (m_WavFile.Open(szName, flags))
	{
		if(m_WavFile.LoadWaveformat())
		{
			if (m_WavFile.FindData())
			{
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

	if (NULL != (WAVEFORMATEX*)m_OriginalWaveFormat)
	{
		Wf = m_OriginalWaveFormat;
	}
	else
	{
		Wf = m_WavFile.GetWaveFormat();
	}

	// sample rate and number of channels might change from the original file
	// new format may not be quite valid for some convertors!!
	Wf.SampleRate() = m_WavFile.GetWaveFormat()->nSamplesPerSec;
	Wf.NumChannels() = m_WavFile.GetWaveFormat()->nChannels;

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
				!strExt.IsEmpty())
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
			// TODO: Add the extension for the supported type
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


		CWaveSoapFileSaveDialog dlg(FALSE, "wav", newName,
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

		dlg.m_Wf = Wf;
		dlg.m_pDocument = this;

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
		// todo: get from saved in the registry
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
	if (bReplace)
	{
		SetPathName(newName);
	}

	return TRUE;        // success
}

void CWaveSoapFrontDoc::SetSelection(long begin, long end, int channel, int caret, int flags)
{
	CSelectionUpdateInfo ui;
	ui.SelBegin = m_SelectionStart;
	ui.SelEnd = m_SelectionEnd;
	ui.SelChannel = m_SelectedChannel;
	ui.CaretPos = m_CaretPosition;
	long length = WaveFileSamples();
	if (begin < 0) begin = 0;
	if (begin > length) begin = length;
	if (end < 0) end = 0;
	if (end > length) end = length;
	if (caret < 0) caret = 0;
	if (caret > length) caret = length;
	if (channel < 0 || channel > 1) channel = ALL_CHANNELS;

	if (flags & SetSelection_SnapToMaximum)
	{
		// read from 'begin' to 'end', find the max
		int SamplesToRead, BeginSample;
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
		int Max = 0, pos = begin;
		while (SamplesToRead > 0)
		{
			__int16 data[128];
			int nWordsToRead = SamplesToRead * WaveChannels();
			if (nWordsToRead > 128) nWordsToRead = 128;
			m_WavFile.ReadAt(data, nWordsToRead * sizeof data[0],
							BeginSample * WaveSampleSize() + WaveDataChunk()->dwDataOffset);
			if (WaveChannels() <= 1)
			{
				for (int i = 0; i < nWordsToRead; i++)
				{
					int tmp = data[i];
					if (tmp < 0)
					{
						tmp = -tmp;
					}
					if (tmp > Max)
					{
						Max = tmp;
						pos = BeginSample + i;
					}
				}
			}
			else
			{
				for (int i = 0; i < nWordsToRead / 2; i++)
				{
					int tmp;
					if (channel != 1)
					{
						tmp = data[i * 2];
						if (tmp < 0)
						{
							tmp = -tmp;
						}
						if (tmp > Max)
						{
							Max = tmp;
							pos = BeginSample + i;
						}
					}
					if (channel != 0)
					{
						tmp = data[i * 2 + 1];
						if (tmp < 0)
						{
							tmp = -tmp;
						}
						if (tmp > Max)
						{
							Max = tmp;
							pos = BeginSample + i;
						}
					}
				}
			}
			SamplesToRead -= nWordsToRead / WaveChannels();
			BeginSample += nWordsToRead / WaveChannels();
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
	UpdateAllViews(NULL, UpdateSelectionChanged, & ui);
}

BOOL CWaveSoapFrontDoc::AllocatePeakData(long NewNumberOfSamples)
{
	// change m_pPeaks size
	// need to synchronize with OnDraw
	int NewWavePeakSize = WaveChannels() * ((NewNumberOfSamples + m_PeakDataGranularity - 1) / m_PeakDataGranularity);
	if (NULL == m_pPeaks
		|| NewWavePeakSize > m_AllocatedWavePeakSize)
	{
		int NewAllocatedWavePeakSize = NewWavePeakSize + 1024;  // reserve more
		WavePeak * NewPeaks = new WavePeak[NewAllocatedWavePeakSize];
		if (NULL == NewPeaks)
		{
			return FALSE;
		}
		if (NULL != m_pPeaks
			&& 0 != m_WavePeakSize)
		{
			memcpy(NewPeaks, m_pPeaks, m_WavePeakSize * sizeof (WavePeak));
		}
		else
		{
			m_WavePeakSize = 0;
		}
		for (int i = m_WavePeakSize; i < NewWavePeakSize; i++)
		{
			NewPeaks[i].high = -0x8000;
			NewPeaks[i].low = 0x7FFF;
		}
		WavePeak * OldPeaks;
		{
			CSimpleCriticalSectionLock lock(m_PeakLock);
			OldPeaks = m_pPeaks;
			m_pPeaks = NewPeaks;
			m_WavePeakSize = NewWavePeakSize;
			m_AllocatedWavePeakSize = NewAllocatedWavePeakSize;
		}
		delete[] OldPeaks;
	}
	else
	{
		for (int i = m_WavePeakSize; i < NewWavePeakSize; i++)
		{
			m_pPeaks[i].high = -0x8000;
			m_pPeaks[i].low = 0x7FFF;
		}
		m_WavePeakSize = NewWavePeakSize;
	}
	return TRUE;
}

void CWaveSoapFrontDoc::SoundChanged(DWORD FileID, long begin, long end,
									int FileLength, DWORD flags)
{
	// notify all views that the sound appearance changed
	if (FileID != WaveFileID())
	{
		// changed a different (some temporary) file
		return;
	}
	// update peak data
	if ((FileLength == -1
			|| AllocatePeakData(FileLength))
		&& 0 == (flags & UpdateSoundDontRescanPeaks)
		&& FileID != NULL
		&& begin != end)
	{
		// rescan peaks
		RescanPeaks(begin, end);
	}

	if (flags & UpdateSoundSamplingRateChanged)
	{
		UpdateAllViews(NULL, UpdateSampleRateChanged);
	}

	CSoundUpdateInfo * pui = new CSoundUpdateInfo;
	pui->FileID = FileID;
	pui->UpdateCode = UpdateSoundChanged;
	pui->Begin = begin;
	pui->End = end;
	pui->Length = FileLength;
	{
		CSimpleCriticalSectionLock lock(m_cs);

		for (CSoundUpdateInfo * pEntry = m_UpdateList.Next()
			; pEntry != m_UpdateList.Head(); pEntry = pEntry->Next())
		{
			// see if the ranges overlap
			if (FileID == pEntry->FileID
				&& pEntry->UpdateCode == UpdateSoundChanged
				&& pEntry->Length == FileLength
				&& begin <= pEntry->End
				&& end >= pEntry->Begin)
			{
				if (begin < pEntry->Begin)
				{
					pEntry->Begin = begin;
				}
				if (pEntry->End < end)
				{
					pEntry->End = end;
				}
				delete pui;
				pui = NULL;
				break;
			}
		}
		if (NULL != pui)
		{
			m_UpdateList.InsertTail(pui);
		}
	}
	::PostMessage(GetApp()->m_pMainWnd->m_hWnd, WM_KICKIDLE, 0, 0);
}

void CWaveSoapFrontDoc::RescanPeaks(long begin, long end)
{
	// if called immediately after data modification, it will get
	// the data directly from the cache
	TRACE("RescanPeaks from %d to %d\n", begin, end);
	int nSampleSize = WaveSampleSize();
	LPMMCKINFO datack = WaveDataChunk();
	DWORD dwDataChunkOffset = datack->dwDataOffset;
	unsigned GranuleSize = WaveChannels() * m_PeakDataGranularity * sizeof(__int16);

	DWORD Pos = nSampleSize * (begin & -m_PeakDataGranularity) + dwDataChunkOffset;

	DWORD EndPos = nSampleSize * ((end | (m_PeakDataGranularity - 1)) + 1);
	if (EndPos > datack->cksize)
	{
		EndPos = datack->cksize;
	}
	EndPos += dwDataChunkOffset;

	while (Pos < EndPos)
	{
		DWORD SizeToRead = EndPos - Pos;
		void * pBuf;
		long lRead = m_WavFile.GetDataBuffer( & pBuf,
											SizeToRead, Pos, 0);
		if (lRead > 0)
		{
			unsigned i;
			long DataToProcess = lRead;
			__int16 * pWaveData = (__int16 *) pBuf;
			DWORD DataOffset = Pos - dwDataChunkOffset;
			unsigned DataForGranule = GranuleSize - DataOffset % GranuleSize;

			if (2 == WaveChannels())
			{
				WavePeak * pPeak = m_pPeaks + (DataOffset / GranuleSize) * 2;
				while (0 != DataToProcess)
				{
					int wpl_l;
					int wpl_h;
					int wpr_l;
					int wpr_h;
					if (0 == DataOffset % GranuleSize)
					{
						wpl_l = 0x7FFF;
						wpl_h = -0x8000;
						wpr_l = 0x7FFF;
						wpr_h = -0x8000;
					}
					else
					{
						wpl_l = pPeak[0].low;
						wpl_h = pPeak[0].high;
						wpr_l = pPeak[1].low;
						wpr_h = pPeak[1].high;
					}

					if (DataForGranule > DataToProcess)
					{
						DataForGranule = DataToProcess;
					}
					DataToProcess -= DataForGranule;

					if (DataOffset & 2)
					{
						if (pWaveData[0] < wpr_l)
						{
							wpr_l = pWaveData[0];
						}
						if (pWaveData[0] > wpr_h)
						{
							wpr_h = pWaveData[0];
						}
						pWaveData++;
						DataOffset += 2;
						DataForGranule -= 2;
					}

					DataOffset += DataForGranule;
					for (i = 0; i < DataForGranule / (sizeof(__int16) * 2); i++, pWaveData += 2)
					{
						if (pWaveData[0] < wpl_l)
						{
							wpl_l = pWaveData[0];
						}
						if (pWaveData[0] > wpl_h)
						{
							wpl_h = pWaveData[0];
						}
						if (pWaveData[1] < wpr_l)
						{
							wpr_l = pWaveData[1];
						}
						if (pWaveData[1] > wpr_h)
						{
							wpr_h = pWaveData[1];
						}
					}

					if (DataForGranule & 2)
					{
						if (pWaveData[0] < wpl_l)
						{
							wpl_l = pWaveData[0];
						}
						if (pWaveData[0] > wpl_h)
						{
							wpl_h = pWaveData[0];
						}
						pWaveData++;
					}

					ASSERT(pPeak - m_pPeaks < m_WavePeakSize);
					pPeak[0].low = wpl_l;
					pPeak[0].high = wpl_h;
					pPeak[1].low = wpr_l;
					pPeak[1].high = wpr_h;
					pPeak += 2;
					DataForGranule = GranuleSize;
				}
			}
			else
			{
				WavePeak * pPeak = m_pPeaks + DataOffset / GranuleSize;
				while (0 != DataToProcess)
				{
					int wp_l;
					int wp_h;
					if (0 == DataOffset % GranuleSize)
					{
						wp_l = 0x7FFF;
						wp_h = -0x8000;
					}
					else
					{
						wp_l = pPeak[0].low;
						wp_h = pPeak[0].high;
					}

					if (DataForGranule > DataToProcess)
					{
						DataForGranule = DataToProcess;
					}
					DataToProcess -= DataForGranule;
					DataOffset += DataForGranule;

					for (i = 0; i < DataForGranule / sizeof(__int16); i++, pWaveData ++)
					{
						if (pWaveData[0] < wp_l)
						{
							wp_l = pWaveData[0];
						}
						if (pWaveData[0] > wp_h)
						{
							wp_h = pWaveData[0];
						}
					}

					ASSERT(pPeak - m_pPeaks < m_WavePeakSize);
					pPeak[0].low = wp_l;
					pPeak[0].high = wp_h;
					pPeak ++;
					DataForGranule = GranuleSize;
				}
			}

			Pos += lRead;
			m_WavFile.ReturnDataBuffer(pBuf, lRead, 0);
		}
		else
		{
			break;
		}
	}
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
	if (m_OperationInProgress)
	{
		return;
	}
	DoCopy(m_SelectionStart, m_SelectionEnd, m_SelectedChannel, NULL);
}

void CWaveSoapFrontDoc::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_SelectionStart != m_SelectionEnd
					&& ! m_OperationInProgress);
}

void CWaveSoapFrontDoc::OnEditCut()
{
	if (m_OperationInProgress
		|| m_bReadOnly
		|| m_SelectionStart == m_SelectionEnd)
	{
		return;
	}
	int ch = m_SelectedChannel;
	if (ChannelsLocked())
	{
		ch = ALL_CHANNELS;
	}
	DoCut(m_SelectionStart, m_SelectionEnd, ch);
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::OnUpdateEditCut(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_SelectionStart != m_SelectionEnd
					&& ! (m_OperationInProgress || m_bReadOnly));
}

void CWaveSoapFrontDoc::OnEditDelete()
{
	if (m_OperationInProgress
		|| m_bReadOnly
		|| m_SelectionStart == WaveFileSamples())
	{
		return;
	}
	int ch = m_SelectedChannel;
	if (ChannelsLocked())
	{
		ch = ALL_CHANNELS;
	}
	DoDelete(m_SelectionStart, m_SelectionEnd, ch);
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::OnUpdateEditDelete(CCmdUI* pCmdUI)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	pCmdUI->Enable( ! (m_OperationInProgress
						|| m_bReadOnly
						|| m_SelectionStart == WaveFileSamples()));
}

void CWaveSoapFrontDoc::OnEditPaste()
{
	if (m_OperationInProgress)
	{
		return;
	}
	int ch = m_SelectedChannel;
	if (ChannelsLocked())
	{
		ch = ALL_CHANNELS;
	}
	DoPaste(m_SelectionStart, m_SelectionEnd, ch, NULL);
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetApp()->m_ClipboardFile.IsOpen()
					&& ! (m_OperationInProgress || m_bReadOnly));
}

void CWaveSoapFrontDoc::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_UndoList.IsEmpty() && ! (m_OperationInProgress || m_bReadOnly));
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
		if ( ! m_OpList.IsEmpty())
		{
			m_OpList.Lock();
			COperationContext * pContext = m_OpList.Next();
			m_OpList.Unlock();
			// TODO: set the status message before calling Init
			if (0 == (pContext->m_Flags & OperationContextInitialized))
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

			int LastPercent = pContext->PercentCompleted;
			// execute one step
			if (0 == (pContext->m_Flags &
					(OperationContextStop | OperationContextFinished)))
			{
				if (! pContext->OperationProc())
				{
					pContext->m_Flags |= OperationContextStop;
				}
				// signal for status update
				if (LastPercent != pContext->PercentCompleted)
				{
					NeedKickIdle = true;
				}
				if (NeedKickIdle)
				{
					if (pContext->PercentCompleted >= 0)
					{
						CString s;
						s.Format(_T("%s%d%%"),
								(LPCTSTR)pContext->GetStatusString(),
								pContext->PercentCompleted);
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
				if (pContext->PercentCompleted >= 0)
				{
					SetCurrentStatusString(pContext->GetStatusString() + _T("Completed"));
				}
				NeedKickIdle = true;

				m_OpList.RemoveEntry(pContext);

				pContext->DeInit();
				TRACE("Retire context %X\n", pContext);
				pContext->Retire();     // usually deletes it
			}
			continue;
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

BOOL CWaveSoapFrontDoc::InitUndoRedo(CUndoRedoContext * pContext)
{
	// see what is necessary to restore
	if (pContext->m_Flags
		& (UndoContextReplaceWholeFile | UndoContextReplaceFormat))
	{
		// nothing to do
		return TRUE;
	}
	CResizeContext * pResize = pContext->m_pExpandShrinkContext;
	if (pContext->m_Flags & CopyShrinkFile)
	{
		// shrink the file
		if (NULL != pResize
			&& ((pContext->m_Flags & RedoContext)
				? UndoEnabled() : RedoEnabled())
			&& pResize->InitUndoRedo(pContext->m_OperationName)
			&& NULL != pResize->m_pUndoContext
			&& 0 == (pContext->m_Flags & RedoContext))
		{
			pResize->m_pUndoContext->m_Flags |= RedoContext;
		}
	}
	else if (pContext->m_Flags & CopyExpandFile)
	{
		if (NULL != pResize
			&& ((pContext->m_Flags & RedoContext)
				? UndoEnabled() : RedoEnabled())
			&& pResize->InitUndoRedo(pContext->m_OperationName)
			&& NULL != pResize->m_pUndoContext
			&& 0 == (pContext->m_Flags & RedoContext))
		{
			pResize->m_pUndoContext->m_Flags |= RedoContext;
		}
		if (FALSE == m_WavFile.SetFileLength(pContext->m_RestoredLength))
		{
			delete pResize->m_pUndoContext;
			pResize->m_pUndoContext = NULL;
			NotEnoughDiskSpaceMessageBox();
			return FALSE;
		}
		MMCKINFO * pDatachunk = m_WavFile.GetDataChunk();
		pDatachunk->cksize = pContext->m_RestoredLength - pDatachunk->dwDataOffset;
		pDatachunk->dwFlags |= MMIO_DIRTY;
		long NewLength = pDatachunk->cksize / m_WavFile.SampleSize();
		SoundChanged(m_WavFile.GetFileID(), 0, 0, NewLength);
	}
	else
	{
		// just copy
		if ((pContext->m_Flags & RedoContext)
			? UndoEnabled() : RedoEnabled())
		{
			CUndoRedoContext * pUndoRedo = new CUndoRedoContext(this, pContext->m_OperationName);
			if (NULL == pUndoRedo)
			{
				NotEnoughMemoryMessageBox();
				return FALSE;
			}
			if ( ! pUndoRedo->InitUndoCopy(m_WavFile,
											pContext->m_DstCopyPos, pContext->m_DstEnd, pContext->m_DstChan))
			{
				delete pUndoRedo;
				return FALSE;
			}
			if (0 == (pContext->m_Flags & RedoContext))
			{
				pUndoRedo->m_Flags |= RedoContext;
			}

			pContext->m_pUndoContext = pUndoRedo;
		}
	}
	// set destination file. It is not set when Undo is just created,
	// to avoid having any hanging references to the work file.
	pContext->m_DstFile = m_WavFile;
	if (NULL != pContext->m_pExpandShrinkContext)
	{
		pContext->m_pExpandShrinkContext->m_DstFile = m_WavFile;
	}
	return TRUE;
}

void CWaveSoapFrontDoc::OnEditUndo()
{
	if (m_OperationInProgress
		|| m_UndoList.IsEmpty())
	{
		return;
	}
	// no critical section lock needed
	CUndoRedoContext * pUndo = (CUndoRedoContext *) m_UndoList.RemoveHead();
	if ( ! InitUndoRedo(pUndo))
	{
		// return head back
		m_UndoList.InsertHead(pUndo);
		return;
	}

	DecrementModified();

	pUndo->Execute();
}

void CWaveSoapFrontDoc::OnEditRedo()
{
	if (m_OperationInProgress
		|| m_RedoList.IsEmpty())
	{
		return;
	}
	// no critical section lock needed
	CUndoRedoContext * pRedo = (CUndoRedoContext *) m_RedoList.RemoveHead();
	if ( ! InitUndoRedo(pRedo))
	{
		m_RedoList.InsertHead(pRedo);
		return;
	}

	IncrementModified(FALSE);   // don't delete redo

	pRedo->Execute();
}

void CWaveSoapFrontDoc::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_RedoList.IsEmpty() && ! (m_OperationInProgress || m_bReadOnly));
}

void CWaveSoapFrontDoc::QueueOperation(COperationContext * pContext)
{
	m_StopOperation = false;
	m_OperationNonCritical = (0 != (pContext->m_Flags & OperationContextNonCritical));
	InterlockedIncrement( & m_OperationInProgress);
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

void CWaveSoapFrontDoc::DoCopy(LONG Start, LONG End, LONG Channel, LPCTSTR FileName)
{
	// create a operation context
	CString OpName;
	DWORD OpenFlags;
	DWORD OperationFlags;
	CWaveFile DstFile;
	if (NULL != FileName && FileName[0] != 0)
	{
		OpName.Format("Copying the selection to \"%s\"...", FileName);
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
	CCopyContext * pContext = new CCopyContext(this, OpName, "Copy");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
	}

	// create a temporary clipboard WAV file
	if (FALSE == DstFile.CreateWaveFile( & m_WavFile, NULL, m_SelectedChannel,
										m_SelectionEnd - m_SelectionStart,
										OpenFlags,
										FileName))
	{
		delete pContext;
		AfxMessageBox(IDS_STRING_UNABLE_TO_CREATE_CLIPBOARD, MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	pContext->m_Flags |= OperationFlags;

	pContext->InitCopy(DstFile, 0, m_SelectionEnd - m_SelectionStart, ALL_CHANNELS,
						m_WavFile, m_SelectionStart, m_SelectionEnd - m_SelectionStart,
						m_SelectedChannel);

	if (OperationFlags & OperationContextClipboard)
	{
		GetApp()->m_ClipboardFile = DstFile;
	}
	// set operation context to the queue
	pContext->Execute();
}

void CWaveSoapFrontDoc::DoPaste(LONG Start, LONG End, LONG Channel, LPCTSTR FileName)
{
	// create a operation context
	CWaveFile * pSrcFile = & GetApp()->m_ClipboardFile;
	// todo: support copy from a file (FileName)
	if ( ! pSrcFile->IsOpen())
	{
		return;
	}
	long NumSamplesToPasteFrom = pSrcFile->NumberOfSamples();
	// check if the sampling rate matches the clipboard
	int SrcSampleRate = pSrcFile->SampleRate();
	int TargetSampleRate = WaveSampleRate();

	CResampleContext * pResampleContext = NULL;

	if (SrcSampleRate != TargetSampleRate)
	{
		CPasteResampleModeDlg dlg;
		dlg.m_SrcSampleRate = SrcSampleRate;
		dlg.m_TargetSampleRate = TargetSampleRate;

		if (IDOK != dlg.DoModal())
		{
			return;
		}
		LONGLONG NumSamplesToPasteFrom64;
		if (0 == dlg.m_ModeSelect)
		{
			// resample
			NumSamplesToPasteFrom64 =
				UInt32x32To64(NumSamplesToPasteFrom, TargetSampleRate) / SrcSampleRate;
		}
		if ( ! CanAllocateWaveFileSamplesDlg(m_WavFile.GetWaveFormat(), NumSamplesToPasteFrom64))
		{
			return;
		}

		NumSamplesToPasteFrom = NumSamplesToPasteFrom64;
		pResampleContext = new CResampleContext(this, "Changing sample rate of clipboard data...", "Resample");
		if (NULL == pResampleContext)
		{
			NotEnoughMemoryMessageBox();
			return;
		}
		// create new temporary file
		CWaveFile DstFile;
		double ResampleQuality = 40.;
		double ResampleRatio = double(TargetSampleRate) / SrcSampleRate;

		if ( ! DstFile.CreateWaveFile(pSrcFile, NULL, ALL_CHANNELS, NumSamplesToPasteFrom,
									CreateWaveFileTempDir
									| CreateWaveFileDeleteAfterClose
									| CreateWaveFilePcmFormat
									| CreateWaveFileTemp,
									NULL))
		{
			delete pResampleContext;
			FileCreationErrorMessageBox(NULL);
			return;
		}

		DstFile.GetWaveFormat()->nSamplesPerSec = TargetSampleRate;
		pResampleContext->InitResample(*pSrcFile, DstFile, ResampleRatio, ResampleQuality);
		pSrcFile = & pResampleContext->m_DstFile;
	}

	if (End > Start)
	{
		CPasteModeDialog dlg;
		dlg.m_PasteMode = m_DefaultPasteMode;
		if (dlg.DoModal() != IDOK)
		{
			delete pResampleContext;
			return;
		}
		m_DefaultPasteMode = dlg.m_PasteMode;
		switch (dlg.m_PasteMode)
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

	int nCopiedChannels = 2;
	if (Channel != ALL_CHANNELS || WaveChannels() < 2)
	{
		nCopiedChannels = 1;
	}
	int ChannelToCopyFrom = ALL_CHANNELS;
	if (nCopiedChannels < pSrcFile->Channels())
	{
		CCopyChannelsSelectDlg dlg;
		dlg.m_ChannelToCopy = m_PrevChannelToCopy + 1;
		if (IDOK != dlg.DoModal())
		{
			delete pResampleContext;
			return;
		}
		ChannelToCopyFrom = dlg.m_ChannelToCopy - 1;
		m_PrevChannelToCopy = ChannelToCopyFrom;
	}

	if ( ! CanExpandWaveFileDlg(m_WavFile, NumSamplesToPasteFrom - (End - Start)))
	{
		delete pResampleContext;
		return;
	}

	CCopyContext * pContext = new CCopyContext(this, _T("Inserting data from clipboard..."), "Paste");
	if (NULL == pContext)
	{
		delete pResampleContext;
		NotEnoughMemoryMessageBox();
		return;
	}

	pContext->m_Flags |= OperationContextClipboard | CopyExpandFile;

	if ( ! pContext->InitCopy(m_WavFile, Start, End - Start, Channel,
							* pSrcFile, 0, NumSamplesToPasteFrom, ChannelToCopyFrom))
	{
		delete pResampleContext;
		delete pContext;
		return;
	}

	if (NULL != pContext->m_pExpandShrinkContext)
	{
		pContext->m_pExpandShrinkContext->InitUndoRedo("Paste");
		// TODO?? if the source selection is not empty,
		// init undo copy
	}
	else
	{
	}
	// set operation context to the queue
	SoundChanged(WaveFileID(), 0, 0, WaveFileSamples());
	if (NULL != pResampleContext)
	{
		pResampleContext->m_pChainedContext = pContext;
		pResampleContext->Execute();
	}
	else
	{
		pContext->Execute();
	}
}

void CWaveSoapFrontDoc::DoCut(LONG Start, LONG End, LONG Channel)
{
	// save the cut area to Undo context, then shrink the file
	// create a operation context
	CCopyContext * pContext = new CCopyContext(this, _T("Copying data to clipboard..."), "Cut");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

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
		delete pContext;
		AfxMessageBox(IDS_STRING_UNABLE_TO_CREATE_CLIPBOARD, MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	pContext->m_Flags |= OperationContextClipboard;

	pContext->InitCopy(DstFile, 0, End - Start, ALL_CHANNELS,
						m_WavFile, Start, End - Start,
						Channel);
	GetApp()->m_ClipboardFile = DstFile;
	// set operation context to the queue
	pContext->Execute();
	CResizeContext * pResizeContext = new CResizeContext(this, _T("Shrinking the file..."), "File Resize");
	pResizeContext->InitShrink(m_WavFile, Start, End - Start, Channel);
	pResizeContext->InitUndoRedo("Cut");
	SetSelection(Start, Start, Channel, Start);
	pResizeContext->Execute();
}

void CWaveSoapFrontDoc::DoDelete(LONG Start, LONG End, LONG Channel)
{
	if (End == Start)
	{
		End++;
	}
	CResizeContext * pResizeContext = new CResizeContext(this, _T("Deleting the selection..."), "Delete");
	pResizeContext->InitShrink(m_WavFile, Start, End - Start, Channel);
	pResizeContext->InitUndoRedo("Delete");
	SetSelection(Start, Start, Channel, Start);
	pResizeContext->Execute();
}

void CWaveSoapFrontDoc::OnEditStop()
{
	if (m_OperationInProgress)
	{
		m_StopOperation = true;
	}
}

void CWaveSoapFrontDoc::OnUpdateEditStop(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_OperationInProgress && ! m_PlayingSound);
}

BOOL CWaveSoapFrontDoc::OnOpenDocument(LPCTSTR lpszPathName, int DocOpenFlags)
{

	// TODO: Add your specialized creation code here
	TRACE("CWaveSoapFrontDoc::OnOpenDocument(%s)\n", lpszPathName);
	m_WavFile.Close();
	m_OriginalWavFile.Close();

	CThisApp * pApp = GetApp();
	m_bDirectMode = ((DocOpenFlags & OpenDocumentDirectMode) != 0);
	m_bReadOnly = ((DocOpenFlags & OpenDocumentReadOnly) != 0);
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
	WAVEFORMATEX * pWf = m_WavFile.GetWaveFormat();
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
	m_OriginalWaveFormat = m_WavFile.GetWaveFormat();

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
		// for compressed file, actual size of file may differ from the
		// initial size
		if (bNeedConversion)
		{
			CDecompressContext * pContext =
				new CDecompressContext(this, "Loading the compressed file...", m_OriginalWavFile.GetWaveFormat());

			pContext->m_Flags |= DecompressSavePeakFile;

			pContext->m_SrcFile = m_OriginalWavFile;
			pContext->m_DstFile = m_WavFile;

			pContext->m_SrcStart = m_OriginalWavFile.GetDataChunk()->dwDataOffset;
			pContext->m_SrcPos = pContext->m_SrcStart;
			pContext->m_SrcEnd = pContext->m_SrcStart +
								m_OriginalWavFile.GetDataChunk()->cksize;

			pContext->m_DstStart = m_WavFile.GetDataChunk()->dwDataOffset;
			pContext->m_DstCopyPos = pContext->m_DstStart;
			pContext->m_CurrentSamples = m_WavFile.NumberOfSamples();

			AllocatePeakData(pContext->m_CurrentSamples);
			// peak data will be created during decompression
			LoadPeaksForCompressedFile(m_WavFile, m_OriginalWavFile);
			SoundChanged(m_WavFile.GetFileID(), 0, 0, WaveFileSamples(), UpdateSoundDontRescanPeaks);
			pContext->Execute();
		}
		else
		{
			// just link the files
			UINT ErrorFormatID = 0;
			if (m_WavFile.SetSourceFile( & m_OriginalWavFile)
				&& m_WavFile.LoadRiffChunk()
				&& m_WavFile.LoadWaveformat())
			{
				if ( ! m_WavFile.FindData())
				{
					ErrorFormatID = IDS_UNABLE_TO_FIND_DATA_CHUNK;
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
			CheckAndLoadPeakFile(m_OriginalWavFile);
			SoundChanged(m_WavFile.GetFileID(), 0, 0, WaveFileSamples());
		}
	}
	else
	{
		CheckAndLoadPeakFile(m_WavFile);
		SoundChanged(m_WavFile.GetFileID(), 0, 0, WaveFileSamples());
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
		SavePeakInfo(m_WavFile, m_WavFile);
		AfxMessageBox(IDS_CANT_COMMIT_FILE_DATA, MB_OK | MB_ICONEXCLAMATION);
		SetModifiedFlag(FALSE);
		m_bCloseThisDocumentNow = true;
		return FALSE;
	}
	SetModifiedFlag(FALSE);
	SavePeakInfo(m_WavFile, m_WavFile);
	// direct file saved with the same format and name
	// information committed, peak info saved, file remains open
	return TRUE;
}

BOOL CWaveSoapFrontDoc::OnSaveBufferedPcmFile(int flags, LPCTSTR FullTargetName)
{
	// non-direct PCM file, the same directory, not saving a copy,
	// the same format
	// todo: InitializeTheRestOfFile in the thread
	if (m_WavFile.InitializeTheRestOfFile(500))
	{
		return PostCommitFileSave(flags, FullTargetName);
	}
	//prepare context and queue it
	CCommitFileSaveContext * pContext = new CCommitFileSaveContext(this, "Saving the file...");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}
	pContext->m_DstFile = m_WavFile;
	pContext->m_FileSaveFlags = flags;
	pContext->m_TargetName = FullTargetName;
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
											m_WavFile.GetWaveFormat(),
											ALL_CHANNELS, m_WavFile.NumberOfSamples(),
											0, NewTempFilename))
	{
		FileCreationErrorMessageBox(NewTempFilename);
		return FALSE;
	}
	LPCTSTR sOp = "Saving the file...";
	if (flags & SaveFile_SaveCopy)
	{
		sOp = "Saving the file copy...";
	}
	CFileSaveContext * pContext = new CFileSaveContext(this, sOp, "File Save");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pContext->m_NewName = FullTargetName;

	pContext->m_SrcFile = m_WavFile;
	pContext->m_DstFile = NewWaveFile;
	pContext->m_SrcStart = 0;
	pContext->m_DstStart = 0;
	pContext->m_SrcCopyPos = 0;
	pContext->m_DstCopyPos = 0;
	pContext->m_SrcEnd = m_WavFile.GetLength();
	pContext->m_DstEnd = pContext->m_SrcEnd;
	// copy 1:1
	pContext->m_SrcChan = ALL_CHANNELS;
	pContext->m_DstChan = ALL_CHANNELS;
	if (flags & SaveFile_SaveCopy)
	{
		pContext->m_Flags |= FileSaveContext_SavingCopy;
	}
	if (flags & SaveFile_SameName)
	{
		pContext->m_Flags |= FileSaveContext_SameName;
	}
	pContext->Execute();
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
		LONGLONG nNewSamples = MulDiv(m_WavFile.NumberOfSamples(), pWf->nSamplesPerSec, m_WavFile.SampleRate());
		if ( ! CanAllocateWaveFileSamplesDlg(pWf, nNewSamples))
		{
			return FALSE;
		}

		if (FALSE == NewWaveFile.CreateWaveFile(& m_OriginalWavFile, pWf, ALL_CHANNELS, nNewSamples,
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
		unsigned long NewSize = m_WavFile.NumberOfSamples() * pWf->wBitsPerSample * pWf->nChannels / 8;
		if (NewSize < 0x100000) NewSize = 0x100000; // 1 meg
		if (NewSize > 0x100 * 0x100000) NewSize = 0x100 * 0x100000; // 256 meg
		if (FALSE == NewWaveFile.CreateWaveFile(& m_OriginalWavFile, pWf, ALL_CHANNELS, NewSize,
												CreateWaveFileDontCopyInfo
												| CreateWaveFileCreateFact
												| CreateWaveFileSizeSpecified,
												NewTempFilename))
		{
			FileCreationErrorMessageBox(NewTempFilename);
			return FALSE;
		}
	}
	LPCTSTR sOp = "Converting and saving the file...";
	if (flags & SaveFile_SaveCopy)
	{
		sOp = "Saving the file copy...";
	}

	CFileSaveContext * pContext = new CFileSaveContext(this, sOp, "File Convert and Save");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pContext->m_NewName = FullTargetName;

	CConversionContext * pConvert = new CConversionContext(this, "", "");
	if (NULL == pConvert)
	{
		delete pContext;
		NotEnoughMemoryMessageBox();
		return FALSE;
	}
	pContext->m_pConvert = pConvert;
	pConvert->m_SrcFile = m_WavFile;
	pConvert->m_DstFile = NewWaveFile;
	pConvert->m_SrcStart = m_WavFile.GetDataChunk()->dwDataOffset;
	pConvert->m_DstStart = NewWaveFile.GetDataChunk()->dwDataOffset;
	pConvert->m_SrcCopyPos = pConvert->m_SrcStart;
	pConvert->m_DstCopyPos = pConvert->m_DstStart;
	pConvert->m_SrcEnd = pConvert->m_SrcStart + m_WavFile.GetDataChunk()->cksize;
	pConvert->m_DstEnd = pConvert->m_DstStart;

	pConvert->m_SrcChan = ALL_CHANNELS;
	pConvert->m_DstChan = ALL_CHANNELS;

	pContext->m_SrcFile = pConvert->m_SrcFile;
	pContext->m_DstFile = pConvert->m_DstFile;
	// fill unused data members:
	pContext->m_SrcStart = pConvert->m_SrcStart;
	pContext->m_DstStart = pConvert->m_DstStart;
	pContext->m_SrcCopyPos = pContext->m_SrcStart;
	pContext->m_DstCopyPos = pContext->m_DstCopyPos;
	pContext->m_SrcEnd = m_WavFile.GetLength();
	pContext->m_DstEnd = pContext->m_SrcEnd;

	pContext->m_SrcChan = ALL_CHANNELS;
	pContext->m_DstChan = ALL_CHANNELS;
	// if target channels is less than source, convert it before resampling,
	if (pWf->nChannels < m_WavFile.GetWaveFormat()->nChannels)
	{
		// ask which channels to save
		CCopyChannelsSelectDlg dlg;
		dlg.m_ChannelToCopy = m_PrevChannelToCopy + 1;
		if (IDOK != dlg.DoModal())
		{
			delete pContext;    // pConvert will be deleted
			return FALSE;
		}
		CChannelConvertor * pChConvertor = new CChannelConvertor;
		if (NULL == pChConvertor)
		{
			delete pContext;    // pConvert will be deleted
			NotEnoughMemoryMessageBox();
			return FALSE;
		}
		m_PrevChannelToCopy = dlg.m_ChannelToCopy - 1;
		pChConvertor->m_ChannelsToProcess = dlg.m_ChannelToCopy - 1;
		pChConvertor->m_InputChannels =  m_WavFile.GetWaveFormat()->nChannels;
		pChConvertor->m_OutputChannels =  pWf->nChannels;

		pConvert->m_ProcBatch.AddWaveProc(pChConvertor);
	}
	if (pWf->nSamplesPerSec != m_WavFile.GetWaveFormat()->nSamplesPerSec)
	{
		CResampleFilter * pFilter = new CResampleFilter;
		if (NULL == pFilter)
		{
			delete pContext;    // pConvert will be deleted
			NotEnoughMemoryMessageBox();
			return FALSE;
		}
		pFilter->InitResample(double(pWf->nSamplesPerSec)
							/ m_WavFile.GetWaveFormat()->nSamplesPerSec, 40., pWf->nChannels);
		pConvert->m_ProcBatch.AddWaveProc(pFilter);
	}
	// if target channels is more than source, convert it after resampling,
	if (pWf->nChannels > m_WavFile.GetWaveFormat()->nChannels)
	{
		CChannelConvertor * pChConvertor = new CChannelConvertor;
		if (NULL == pChConvertor)
		{
			delete pContext;    // pConvert will be deleted
			NotEnoughMemoryMessageBox();
			return FALSE;
		}
		pChConvertor->m_InputChannels =  m_WavFile.GetWaveFormat()->nChannels;
		pChConvertor->m_OutputChannels =  pWf->nChannels;
		pChConvertor->m_ChannelsToProcess = ALL_CHANNELS;
		pConvert->m_ProcBatch.AddWaveProc(pChConvertor);
	}

	if (pWf->wFormatTag != WAVE_FORMAT_PCM
		|| pWf->wBitsPerSample != 16)
	{
		CAudioConvertor * pAcmConvertor = new CAudioConvertor;
		if (NULL == pAcmConvertor)
		{
			delete pContext;    // pConvert will be deleted
			NotEnoughMemoryMessageBox();
			return FALSE;
		}
		pConvert->m_ProcBatch.AddWaveProc(pAcmConvertor);
		WAVEFORMATEX SrcFormat;
		SrcFormat.nChannels = pWf->nChannels;
		SrcFormat.nSamplesPerSec = pWf->nSamplesPerSec;
		SrcFormat.nBlockAlign = SrcFormat.nChannels * sizeof (__int16);
		SrcFormat.nAvgBytesPerSec = SrcFormat.nBlockAlign * SrcFormat.nSamplesPerSec;
		SrcFormat.cbSize = 0;
		SrcFormat.wFormatTag = WAVE_FORMAT_PCM;
		SrcFormat.wBitsPerSample = 16;
		if (! pAcmConvertor->InitConversion( & SrcFormat, pWf))
		{
			delete pContext;
			//todo: enable to convert dialog
			//NotEnoughMemoryMessageBox();
			AfxMessageBox(IDS_STRING_UNABLE_TO_CONVERT);
			return FALSE;
		}
	}
	//pContext->m_pWf = CopyWaveformat(pWf);
	//if (NULL == pContext->m_pWf)
	//{
	//delete pContext;
	//return FALSE;
	//}

	if (flags & SaveFile_SaveCopy)
	{
		pContext->m_Flags |= FileSaveContext_SavingCopy;
	}
	pContext->Execute();
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
							m_WavFile.GetWaveFormat()->nAvgBytesPerSec);
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

	CFileSaveContext * pContext = new CFileSaveContext(this,
														"Compressing and saving Mp3 file...", "Mp3 File Compress and Save");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pContext->m_NewName = FullTargetName;
	pContext->m_NewFileTypeFlags = OpenDocumentMp3File;
	CConversionContext * pConvert = new CConversionContext(this, "", "");

	if (NULL == pConvert)
	{
		delete pContext;
		NotEnoughMemoryMessageBox();
		return FALSE;
	}
	pContext->m_pConvert = pConvert;
	pConvert->m_SrcFile = m_WavFile;
	pConvert->m_DstFile = NewWaveFile;
	pConvert->m_SrcStart = m_WavFile.GetDataChunk()->dwDataOffset;
	pConvert->m_DstStart = 0;
	pConvert->m_SrcCopyPos = pConvert->m_SrcStart;
	pConvert->m_DstCopyPos = pConvert->m_DstStart;
	pConvert->m_SrcEnd = pConvert->m_SrcStart + m_WavFile.GetDataChunk()->cksize;
	pConvert->m_DstEnd = pConvert->m_DstStart;

	pConvert->m_SrcChan = ALL_CHANNELS;
	pConvert->m_DstChan = ALL_CHANNELS;

	pContext->m_SrcFile = pConvert->m_SrcFile;
	pContext->m_DstFile = pConvert->m_DstFile;
	// fill unused data members:
	pContext->m_SrcStart = pConvert->m_SrcStart;
	pContext->m_DstStart = pConvert->m_DstStart;
	pContext->m_SrcCopyPos = pContext->m_SrcStart;
	pContext->m_DstCopyPos = pContext->m_DstCopyPos;
	pContext->m_SrcEnd = m_WavFile.GetLength();
	pContext->m_DstEnd = pContext->m_DstStart;

	pContext->m_SrcChan = ALL_CHANNELS;
	pContext->m_DstChan = ALL_CHANNELS;

	if (WAVE_FORMAT_MPEGLAYER3 == pWf->wFormatTag)
	{
		CAudioConvertor * pAcmConvertor = new CAudioConvertor;
		if (NULL == pAcmConvertor)
		{
			delete pContext;    // pConvert will be deleted
			NotEnoughMemoryMessageBox();
			return FALSE;
		}
		pConvert->m_ProcBatch.AddWaveProc(pAcmConvertor);
		WAVEFORMATEX SrcFormat;
		SrcFormat.nChannels = pWf->nChannels;
		SrcFormat.nSamplesPerSec = pWf->nSamplesPerSec;
		SrcFormat.nBlockAlign = SrcFormat.nChannels * sizeof (__int16);
		SrcFormat.nAvgBytesPerSec = SrcFormat.nBlockAlign * SrcFormat.nSamplesPerSec;
		SrcFormat.cbSize = 0;
		SrcFormat.wFormatTag = WAVE_FORMAT_PCM;
		SrcFormat.wBitsPerSample = 16;
		if (! pAcmConvertor->InitConversion( & SrcFormat, pWf))
		{
			delete pContext;
			//todo: unable to convert dialog
			return FALSE;
		}
	}
	else
	{
		CLameEncConvertor * pMp3Convertor = new CLameEncConvertor;
		pConvert->m_ProcBatch.AddWaveProc(pMp3Convertor);
		if ( ! pMp3Convertor->Open(pWf))
		{
			delete pContext;
			return FALSE;
		}
	}

	if (flags & SaveFile_SaveCopy)
	{
		pContext->m_Flags |= FileSaveContext_SavingCopy;
	}
	pContext->Execute();
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
							m_WavFile.GetWaveFormat()->nAvgBytesPerSec);
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

	CFileSaveContext * pContext = new CFileSaveContext(this,
														"Compressing and saving Windows Media file...", "");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pContext->m_NewName = FullTargetName;
	pContext->m_DstFile = NewWaveFile;
	pContext->m_NewFileTypeFlags = OpenDocumentWmaFile;
	CWmaSaveContext * pConvert = new CWmaSaveContext(this, "", "");

	if (NULL == pConvert)
	{
		delete pContext;
		NotEnoughMemoryMessageBox();
		return FALSE;
	}
	pContext->m_pConvert = pConvert;
	pConvert->m_SrcFile = m_WavFile;
	pConvert->m_DstFile = NewWaveFile;
	pConvert->m_SrcStart = m_WavFile.GetDataChunk()->dwDataOffset;
	pConvert->m_DstStart = 0;
	pConvert->m_SrcCopyPos = pConvert->m_SrcStart;
	pConvert->m_DstCopyPos = pConvert->m_DstStart;
	pConvert->m_SrcEnd = pConvert->m_SrcStart + m_WavFile.GetDataChunk()->cksize;
	pConvert->m_DstEnd = pConvert->m_DstStart;

	pConvert->m_SrcChan = ALL_CHANNELS;
	pConvert->m_DstChan = ALL_CHANNELS;

	pContext->m_SrcFile = pConvert->m_SrcFile;
	pContext->m_DstFile = pConvert->m_DstFile;
	// fill unused data members:
	pContext->m_SrcStart = pConvert->m_SrcStart;
	pContext->m_DstStart = pConvert->m_DstStart;
	pContext->m_SrcCopyPos = pContext->m_SrcStart;
	pContext->m_DstCopyPos = pContext->m_DstCopyPos;
	pContext->m_SrcEnd = m_WavFile.GetLength();
	pContext->m_DstEnd = pContext->m_DstStart;

	pContext->m_SrcChan = ALL_CHANNELS;
	pContext->m_DstChan = ALL_CHANNELS;

#if 0
	WAVEFORMATEX SrcFormat;
	SrcFormat.nChannels = pWf->nChannels;
	SrcFormat.nSamplesPerSec = pWf->nSamplesPerSec;
	SrcFormat.nBlockAlign = SrcFormat.nChannels * sizeof (__int16);
	SrcFormat.nAvgBytesPerSec = SrcFormat.nBlockAlign * SrcFormat.nSamplesPerSec;
	SrcFormat.cbSize = 0;
	SrcFormat.wFormatTag = WAVE_FORMAT_PCM;
	SrcFormat.wBitsPerSample = 16;
#endif

	if (flags & SaveFile_SaveCopy)
	{
		pContext->m_Flags |= FileSaveContext_SavingCopy;
	}

	pContext->Execute();
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
	DWORD FileSize = MulDiv(pWf->nAvgBytesPerSec, m_WavFile.GetDataChunk()->cksize,
							m_WavFile.GetWaveFormat()->nAvgBytesPerSec);
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

	CFileSaveContext * pContext = new CFileSaveContext(this,
														"Saving the file in raw binary format...", "Raw File Save");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}

	pContext->m_NewName = FullTargetName;
	pContext->m_NewFileTypeFlags = OpenDocumentMp3File;
	CConversionContext * pConvert = new CConversionContext(this, "", "");

	if (NULL == pConvert)
	{
		delete pContext;
		NotEnoughMemoryMessageBox();
		return FALSE;
	}
	pContext->m_pConvert = pConvert;
	pConvert->m_SrcFile = m_WavFile;
	pConvert->m_DstFile = NewWaveFile;
	pConvert->m_SrcStart = m_WavFile.GetDataChunk()->dwDataOffset;
	pConvert->m_DstStart = 0;
	pConvert->m_SrcCopyPos = pConvert->m_SrcStart;
	pConvert->m_DstCopyPos = pConvert->m_DstStart;
	pConvert->m_SrcEnd = pConvert->m_SrcStart + m_WavFile.GetDataChunk()->cksize;
	pConvert->m_DstEnd = pConvert->m_DstStart;

	pConvert->m_SrcChan = ALL_CHANNELS;
	pConvert->m_DstChan = ALL_CHANNELS;

	pContext->m_SrcFile = pConvert->m_SrcFile;
	pContext->m_DstFile = pConvert->m_DstFile;
	// fill unused data members:
	pContext->m_SrcStart = pConvert->m_SrcStart;
	pContext->m_DstStart = pConvert->m_DstStart;
	pContext->m_SrcCopyPos = pContext->m_SrcStart;
	pContext->m_DstCopyPos = pContext->m_DstCopyPos;
	pContext->m_SrcEnd = m_WavFile.GetLength();
	pContext->m_DstEnd = pContext->m_DstStart;

	pContext->m_SrcChan = ALL_CHANNELS;
	pContext->m_DstChan = ALL_CHANNELS;

	if (WAVE_FORMAT_PCM != pWf->wFormatTag
		|| 16 != pWf->wBitsPerSample)
	{
		CAudioConvertor * pAcmConvertor = new CAudioConvertor;
		if (NULL == pAcmConvertor)
		{
			delete pContext;    // pConvert will be deleted
			NotEnoughMemoryMessageBox();
			return FALSE;
		}
		pConvert->m_ProcBatch.AddWaveProc(pAcmConvertor);
		WAVEFORMATEX SrcFormat;
		SrcFormat.nChannels = pWf->nChannels;
		SrcFormat.nSamplesPerSec = pWf->nSamplesPerSec;
		SrcFormat.nBlockAlign = SrcFormat.nChannels * sizeof (__int16);
		SrcFormat.nAvgBytesPerSec = SrcFormat.nBlockAlign * SrcFormat.nSamplesPerSec;
		SrcFormat.cbSize = 0;
		SrcFormat.wFormatTag = WAVE_FORMAT_PCM;
		SrcFormat.wBitsPerSample = 16;
		if (! pAcmConvertor->InitConversion( & SrcFormat, pWf))
		{
			delete pContext;
			//todo: enable to convert dialog
			return FALSE;
		}
	}
	else if (flags & SaveRawFileMsbFirst)
	{
		CByteSwapConvertor * pSwapConvertor = new CByteSwapConvertor;
		pConvert->m_ProcBatch.AddWaveProc(pSwapConvertor);
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
		&& pWf->nSamplesPerSec == m_WavFile.GetWaveFormat()->nSamplesPerSec
		&& pWf->nChannels == m_WavFile.GetWaveFormat()->nChannels)
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

void CWaveSoapFrontDoc::PostFileSave(CFileSaveContext * pContext)
{
	WAVEFORMATEX OldFormat = * m_WavFile.GetWaveFormat();
	WAVEFORMATEX NewFormat = * pContext->m_DstFile.GetWaveFormat();

	pContext->m_SrcFile.Close();
	// copy security descriptor
	if (m_OriginalWavFile.IsOpen()
		&& (pContext->m_Flags & FileSaveContext_SameName))
	{
		// it isn't a new file
		DWORD dwLength = 0;
		PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
		if ( ! GetFileSecurity(pContext->m_NewName, DACL_SECURITY_INFORMATION,
								NULL, 0, &dwLength)
			&& ERROR_INSUFFICIENT_BUFFER == GetLastError())
		{
			pSecurityDescriptor = (PSECURITY_DESCRIPTOR) new BYTE[dwLength];
			if (NULL != pSecurityDescriptor
				&& ::GetFileSecurity(pContext->m_NewName,
									DACL_SECURITY_INFORMATION,
									pSecurityDescriptor, dwLength, &dwLength))
			{
				SetFileSecurity(pContext->m_DstFile.GetName(), DACL_SECURITY_INFORMATION, pSecurityDescriptor);
			}
			delete[] (BYTE*)pSecurityDescriptor;
		}
	}
	m_OriginalWavFile.Close();  // close to allow delete and rename
	if (m_bDirectMode || m_bReadOnly)
	{
		m_WavFile.Close();   // close to allow delete and rename
	}
	// delete the old file
	DeleteFile(pContext->m_NewName);
	pContext->m_DstFile.CommitChanges();
	int ReopenDocumentFlags = 0;
	// rename the new file
	if ( ! pContext->m_DstFile.Rename(pContext->m_NewName, 0))
	{
		m_bClosePending = false;
		CString s;
		s.Format(IDS_UNABLE_TO_RENAME_TEMPORARY_FILE, pContext->m_DstFile.GetName(), pContext->m_NewName);
		AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	else
	{
		SetModifiedFlag(FALSE);
		// need to commit file to save its peak info with the correct timestamp
		pContext->m_DstFile.Commit();
		// we always reopen the document
		if (WAVE_FORMAT_PCM == NewFormat.wFormatTag
			&& 16 == NewFormat.wBitsPerSample)
		{
			SavePeakInfo(m_WavFile, pContext->m_DstFile);
			if (m_bClosePending)
			{
				pContext->m_DstFile.Close();
				SetModifiedFlag(FALSE);
				m_bCloseThisDocumentNow = true; // the document will be deleted
				return;
			}
			if (! m_bDirectMode || m_bReadOnly)
			{
				// if PCM format and read-only or non-direct, ask about reopening as direct
				CReopenDialog ReopenDlg;
				ReopenDlg.m_Prompt.Format(IDS_REOPEN_IN_DIRECT_MODE, pContext->m_NewName);

				CDocumentPopup pop(this);

				int result = ReopenDlg.DoModal();
				if (IDOK == result)
				{
					ReopenDocumentFlags = OpenDocumentDirectMode;
				}
				else if (IDCANCEL == result)
				{
					pContext->m_DstFile.Close();
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
					SavePeakInfo(m_WavFile, pContext->m_DstFile);
				}
				pContext->m_DstFile.Close();
				SetModifiedFlag(FALSE);
				m_bCloseThisDocumentNow = true; // the document will be deleted
				return;
			}
			// if non-PCM format, ask about reloading the file
			// if format changed, always reload
			if (OldFormat.nSamplesPerSec == NewFormat.nSamplesPerSec
				&& OldFormat.nChannels == NewFormat.nChannels)
			{
				SavePeakInfo(m_WavFile, pContext->m_DstFile);
				CReopenCompressedFileDialog dlg;
				dlg.m_Text.Format(IDS_RELOAD_COMPRESSED_FILE, LPCTSTR(pContext->m_NewName));

				CDocumentPopup pop(this);
				int result = dlg.DoModal();

				if (IDOK == result)
				{
					// set new title
					SetPathName(pContext->m_NewName);
					m_OriginalWavFile = pContext->m_DstFile;
					m_OriginalWaveFormat = pContext->m_DstFile.GetWaveFormat();
					pContext->m_DstFile.Close();
					ASSERT(m_WavFile.IsOpen());
					//todo
					return; //??
				}
				else if (IDCANCEL == result)
				{
					pContext->m_DstFile.Close();
					//SetModifiedFlag(FALSE);
					m_bCloseThisDocumentNow = true; // the document will be deleted
					return;
				}
			}
			else
			{
				// samples or channels changed
				CReopenConvertedFileDlg dlg;
				dlg.m_Text.Format(IDS_SHOULD_RELOAD_COMPRESSED_FILE, LPCTSTR(pContext->m_NewName));

				CDocumentPopup pop(this);

				int result = dlg.DoModal();
				if (IDCANCEL == result)
				{
					pContext->m_DstFile.Close();
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

	pContext->m_DstFile.Close();
	// file will be reopened
	if (OldFormat.nSamplesPerSec != NewFormat.nSamplesPerSec)
	{
		DeleteUndo();
		DeleteRedo();
		// recalculate selection
		long SelStart = MulDiv(m_SelectionStart, NewFormat.nSamplesPerSec,
								OldFormat.nSamplesPerSec);
		long SelEnd = MulDiv(m_SelectionEnd, NewFormat.nSamplesPerSec,
							OldFormat.nSamplesPerSec);
		long Caret = MulDiv(m_CaretPosition, NewFormat.nSamplesPerSec,
							OldFormat.nSamplesPerSec);
		int Chan = m_SelectedChannel;
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
		m_SelectedChannel = ALL_CHANNELS;
	}
	if ( ! OnOpenDocument(pContext->m_NewName, ReopenDocumentFlags | m_FileTypeFlags))
	{
		// message box, then close the document
		CString s;
		s.Format(IDS_UNABLE_TO_REOPEN_AS, pContext->m_NewName);
		AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
		m_bCloseThisDocumentNow = true; // the document will be deleted
		return;
	}
	SetPathName(pContext->m_NewName);
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
		SavePeakInfo(m_WavFile, m_WavFile);
		SetModifiedFlag(FALSE);
		if (m_bClosePending)
		{
			m_WavFile.Close();
			m_bCloseThisDocumentNow = true;
			return TRUE;
		}

		CReopenDialog ReopenDlg;
		ReopenDlg.m_Prompt.Format(IDS_REOPEN_IN_DIRECT_MODE, m_WavFile.GetName());
		int result;
		{
			CDocumentPopup pop(this);
			result = ReopenDlg.DoModal();
		}

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
	while ( ! m_UpdateList.IsEmpty())
	{
		CSoundUpdateInfo * pInfo;
		{
			CSimpleCriticalSectionLock lock(m_cs);
			pInfo = m_UpdateList.RemoveHead();
		}
		UpdateAllViews(NULL, pInfo->UpdateCode, pInfo);
		delete pInfo;
	}
	while ( ! m_RetiredList.IsEmpty())
	{
		COperationContext * pContext;
		{
			CSimpleCriticalSectionLock lock(m_cs);
			pContext = m_RetiredList.RemoveHead();
		}
		pContext->PostRetire(FALSE);     // deletes it usually
	}
	m_bInOnIdle = false;
}


void CWaveSoapFrontDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(IsModified() && ! m_bReadOnly && ! m_OperationInProgress);
}

void CWaveSoapFrontDoc::OnUpdateFileSaveAs(CCmdUI* pCmdUI)
{
	// in direct or readonly mode only Save Copy command is allowed
	pCmdUI->Enable(! m_OperationInProgress && ! m_bDirectMode && ! m_bReadOnly);
}

void CWaveSoapFrontDoc::OnUpdateFileSaveCopyAs(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(! m_OperationInProgress);
}
void CWaveSoapFrontDoc::OnEditSelectAll()
{
	long len = WaveFileSamples();
	SetSelection(0, len, ALL_CHANNELS, len, SetSelection_MakeCaretVisible);
}

void CWaveSoapFrontDoc::OnEditSelection()
{
	CSelectionDialog dlg;
	dlg.m_Start = m_SelectionStart;
	dlg.m_End = m_SelectionEnd;
	dlg.m_CaretPosition = m_CaretPosition;
	dlg.m_Length = m_SelectionEnd - m_SelectionStart;
	dlg.m_FileLength = WaveFileSamples();
	dlg.m_Chan = m_SelectedChannel + 1;
	dlg.m_pWf = WaveFormat();
	dlg.m_TimeFormat = GetApp()->m_SoundTimeFormat;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	SetSelection(dlg.m_Start, dlg.m_End, dlg.m_Chan - 1, dlg.m_End, true);

}

void CWaveSoapFrontDoc::OnUpdateEditSelection(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_OperationInProgress);
}

static void LimitUndoRedo(KListEntry<COperationContext> * pEntry, int MaxNum, size_t MaxSize)
{
	size_t Size = 0;
	int Num = 0;
	for (COperationContext * pContext = pEntry->Next()
		; pContext != pEntry->Head(); pContext = pContext->Next())
	{
		Num++;
		CUndoRedoContext * pUndoRedo = dynamic_cast<CUndoRedoContext *>(pContext);
		if (NULL != pUndoRedo
			&& pUndoRedo->m_SrcFile.IsOpen())
		{
			Size += pUndoRedo->m_SrcFile.GetLength();
		}
		if ((MaxNum != 0 && Num > MaxNum)
			|| (MaxSize != 0 && Size > MaxSize))
		{
			while (pContext->Next() != pEntry->Head())
			{
				delete pContext->RemoveHead();
			}
			break;
		}
	}
}

void CWaveSoapFrontDoc::AddUndoRedo(CUndoRedoContext * pContext)
{
	CThisApp * pApp = GetApp();
	if (pContext->m_Flags & RedoContext)
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
	else
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
}

void CWaveSoapFrontDoc::OnUpdateSoundPlay(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(! m_OperationInProgress && m_WavFile.IsOpen() && WaveFileSamples() != 0);
}

void CWaveSoapFrontDoc::OnSoundPlay()
{
	if (m_OperationInProgress)
	{
		return;
	}
	CSoundPlayContext * pContext = new CSoundPlayContext(this);
	MMCKINFO * pDatack = WaveDataChunk();
	pContext->m_SamplePlayed = m_SelectionStart;
	pContext->m_FirstSamplePlayed = m_SelectionStart;
	pContext->m_Begin = m_SelectionStart * WaveSampleSize() + pDatack->dwDataOffset;
	if (m_SelectionStart == m_SelectionEnd)
	{
		pContext->m_End = pDatack->dwDataOffset + pDatack->cksize;
	}
	else
	{
		pContext->m_End = m_SelectionEnd * WaveSampleSize() + pDatack->dwDataOffset;
	}
	pContext->m_CurrentPlaybackPos = pContext->m_Begin;
	pContext->m_Chan = m_SelectedChannel;

	// TODO: move Open, Allocate to the Context->Init(), error messages to PostRetire
	CThisApp * pApp = GetApp();
	pContext->m_PlaybackDevice = pApp->m_DefaultPlaybackDevice;
	if (false == pContext->m_WaveOut.AllocateBuffers(pApp->m_SizePlaybackBuffers,
													pApp->m_NumPlaybackBuffers))
	{
		AfxMessageBox(IDS_STRING_UNABLE_TO_ALLOCATE_AUDIO_BUFFERS,
					MB_OK | MB_ICONEXCLAMATION);
		delete pContext;
		return;
	}
	if (MMSYSERR_NOERROR != pContext->m_WaveOut.Open(
													pContext->m_PlaybackDevice, WaveFormat(), WAVE_FORMAT_QUERY))
	{
		AfxMessageBox(IDS_STRING_UNABLE_TO_OPEN_AUDIO_DEVICE,
					MB_OK | MB_ICONEXCLAMATION);
		delete pContext;
		return;
	}
	m_PlayingSound = true;
	pContext->Execute();
}

void CWaveSoapFrontDoc::OnUpdateSoundStop(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_OperationInProgress && m_PlayingSound);
}

void CWaveSoapFrontDoc::OnUpdateIndicatorFileSize(CCmdUI* pCmdUI)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	SetStatusString(pCmdUI,
					SampleToString(WaveFileSamples(), WaveFormat()->nSamplesPerSec,
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
						SampleToString(m_SelectionEnd - m_SelectionStart, WaveFormat()->nSamplesPerSec,
										GetApp()->m_SoundTimeFormat));
	}
	else
	{
		SetStatusString(pCmdUI, _T("            "));
	}
}

void CWaveSoapFrontDoc::OnUpdateIndicatorCurrentPos(CCmdUI* pCmdUI)
{
	unsigned TimeMs;
	int TimeFormat = GetApp()->m_SoundTimeFormat;

	if ( ! m_WavFile.IsOpen())
	{
		return;
	}

	CSoundPlayContext * pCx = NULL;
	if (m_PlayingSound && ! m_OpList.IsEmpty())
	{
		pCx = dynamic_cast<CSoundPlayContext *>(m_OpList.Next());
	}

	if (m_PlayingSound && NULL != pCx)
	{
		if ((TimeFormat & SampleToString_Mask) == SampleToString_Sample)
		{
			SetStatusString(pCmdUI,
							SampleToString(pCx->m_SamplePlayed, WaveFormat()->nSamplesPerSec,
											TimeFormat));
		}
		else
		{
			TimeMs = double(pCx->m_SamplePlayed)
					/ m_WavFile.GetWaveFormat()->nSamplesPerSec * 1000.;
			TimeMs -= TimeMs % 100;
			int BeginTimeMs = double(pCx->m_FirstSamplePlayed)
							/ WaveFormat()->nSamplesPerSec * 1000.;
			if (TimeMs < BeginTimeMs)
			{
				TimeMs = BeginTimeMs;
			}
			SetStatusString(pCmdUI, TimeToHhMmSs(TimeMs, TimeFormat));
		}
	}
	else
	{
		SetStatusString(pCmdUI, SampleToString(m_CaretPosition,
												WaveFormat()->nSamplesPerSec, TimeFormat));
	}
}

void CWaveSoapFrontDoc::OnUpdateIndicatorSampleRate(CCmdUI* pCmdUI)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	SetStatusString(pCmdUI, LtoaCS(WaveFormat()->nSamplesPerSec));
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
	if ( ! m_OperationInProgress)
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
		if ( ! m_OpList.IsEmpty())
		{
			pCx = dynamic_cast<CSoundPlayContext *>(m_OpList.Next());
			if (pCx)
			{
				pCx->m_bPauseRequested = true;
			}
		}
		m_StopOperation = true;
	}
}

void CWaveSoapFrontDoc::OnUpdateSoundPause(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_OperationInProgress && m_PlayingSound);
}

BOOL CWaveSoapFrontDoc::SaveModified()
{
	if (m_OperationInProgress)
	{
		if (m_OperationNonCritical)
		{
			m_StopOperation = TRUE;
			DWORD BeginTime = GetTickCount();
			do {
				Sleep(50);
				OnIdle();
			}
			while (m_OperationInProgress && GetTickCount() - BeginTime < 5000);
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
	return CDocument::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

inline BOOL CWaveSoapFrontDoc::IsModified()
{
	return m_ModificationSequenceNumber != 0;
}

void CWaveSoapFrontDoc::IncrementModified(BOOL bDeleteRedo, int KeepPreviousUndo)
{
	// bDeleteRedo is FALSE for Redo command ONLY
	BOOL OldModified = CWaveSoapFrontDoc::IsModified();
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

	if (CWaveSoapFrontDoc::IsModified() != OldModified)
	{
		UpdateFrameTitles();        // will cause name change in views
	}
}

void CWaveSoapFrontDoc::DecrementModified()   // called at UNDO
{
	BOOL OldModified = CWaveSoapFrontDoc::IsModified();
	m_ModificationSequenceNumber--;

	if (! RedoEnabled())
	{
		// since it is unable to redo, delete all previous redo
		DeleteRedo();
	}

	if (CWaveSoapFrontDoc::IsModified() != OldModified)
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
		BOOL OldModified = CWaveSoapFrontDoc::IsModified();
		m_ModificationSequenceNumber = 0;
		if (OldModified)
		{
			UpdateFrameTitles();        // will cause name change in views
		}
	}
}

void CWaveSoapFrontDoc::PlaybackPositionNotify(long position, int channel)
{
	CSoundUpdateInfo * pui = new CSoundUpdateInfo;
	if (NULL == pui)
	{
		return;
	}
	pui->UpdateCode = UpdatePlaybackPositionChanged;
	pui->FileID = m_WavFile.GetFileID();
	pui->Begin = position;
	pui->End = channel;

	pui->Length = -1;
	{
		CSimpleCriticalSectionLock lock(m_cs);

		for (CSoundUpdateInfo * pEntry = m_UpdateList.Next()
			; pEntry != m_UpdateList.Head(); pEntry = pEntry->Next())
		{
			if (pEntry->UpdateCode == UpdatePlaybackPositionChanged)
			{
				CSoundUpdateInfo * pTmpEntry = pEntry->Prev();
				m_UpdateList.RemoveEntry(pEntry);
				delete pEntry;
				pEntry = pTmpEntry;
			}
		}
		m_UpdateList.InsertTail(pui);
	}
	::PostMessage(GetApp()->m_pMainWnd->m_hWnd, WM_KICKIDLE, 0, 0);
}

// return TRUE if there was UpdatePlaybackPositionChanged request queued
// return FALSE otherwise
BOOL CWaveSoapFrontDoc::ProcessUpdatePlaybackPosition()
{
	m_cs.Lock();

	for (CSoundUpdateInfo * pEntry = m_UpdateList.Next()
		; pEntry != m_UpdateList.Head(); pEntry = pEntry->Next())
	{
		if (pEntry->UpdateCode == UpdatePlaybackPositionChanged)
		{
			m_UpdateList.RemoveEntry(pEntry);
			m_cs.Unlock();

			UpdateAllViews(NULL, pEntry->UpdateCode, pEntry);

			delete pEntry;
			return TRUE;
		}
	}

	m_cs.Unlock();
	return FALSE;
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
	if (pWf->nSamplesPerSec == SampleRate)
	{
		return;
	}
	if (UndoEnabled())
	{
		CUndoRedoContext * pContext = new CUndoRedoContext(this, "Sample Rate Change");
		if (NULL == pContext)
		{
		}
		else
		{
			pContext->m_Flags |= UndoContextReplaceFormat;
			pContext->m_OldWaveFormat = * pWf;
			AddUndoRedo(pContext);
		}
	}
	pWf->nSamplesPerSec = SampleRate;
	m_WavFile.GetFmtChunk()->dwFlags |= MMIO_DIRTY;
	SetModifiedFlag();
	UpdateAllViews(NULL, UpdateSampleRateChanged);
}

void CWaveSoapFrontDoc::OnUpdateSampleRate(CCmdUI* pCmdUI, unsigned SampleRate)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	pCmdUI->SetRadio(m_WavFile.SampleRate() == SampleRate);
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

void CWaveSoapFrontDoc::ChangeChannels(int nChannels)
{
	if (m_bReadOnly
		|| ! m_WavFile.IsOpen()
		|| nChannels == WaveChannels())
	{
		return;
	}
	WAVEFORMATEX * pWf = m_WavFile.GetWaveFormat();
	int SampleCount = WaveFileSamples();
	if (0 == SampleCount)
	{
		// easy change
		if (UndoEnabled())
		{
			CUndoRedoContext * pContext = new CUndoRedoContext(this, "Number Of Channels Change");
			if (NULL == pContext)
			{
				NotEnoughMemoryMessageBox();
				return;
			}
			else
			{
				pContext->m_Flags |= UndoContextReplaceFormat;
				pContext->m_OldWaveFormat = * pWf;
				AddUndoRedo(pContext);
			}
		}

		pWf->nChannels = nChannels;
		m_WavFile.GetFmtChunk()->dwFlags |= MMIO_DIRTY;
		SetModifiedFlag();
		UpdateAllViews(NULL);
		// update wave view boundaries
		SoundChanged(m_WavFile.GetFileID(), 0, 0, 0);
		UpdateAllViews(NULL, CWaveSoapFrontDoc::UpdateWholeFileChanged);
		return;
	}
	WAVEFORMATEX NewFormat = *pWf;
	NewFormat.nChannels = nChannels;
	int nSrcChan = ALL_CHANNELS;
	if ( ! CanAllocateWaveFileSamplesDlg( & NewFormat, SampleCount))
	{
		return;
	}

	if (nChannels < WaveChannels())
	{
		CCopyChannelsSelectDlg dlg;
		dlg.m_ChannelToCopy = m_PrevChannelToCopy + 1;
		if (IDOK != dlg.DoModal())
		{
			return;
		}
		nSrcChan = dlg.m_ChannelToCopy - 1;
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
	CChannelConvertor * pConvert = new CChannelConvertor;
	if (NULL == pConvert)
	{
		NotEnoughMemoryMessageBox();
		return;
	}
	CConversionContext * pContext = new CConversionContext(this, "Changing number of channels...",
															"Number Of Channels Change");
	if (NULL == pContext)
	{
		delete pConvert;
		NotEnoughMemoryMessageBox();
		return;
	}
	pConvert->m_InputChannels = WaveChannels();
	pConvert->m_OutputChannels = nChannels;
	pConvert->m_ChannelsToProcess = nSrcChan;

	pContext->m_ProcBatch.AddWaveProc(pConvert);
	// create new temporary file
	CWaveFile DstFile;
	if ( ! DstFile.CreateWaveFile( & m_WavFile, & NewFormat, ALL_CHANNELS, SampleCount,
									CreateWaveFileDeleteAfterClose
									| CreateWaveFilePcmFormat
									| CreateWaveFileTemp,
									NULL))
	{
		delete pContext;
		FileCreationErrorMessageBox(NULL);
		return;
	}
	pContext->InitDestination(DstFile, 0, SampleCount, ALL_CHANNELS, FALSE);
	pContext->InitSource(m_WavFile, 0, SampleCount, ALL_CHANNELS);

	if (UndoEnabled())
	{
		CUndoRedoContext * pUndo = new CUndoRedoContext(this, "Number Of Channels Change");
		if (NULL == pUndo)
		{
			NotEnoughMemoryMessageBox();
			return;
		}
		pUndo->m_Flags |= UndoContextReplaceWholeFile;
		pUndo->m_SrcFile = m_WavFile;
		pContext->m_pUndoContext = pUndo;
	}
	pContext->m_Flags |= ConvertContextReplaceWholeFile;
	pContext->Execute();
}

void CWaveSoapFrontDoc::OnChannelsMono()
{
	ChangeChannels(1);
}

void CWaveSoapFrontDoc::OnUpdateChannelsMono(CCmdUI* pCmdUI)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	pCmdUI->SetRadio(WaveChannels() == 1);
}

void CWaveSoapFrontDoc::OnChannelsStereo()
{
	ChangeChannels(2);
}

void CWaveSoapFrontDoc::OnUpdateChannelsStereo(CCmdUI* pCmdUI)
{
	if ( ! m_WavFile.IsOpen())
	{
		return;
	}
	pCmdUI->SetRadio(WaveChannels() == 2);
}

void CWaveSoapFrontDoc::OnProcessChangevolume()
{
	if (m_OperationInProgress || m_bReadOnly)
	{
		return;
	}
	long start = m_SelectionStart;
	long end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}
	int channel = m_SelectedChannel;
	if (ChannelsLocked())
	{
		channel = ALL_CHANNELS;
	}

	CVolumeChangeDialog dlg;
	CThisApp * pApp = GetApp();

	dlg.m_Start = start;
	dlg.m_End = end;
	dlg.m_CaretPosition = m_CaretPosition;
	dlg.m_Chan = channel;
	dlg.m_pWf = m_WavFile.GetWaveFormat();
	dlg.m_bLockChannels = m_bChannelsLocked;
	dlg.m_bUndo = UndoEnabled();
	dlg.m_TimeFormat = GetApp()->m_SoundTimeFormat;
	dlg.m_FileLength = WaveFileSamples();

	if (1 == WaveChannels())
	{
		dlg.SetTemplate(IDD_DIALOG_VOLUME_CHANGE_MONO);
	}

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	CVolumeChangeContext * pContext =
		new CVolumeChangeContext(this, "Changing volume...", "Volume Change");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	if (0 == dlg.m_DbPercent)   // dBs
	{
		pContext->m_VolumeLeft = pow(10., dlg.m_dVolumeLeftDb / 20.);
		if (dlg.m_bLockChannels || WaveChannels() == 1)
		{
			dlg.m_Chan = ALL_CHANNELS;
			pContext->m_VolumeRight = pContext->m_VolumeLeft;
		}
		else
		{
			pContext->m_VolumeRight = pow(10., dlg.m_dVolumeRightDb / 20.);
		}
	}
	else // percents
	{
		pContext->m_VolumeLeft = 0.01 * dlg.m_dVolumeLeftPercent;
		if (dlg.m_bLockChannels || WaveChannels() == 1)
		{
			dlg.m_Chan = ALL_CHANNELS;
			pContext->m_VolumeRight = pContext->m_VolumeLeft;
		}
		else
		{
			pContext->m_VolumeRight = 0.01 * dlg.m_dVolumeRightPercent;
		}
	}

	if (1. == pContext->m_VolumeLeft
		&& 1. == pContext->m_VolumeRight)
	{
		// nothing to do
		delete pContext;
		return;
	}
	// check if the values can exceed the maximum
	int MinL, MaxL, MinR, MaxR;
	GetSoundMinMax(MinL, MaxL, MinR, MaxR, dlg.m_Start, dlg.m_End);
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

	if ((dlg.m_Chan != 1 && pContext->m_VolumeLeft * MaxL > 32767.)
		|| (WaveChannels() > 1 && dlg.m_Chan != 0
			&& pContext->m_VolumeRight * MaxR > 32767.))
	{
		CString s;
		s.Format(IDS_SOUND_MAY_BE_CLIPPED, GetTitle());
		if (IDYES != AfxMessageBox(s, MB_YESNO | MB_ICONQUESTION))
		{
			delete pContext;
			return;
		}
	}

	if ( ! pContext->InitDestination(m_WavFile, dlg.m_Start,
									dlg.m_End, dlg.m_Chan, dlg.m_bUndo))
	{
		delete pContext;
		return;
	}
	pContext->Execute();
	SetModifiedFlag(TRUE, dlg.m_bUndo);
}

void CWaveSoapFrontDoc::GetSoundMinMax(int & MinL, int & MaxL,
										int & MinR, int & MaxR,
										long begin, long end)
{
	MinL = 0;
	MinR = 0;
	MaxL = 0;
	MaxR = 0;
	int IntMinR = 0x7FFF;
	int IntMinL = 0x7FFF;
	int IntMaxR = -0x8000;
	int IntMaxL = -0x8000;
	int PeakBegin = (begin & ~m_PeakDataGranularity) / m_PeakDataGranularity;
	int PeakEnd = ((end + m_PeakDataGranularity - 1) & ~m_PeakDataGranularity) / m_PeakDataGranularity;
	if (NULL == m_pPeaks)
	{
		return;
	}
	if (WaveChannels() == 1)
	{
		if (PeakEnd > m_WavePeakSize)
		{
			PeakEnd = m_WavePeakSize;
		}
		for (int i = PeakBegin; i < PeakEnd; i++)
		{
			if (m_pPeaks[i].high > m_pPeaks[i].low)
			{
				if (IntMaxL < m_pPeaks[i].high)
				{
					IntMaxL = m_pPeaks[i].high;
				}
				if (IntMinL > m_pPeaks[i].low)
				{
					IntMinL = m_pPeaks[i].low;
				}
			}
		}
		MaxL = IntMaxL;
		MinL = IntMinL;
	}
	else
	{
		if (PeakEnd > m_WavePeakSize / 2)
		{
			PeakEnd = m_WavePeakSize / 2;
		}
		for (int i = PeakBegin; i < PeakEnd; i++)
		{
			if (m_pPeaks[i * 2].high > m_pPeaks[i * 2].low)
			{
				if (IntMaxL < m_pPeaks[i * 2].high)
				{
					IntMaxL = m_pPeaks[i * 2].high;
				}
				if (IntMinL > m_pPeaks[i * 2].low)
				{
					IntMinL = m_pPeaks[i * 2].low;
				}
			}
			if (m_pPeaks[i * 2 + 1].high > m_pPeaks[i * 2 + 1].low)
			{
				if (IntMaxR < m_pPeaks[i * 2 + 1].high)
				{
					IntMaxR = m_pPeaks[i * 2 + 1].high;
				}
				if (IntMinR > m_pPeaks[i * 2 + 1].low)
				{
					IntMinR = m_pPeaks[i * 2 + 1].low;
				}
			}
		}
		MinL = IntMinL;
		MinR = IntMinR;
		MaxL = IntMaxL;
		MaxR = IntMaxR;
	}
}

void CWaveSoapFrontDoc::OnUpdateProcessChangevolume(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bReadOnly && ! m_OperationInProgress
					&& m_WavFile.IsOpen() && WaveFileSamples() != 0);
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
		char flag = 1;
		if (m_bDirectMode)
		{
			flag |= OpenDocumentDirectMode;
		}
		if (m_bReadOnly)
		{
			flag |= OpenDocumentReadOnly;
		}

		AfxGetApp()->AddToRecentFileList(CString(flag, 1) + m_strPathName);
	}

	ASSERT_VALID(this);
}

void CWaveSoapFrontDoc::OnUpdateProcessDcoffset(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bReadOnly && ! m_OperationInProgress && m_WavFile.IsOpen() && WaveFileSamples() != 0);
}

void CWaveSoapFrontDoc::OnProcessDcoffset()
{
	if (m_OperationInProgress || m_bReadOnly)
	{
		return;
	}
	long start = m_SelectionStart;
	long end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}
	int channel = m_SelectedChannel;
	if (ChannelsLocked())
	{
		channel = ALL_CHANNELS;
	}

	CDcOffsetDialog dlg;
	CThisApp * pApp = GetApp();
	dlg.m_Start = start;
	dlg.m_End = end;
	dlg.m_CaretPosition = m_CaretPosition;
	dlg.m_Chan = channel;
	dlg.m_pWf = m_WavFile.GetWaveFormat();
	dlg.m_bUndo = UndoEnabled();
	dlg.m_TimeFormat = GetApp()->m_SoundTimeFormat;
	dlg.m_FileLength = WaveFileSamples();

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	CDcOffsetContext * pContext =
		new CDcOffsetContext(this, "Adjusting DC...", "DC Adjust");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	if (0 == dlg.m_DcSelectMode)   // Calculate DC offset
	{
		pContext->m_OffsetLeft = 0;
		pContext->m_OffsetRight = 0;
		pContext->m_pScanContext = new CStatisticsContext(this, "Scanning for DC offset...", "");
		if (NULL == pContext->m_pScanContext)
		{
			delete pContext;
			NotEnoughMemoryMessageBox();
			return;
		}
		pContext->m_pScanContext->m_Flags |= StatisticsContext_DcOnly;
		long EndScanSample = dlg.m_End;
		if (dlg.m_b5SecondsDC)
		{
			// 5.4 seconds is actually scanned, to compensate for turntable rotation
			EndScanSample = dlg.m_Start
							+ 54 * m_WavFile.GetWaveFormat()->nSamplesPerSec / 10;
			if (EndScanSample > dlg.m_End)
			{
				EndScanSample = dlg.m_End;
			}
		}
		pContext->m_pScanContext->InitDestination(m_WavFile, dlg.m_Start,
												EndScanSample, dlg.m_Chan, FALSE);
		pContext->m_Flags |= ContextScanning;
	}
	else // Use specified DC offset
	{
		pContext->m_OffsetLeft = dlg.m_nDcOffset;
		pContext->m_OffsetRight = dlg.m_nDcOffset;
		// check if the values can exceed the maximum
		if (0 == dlg.m_nDcOffset)
		{
			// nothing to do
			delete pContext;
			return;
		}
		int MinL, MaxL, MinR, MaxR;
		GetSoundMinMax(MinL, MaxL, MinR, MaxR, dlg.m_Start, dlg.m_End);

		MinL += dlg.m_nDcOffset;
		MaxL += dlg.m_nDcOffset;
		MinR += dlg.m_nDcOffset;
		MaxR += dlg.m_nDcOffset;

		if ((dlg.m_Chan != 1 && (MaxL > 0x7FFF || MinL < -0x8000))
			|| (WaveChannels() > 1 && dlg.m_Chan != 0
				&& (MaxR > 0x7FFF || MinR < -0x8000)))
		{
			CString s;
			s.Format(IDS_SOUND_MAY_BE_CLIPPED, GetTitle());
			if (IDYES != AfxMessageBox(s, MB_YESNO | MB_ICONQUESTION))
			{
				delete pContext;
				return;
			}
		}
	}


	if ( ! pContext->InitDestination(m_WavFile, dlg.m_Start,
									dlg.m_End, dlg.m_Chan, dlg.m_bUndo))
	{
		delete pContext;
		return;
	}
	pContext->Execute();
	SetModifiedFlag(TRUE, dlg.m_bUndo);
}

void CWaveSoapFrontDoc::OnUpdateProcessInsertsilence(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bReadOnly && ! m_OperationInProgress);
}

void CWaveSoapFrontDoc::OnProcessInsertsilence()
{
	if (m_OperationInProgress
		|| m_bReadOnly)
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
	int channel = m_SelectedChannel;
	if (ChannelsLocked())
	{
		channel = ALL_CHANNELS;
	}
	CInsertSilenceDialog dlg;
	dlg.m_Start = m_CaretPosition;
	dlg.m_Length = 0;
	dlg.m_pWf = WaveFormat();
	dlg.m_FileLength = WaveFileSamples();
	dlg.m_nChannel = channel + 1;
	dlg.m_TimeFormat = GetApp()->m_SoundTimeFormat;
	dlg.m_CaretPosition = m_CaretPosition;

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	GetApp()->m_SoundTimeFormat = dlg.m_TimeFormat;
	channel = dlg.m_nChannel - 1;

	if (dlg.m_Length <= 0)
	{
		// nothing to change
		return;
	}

	if ( ! CanExpandWaveFileDlg(m_WavFile, dlg.m_Length))
	{
		return;
	}

	CInsertSilenceContext * pContext = new CInsertSilenceContext(this,
																"Inserting silence...", "Insert Silence");

	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	if ( ! pContext->InitExpand(m_WavFile, dlg.m_Start,
								dlg.m_Length, channel, FALSE))   // undo isn't necessary
	{
		delete pContext;
		return;
	}
	SoundChanged(WaveFileID(), 0, 0, WaveFileSamples());
	pContext->Execute();
	SetModifiedFlag(TRUE, TRUE);    // kepp previous undo
}

void CWaveSoapFrontDoc::OnUpdateProcessMute(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bReadOnly
					&& ! m_OperationInProgress
					&& m_SelectionStart != m_SelectionEnd);
}

void CWaveSoapFrontDoc::OnProcessMute()
{
	long start = m_SelectionStart;
	long end = m_SelectionEnd;
	if (m_OperationInProgress
		|| m_bReadOnly
		|| start == end)
	{
		// don't do anything
		return;
	}
	int channel = m_SelectedChannel;
	if (ChannelsLocked())
	{
		channel = ALL_CHANNELS;
	}

	CVolumeChangeContext * pContext =
		new CVolumeChangeContext(this, "Muting the selection...", "Mute");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	pContext->m_VolumeLeft = 0.;
	pContext->m_VolumeRight = 0.;

	if ( ! pContext->InitDestination(m_WavFile, start,
									end, channel, UndoEnabled()))
	{
		delete pContext;
		return;
	}
	pContext->Execute();
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::OnUpdateProcessNormalize(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bReadOnly && ! m_OperationInProgress && m_WavFile.IsOpen() && WaveFileSamples() != 0);
}

void CWaveSoapFrontDoc::OnProcessNormalize()
{
	if (m_OperationInProgress || m_bReadOnly)
	{
		return;
	}
	long start = m_SelectionStart;
	long end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}
	int channel = m_SelectedChannel;
	if (ChannelsLocked())
	{
		channel = ALL_CHANNELS;
	}

	CNormalizeSoundDialog dlg;
	CThisApp * pApp = GetApp();
	dlg.m_Start = start;
	dlg.m_End = end;
	dlg.m_CaretPosition = m_CaretPosition;
	dlg.m_Chan = channel;
	dlg.m_pWf = m_WavFile.GetWaveFormat();
	dlg.m_bLockChannels = m_bChannelsLocked;
	dlg.m_bUndo = UndoEnabled();
	dlg.m_TimeFormat = GetApp()->m_SoundTimeFormat;
	dlg.m_FileLength = WaveFileSamples();

	//if (1 == WaveChannels())
	//{
	//dlg.SetTemplate(IDD_DIALOG_VOLUME_CHANGE_MONO);
	//}

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	CNormalizeContext * pContext =
		new CNormalizeContext(this, "Normalizing sound level...", "Normalize");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	if (dlg.m_bLockChannels || WaveChannels() == 1)
	{
		dlg.m_Chan = ALL_CHANNELS;
	}
	if (0 == dlg.m_DbPercent)   // dBs
	{
		pContext->m_LimitLevel = pow(10., dlg.m_dLevelDb / 20.);
	}
	else // percents
	{
		pContext->m_LimitLevel = 0.01 * dlg.m_dLevelPercent;
	}

	if ( ! pContext->InitDestination(m_WavFile, dlg.m_Start,
									dlg.m_End, dlg.m_Chan, dlg.m_bUndo))
	{
		delete pContext;
		return;
	}
	CStatisticsContext * pStatContext =
		new CStatisticsContext(this, "Scanning for maximum amplitude...", "");
	if (NULL == pStatContext)
	{
		delete pContext;
		NotEnoughMemoryMessageBox();
		return;
	}
	pStatContext->InitDestination(m_WavFile, dlg.m_Start,
								dlg.m_End, dlg.m_Chan, FALSE);
	pStatContext->m_Flags |= StatisticsContext_MinMaxOnly;

	pContext->m_bEqualChannels = dlg.m_bLockChannels;
	pContext->m_pScanContext = pStatContext;
	pContext->m_Flags |= ContextScanning;

	pContext->Execute();
	SetModifiedFlag(TRUE, dlg.m_bUndo);
}

void CWaveSoapFrontDoc::OnUpdateProcessResample(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bReadOnly && ! m_OperationInProgress);
}

void CWaveSoapFrontDoc::OnProcessResample()
{
	if (m_bReadOnly || m_OperationInProgress)
	{
		return;
	}

	CResampleDialog dlg;
	CThisApp * pApp = GetApp();
	dlg.m_bUndo = UndoEnabled();

	dlg.m_OldSampleRate = WaveFormat()->nSamplesPerSec;
	dlg.m_bCanOnlyChangeSamplerate = WaveFileSamples() == 0;

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	double ResampleQuality = 40.;

	long OldSamplingRate = WaveFormat()->nSamplesPerSec;
	long NewSamplingRate;
	double ResampleRatio;
	if (dlg.m_bChangeSamplingRate)
	{
		NewSamplingRate = dlg.m_NewSampleRate;
		ResampleRatio = double(NewSamplingRate) / OldSamplingRate;
	}
	else
	{
		NewSamplingRate = WaveFormat()->nSamplesPerSec * dlg.m_TempoChange;
		ResampleRatio = dlg.m_TempoChange;
	}
	if (dlg.m_bChangeRateOnly)
	{
		SetSampleRate(NewSamplingRate);
		return;
	}

	LONGLONG NewSampleCount =
		UInt32x32To64(WaveFileSamples(), NewSamplingRate) / OldSamplingRate;
	if ( ! CanAllocateWaveFileSamplesDlg(m_WavFile.GetWaveFormat(), NewSampleCount))
	{
		return;
	}

	CResampleContext * pContext = new CResampleContext(this, "Changing sample rate...", "Resample");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}
	// create new temporary file
	CWaveFile DstFile;

	if ( ! DstFile.CreateWaveFile( & m_WavFile, NULL, ALL_CHANNELS, NewSampleCount,
									//CreateWaveFileTempDir |
									CreateWaveFileDeleteAfterClose
									| CreateWaveFilePcmFormat
									| CreateWaveFileTemp,
									NULL))
	{
		delete pContext;
		FileCreationErrorMessageBox(NULL);
		return;
	}
	if (dlg.m_bChangeSamplingRate)
	{
		DstFile.GetWaveFormat()->nSamplesPerSec = NewSamplingRate;
	}
	pContext->InitResample(m_WavFile, DstFile, ResampleRatio, ResampleQuality);
	// mark for whole file conversion
	pContext->m_Flags |= ConvertContextReplaceWholeFile;

	// add UNDO
	if (UndoEnabled())
	{
		CUndoRedoContext * pUndo = new CUndoRedoContext(this, "Resample");
		if (NULL == pUndo)
		{
			NotEnoughMemoryMessageBox();
			return;
		}
		pUndo->m_Flags |= UndoContextReplaceWholeFile;
		pUndo->m_SrcFile = m_WavFile;
		pContext->m_pUndoContext = pUndo;
	}
	pContext->Execute();
	// don't mark the file as modified yet
	//SetModifiedFlag();
}

void CWaveSoapFrontDoc::OnFileStatistics()
{
	// disable for zero length file
	if (m_OperationInProgress || ! m_WavFile.IsOpen() || WaveFileSamples() == 0)
	{
		return;
	}
	CStatisticsContext * pContext = new CStatisticsContext(this, "Scanning the file for statistics...", "");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}
	long begin = m_SelectionStart;
	long end = m_SelectionEnd;
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
	pCmdUI->Enable( ! m_OperationInProgress && m_WavFile.IsOpen() && WaveFileSamples() != 0);
}

void CWaveSoapFrontDoc::OnUpdateEditGoto(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_OperationInProgress && m_WavFile.IsOpen() && WaveFileSamples() != 0);
}

void CWaveSoapFrontDoc::OnEditGoto()
{
	if (m_OperationInProgress || ! m_WavFile.IsOpen() || WaveFileSamples() == 0)
	{
		return;
	}
	CGotoDialog dlg;
	dlg.m_Position = m_CaretPosition;
	dlg.m_pWf = m_WavFile.GetWaveFormat();
	dlg.m_TimeFormat = GetApp()->m_SoundTimeFormat;
	dlg.m_FileLength = WaveFileSamples();
	if (IDOK != dlg.DoModal())
	{
		return;
	}
	GetApp()->m_SoundTimeFormat = dlg.m_TimeFormat;
	SetSelection(dlg.m_Position, dlg.m_Position,
				m_SelectedChannel, dlg.m_Position, SetSelection_MoveCaretToCenter);
}

void CWaveSoapFrontDoc::OnUpdateProcessInvert(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bReadOnly && ! m_OperationInProgress
					&& m_WavFile.IsOpen() && WaveFileSamples() != 0);
}

void CWaveSoapFrontDoc::OnProcessInvert()
{
	if (m_OperationInProgress || m_bReadOnly
		|| ! m_WavFile.IsOpen() || WaveFileSamples() == 0)
	{
		return;
	}

	long start = m_SelectionStart;
	long end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}
	int channel = m_SelectedChannel;
	if (ChannelsLocked())
	{
		channel = ALL_CHANNELS;
	}

	CVolumeChangeContext * pContext =
		new CVolumeChangeContext(this, "Inverting the waveform...", "Inversion");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	pContext->m_VolumeLeft = -1.;
	pContext->m_VolumeRight = -1.;

	if ( ! pContext->InitDestination(m_WavFile, start,
									end, channel, UndoEnabled()))
	{
		delete pContext;
		return;
	}
	pContext->Execute();
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::OnUpdateViewRescanPeaks(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_OperationInProgress && m_WavFile.IsOpen() && WaveFileSamples() != 0);
}

void CWaveSoapFrontDoc::OnViewRescanPeaks()
{
	if (m_OperationInProgress)
	{
		return;
	}
	BuildPeakInfo(FALSE);   // don't save it right now
}

void CWaveSoapFrontDoc::OnUpdateProcessSynthesisExpressionEvaluation(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bReadOnly
					&& ! m_OperationInProgress
					&& m_WavFile.IsOpen());
}

void CWaveSoapFrontDoc::OnProcessSynthesisExpressionEvaluation()
{
	if (m_bReadOnly
		|| m_OperationInProgress)
	{
		return;
	}
	long start = m_SelectionStart;
	long end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}
	int channel = m_SelectedChannel;
	if (ChannelsLocked())
	{
		channel = ALL_CHANNELS;
	}

	CExpressionEvaluationDialog dlg;
	CThisApp * pApp = GetApp();
	dlg.m_bUndo = UndoEnabled();
	dlg.m_Start = start;
	dlg.m_End = end;
	dlg.m_CaretPosition = m_CaretPosition;
	dlg.m_Chan = channel;
	dlg.m_pWf = m_WavFile.GetWaveFormat();
	dlg.m_bLockChannels = m_bChannelsLocked;
	dlg.m_TimeFormat = pApp->m_SoundTimeFormat;
	dlg.m_FileLength = WaveFileSamples();

	CExpressionEvaluationContext * pContext = new CExpressionEvaluationContext(this, "Calculating the waveform...",
												"Expression evaluation");
	dlg.m_pContext = pContext;
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	if (IDOK != dlg.DoModal())
	{
		delete dlg.m_pContext;
		return;
	}

	pContext->m_dFrequencyArgument = dlg.m_OperandsTabDlg.m_dFrequency;
	pContext->m_dFrequencyArgument1 = dlg.m_OperandsTabDlg.m_dFrequency1;
	pContext->m_dFrequencyArgument2 = dlg.m_OperandsTabDlg.m_dFrequency2;
	pContext->m_dFrequencyArgument3 = dlg.m_OperandsTabDlg.m_dFrequency3;

	if ( ! dlg.m_pContext->InitDestination(m_WavFile, dlg.m_Start, dlg.m_End,
											dlg.m_Chan, dlg.m_bUndo))
	{
		delete pContext;
		return;
	}

	dlg.m_pContext->Execute();
	SetModifiedFlag(TRUE, dlg.m_bUndo);
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
	CRawFileParametersDlg dlg;
	dlg.m_SourceFileSize = m_OriginalWavFile.GetFileSize(NULL);
	if (IDOK != dlg.DoModal())
	{
		return FALSE;
	}

	WAVEFORMATEX wf;
	wf.cbSize = 0;
	wf.nChannels = 1;
	if (dlg.m_bStereo)
	{
		wf.nChannels = 2;
	}

	wf.wBitsPerSample = 8;
	if (dlg.m_bBits16)
	{
		wf.wBitsPerSample = 16;
	}

	wf.nSamplesPerSec = dlg.m_SamplingRate;
	if (8 == wf.wBitsPerSample
		&& dlg.m_Compression != 0)
	{
		if (1 == dlg.m_Compression)
		{
			wf.wFormatTag = WAVE_FORMAT_ALAW;
		}
		else
		{
			wf.wFormatTag = WAVE_FORMAT_MULAW;
		}
	}
	else
	{
		wf.wFormatTag = WAVE_FORMAT_PCM;
	}
	wf.nBlockAlign = wf.wBitsPerSample * wf.nChannels / 8;
	wf.nAvgBytesPerSec = wf.nBlockAlign * wf.nSamplesPerSec;
	LONG nNewFileSamples = (dlg.m_SourceFileSize - dlg.m_HeaderLength - dlg.m_TrailerLength)
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

	CDecompressContext * pContext =
		new CDecompressContext(this, "Loading the raw sound file...", & wf);

	m_OriginalWaveFormat = & wf;

	pContext->m_SrcFile = m_OriginalWavFile;
	pContext->m_DstFile = m_WavFile;

	pContext->m_SrcStart = dlg.m_HeaderLength;
	pContext->m_SrcPos = pContext->m_SrcStart;
	pContext->m_SrcEnd = dlg.m_SourceFileSize - dlg.m_TrailerLength;

	pContext->m_DstStart = m_WavFile.GetDataChunk()->dwDataOffset;
	pContext->m_DstCopyPos = pContext->m_DstStart;

	pContext->m_CurrentSamples = nNewFileSamples;
	if (16 == wf.wBitsPerSample)
	{
		pContext->m_bSwapBytes = dlg.m_bMsbFirst;
		if (dlg.m_bMsbFirst)
		{
			m_FileTypeFlags |= OpenRawFileMsbFirst;
		}
	}

	AllocatePeakData(nNewFileSamples);
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

	TRACE("CWaveSoapFrontDoc::OpenWmaFileDocument(%s)\n", lpszPathName);

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

	CWmaDecodeContext * pWmaContext = new CWmaDecodeContext(this, "Loading the compressed file...");
	if (NULL == pWmaContext)
	{
		NotEnoughMemoryMessageBox();
		return FALSE;
	}
	pWmaContext->m_Flags |= DecompressSavePeakFile;

	CoInitializeEx(NULL, COINIT_MULTITHREADED );
	BOOL res = pWmaContext->Open(m_OriginalWavFile);
	if ( ! res)
	{
		delete pWmaContext;
		CString s;
		s.Format(IDS_CANT_OPEN_WMA_DECODER, lpszPathName);
		AfxMessageBox(s, MB_ICONEXCLAMATION | MB_OK);
		CoUninitialize();
		return FALSE;
	}

	if ( ! m_WavFile.CreateWaveFile(NULL, pWmaContext->m_Decoder.m_DstWf,
									ALL_CHANNELS, pWmaContext->m_CurrentSamples,  // initiali sample count
									CreateWaveFileTempDir
									| CreateWaveFileDeleteAfterClose
									| CreateWaveFilePcmFormat
									| CreateWaveFileTemp, NULL))
	{
		AfxMessageBox(IDS_UNABLE_TO_CREATE_TEMPORARY_FILE, MB_OK | MB_ICONEXCLAMATION);
		delete pWmaContext;
		CoUninitialize();
		return FALSE;
	}

	m_OriginalWaveFormat = pWmaContext->m_Decoder.m_DstWf;
	pWmaContext->SetDstFile(m_WavFile);
	AllocatePeakData(pWmaContext->m_CurrentSamples);

	LoadPeaksForCompressedFile(m_WavFile, m_OriginalWavFile);

	pWmaContext->Execute();
	CoUninitialize();
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
	pCmdUI->Enable(0 != InterpolateSamples
					&& InterpolateSamples <= MaxInterpolatedLength   // 128
					&& m_SelectionStart >= InterpolationOverlap
					&& m_WavFile.IsOpen()
					&& m_SelectionEnd + InterpolationOverlap < WaveFileSamples()
					&& ! m_OperationInProgress
					&& ! m_bReadOnly);
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
	if (0 == InterpolateSamples
		|| InterpolateSamples > MaxInterpolatedLength   // 128
		|| m_SelectionStart < InterpolationOverlap
		|| ! m_WavFile.IsOpen()
		|| m_SelectionEnd + InterpolationOverlap >= WaveFileSamples()
		|| m_OperationInProgress
		|| m_bReadOnly)
	{
		return;
	}
	// interpolate the selected area.
	int SampleSize = WaveSampleSize();
	int nChannels = WaveChannels();
	MMCKINFO * datack = WaveDataChunk();
	DWORD ReadStartOffset = datack->dwDataOffset +
							SampleSize * (m_SelectionStart - InterpolationOverlap);
	DWORD WriteStartOffset = datack->dwDataOffset
							+ SampleSize * (m_SelectionStart - PreInterpolateSamples);
	// allocate the buffer
	int BufferSamples = InterpolateSamples + 2 * InterpolationOverlap;
	int InterpolateOffset = InterpolationOverlap;
	int WriteBufferOffset = InterpolationOverlap  - PreInterpolateSamples;

	__int16 * pBuf = new __int16[BufferSamples * nChannels];
	if (NULL == pBuf)
	{
		return;
	}

	int ReadBytes = BufferSamples * SampleSize;
	int WriteBytes = (InterpolateSamples + PreInterpolateSamples + PostInterpolateSamples)
					* SampleSize;
	if (ReadBytes != m_WavFile.ReadAt(pBuf, ReadBytes, ReadStartOffset))
	{
		delete[] pBuf;
		return;
	}
	// create undo
	if (UndoEnabled())
	{
		CUndoRedoContext * pUndo = new CUndoRedoContext(this, _T("Interpolate"));
		if ( ! pUndo->InitUndoCopy(m_WavFile,
									WriteStartOffset,
									WriteStartOffset + WriteBytes,
									m_SelectedChannel))
		{
			delete pUndo;
			delete[] pBuf;
			return;
		}
		pUndo->SaveUndoData(pBuf + WriteBufferOffset * nChannels, WriteBytes,
							WriteStartOffset, m_SelectedChannel);
		AddUndoRedo(pUndo);
	}

	// now, do the interpolation
	CClickRemoval crm;
	if (m_SelectedChannel != 1) // mono or not right channel only
	{
		crm.InterpolateGap(pBuf, InterpolateOffset, InterpolateSamples, nChannels, BigGap);
	}
	if (nChannels == 2
		&& m_SelectedChannel != 0) // mono or not right channel only
	{
		crm.InterpolateGap(pBuf + 1, InterpolateOffset, InterpolateSamples, nChannels, BigGap);
	}

	// write the data back
	m_WavFile.WriteAt(pBuf + WriteBufferOffset * nChannels, WriteBytes, WriteStartOffset);
	SetModifiedFlag(TRUE);
	SoundChanged(m_WavFile.GetFileID(), m_SelectionStart - PreInterpolateSamples,
				m_SelectionEnd + PostInterpolateSamples);
	delete[] pBuf;

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
	pCmdUI->Enable( ! m_bReadOnly
					&& ! m_OperationInProgress
					&& m_WavFile.IsOpen() && WaveFileSamples() != 0
					&& (m_SelectionEnd - m_SelectionStart > 16
						|| m_SelectionEnd == m_SelectionStart));

}

void CWaveSoapFrontDoc::OnProcessDoUlf()
{
	if (m_bReadOnly
		|| m_OperationInProgress)
	{
		return;
	}
	long start = m_SelectionStart;
	long end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}
	int channel = m_SelectedChannel;
	if (ChannelsLocked())
	{
		channel = ALL_CHANNELS;
	}

	CLowFrequencySuppressDialog dlg;
	CThisApp * pApp = GetApp();

	dlg.m_bUndo = UndoEnabled();

	dlg.m_Start = start;
	dlg.m_End = end;
	dlg.m_CaretPosition = m_CaretPosition;
	dlg.m_Chan = channel;
	dlg.m_pWf = m_WavFile.GetWaveFormat();
	dlg.m_bLockChannels = m_bChannelsLocked;
	dlg.m_TimeFormat = pApp->m_SoundTimeFormat;
	dlg.m_FileLength = WaveFileSamples();
	if (IDOK != dlg.DoModal())
	{
		return;
	}

	CConversionContext * pContext = new CConversionContext(this, "Reducing low frequency static...",
															"Low Frequency Suppression");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}
	if ( ! pContext->InitDestination(m_WavFile, dlg.m_Start,
									dlg.m_End, dlg.m_Chan, dlg.m_bUndo))
	{
		delete pContext;
		return;
	}
	pContext->m_SrcFile = m_WavFile;
	pContext->m_SrcStart = pContext->m_DstStart;
	pContext->m_SrcCopyPos = pContext->m_SrcStart;
	pContext->m_SrcEnd = pContext->m_DstEnd;

	pContext->m_SrcChan = dlg.m_Chan;

	CHumRemoval * pUlfProc = new CHumRemoval;
	if (NULL == pUlfProc)
	{
		delete pContext;
		NotEnoughMemoryMessageBox();
		return;
	}
	pUlfProc->SetAndValidateWaveformat(WaveFormat());
	pUlfProc->m_ChannelsToProcess = dlg.m_Chan;
	pUlfProc->EnableDifferentialSuppression(dlg.m_DifferentialModeSuppress);
	pUlfProc->EnableLowFrequencySuppression(dlg.m_LowFrequencySuppress);
	pUlfProc->SetDifferentialCutoff(dlg.m_dDiffNoiseRange);
	pUlfProc->SetHighpassCutoff(dlg.m_dLfNoiseRange);

	pContext->m_ProcBatch.AddWaveProc(pUlfProc);

	pContext->Execute();
	SetModifiedFlag(TRUE, dlg.m_bUndo);
}

void CWaveSoapFrontDoc::OnUpdateProcessDoDeclicking(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bReadOnly
					&& ! m_OperationInProgress
					&& m_WavFile.IsOpen() && WaveFileSamples() != 0);
}

void CWaveSoapFrontDoc::OnProcessDoDeclicking()
{
	if (m_bReadOnly
		|| m_OperationInProgress)
	{
		return;
	}
	long start = m_SelectionStart;
	long end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}
	int channel = m_SelectedChannel;
	if (ChannelsLocked())
	{
		channel = ALL_CHANNELS;
	}

	CDeclickDialog dlg;
	CThisApp * pApp = GetApp();

	dlg.m_bUndo = UndoEnabled();
	dlg.m_Start = start;
	dlg.m_End = end;
	dlg.m_CaretPosition = m_CaretPosition;
	dlg.m_Chan = channel;
	dlg.m_pWf = m_WavFile.GetWaveFormat();
	dlg.m_bLockChannels = m_bChannelsLocked;
	dlg.m_TimeFormat = pApp->m_SoundTimeFormat;
	dlg.m_FileLength = WaveFileSamples();
	if (IDOK != dlg.DoModal())
	{
		return;
	}
	CConversionContext * pContext = new CConversionContext(this, "Removing vinyl disk clicks...",
															"Declicking");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	CClickRemoval * pDeclick = new CClickRemoval;
	if (NULL == pDeclick)
	{
		delete pContext;
		NotEnoughMemoryMessageBox();
		return;
	}

	pDeclick->SetAndValidateWaveformat(dlg.m_pWf);
	dlg.SetDeclickData(pDeclick);

	pContext->m_ProcBatch.AddWaveProc(pDeclick);

	if ( ! pContext->InitDestination(m_WavFile, dlg.m_Start,
									dlg.m_End, dlg.m_Chan, dlg.m_bUndo))
	{
		delete pContext;
		return;
	}
	pContext->m_SrcFile = m_WavFile;
	pContext->m_SrcStart = pContext->m_DstStart;
	pContext->m_SrcCopyPos = pContext->m_SrcStart;
	pContext->m_SrcEnd = pContext->m_DstEnd;

	pContext->m_SrcChan = dlg.m_Chan;

	pContext->Execute();
	SetModifiedFlag(TRUE, dlg.m_bUndo);

}

void CWaveSoapFrontDoc::OnUpdateProcessNoiseReduction(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bReadOnly
					&& ! m_OperationInProgress
					&& m_WavFile.IsOpen() && WaveFileSamples() != 0);
}

void CWaveSoapFrontDoc::OnProcessNoiseReduction()
{
	if (m_bReadOnly
		|| m_OperationInProgress)
	{
		return;
	}
	long start = m_SelectionStart;
	long end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}
	int channel = m_SelectedChannel;
	if (ChannelsLocked())
	{
		channel = ALL_CHANNELS;
	}

	CNoiseReductionDialog dlg;
	CThisApp * pApp = GetApp();

	dlg.m_bUndo = UndoEnabled();
	dlg.m_Start = start;
	dlg.m_End = end;
	dlg.m_CaretPosition = m_CaretPosition;
	dlg.m_Chan = channel;
	dlg.m_pWf = m_WavFile.GetWaveFormat();
	dlg.m_bLockChannels = m_bChannelsLocked;
	dlg.m_TimeFormat = pApp->m_SoundTimeFormat;
	dlg.m_FileLength = WaveFileSamples();

	ULONG result = dlg.DoModal();
	if (IDC_BUTTON_SET_THRESHOLD == result)
	{
		// get active frame, enable spectrum section, store threshold
		CChildFrame * pFrame =
			dynamic_cast<CChildFrame*>(((CFrameWnd*)pApp->GetMainWnd())->GetActiveFrame());
		if (NULL != pFrame)
		{
			CSpectrumSectionView * pSectionView =
				dynamic_cast<CSpectrumSectionView *>
				(pFrame->m_wClient.GetDlgItem(CWaveMDIChildClient::SpectrumSectionViewID));
			if (NULL != pSectionView)
			{
				pSectionView->m_bShowNoiseThreshold = TRUE;
				pSectionView->m_dNoiseThresholdLow = dlg.m_dNoiseThresholdLow;
				pSectionView->m_dNoiseThresholdHigh = dlg.m_dNoiseThresholdHigh;
				pSectionView->nBeginFrequency = dlg.m_dLowerFrequency;

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

	CConversionContext * pContext = new CConversionContext(this, "Removing background noise...",
															"Noise Reduction");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	CNoiseReduction * pNoiseReduction = new CNoiseReduction;
	if (NULL == pNoiseReduction)
	{
		delete pContext;
		NotEnoughMemoryMessageBox();
		return;
	}

	pNoiseReduction->SetAndValidateWaveformat(dlg.m_pWf);
	dlg.SetNoiseReductionData(pNoiseReduction);

	pContext->m_ProcBatch.AddWaveProc(pNoiseReduction);

	if ( ! pContext->InitDestination(m_WavFile, dlg.m_Start,
									dlg.m_End, dlg.m_Chan, dlg.m_bUndo))
	{
		delete pContext;
		return;
	}
	pContext->m_SrcFile = m_WavFile;
	pContext->m_SrcStart = pContext->m_DstStart;
	pContext->m_SrcCopyPos = pContext->m_SrcStart;
	pContext->m_SrcEnd = pContext->m_DstEnd;

	pContext->m_SrcChan = dlg.m_Chan;

	pContext->Execute();
	SetModifiedFlag(TRUE, dlg.m_bUndo);

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
	if (m_OperationInProgress || m_bReadOnly
		|| ! m_WavFile.IsOpen())
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
	pCmdUI->Enable(m_WavFile.IsOpen() && ! (m_OperationInProgress || m_bReadOnly));
}

void CWaveSoapFrontDoc::OnSamplerateCustom()
{
	if ( ! m_WavFile.IsOpen()
		|| m_OperationInProgress
		|| m_bReadOnly)
	{
		return;
	}
	CCustomSampleRateDlg dlg;
	dlg.m_SampleRate = m_WavFile.SampleRate();
	if (IDOK == dlg.DoModal()
		&& dlg.m_SampleRate > 0)
	{
		SetSampleRate(dlg.m_SampleRate);
	}
}

void CWaveSoapFrontDoc::OnUpdateSamplerateCustom(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_WavFile.IsOpen() && ! (m_OperationInProgress || m_bReadOnly));
}

void CWaveSoapFrontDoc::DeletePermanentUndoRedo()
{
	COperationContext * pContext;
	for (pContext = m_UndoList.Next()
		; pContext != m_UndoList.Head(); pContext = pContext->Next())
	{
		if ((pContext->m_Flags & UndoContextReplaceWholeFile)
			&& ! ((CUndoRedoContext *)pContext)->m_SrcFile.IsTemporaryFile())
		{
			pContext = pContext->Prev();
			while (pContext->Next() != m_UndoList.Head())
			{
				delete pContext->RemoveHead();
			}
			break;
		}
	}
	for (pContext = m_RedoList.Next()
		; pContext != m_RedoList.Head(); pContext = pContext->Next())
	{
		if ((pContext->m_Flags & UndoContextReplaceWholeFile)
			&& ! ((CUndoRedoContext *)pContext)->m_SrcFile.IsTemporaryFile())
		{
			pContext = pContext->Prev();
			while (pContext->Next() != m_RedoList.Head())
			{
				delete pContext->RemoveHead();
			}
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
		pCmdUI->Enable(! m_OperationInProgress);
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
		pCmdUI->Enable(! m_OperationInProgress);
	}
}

void CWaveSoapFrontDoc::OnEditClearUndo()
{
	DeleteUndo();
}

void CWaveSoapFrontDoc::OnUpdateEditClearUndo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(! m_OperationInProgress && ! m_UndoList.IsEmpty());
}

void CWaveSoapFrontDoc::OnEditClearRedo()
{
	DeleteRedo();
}

void CWaveSoapFrontDoc::OnUpdateEditClearRedo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(! m_OperationInProgress && ! m_RedoList.IsEmpty());
}

void CWaveSoapFrontDoc::OnProcessEqualizer()
{
	if (m_bReadOnly
		|| m_OperationInProgress
		|| ! m_WavFile.IsOpen()
		|| WaveFileSamples() <= 2)
	{
		return;
	}

	long start = m_SelectionStart;
	long end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}
	int channel = m_SelectedChannel;
	if (ChannelsLocked())
	{
		channel = ALL_CHANNELS;
	}

	CEqualizerDialog dlg;
	CThisApp * pApp = GetApp();
	dlg.m_bUndo = UndoEnabled();
	dlg.m_Start = start;
	dlg.m_End = end;
	dlg.m_CaretPosition = m_CaretPosition;
	dlg.m_Chan = channel;
	dlg.m_pWf = m_WavFile.GetWaveFormat();
	dlg.m_bLockChannels = m_bChannelsLocked;
	dlg.m_TimeFormat = pApp->m_SoundTimeFormat;
	dlg.m_FileLength = WaveFileSamples();
	if (IDOK != dlg.DoModal())
	{
		return;
	}
	CEqualizerContext * pContext =
		new CEqualizerContext(this, "Applying equalizer...", "Equalizer");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	for (int i = 0; i < dlg.m_nBands; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			pContext->m_BandCoefficients[i][j] = dlg.m_wGraph.m_BandCoefficients[i][j];
		}
	}

	pContext->m_NumOfBands = dlg.m_nBands;
	pContext->m_bZeroPhase = dlg.m_wGraph.m_bZeroPhase;

	if ( ! pContext->InitDestination(m_WavFile, dlg.m_Start,
									dlg.m_End, dlg.m_Chan, dlg.m_bUndo))
	{
		delete pContext;
		return;
	}
	pContext->Execute();
	SetModifiedFlag(TRUE, dlg.m_bUndo);
}

void CWaveSoapFrontDoc::OnUpdateProcessEqualizer(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bReadOnly
					&& ! m_OperationInProgress
					&& m_WavFile.IsOpen() && WaveFileSamples() > 2 );
}

void CWaveSoapFrontDoc::OnProcessSwapchannels()
{
	long start = m_SelectionStart;
	long end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}
	if (m_OperationInProgress
		|| m_bReadOnly
		|| start == end
		|| WaveChannels() != 2)
	{
		// don't do anything
		return;
	}

	CSwapChannelsContext * pContext =
		new CSwapChannelsContext(this, "Swapping the channels...", "Swap channels");
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
	pCmdUI->Enable( ! m_bReadOnly
					&& ! m_OperationInProgress
					&& m_WavFile.IsOpen() && WaveFileSamples() > 0
					&& m_WavFile.Channels() == 2);
}

void CWaveSoapFrontDoc::OnProcessFilter()
{
	if (m_bReadOnly
		|| m_OperationInProgress
		|| ! m_WavFile.IsOpen()
		|| WaveFileSamples() <= 2)
	{
		return;
	}

	long start = m_SelectionStart;
	long end = m_SelectionEnd;
	if (start == end)
	{
		// select all
		start = 0;
		end = WaveFileSamples();
	}
	int channel = m_SelectedChannel;
	if (ChannelsLocked())
	{
		channel = ALL_CHANNELS;
	}

	CFilterDialog dlg;

	CThisApp * pApp = GetApp();
	dlg.m_bUndo = UndoEnabled();
	dlg.m_Start = start;
	dlg.m_End = end;
	dlg.m_CaretPosition = m_CaretPosition;
	dlg.m_Chan = channel;
	dlg.m_pWf = m_WavFile.GetWaveFormat();
	dlg.m_bLockChannels = m_bChannelsLocked;
	dlg.m_TimeFormat = pApp->m_SoundTimeFormat;
	dlg.m_FileLength = WaveFileSamples();

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	CFilterContext * pContext =
		new CFilterContext(this, "Applying filter...", "Filter");
	if (NULL == pContext)
	{
		NotEnoughMemoryMessageBox();
		return;
	}

	for (int i = 0; i < MaxFilterOrder; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			pContext->m_LpfCoeffs[i][j] = dlg.m_wGraph.m_LpfCoeffs[i][j];
			pContext->m_HpfCoeffs[i][j] = dlg.m_wGraph.m_HpfCoeffs[i][j];
			pContext->m_NotchCoeffs[i][j] = dlg.m_wGraph.m_NotchCoeffs[i][j];
		}
	}

	pContext->m_bZeroPhase = dlg.m_wGraph.m_bZeroPhase;
	if (dlg.m_wGraph.m_bLowPass)
	{
		pContext->m_nLpfOrder = dlg.m_wGraph.m_nLpfOrder;
	}
	else
	{
		pContext->m_nLpfOrder = 0;
	}

	if (dlg.m_wGraph.m_bHighPass)
	{
		pContext->m_nHpfOrder = dlg.m_wGraph.m_nHpfOrder;
	}
	else
	{
		pContext->m_nHpfOrder = 0;
	}

	if (dlg.m_wGraph.m_bNotchFilter)
	{
		pContext->m_nNotchOrder = dlg.m_wGraph.m_nNotchOrder;
	}
	else
	{
		pContext->m_nNotchOrder = 0;
	}

	if ( ! pContext->InitDestination(m_WavFile, dlg.m_Start,
									dlg.m_End, dlg.m_Chan, dlg.m_bUndo))
	{
		delete pContext;
		return;
	}
	pContext->Execute();
	SetModifiedFlag(TRUE, dlg.m_bUndo);
}

void CWaveSoapFrontDoc::OnUpdateProcessFilter(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bReadOnly
					&& ! m_OperationInProgress
					&& m_WavFile.IsOpen() && WaveFileSamples() > 0);
}

BOOL CWaveSoapFrontDoc::DoFileSave()
{
	DWORD dwAttrib;
	if (m_strPathName.IsEmpty()
		|| (0xFFFFFFFF != (dwAttrib = GetFileAttributes(m_strPathName))
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

