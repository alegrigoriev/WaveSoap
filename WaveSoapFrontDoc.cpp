// WaveSoapFrontDoc.cpp : implementation of the CWaveSoapFrontDoc class
//

#include "stdafx.h"
//#include "WaveSoapFront.h"
#include "MainFrm.h"
//#include "WaveSoapFrontDoc.h"
#include "OperationContext.h"
#include "resource.h"
#include <afxpriv.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontDoc

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
	m_bDirectMode(false),
	m_bInOnIdle(false),
	m_bChannelsLocked(true),
	m_pCurrentContext(NULL),
	m_pQueuedOperation(NULL),
	m_pUndoList(NULL),
	m_pRedoList(NULL),
	m_pUpdateList(NULL),
	m_pRetiredList(NULL),
	m_OperationInProgress(0),
	m_PlayingSound(false),
	m_OperationNonCritical(false),
	m_Thread(ThreadProc, this),
	m_ModificationSequenceNumber(0),
	m_PrevChannelToCopy(2),
	m_DefaultPasteMode(0),
	m_SelectedChannel(2)
{
	m_bUndoEnabled = (FALSE != GetApp()->m_bUndoEnabled);
	m_bRedoEnabled = (FALSE != GetApp()->m_bRedoEnabled);
	m_Thread.m_bAutoDelete = FALSE;
	m_hThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	TRACE("CWaveSoapFrontDoc::CWaveSoapFrontDoc()\n");
	memset(& WavFileInfo, 0, sizeof WavFileInfo);
	m_bRunThread = TRUE;
	m_Thread.CreateThread(0, 0x10000);

}

CWaveSoapFrontDoc::~CWaveSoapFrontDoc()
{
	TRACE("CWaveSoapFrontDoc::~CWaveSoapFrontDoc()\n");
	ASSERT(0 == m_OperationInProgress);
	// free RetiredList and UpdateList
	while (m_pRetiredList)
	{
		COperationContext * pContext = m_pRetiredList;
		m_pRetiredList = pContext->pNext;
		if (m_pRetiredList)
		{
			m_pRetiredList->pPrev = NULL;
		}
		delete pContext;
	}
	while (m_pUpdateList)
	{
		CSoundUpdateInfo * pUpdate = m_pUpdateList;
		m_pUpdateList = pUpdate->pNext;
		delete pUpdate;
	}
	DeleteUndo();
	DeleteRedo();
	// stop the thread
	if (NULL != m_Thread.m_hThread)
	{
		m_bRunThread = FALSE;
		SetEvent(m_hThreadEvent);
		if (WAIT_TIMEOUT == WaitForSingleObject(m_Thread.m_hThread, 5000))
		{
			TerminateThread(m_Thread.m_hThread, -1);
		}
	}
	if (m_hThreadEvent != NULL)
	{
		CloseHandle(m_hThreadEvent);
		m_hThreadEvent = NULL;
	}
#if 0
	if (m_pWaveBuffer)
	{
		delete[] m_pWaveBuffer;
		m_pWaveBuffer = NULL;
	}
#endif
	if (m_pPeaks != NULL)
	{
		delete[] m_pPeaks;
		m_pPeaks = NULL;
	}
}

BOOL CWaveSoapFrontDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	TRACE("CWaveSoapFrontDoc::OnNewDocument\n");
	// (SDI documents will reuse this document)

	m_CaretPosition = 0;
	m_SelectionStart = 0;
	m_SelectionEnd = 0;
	m_SelectedChannel = 2;  // both channels
	m_TimeSelectionMode = TRUE;
	m_bReadOnly = false;
	CWaveFile * pTemplateFile = & GetApp()->m_NewTemplateFile;
	if ( ! pTemplateFile->IsOpen())
	{
		pTemplateFile = NULL;
	}

	if ( FALSE == m_WavFile.CreateWaveFile(pTemplateFile, 2, // 2 channels
											0, // empty
											CreateWaveFileTempDir
											// keep the file for debug
#if 1 || !defined _DEBUG
											| CreateWaveFileDeleteAfterClose
#endif
											| CreateWaveFilePcmFormat
											| CreateWaveFileTemp,
											NULL))
	{
		AfxMessageBox(IDS_UNABLE_TO_CREATE_NEW_FILE, MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	m_SizeOfWaveData = WaveDataChunk()->cksize;
	m_WavFile.GetFileInformationByHandle(& WavFileInfo);
	return TRUE;
}

void CWaveSoapFrontDoc::LoadPeakFile()
{
	// if peak file exists and the wav file length/date/time matches the stored
	// length/date/time, then use this peak file.
	// otherwise scan the wav file and build the new peak file
	CFile PeakFile;
	PeakFileHeader pfh;
	if (PeakFile.Open(szPeakFilename,
					CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary))
	{
		if (offsetof(PeakFileHeader, WaveFileTime)
			== PeakFile.Read( & pfh, offsetof(PeakFileHeader, WaveFileTime))
			&& PeakFileHeader::pfhSignature == pfh.dwSignature
			&& pfh.dwVersion == PeakFileHeader::pfhMaxVersion
			&& pfh.dwSize == sizeof (PeakFileHeader)
			// read the rest of the header
			&& pfh.dwSize - offsetof(PeakFileHeader, WaveFileTime)
			== PeakFile.Read( & pfh.WaveFileTime, pfh.dwSize - offsetof(PeakFileHeader, WaveFileTime))
			&& pfh.WaveFileTime.dwHighDateTime == WavFileInfo.ftLastWriteTime.dwHighDateTime
			&& pfh.WaveFileTime.dwLowDateTime == WavFileInfo.ftLastWriteTime.dwLowDateTime
			&& pfh.dwWaveFileSize == WavFileInfo.nFileSizeLow
			&& 0 == memcmp(& pfh.wfFormat, WaveFormat(), sizeof pfh.wfFormat)
			&& pfh.PeakInfoSize
			== CalculatePeakInfoSize() * sizeof (WavePeak)
			)
		{
			// allocate data and read it
			m_WavePeakSize = 0;
			if ( ! AllocatePeakData(m_WavFile.NumberOfSamples()))
			{
				TRACE("Unable to allocate peak info buffer\n");
				return;
			}

			if (pfh.PeakInfoSize == PeakFile.Read(m_pPeaks, pfh.PeakInfoSize))
			{
				return;
			}
			TRACE("Unable to read peak data\n");
			// rebuild the info from the WAV file
		}
		PeakFile.Close();
	}
	BuildPeakInfo();
}

void CWaveSoapFrontDoc::BuildPeakInfo()
{
	// read the DATA chunk of wavefile
	if ( ! AllocatePeakData(m_WavFile.NumberOfSamples()))
	{
		return;
	}

	CScanPeaksContext * pContext = new CScanPeaksContext(this);
	if (NULL == pContext)
	{
		return;
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
	QueueOperation(pContext);
}

void CWaveSoapFrontDoc::SavePeakInfo()
{
	CFile PeakFile;
	PeakFileHeader pfh;
	if (PeakFile.Open(szPeakFilename,
					CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeBinary))
	{
		pfh.dwSize = sizeof PeakFileHeader;
		pfh.dwSignature = PeakFileHeader::pfhSignature;
		pfh.dwVersion = PeakFileHeader::pfhMaxVersion;
		pfh.dwWaveFileSize = m_WavFile.m_File.GetFileSize(NULL);
		pfh.Granularity = m_PeakDataGranularity;
		pfh.PeakInfoSize = CalculatePeakInfoSize() * sizeof (WavePeak);
		pfh.WaveFileTime = WavFileInfo.ftLastWriteTime;
		pfh.wfFormat = * WaveFormat();

		PeakFile.Write( & pfh, sizeof pfh);
		PeakFile.Write(m_pPeaks, pfh.PeakInfoSize);

		PeakFile.Close();
	}
}

#if 0
void CWaveSoapFrontDoc::FreeWaveBuffer()
{
	if (NULL != m_pWaveBuffer)
	{
		delete[] m_pWaveBuffer;
		m_pWaveBuffer = NULL;
	}
	m_WaveBufferSize = 0;
}

BOOL CWaveSoapFrontDoc::AllocateWaveBuffer(size_t size)
{
	ASSERT(size > 0x10 && 0 == (size & 3) && size < 0x1000000); // 16MB max
	if (NULL != m_pWaveBuffer
		&& m_WaveBufferSize >= size)
	{
		return TRUE;
	}
	FreeWaveBuffer();
	m_pWaveBuffer = new __int16[size / sizeof(__int16)];
	if (NULL != m_pWaveBuffer)
	{
		m_WaveBufferSize = size;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
#endif

BOOL CWaveSoapFrontDoc::OpenWaveFile(LPCTSTR szName, DWORD flags)
{
	CString s;
	UINT format = 0;
	if (m_WavFile.Open(szName, flags))
	{
		if(m_WavFile.LoadWaveformat())
		{
			if (m_WavFile.FindData())
			{
				m_SizeOfWaveData = WaveDataChunk()->cksize;
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
		ar << m_szWaveFilename;
		ar << szPeakFilename;
	}
	else
	{
		// TODO: add loading code here
		ar >> m_szWaveFilename;
		ar >> szPeakFilename;
	}
}

BOOL CWaveSoapFrontDoc::DoSave(LPCTSTR lpszPathName, BOOL bReplace)
	// Save the document data to a file
	// lpszPathName = path name where to save document file
	// if lpszPathName is NULL then the user will be prompted (SaveAs)
	// note: lpszPathName can be different than 'm_strPathName'
	// if 'bReplace' is TRUE will change file name if successful (SaveAs)
	// if 'bReplace' is FALSE will not change path name (SaveCopyAs)
{
	CString newName = lpszPathName;
	if (newName.IsEmpty())
	{
		CDocTemplate* pTemplate = GetDocTemplate();
		ASSERT(pTemplate != NULL);

		newName = m_strPathName;
		if (bReplace && newName.IsEmpty())
		{
			newName = m_strTitle;
			// check for dubious filename
			// AGr: the only change from the original CDocument::
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

		if (!AfxGetApp()->DoPromptFileName(newName,
											bReplace ? AFX_IDS_SAVEFILE : AFX_IDS_SAVEFILECOPY,
											OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, pTemplate))
			return FALSE;       // don't even attempt to save
	}

	CWaitCursor wait;

	if (!OnSaveDocument(newName))
	{
		if (lpszPathName == NULL)
		{
			// be sure to delete the file
			TRY
			{
				CFile::Remove(newName);
			}
			CATCH_ALL(e)
			{
				TRACE0("Warning: failed to delete file after failed SaveAs.\n");
				//DELETE_EXCEPTION(e);
			}
			END_CATCH_ALL
		}
		return FALSE;
	}

	// reset the title and change the document name
	if (bReplace)
		SetPathName(newName);

	return TRUE;        // success
}

void CWaveSoapFrontDoc::SetSelection(long begin, long end, int channel, int caret, BOOL bMakeCaretVisible)
{
	CSelectionUpdateInfo ui;
	ui.SelBegin = m_SelectionStart;
	ui.SelEnd = m_SelectionEnd;
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
	ui.SelChannel = m_SelectedChannel;
	m_SelectedChannel = channel;
	ui.CaretPos = m_CaretPosition;
	m_CaretPosition = caret;
	ui.m_bMakeCaretVisible = bMakeCaretVisible;
	UpdateAllViews(NULL, UpdateSelectionChanged, & ui);
}

BOOL CWaveSoapFrontDoc::AllocatePeakData(long NewNumberOfSamples)
{
	// change m_pPeaks size
	// need to synchronize with OnDraw
	int NewWavePeakSize = WaveChannels() * ((NewNumberOfSamples + m_PeakDataGranularity - 1) / m_PeakDataGranularity);
	if (NewWavePeakSize > m_AllocatedWavePeakSize)
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
	CSoundUpdateInfo * pui = new CSoundUpdateInfo;
	pui->FileID = FileID;
	pui->UpdateCode = UpdateSoundChanged;
	pui->Begin = begin;
	pui->End = end;
	pui->Length = FileLength;
	{
		CSimpleCriticalSectionLock lock(m_cs);
		if (NULL == m_pUpdateList)
		{
			m_pUpdateList = pui;
		}
		else
		{
			CSoundUpdateInfo *pInfo = m_pUpdateList;
			while (1)
			{
				// see if the ranges overlap
				if (FileID == pInfo->FileID
					&& pInfo->UpdateCode == UpdateSoundChanged
					&& pInfo->Length == FileLength
					&& begin <= pInfo->End
					&& end >= pInfo->Begin)
				{
					if (begin < pInfo->Begin)
					{
						pInfo->Begin = begin;
					}
					if (pInfo->End < end)
					{
						pInfo->End = end;
					}
					delete pui;
					break;
				}
				if (NULL == pInfo->pNext)
				{
					pInfo->pNext = pui;
					break;
				}
				pInfo = pInfo->pNext;
			}
		}
	}
	::PostMessage(GetApp()->m_pMainWnd->m_hWnd, WM_KICKIDLE, 0, 0);
}

void CWaveSoapFrontDoc::RescanPeaks(long begin, long end)
{
	// if called immediately after data modification, it will get
	// the data directly from the cache
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
		long lRead = m_WavFile.m_File.GetDataBuffer( & pBuf,
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
			m_WavFile.m_File.ReturnDataBuffer(pBuf, lRead, 0);
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
		ch = 2;
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
		ch = 2;
	}
	DoDelete(m_SelectionStart, m_SelectionEnd, ch);
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::OnUpdateEditDelete(CCmdUI* pCmdUI)
{
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
		ch = 2;
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
	pCmdUI->Enable(NULL != m_pUndoList && ! (m_OperationInProgress || m_bReadOnly));
}

UINT CWaveSoapFrontDoc::_ThreadProc(void)
{
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
		if (m_pCurrentContext)
		{
			if (0 == (m_pCurrentContext->m_Flags & OperationContextInitialized))
			{
				if ( ! m_pCurrentContext->Init())
				{
					m_pCurrentContext->m_Flags |= OperationContextStop;
				}
				m_CurrentStatusString = m_pCurrentContext->GetStatusString();
				m_pCurrentContext->m_Flags |= OperationContextInitialized;
				NeedKickIdle = true;
			}

			if (m_StopOperation)
			{
				m_pCurrentContext->m_Flags |= OperationContextStopRequested;
			}

			int LastPercent = m_pCurrentContext->PercentCompleted;
			// execute one step
			if (0 == (m_pCurrentContext->m_Flags &
					(OperationContextStop | OperationContextFinished)))
			{
				if (! m_pCurrentContext->OperationProc())
				{
					m_pCurrentContext->m_Flags |= OperationContextStop;
				}
				// signal for status update
				if (LastPercent != m_pCurrentContext->PercentCompleted)
				{
					NeedKickIdle = true;
				}
				if (NeedKickIdle)
				{
					if (m_pCurrentContext->PercentCompleted >= 0)
					{
						m_CurrentStatusString.Format(_T("%s%d%%"),
													(LPCTSTR)m_pCurrentContext->GetStatusString(),
													m_pCurrentContext->PercentCompleted);
					}
					else
					{
						m_CurrentStatusString = m_pCurrentContext->GetStatusString();
					}
				}
			}
			if (m_pCurrentContext->m_Flags &
				(OperationContextStop | OperationContextFinished))
			{
				if (m_pCurrentContext->PercentCompleted >= 0)
				{
					m_CurrentStatusString =
						m_pCurrentContext->GetStatusString() + _T("Completed");
				}
				NeedKickIdle = true;
				COperationContext * pContext =
					(COperationContext *)InterlockedExchange(PLONG( & m_pCurrentContext), NULL);
				pContext->DeInit();
				pContext->Retire();     // usually deletes it
			}
			continue;
		}
		WaitForSingleObject(m_hThreadEvent, INFINITE);
	}
	return 0;
}

BOOL CWaveSoapFrontDoc::DoUndoRedo(CUndoRedoContext * pContext)
{
	// see what is necessary to restore
	CResizeContext * pResize = pContext->m_pExpandShrinkContext;
	if (pContext->m_Flags & CopyShrinkFile)
	{
		// shrink the file
		// todo: create redo
		if (NULL != pResize
			&& pResize->InitUndo("Redoing...")
			&& NULL != pResize->m_pUndoContext
			&& 0 == (pContext->m_Flags & RedoContext))
		{
			pResize->m_pUndoContext->m_Flags |= RedoContext;
		}
	}
	else if (pContext->m_Flags & CopyExpandFile)
	{
		if (NULL != pResize
			&& pResize->InitUndo("Redoing...")
			&& NULL != pResize->m_pUndoContext
			&& 0 == (pContext->m_Flags & RedoContext))
		{
			pResize->m_pUndoContext->m_Flags |= RedoContext;
		}
		if (FALSE == pContext->m_DstFile.m_File.SetFileLength(pContext->m_RestoredLength))
		{
			delete pResize->m_pUndoContext;
			pResize->m_pUndoContext = NULL;
			return FALSE;
		}
		MMCKINFO * pDatachunk = pContext->m_DstFile.GetDataChunk();
		pDatachunk->cksize = pContext->m_RestoredLength - pDatachunk->dwDataOffset;
		long NewLength = pDatachunk->cksize / pContext->m_DstFile.SampleSize();
		SoundChanged(pContext->m_DstFile.GetFileID(), 0, 0, NewLength);
		// todo: create redo
	}
	else
	{
		// just copy
		// todo: create redo
		if ((pContext->m_Flags & RedoContext)
			? RedoEnabled() : UndoEnabled())
		{
			CUndoRedoContext * pUndoRedo = new CUndoRedoContext(this, "Redoing...");
			if (NULL == pUndoRedo)
			{
				return FALSE;
			}
			if ( ! pUndoRedo->InitUndoCopy(m_WavFile,
											pContext->m_DstCopyPos, pContext->m_DstEnd, pContext->m_DstChan))
			{
				delete pUndoRedo;
				return FALSE;
			}
		}
	}
	return TRUE;
}

void CWaveSoapFrontDoc::OnEditUndo()
{
	if (m_OperationInProgress
		|| NULL == m_pUndoList)
	{
		return;
	}
	// no critical section lock needed
	CUndoRedoContext * pUndo = (CUndoRedoContext *) m_pUndoList;
	if ( ! DoUndoRedo(pUndo))
	{
		return;
	}

	m_pUndoList = pUndo->pNext;
	pUndo->pNext = NULL;

	DecrementModified();

	QueueOperation(pUndo);
}

void CWaveSoapFrontDoc::OnEditRedo()
{
	if (m_OperationInProgress
		|| NULL == m_pRedoList)
	{
		return;
	}
	// no critical section lock needed
	CUndoRedoContext * pRedo = (CUndoRedoContext *) m_pRedoList;
	if ( ! DoUndoRedo(pRedo))
	{
		return;
	}

	m_pRedoList = pRedo->pNext;
	pRedo->pNext = NULL;

	IncrementModified(FALSE);   // don't delete redo

	QueueOperation(pRedo);
}

void CWaveSoapFrontDoc::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(NULL != m_pRedoList && ! (m_OperationInProgress || m_bReadOnly));
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
		{
			CSimpleCriticalSectionLock lock(m_cs);
			if (NULL == m_pCurrentContext)
			{
				m_pCurrentContext = pContext;
			}
			else
			{
				COperationContext * pLast = m_pCurrentContext;
				while (pLast->pNext != NULL)
				{
					pLast = pLast->pNext;
				}
				pLast->pNext = pContext;
			}
		}
		SetEvent(m_hThreadEvent);
	}
}

void CWaveSoapFrontDoc::DoCopy(LONG Start, LONG End, LONG Channel, LPCTSTR FileName)
{
	// create a operation context
	CCopyContext * pContext = new CCopyContext(this, _T("Copying data to clipboard..."));

	CWaveFile DstFile;
	// create a temporary clipboard WAV file
	if (FALSE == DstFile.CreateWaveFile( & m_WavFile, m_SelectedChannel,
										m_SelectionEnd - m_SelectionStart,
										CreateWaveFileTempDir
										// keep the file for debug
										| CreateWaveFileDeleteAfterClose
										| CreateWaveFileDontCopyInfo
										| CreateWaveFilePcmFormat
										| CreateWaveFileTemp,
										FileName))
	{
		delete pContext;
		AfxMessageBox(IDS_STRING_UNABLE_TO_CREATE_CLIPBOARD, MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	pContext->m_Flags |= OperationContextClipboard;

	pContext->InitCopy(DstFile, 0, m_SelectionEnd - m_SelectionStart, 2,
						m_WavFile, m_SelectionStart, m_SelectionEnd - m_SelectionStart,
						m_SelectedChannel);

	GetApp()->m_ClipboardFile = DstFile;
	// set operation context to the queue
	QueueOperation(pContext);
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
	if (End > Start)
	{
		CPasteModeDialog dlg;
		dlg.m_PasteMode = m_DefaultPasteMode;
		if (dlg.DoModal() != IDOK)
		{
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
	if (Channel < 2 || WaveChannels() < 2)
	{
		nCopiedChannels = 1;
	}
	int ChannelToCopyFrom = 2;
	if (nCopiedChannels < pSrcFile->Channels())
	{
		CCopyChannelsSelectDlg dlg;
		dlg.m_ChannelToCopy = m_PrevChannelToCopy;
		if (IDOK != dlg.DoModal())
		{
			return;
		}
		m_PrevChannelToCopy = dlg.m_ChannelToCopy;
		ChannelToCopyFrom = dlg.m_ChannelToCopy;
	}

	CCopyContext * pContext = new CCopyContext(this, _T("Inserting data from clipboard..."));

	pContext->m_Flags |= OperationContextClipboard | CopyExpandFile;

	if ( ! pContext->InitCopy(m_WavFile, Start, End - Start, Channel,
							* pSrcFile, 0, NumSamplesToPasteFrom, ChannelToCopyFrom))
	{
		delete pContext;
		// error
		return;
	}

	if (NULL != pContext->m_pExpandShrinkContext)
	{
		pContext->m_pExpandShrinkContext->InitUndo("Undoing Paste...");
		// if the source selection is not empty,
		// init undo copy
	}
	else
	{
		// todo: create UNDO with only copy
	}
	// set operation context to the queue
	SoundChanged(WaveFileID(), 0, 0, WaveFileSamples());
	QueueOperation(pContext);
}

void CWaveSoapFrontDoc::DoCut(LONG Start, LONG End, LONG Channel)
{
	// save the cut area to Undo context, then shrink the file
	// create a operation context
	CCopyContext * pContext = new CCopyContext(this, _T("Copying data to clipboard..."));

	CWaveFile DstFile;
	// create a temporary clipboard WAV file
	if (FALSE == DstFile.CreateWaveFile( & m_WavFile, Channel,
										End - Start,
										CreateWaveFileTempDir
										// keep the file for debug
										| CreateWaveFileDeleteAfterClose
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

	pContext->InitCopy(DstFile, 0, End - Start, 2,
						m_WavFile, Start, End - Start,
						Channel);
	GetApp()->m_ClipboardFile = DstFile;
	// set operation context to the queue
	QueueOperation(pContext);
	CResizeContext * pResizeContext = new CResizeContext(this, _T("Shrinking the file..."));
	pResizeContext->InitShrink(m_WavFile, Start, End - Start, Channel);
	pResizeContext->InitUndo("Undoing Cut...");
	SetSelection(Start, Start, Channel, Start);
	QueueOperation(pResizeContext);
}

void CWaveSoapFrontDoc::DoDelete(LONG Start, LONG End, LONG Channel)
{
	if (End == Start)
	{
		End++;
	}
	CResizeContext * pResizeContext = new CResizeContext(this, _T("Deleting the selection..."));
	pResizeContext->InitShrink(m_WavFile, Start, End - Start, Channel);
	pResizeContext->InitUndo("Undoing Delete...");
	SetSelection(Start, Start, Channel, Start);
	QueueOperation(pResizeContext);
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

BOOL CWaveSoapFrontDoc::OnOpenDocument(LPCTSTR lpszPathName)
{

	// TODO: Add your specialized creation code here
	TRACE("CWaveSoapFrontDoc::OnOpenDocument(%s)\n", lpszPathName);

	m_CaretPosition = 0;
	m_SelectionStart = 0;
	m_SelectionEnd = 0;
	m_SelectedChannel = 2;  // both channels
	m_TimeSelectionMode = TRUE;
	CWaveSoapFrontApp * pApp = GetApp();
	m_bDirectMode = (pApp->m_bDirectMode != 0);
	m_bReadOnly = (pApp->m_bReadOnly != 0);
	if (m_bReadOnly)
	{
		m_bDirectMode = TRUE;
	}

	m_szWaveFilename = lpszPathName;

	DWORD flags = MmioFileOpenReadOnly;
	if (m_bDirectMode && ! m_bReadOnly)
	{
		flags = MmioFileOpenExisting | MmioFileAllowReadOnlyFallback;
	}

	if (FALSE == OpenWaveFile(lpszPathName, flags))
	{
		return FALSE;
	}
	m_WavFile.GetFileInformationByHandle(& WavFileInfo);
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
		if (m_WavFile.m_File.IsReadOnly())
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
	szPeakFilename = m_szWaveFilename;
	if (0 == szPeakFilename.Right(4).CompareNoCase(_T(".WAV")))
	{
		szPeakFilename.Delete(szPeakFilename.GetLength() - 4, 4);
	}
	szPeakFilename += _T(".wspk");
	// if non-direct mode, create a temp file.
	if ( ! m_bDirectMode)
	{
		CWaveFile SrcFile;
		SrcFile = m_WavFile;
		m_WavFile.Close();
		if (bNeedConversion)
		{
			flags = CreateWaveFileTempDir
					// don't keep the file
					| CreateWaveFileDeleteAfterClose
					| CreateWaveFilePcmFormat
					| CreateWaveFileTemp;
		}
		else
		{
			flags = CreateWaveFileTempDir
					// don't keep the file
					| CreateWaveFileDeleteAfterClose
					| CreateWaveFilePcmFormat
					| CreateWaveFileDontInitStructure
					| CreateWaveFileTemp;
		}
		if (! m_WavFile.CreateWaveFile( & SrcFile, pWf->nChannels,
										SrcFile.NumberOfSamples(), flags, NULL))
		{
			AfxMessageBox(IDS_UNABLE_TO_CREATE_TEMPORARY_FILE, MB_OK | MB_ICONEXCLAMATION);
			return FALSE;
		}
		// for compressed file, actual size of file may differ from the
		// initial size
		if (bNeedConversion)
		{
			CDecompressContext * pContext =
				new CDecompressContext(this, "Loading the compressed file...");
			pContext->m_SrcFile = SrcFile;
			pContext->m_DstFile = m_WavFile;
			pContext->m_SrcStart = SrcFile.GetDataChunk()->dwDataOffset;
			pContext->m_SrcPos = pContext->m_SrcStart;
			pContext->m_SrcEnd = pContext->m_SrcStart +
								SrcFile.GetDataChunk()->cksize;
			pContext->m_DstStart = m_WavFile.GetDataChunk()->dwDataOffset;
			pContext->m_DstPos = pContext->m_DstStart;
			pContext->m_CurrentSamples = m_WavFile.NumberOfSamples();
			AllocatePeakData(pContext->m_CurrentSamples);
			// peak data will be created during decompression
			QueueOperation(pContext);
		}
		else
		{
			// just link the files
			UINT format = 0;
			if (m_WavFile.m_File.SetSourceFile( & SrcFile.m_File)
				&& m_WavFile.m_File.SetFileLength(SrcFile.m_File.GetLength())
				&& m_WavFile.LoadRiffChunk()
				&& m_WavFile.LoadWaveformat())
			{
				if (m_WavFile.FindData())
				{
					m_SizeOfWaveData = WaveDataChunk()->cksize;
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
			if (format)
			{
				CString s;
				s.Format(format, lpszPathName);
				AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}
			LoadPeakFile();
		}
	}
	else
	{
		LoadPeakFile();
	}
	// if file is open in direct mode or read-only, leave it as is,
	// otherwise open
	return TRUE;
}

BOOL CWaveSoapFrontDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	// TODO: Add your specialized code here and/or call the base class

	return TRUE;
}

void CWaveSoapFrontDoc::OnIdle()
{
	if (m_bInOnIdle)
	{
		return;
	}
	m_bInOnIdle = true;
	while (m_pUpdateList)
	{
		CSoundUpdateInfo * pInfo;
		{
			CSimpleCriticalSectionLock lock(m_cs);
			pInfo = m_pUpdateList;
			if (NULL == pInfo)
			{
				break;
			}
			m_pUpdateList = pInfo->pNext;
		}
		if (pInfo->Length != -1)
		{
			m_SizeOfWaveData = pInfo->Length * WaveSampleSize();
		}
		UpdateAllViews(NULL, pInfo->UpdateCode, pInfo);
		delete pInfo;
	}
	while (m_pRetiredList)
	{
		COperationContext * pContext;
		{
			CSimpleCriticalSectionLock lock(m_cs);
			pContext = m_pRetiredList;
			m_pRetiredList = pContext->pNext;
		}
		pContext->PostRetire();     // deletes it usually
	}
	m_bInOnIdle = false;
}


void CWaveSoapFrontDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(IsModified() && ! m_bReadOnly);
}

void CWaveSoapFrontDoc::OnEditSelectAll()
{
	long len = WaveFileSamples();
	SetSelection(0, len, 2, len, true);
}

void CWaveSoapFrontDoc::OnEditSelection()
{

}

void CWaveSoapFrontDoc::OnUpdateEditSelection(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here

}

static void LimitUndoRedo(COperationContext ** ppContext, int MaxNum, size_t MaxSize)
{
	size_t Size = 0;
	int Num = 0;
	while (* ppContext != NULL)
	{
		COperationContext * pContext = *ppContext;
		Num++;
		CUndoRedoContext * pUndoRedo = dynamic_cast<CUndoRedoContext *>(pContext);
		if (NULL != pUndoRedo
			&& pUndoRedo->m_SrcFile.IsOpen())
		{
			Size += pUndoRedo->m_SrcFile.m_File.GetLength();
		}
		ppContext = & pContext->pNext;
		if (Num > MaxNum
			|| Size > MaxSize)
		{
			COperationContext * tmp;
			pContext = *ppContext;
			* ppContext = NULL;
			while (pContext != NULL)
			{
				tmp = pContext;
				pContext = pContext->pNext;
				delete tmp;
			}
			break;
		}
	}
}

void CWaveSoapFrontDoc::AddUndoRedo(CUndoRedoContext * pContext)
{
	if (pContext->m_Flags & RedoContext)
	{
		pContext->pNext = m_pRedoList;
		m_pRedoList = pContext;
		// free extra redo, if count or size limit is exceeded
		LimitUndoRedo( & m_pRedoList, GetApp()->m_MaxRedoDepth, GetApp()->m_MaxRedoSize);
	}
	else
	{
		pContext->pNext = m_pUndoList;
		m_pUndoList = pContext;
		// free extra undo, if count or size limit is exceeded
		LimitUndoRedo( & m_pUndoList, GetApp()->m_MaxUndoDepth, GetApp()->m_MaxUndoSize);
	}
}

void CWaveSoapFrontDoc::OnUpdateSoundPlay(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(! m_OperationInProgress);
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
	pContext->m_PlaybackDevice = GetApp()->m_DefaultPlaybackDevice;
	if (false == pContext->m_WaveOut.AllocateBuffers(GetApp()->m_SizePlaybackBuffers,
													GetApp()->m_NumPlaybackBuffers))
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
	QueueOperation(pContext);
}

void CWaveSoapFrontDoc::OnUpdateSoundStop(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_OperationInProgress && m_PlayingSound);
}

void CWaveSoapFrontDoc::OnUpdateIndicatorFileSize(CCmdUI* pCmdUI)
{
	SetStatusString(pCmdUI, TimeToHhMmSs(double(WaveFileSamples())
										/ m_WavFile.GetWaveFormat()->nSamplesPerSec * 1000.,
										TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs));
}

void CWaveSoapFrontDoc::OnUpdateIndicatorSelectionLength(CCmdUI* pCmdUI)
{
	if (m_SelectionStart != m_SelectionEnd)
	{
		SetStatusString(pCmdUI,
						TimeToHhMmSs(double(m_SelectionEnd - m_SelectionStart)
									/ m_WavFile.GetWaveFormat()->nSamplesPerSec * 1000.,
									TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs));
	}
	else
	{
		SetStatusString(pCmdUI, _T("            "));
	}
}

void CWaveSoapFrontDoc::OnUpdateIndicatorCurrentPos(CCmdUI* pCmdUI)
{
	int TimeMs;
	CSoundPlayContext * pCx = dynamic_cast<CSoundPlayContext *>(m_pCurrentContext);
	if (m_PlayingSound && NULL != pCx)
	{
		TimeMs = double(pCx->m_SamplePlayed)
				/ m_WavFile.GetWaveFormat()->nSamplesPerSec * 1000.;
		TimeMs -= TimeMs % 100;
		int BeginTimeMs = double(pCx->m_FirstSamplePlayed)
						/ m_WavFile.GetWaveFormat()->nSamplesPerSec * 1000.;
		if (TimeMs < BeginTimeMs)
		{
			TimeMs = BeginTimeMs;
		}
	}
	else
	{
		TimeMs = double(m_CaretPosition)
				/ m_WavFile.GetWaveFormat()->nSamplesPerSec * 1000.;
	}
	SetStatusString(pCmdUI, TimeToHhMmSs(TimeMs,
										TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs));
}

void CWaveSoapFrontDoc::OnUpdateIndicatorSampleRate(CCmdUI* pCmdUI)
{
	SetStatusString(pCmdUI, LtoaCS(WaveFormat()->nSamplesPerSec));
}

void CWaveSoapFrontDoc::OnUpdateIndicatorSampleSize(CCmdUI* pCmdUI)
{
	SetStatusString(pCmdUI, _T("16-bit"));
}

void CWaveSoapFrontDoc::OnUpdateIndicatorChannels(CCmdUI* pCmdUI)
{
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
		CSoundPlayContext * pCx = dynamic_cast<CSoundPlayContext *>(m_pCurrentContext);
		if (pCx)
		{
			pCx->m_bPauseRequested = true;
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
	while (m_pUndoList)
	{
		COperationContext * pContext = m_pUndoList;
		m_pUndoList = pContext->pNext;
		delete pContext;
	}
}

void CWaveSoapFrontDoc::DeleteRedo()
{
	while (m_pRedoList)
	{
		COperationContext * pContext = m_pRedoList;
		m_pRedoList = pContext->pNext;
		delete pContext;
	}
}

BOOL CWaveSoapFrontDoc::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	OnIdle();   // retire all contexts
	return CDocument::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

inline BOOL CWaveSoapFrontDoc::IsModified()
{
	return m_ModificationSequenceNumber != 0;
}

void CWaveSoapFrontDoc::IncrementModified(BOOL bDeleteRedo)
{
	// bDeleteRedo is FALSE for Redo command
	BOOL OldModified = CWaveSoapFrontDoc::IsModified();
	m_ModificationSequenceNumber++;
	if (bDeleteRedo)
	{
		DeleteRedo();
	}
	if (! UndoEnabled())
	{
		// since it is unable to restore undo, delete all previous undo
		DeleteUndo();
	}

	if (CWaveSoapFrontDoc::IsModified() != OldModified)
	{
		UpdateFrameCounts();        // will cause name change in views
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
		UpdateFrameCounts();        // will cause name change in views
	}
}

void CWaveSoapFrontDoc::SetModifiedFlag(BOOL bModified)
{
	if (bModified)
	{
		IncrementModified();
	}
	else
	{
		BOOL OldModified = CWaveSoapFrontDoc::IsModified();
		m_ModificationSequenceNumber = 0;
		if (OldModified)
		{
			UpdateFrameCounts();        // will cause name change in views
		}
	}
}

void CWaveSoapFrontDoc::PlaybackPositionNotify(long position, int channel)
{
	CSoundUpdateInfo * pui = new CSoundUpdateInfo;
	pui->UpdateCode = UpdatePlaybackPositionChanged;
	pui->FileID = m_WavFile.GetFileID();
	pui->UpdateCode = UpdatePlaybackPositionChanged;
	pui->Begin = position;
	pui->End = channel;

	pui->Length = -1;
	{
		CSimpleCriticalSectionLock lock(m_cs);
		if (NULL == m_pUpdateList)
		{
			m_pUpdateList = pui;
		}
		else
		{
			CSoundUpdateInfo *pInfo = m_pUpdateList;
			while (1)
			{
				// see if the ranges overlap
				if (pInfo->UpdateCode == UpdatePlaybackPositionChanged)
				{
					pInfo->Begin = position;
					pInfo->End = channel;
					delete pui;
					break;
				}
				if (NULL == pInfo->pNext)
				{
					pInfo->pNext = pui;
					break;
				}
				pInfo = pInfo->pNext;
			}
		}
	}
	::PostMessage(GetApp()->m_pMainWnd->m_hWnd, WM_KICKIDLE, 0, 0);
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

void CWaveSoapFrontDoc::OnUpdateSampleRate(CCmdUI* pCmdUI, unsigned SampleRate)
{
	pCmdUI->SetRadio(m_WavFile.SampleRate() == SampleRate);
}

void CWaveSoapFrontDoc::OnSamplerate11025()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateSamplerate11025(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 11025);
}

void CWaveSoapFrontDoc::OnSamplerate16000()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateSamplerate16000(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 16000);
}

void CWaveSoapFrontDoc::OnSamplerate22050()
{
}

void CWaveSoapFrontDoc::OnUpdateSamplerate22050(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 22050);
}

void CWaveSoapFrontDoc::OnSamplerate32000()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateSamplerate32000(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 32000);
}

void CWaveSoapFrontDoc::OnSamplerate44100()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateSamplerate44100(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 44100);
}

void CWaveSoapFrontDoc::OnSamplerate48k()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateSamplerate48k(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 48000);
}

void CWaveSoapFrontDoc::OnSamplerate7200()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateSamplerate7200(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 7200);
}

void CWaveSoapFrontDoc::OnSamplerate8000()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateSamplerate8000(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 8000);
}

void CWaveSoapFrontDoc::OnSamplerate96k()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateSamplerate96k(CCmdUI* pCmdUI)
{
	OnUpdateSampleRate(pCmdUI, 96000);
}

void CWaveSoapFrontDoc::OnChannelsMono()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateChannelsMono(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(WaveChannels() == 1);
}

void CWaveSoapFrontDoc::OnChannelsStereo()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateChannelsStereo(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(WaveChannels() == 2);
}
/////////////////////////////////////////////////////////////////////////////
// CCopyChannelsSelectDlg dialog


CCopyChannelsSelectDlg::CCopyChannelsSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCopyChannelsSelectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCopyChannelsSelectDlg)
	m_ChannelToCopy = -1;
	//}}AFX_DATA_INIT
}


void CCopyChannelsSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyChannelsSelectDlg)
	DDX_Radio(pDX, IDC_RADIO_LEFT, m_ChannelToCopy);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCopyChannelsSelectDlg, CDialog)
	//{{AFX_MSG_MAP(CCopyChannelsSelectDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyChannelsSelectDlg message handlers
/////////////////////////////////////////////////////////////////////////////
// CPasteModeDialog dialog


CPasteModeDialog::CPasteModeDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CPasteModeDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPasteModeDialog)
	m_PasteMode = -1;
	//}}AFX_DATA_INIT
}


void CPasteModeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPasteModeDialog)
	DDX_Radio(pDX, IDC_RADIO_SELECT, m_PasteMode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPasteModeDialog, CDialog)
	//{{AFX_MSG_MAP(CPasteModeDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPasteModeDialog message handlers
