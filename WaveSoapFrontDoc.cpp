// WaveSoapFrontDoc.cpp : implementation of the CWaveSoapFrontDoc class
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "MainFrm.h"
#include "WaveSoapFrontDoc.h"
#include "WaveSupport.h"
#include "resource.h"
#include <afxpriv.h>
#include <mmreg.h>
#include <msacm.h>
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
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
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
	ON_COMMAND(IDC_SOUND_PAUSE, OnSoundPause)
	ON_UPDATE_COMMAND_UI(IDC_SOUND_PAUSE, OnUpdateSoundPause)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontDoc construction/destruction

CWaveSoapFrontDoc::CWaveSoapFrontDoc()
	:m_pPeaks(NULL), m_WavePeakSize(0), m_PeakDataGranularity(512),
//m_pWaveBuffer(NULL), m_WaveBufferSize(0),
	m_CaretPosition(0),
	m_TimeSelectionMode(TRUE),
	m_SelectionStart(0),
	m_SelectionEnd(0),
	m_hThreadEvent(NULL),
	m_bRunThread(false),
	m_bReadOnly(true),
	m_bDirectMode(false),
	m_pCurrentContext(NULL),
	m_pQueuedOperation(NULL),
	m_pUndoList(NULL),
	m_pRedoList(NULL),
	m_pUpdateList(NULL),
	m_OperationInProgress(false),
	m_OperationNonCritical(false),
	m_Thread(ThreadProc, this),
	m_SelectedChannel(2)
{
	// TODO: add one-time construction code here
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

	if ( FALSE == m_WavFile.CreateWaveFile(NULL, 2, // 2 channels
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

class CScanPeaksContext : public COperationContext
{
	friend class CWaveSoapFrontDoc;
public:
	DWORD m_Position;
	DWORD m_Start;
	DWORD m_End;
	int m_GranuleSize;
	CString sOp;
	CScanPeaksContext(CWaveSoapFrontDoc * pDoc)
		: COperationContext(pDoc, OperationContextDiskIntensive),
		sOp("Scanning the file for peaks..."),
		m_Start(0), m_End(0), m_Position(0)
	{

	}
	~CScanPeaksContext() {}
	virtual BOOL OperationProc();
	virtual CString GetStatusString() { return sOp; }
};

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
			if ( ! AllocatePeakData(pfh.PeakInfoSize / sizeof (WavePeak)))
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
	if ( ! AllocatePeakData(CalculatePeakInfoSize()))
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

BOOL CScanPeaksContext::OperationProc()
{
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	if (m_Position < m_End)
	{
		DWORD dwStartTime = timeGetTime();
		DWORD dwOperationBegin = m_Position;
		do
		{
			DWORD SizeToRead = m_End - m_Position;
			void * pBuf;
			long lRead = pDocument->m_WavFile.m_File.GetDataBuffer( & pBuf,
							SizeToRead, m_Position, CDirectFile::GetBufferAndPrefetchNext);
			if (lRead > 0)
			{
				unsigned i;
				long DataToProcess = lRead;
				__int16 * pWaveData = (__int16 *) pBuf;
				DWORD DataOffset = m_Position - pDocument->WaveDataChunk()->dwDataOffset;
				unsigned DataForGranule = m_GranuleSize - DataOffset % m_GranuleSize;

				if (2 == pDocument->WaveChannels())
				{
					WavePeak * pPeak = pDocument->m_pPeaks + (DataOffset / m_GranuleSize) * 2;
					while (0 != DataToProcess)
					{
						int wpl_l = pPeak[0].low;
						int wpl_h = pPeak[0].high;
						int wpr_l = pPeak[1].low;
						int wpr_h = pPeak[1].high;

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
							DataForGranule -= 2;
						}

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

						ASSERT(pPeak - pDocument->m_pPeaks < pDocument->m_WavePeakSize);
						if (pPeak[0].low > wpl_l)
						{
							pPeak[0].low = wpl_l;
						}
						if (pPeak[0].high < wpl_h)
						{
							pPeak[0].high = wpl_h;
						}
						if (pPeak[1].low > wpr_l)
						{
							pPeak[1].low = wpr_l;
						}
						if (pPeak[1].high < wpr_h)
						{
							pPeak[1].high = wpr_h;
						}
						pPeak += 2;
						DataForGranule = m_GranuleSize;
					}
				}
				else
				{
					WavePeak * pPeak = pDocument->m_pPeaks + DataOffset / m_GranuleSize;
					while (0 != DataToProcess)
					{
						int wp_l = pPeak[0].low;
						int wp_h = pPeak[0].high;

						if (DataForGranule > DataToProcess)
						{
							DataForGranule = DataToProcess;
						}
						DataToProcess -= DataForGranule;

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

						ASSERT(pPeak - pDocument->m_pPeaks < pDocument->m_WavePeakSize);
						if (pPeak[0].low > wp_l)
						{
							pPeak[0].low = wp_l;
						}
						if (pPeak[0].high < wp_h)
						{
							pPeak[0].high = wp_h;
						}
						pPeak ++;
						DataForGranule = m_GranuleSize;
					}
				}

				m_Position += lRead;
				pDocument->m_WavFile.m_File.ReturnDataBuffer(pBuf, lRead, 0);
			}
			else
			{
				m_Flags |= OperationContextStop;
				break;
				return FALSE;
			}
		}
		while (m_Position < m_End
				&& timeGetTime() - dwStartTime < 200);
		if (m_End > m_Start)
		{
			PercentCompleted = 100i64 * (m_Position - m_Start) / (m_End - m_Start);
		}
		// notify the view
		int nSampleSize = pDocument->WaveSampleSize();
		int nFirstSample = (dwOperationBegin - pDocument->WaveDataChunk()->dwDataOffset)
							/ nSampleSize;
		int nLastSample = (m_Position - pDocument->WaveDataChunk()->dwDataOffset)
						/ nSampleSize;
		pDocument->SoundChanged(pDocument->WaveFileID(),
								nFirstSample, nLastSample, -1, UpdateSoundDontRescanPeaks);
		return TRUE;
	}
	else
	{
		pDocument->SavePeakInfo();
		m_Flags |= OperationContextFinished;
		PercentCompleted = 100;
		return TRUE;
	}

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
		pfh.PeakInfoSize = m_WavePeakSize * sizeof (WavePeak);
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
				m_WavFile.GetFileInformationByHandle(& WavFileInfo);
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

void CWaveSoapFrontDoc::SetSelection(long begin, long end, int channel, int caret)
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
	UpdateAllViews(NULL, UpdateSelectionChanged, & ui);
}

BOOL CWaveSoapFrontDoc::AllocatePeakData(long NewNumberOfSamples)
{
	// change m_pPeaks size
	// need to synchronize with OnDraw
	int NewWavePeakSize = WaveChannels() * ((NewNumberOfSamples + m_PeakDataGranularity - 1) / m_PeakDataGranularity);
	if (NewWavePeakSize > m_WavePeakSize)
	{
		NewWavePeakSize += 1024;  // reserve more
		WavePeak * NewPeaks = new WavePeak[NewWavePeakSize];
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
		WavePeak * OldPeaks = m_pPeaks;
		{
			CSimpleCriticalSectionLock lock(m_PeakLock);
			m_pPeaks = NewPeaks;
			m_WavePeakSize = NewWavePeakSize;
		}
		delete[] OldPeaks;
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
			while (NULL != pInfo->pNext)
			{
				pInfo = pInfo->pNext;
			}
			pInfo->pNext = pui;
		}
	}
	::PostMessage(GetApp()->m_pMainWnd->m_hWnd, WM_KICKIDLE, 0, 0);
}

void CWaveSoapFrontDoc::RescanPeaks(long begin, long end)
{
	// if called immediately after data modification, it will get
	// the data directly from the cache
	int nSampleSize = WaveSampleSize();
	DWORD dwDataOffset = WaveDataChunk()->dwDataOffset;
	unsigned GranuleSize = WaveChannels() * m_PeakDataGranularity * sizeof(__int16);

	DWORD Pos = nSampleSize * (begin & -m_PeakDataGranularity) + dwDataOffset;

	DWORD EndPos = nSampleSize * ((end + m_PeakDataGranularity - 1) & -m_PeakDataGranularity)
					+ dwDataOffset;
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
			DWORD DataOffset = Pos - dwDataOffset;
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
					if (pPeak[0].low > wpl_l)
					{
						pPeak[0].low = wpl_l;
					}
					if (pPeak[0].high < wpl_h)
					{
						pPeak[0].high = wpl_h;
					}
					if (pPeak[1].low > wpr_l)
					{
						pPeak[1].low = wpr_l;
					}
					if (pPeak[1].high < wpr_h)
					{
						pPeak[1].high = wpr_h;
					}
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
					if (pPeak[0].low > wp_l)
					{
						pPeak[0].low = wp_l;
					}
					if (pPeak[0].high < wp_h)
					{
						pPeak[0].high = wp_h;
					}
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
	if (m_OperationInProgress)
	{
		return;
	}
	DoCut(m_SelectionStart, m_SelectionEnd, m_SelectedChannel, NULL);
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::OnUpdateEditCut(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_SelectionStart != m_SelectionEnd
					&& ! (m_OperationInProgress || m_bReadOnly));
}

void CWaveSoapFrontDoc::OnEditPaste()
{
	if (m_OperationInProgress)
	{
		return;
	}
	DoPaste(m_SelectionStart, m_SelectionEnd, m_SelectedChannel, NULL);
	SetModifiedFlag(TRUE);
}

void CWaveSoapFrontDoc::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetApp()->m_ClipboardFile.IsOpen()
					&& ! (m_OperationInProgress || m_bReadOnly));
}

void CWaveSoapFrontDoc::OnEditUndo()
{
	if (m_OperationInProgress
		|| NULL == m_pUndoList)
	{
		return;
	}
	SetModifiedFlag(TRUE);
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
					m_CurrentStatusString.Format(_T("%s%d%%"),
												(LPCTSTR)m_pCurrentContext->GetStatusString(),
												m_pCurrentContext->PercentCompleted);
				}
			}
			if (m_pCurrentContext->m_Flags &
				(OperationContextStop | OperationContextFinished))
			{
				m_CurrentStatusString =
					m_pCurrentContext->GetStatusString() + _T("Completed");
				NeedKickIdle = true;
				COperationContext * pContext =
					(COperationContext *)InterlockedExchange(PLONG( & m_pCurrentContext), NULL);
				pContext->DeInit();
				m_OperationInProgress = false;
				delete pContext;
			}
			continue;
		}
		WaitForSingleObject(m_hThreadEvent, INFINITE);
	}
	return 0;
}

enum {
	CopyExpandFile = 0x80000000,
	CopyShrinkFile = 0x40000000,
};

class CResizeContext : public COperationContext
{
	friend class CWaveSoapFrontDoc;
	CWaveFile m_DstFile;
	// Start, End and position are in bytes
	DWORD m_SrcStart;
	DWORD m_SrcEnd;
	int m_DstChan;
	DWORD m_SrcCopyPos;

	DWORD m_DstStart;
	DWORD m_DstCopyPos;
	DWORD m_DstEnd;
	BOOL ExpandProc();
	BOOL ShrinkProc();

public:
	CString sOp;
	CResizeContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString)
		: COperationContext(pDoc, OperationContextDiskIntensive),
		sOp(StatusString)
	{

	}
	~CResizeContext() {}
	BOOL InitExpand(CWaveFile & File, LONG StartSample, LONG Length, int Channel);
	BOOL InitShrink(CWaveFile & File, LONG StartSample, LONG Length, int Channel);
	virtual BOOL OperationProc();
	virtual CString GetStatusString() { return sOp; }
};

class CCopyContext : public COperationContext
{
	friend class CWaveSoapFrontDoc;
	CWaveFile m_SrcFile;
	CWaveFile m_DstFile;
	// Start, End and position are in bytes
	DWORD m_SrcStart;
	DWORD m_SrcEnd;
	int m_SrcChan;
	DWORD m_SrcCopyPos;

	DWORD m_DstStart;
	DWORD m_DstCopyPos;
	DWORD m_DstEnd;
	int m_DstChan;
	class CResizeContext * m_pExpandShrinkContext;

public:
	CString sOp;
	CCopyContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString)
		: COperationContext(pDoc, OperationContextDiskIntensive),
		sOp(StatusString),
		m_pExpandShrinkContext(NULL)
	{

	}
	~CCopyContext();
	BOOL InitCopy(CWaveFile & DstFile,
				LONG DstStartSample, LONG DstLength, LONG DstChannel,
				CWaveFile & SrcFile,
				LONG SrcStartSample, LONG SrcLength, LONG SrcChannel
				);
	BOOL InitExpand(LONG StartSample, LONG Length, int Channel);
	virtual BOOL OperationProc();
	virtual CString GetStatusString()
	{
		if (m_pExpandShrinkContext)
		{
			return m_pExpandShrinkContext->GetStatusString();
		}
		return sOp;
	}
};

BOOL CResizeContext::InitExpand(CWaveFile & File, LONG StartSample, LONG Length, int Channel)
{
	m_DstFile = File;
	DWORD SampleSize = m_DstFile.SampleSize();
	m_DstChan = Channel;
	// first, expand the file itself
	LPMMCKINFO pDatachunk = m_DstFile.GetDataChunk();
	if (NULL == pDatachunk)
	{
		return FALSE;
	}

	m_SrcEnd = pDatachunk->dwDataOffset + pDatachunk->cksize;
	m_SrcCopyPos = m_SrcEnd;
	m_SrcStart = pDatachunk->dwDataOffset +
				StartSample * SampleSize;

	size_t NewDataLength = (m_DstFile.NumberOfSamples() + Length) * SampleSize;

	if (FALSE == m_DstFile.m_File.SetFileLength(pDatachunk->dwDataOffset + NewDataLength))
	{
		return FALSE;
	}
	pDatachunk->cksize = NewDataLength;
	// when expanding, data is moved starting from the end of the file,
	// but I use SrcStart as begin of the moved area and SrcEns as end (SrcStart <= SrcEnd)

	m_DstStart = pDatachunk->dwDataOffset +
				(StartSample + Length) * SampleSize;

	m_DstEnd = pDatachunk->dwDataOffset + NewDataLength;
	m_DstCopyPos = m_DstEnd;

	m_Flags |= CopyExpandFile;
	return TRUE;
}

BOOL CWaveSoapFrontDoc::QueueOperation(COperationContext * pContext)
{
	if (m_OperationInProgress)
	{
		delete pContext;
		AfxMessageBox(IDS_STRING_OPERATION_IN_PROGRESS,
					MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}
	m_StopOperation = false;
	m_OperationNonCritical = (0 != (pContext->m_Flags & OperationContextNonCritical));
	m_OperationInProgress = TRUE;
	if (pContext->m_Flags & (OperationContextDiskIntensive | OperationContextClipboard))
	{
		return GetApp()->QueueOperation(pContext);
	}
	// the operation is performed by the document thread
	m_pCurrentContext = pContext;
	SetEvent(m_hThreadEvent);
	return TRUE;
}

BOOL CCopyContext::InitCopy(CWaveFile & DstFile,
							LONG DstStartSample, LONG DstLength, LONG DstChannel,
							CWaveFile & SrcFile,
							LONG SrcStartSample, LONG SrcLength, LONG SrcChannel
							)
{
	m_DstFile = DstFile;
	m_SrcFile = SrcFile;

	m_SrcChan = SrcChannel;

	DWORD SrcSampleSize = m_SrcFile.SampleSize();
	m_SrcStart = m_SrcFile.GetDataChunk()->dwDataOffset +
				SrcStartSample * SrcSampleSize;
	m_SrcCopyPos = m_SrcStart;
	m_SrcEnd = m_SrcFile.GetDataChunk()->dwDataOffset +
				(SrcStartSample + SrcLength) * SrcSampleSize;

	DWORD DstSampleSize = m_DstFile.SampleSize();

	m_DstCopyPos = m_DstFile.GetDataChunk()->dwDataOffset +
					DstStartSample * DstSampleSize;

	m_DstEnd = m_DstFile.GetDataChunk()->dwDataOffset +
				(DstStartSample + SrcLength) * DstSampleSize;
	m_DstChan = DstChannel;
	if (SrcLength > DstLength)
	{
		m_pExpandShrinkContext = new CResizeContext(pDocument, _T("Expanding the file..."));
		if (NULL == m_pExpandShrinkContext)
		{
			return FALSE;
		}
		if ( ! m_pExpandShrinkContext->InitExpand(DstFile, DstStartSample + DstLength,
												SrcLength - DstLength, DstChannel))
		{
			delete m_pExpandShrinkContext;
			m_pExpandShrinkContext = NULL;
			return FALSE;
		}

		m_Flags |= CopyExpandFile;
	}
	else if (SrcLength < DstLength)
	{
		m_Flags |= CopyShrinkFile;
	}

	return TRUE;
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
#if 1 || !defined _DEBUG
										| CreateWaveFileDeleteAfterClose
#endif
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

CCopyContext::~CCopyContext()
{
	if (m_pExpandShrinkContext)
	{
		delete m_pExpandShrinkContext;
		m_pExpandShrinkContext = NULL;
	}
}

BOOL CCopyContext::OperationProc()
{
	// get buffers from source file and copy them to m_CopyFile
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	if (m_Flags & (CopyExpandFile | CopyShrinkFile))
	{
		if (NULL != m_pExpandShrinkContext)
		{
			if ( 0 == (m_pExpandShrinkContext->m_Flags & (OperationContextStop | OperationContextFinished)))
			{
				if ( ! m_pExpandShrinkContext->OperationProc())
				{
					m_pExpandShrinkContext->m_Flags |= OperationContextStop;
				}
			}
			PercentCompleted = m_pExpandShrinkContext->PercentCompleted;
			if (m_pExpandShrinkContext->m_Flags &
				(OperationContextStop | OperationContextFinished))
			{
				delete m_pExpandShrinkContext;
				m_pExpandShrinkContext = NULL;
				m_Flags &= ~(CopyExpandFile | CopyShrinkFile);
			}
			return TRUE;
		}
		m_Flags &= ~(CopyExpandFile | CopyShrinkFile);
		PercentCompleted = 0;
	}

	if (m_SrcCopyPos >= m_SrcEnd)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	DWORD dwStartTime = timeGetTime();
	DWORD dwOperationBegin = m_DstCopyPos;

	DWORD LeftToRead = 0;
	DWORD LeftToWrite = 0;
	DWORD WasRead = 0;
	DWORD WasLockedToWrite = 0;
	void * pOriginalSrcBuf;
	char * pSrcBuf;
	void * pOriginalDstBuf;
	char * pDstBuf;

	if (m_SrcCopyPos > m_SrcEnd
		|| m_DstCopyPos > m_DstEnd)
	{
		return FALSE;
	}
	do
	{
		if (0 == LeftToRead)
		{
			DWORD SizeToRead = m_SrcEnd - m_SrcCopyPos;
			if (SizeToRead > 0x10000)
			{
				SizeToRead = 0x10000;
			}
			WasRead = m_SrcFile.m_File.GetDataBuffer( & pOriginalSrcBuf,
													SizeToRead, m_SrcCopyPos, CDirectFile::GetBufferAndPrefetchNext);
			if (0 == WasRead)
			{
				if (0 != WasLockedToWrite)
				{
					m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
													CDirectFile::ReturnBufferDirty);
				}
				return FALSE;
			}
			pSrcBuf = (char *) pOriginalSrcBuf;
			LeftToRead = WasRead;
		}
		if (0 == LeftToWrite)
		{
			DWORD flags = CDirectFile::GetBufferAndPrefetchNext;
			if (2 == m_DstChan)
			{
				flags = CDirectFile::GetBufferWriteOnly;
			}
			DWORD SizeToWrite = m_DstEnd - m_DstCopyPos;
			WasLockedToWrite = m_DstFile.m_File.GetDataBuffer( & pOriginalDstBuf,
																SizeToWrite, m_DstCopyPos, flags);

			if (0 == WasLockedToWrite)
			{
				if (0 != WasRead)
				{
					m_SrcFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
													CDirectFile::ReturnBufferDiscard);
				}
				return FALSE;
			}
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;
		}
		// copy whatever possible
		if ((m_DstFile.Channels() == 1
				&& m_SrcFile.Channels() == 1)
			|| (m_SrcChan == 2
				&& m_DstFile.Channels() == 2
				&& m_DstChan == 2
				&& m_SrcFile.Channels() == 2))
		{
			size_t ToCopy = __min(LeftToRead, LeftToWrite);
			memmove(pDstBuf, pSrcBuf, ToCopy);
			pDstBuf += ToCopy;
			pSrcBuf += ToCopy;
			LeftToRead -= ToCopy;
			LeftToWrite -= ToCopy;
			m_SrcCopyPos += ToCopy;
			m_DstCopyPos += ToCopy;
		}
		else
		{
			// copy one channel
			// sample pair may get across buffer boundary,
			// we need to account this
			__int16 * pSrc = (__int16 *) pSrcBuf;
			__int16 * pDst = (__int16 *) pDstBuf;
			int i;
			if (m_SrcFile.Channels() == 1)
			{
				ASSERT(m_DstFile.Channels() == 2);
				// source is mono, destination is stereo.
				// copy can go either to one channel, or to both
				if (2 == m_DstChan)
				{
					// copy to both channels
					if ((m_DstCopyPos - m_DstStart) & 2)
					{
						// right channel sample left not copied
						pDst[0] = pSrc[0];
						pDst++;
						pSrc++;
						LeftToRead -= 2;
						LeftToWrite -= 2;
						m_DstCopyPos += 2;
						m_SrcCopyPos += 2;
					}
					size_t ToCopy = __min(LeftToRead / sizeof (__int16),
										LeftToWrite / (2 * sizeof(__int16)));
					for (i = 0; i < ToCopy; i++, pSrc++, pDst += 2)
					{
						pDst[0] = pSrc[0];
						pDst[1] = pSrc[0];
					}
					LeftToRead -= ToCopy * sizeof (__int16);
					m_SrcCopyPos += ToCopy * sizeof (__int16);
					LeftToWrite -= ToCopy * (2 * sizeof (__int16));
					m_DstCopyPos += ToCopy * (2 * sizeof (__int16));
					if (2 == LeftToWrite && LeftToRead != 0)
					{
						// only room for left channel sample
						pDst[0] = pSrc[0];
						m_DstCopyPos += 2;
						LeftToWrite -= 2;
						pDst++;
						// rigth channel will be copied later
					}
				}
				else
				{
					// copy to one channel
					// source - one channel,
					// destination - one of two
					ASSERT(m_DstFile.Channels() == 2);
					if (((m_DstCopyPos - m_DstStart) & 2)
						!= m_DstChan * 2)
					{
						// skip this word
						pDst++;
						m_DstCopyPos += 2;
						LeftToWrite -= 2;
					}
					size_t ToCopy = __min(LeftToRead / sizeof (__int16),
										LeftToWrite / (2 * sizeof(__int16)));
					for (i = 0; i < ToCopy; i++, pSrc++, pDst += 2)
					{
						pDst[0] = pSrc[0];
					}
					LeftToRead -= ToCopy * sizeof (__int16);
					m_SrcCopyPos += ToCopy * sizeof (__int16);
					LeftToWrite -= ToCopy * (2 * sizeof (__int16));
					m_DstCopyPos += ToCopy * (2 * sizeof (__int16));
				}
			}
			else if (m_DstFile.Channels() == 1)
			{
				ASSERT(m_SrcFile.Channels() == 2);
				int i;
				// source is stereo, destination is mono.
				// copy can go either from one channel, or from both
				if (2 == m_SrcChan)
				{
					// copy From both channels (sum)
					if ((m_SrcCopyPos - m_SrcStart) & 2)
					{
						// right channel sample left not copied
						// left channel sample was copied before to dst[0]
						pDst[0] = (pDst[0] + pSrc[0]) / 2;
						pDst++;
						pSrc++;
						LeftToRead -= 2;
						LeftToWrite -= 2;
						m_DstCopyPos += 2;
						m_SrcCopyPos += 2;
					}
					size_t ToCopy = __min(LeftToRead / (2 * sizeof(__int16)),
										LeftToWrite / sizeof (__int16));
					for (i = 0; i < ToCopy; i++, pDst++, pSrc += 2)
					{
						pDst[0] = (pSrc[0] + pSrc[1]) / 2;
					}
					LeftToRead -= ToCopy * (2 * sizeof (__int16));
					m_SrcCopyPos += ToCopy * (2 * sizeof (__int16));
					LeftToWrite -= ToCopy * sizeof (__int16);
					m_DstCopyPos += ToCopy * sizeof (__int16);

					if (2 == LeftToRead && LeftToWrite != 0)
					{
						// only left channel sample available
						pDst[0] = pSrc[0];
						m_SrcCopyPos += 2;
						LeftToRead -= 2;
						pSrc++;
						// rigth channel will be added later
					}
				}
				else
				{
					// copy from one channel
					// source - one of two,
					// destination - one channel
					ASSERT(m_SrcFile.Channels() == 2);
					if (((m_SrcCopyPos - m_SrcStart) & 2)
						!= m_SrcChan * 2)
					{
						// skip this word
						pSrc++;
						m_SrcCopyPos += 2;
						LeftToRead -= 2;
					}
					size_t ToCopy = __min(LeftToRead / (2 * sizeof(__int16)),
										LeftToWrite / sizeof (__int16));
					for (i = 0; i < ToCopy; i++, pDst++, pSrc += 2)
					{
						pDst[0] = pSrc[0];
					}
					LeftToRead -= ToCopy * (2 * sizeof (__int16));
					m_SrcCopyPos += ToCopy * (2 * sizeof (__int16));
					LeftToWrite -= ToCopy * sizeof (__int16);
					m_DstCopyPos += ToCopy * sizeof (__int16);
				}
			}
			else
			{
				// both are 2 channel
				if (((m_DstCopyPos - m_DstStart) & 2)
					!= m_DstChan * 2)
				{
					// skip this word
					pDst++;
					m_DstCopyPos += 2;
					LeftToWrite -= 2;
				}
				if (((m_SrcCopyPos - m_SrcStart) & 2)
					!= m_SrcChan * 2)
				{
					// skip this word
					pSrc++;
					m_SrcCopyPos += 2;
					LeftToRead -= 2;
				}
				size_t ToCopy = __min(LeftToRead / (2 * sizeof(__int16)),
									LeftToWrite / (2 * sizeof(__int16)));
				for (i = 0; i < ToCopy; i++, pDst += 2, pSrc += 2)
				{
					pDst[0] = pSrc[0];
				}
				LeftToRead -= ToCopy * (2 * sizeof (__int16));
				m_SrcCopyPos += ToCopy * (2 * sizeof (__int16));
				LeftToWrite -= ToCopy * (2 * sizeof (__int16));
				m_DstCopyPos += ToCopy * (2 * sizeof (__int16));
				if (LeftToRead >= 2 && LeftToWrite >= 2)
				{
					pDst[0] = pSrc[0];
					pDst++;
					pSrc++;
					m_DstCopyPos += 2;
					LeftToWrite -= 2;
					m_SrcCopyPos += 2;
					LeftToRead -= 2;
				}
			}

			pSrcBuf = (char *) pSrc;
			pDstBuf = (char *) pDst;
		}
		ASSERT(pDstBuf + LeftToWrite == WasLockedToWrite + (char*)pOriginalDstBuf);
		ASSERT(pSrcBuf + LeftToRead == WasRead + (char*)pOriginalSrcBuf);
		if (0 == LeftToRead)
		{
			m_SrcFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);
			WasRead = 0;
		}
		if (0 == LeftToWrite)
		{
			m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
											CDirectFile::ReturnBufferDirty);
			WasLockedToWrite = 0;
		}
	}
	while (m_SrcCopyPos < m_SrcEnd
			&& timeGetTime() - dwStartTime < 200
			);
	if (0 != WasRead)
	{
		m_SrcFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
	}
	if (0 != WasLockedToWrite)
	{
		m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
	}
	// notify the view
	int nSampleSize = m_DstFile.SampleSize();
	int nFirstSample = (dwOperationBegin - m_DstFile.GetDataChunk()->dwDataOffset)
						/ nSampleSize;
	int nLastSample = (m_DstCopyPos - m_DstFile.GetDataChunk()->dwDataOffset)
					/ nSampleSize;
	pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample);

	if (m_SrcEnd > m_SrcStart)
	{
		PercentCompleted = 100i64 * (m_SrcCopyPos - m_SrcStart) / (m_SrcEnd - m_SrcStart);
	}
	return TRUE;

}

BOOL CResizeContext::OperationProc()
{
	// change size of the file
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	if (m_Flags & CopyExpandFile)
	{
		return ExpandProc();
	}
	if (m_Flags & CopyShrinkFile)
	{
		return ShrinkProc();
	}
	return FALSE;
}

BOOL CResizeContext::ShrinkProc()
{
	if (m_DstCopyPos >= m_DstEnd)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	DWORD dwStartTime = timeGetTime();
	DWORD dwOperationBegin = m_DstCopyPos;

	DWORD LeftToRead = 0;
	DWORD LeftToWrite = 0;
	DWORD WasRead = 0;
	DWORD WasLockedToWrite = 0;
	void * pOriginalSrcBuf;
	char * pSrcBuf;
	void * pOriginalDstBuf;
	char * pDstBuf;
	DWORD DstFlags = 0;
	if (m_DstChan == 2 // all data is copied
		&& m_SrcCopyPos - m_DstCopyPos >= 0x10000)
	{
		DstFlags = CDirectFile::GetBufferWriteOnly;
	}
	if (m_SrcCopyPos > m_DstEnd
		|| m_DstCopyPos > m_DstEnd)
	{
		return FALSE;
	}
	do
	{
		if (0 == LeftToRead)
		{
			DWORD SizeToRead = m_SrcEnd - m_SrcCopyPos;
			if (SizeToRead > 0x10000)
			{
				SizeToRead = 0x10000;
			}
			WasRead = m_DstFile.m_File.GetDataBuffer( & pOriginalSrcBuf,
													SizeToRead, m_SrcCopyPos, CDirectFile::GetBufferAndPrefetchNext);
			if (0 == WasRead)
			{
				if (0 != WasLockedToWrite)
				{
					m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
													CDirectFile::ReturnBufferDirty);
				}
				return FALSE;
			}
			pSrcBuf = (char *) pOriginalSrcBuf;
			LeftToRead = WasRead;
		}
		if (0 == LeftToWrite)
		{
			DWORD SizeToWrite = m_DstEnd - m_DstCopyPos;
			WasLockedToWrite = m_DstFile.m_File.GetDataBuffer( & pOriginalDstBuf,
																SizeToWrite, m_DstCopyPos, DstFlags);

			if (0 == WasLockedToWrite)
			{
				if (0 != WasRead)
				{
					m_DstFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
													CDirectFile::ReturnBufferDiscard);
				}
				return FALSE;
			}
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;
		}
		// copy whatever possible
		if (m_DstChan == 2)
		{
			size_t ToCopy = __min(LeftToRead, LeftToWrite);
			memmove(pDstBuf, pSrcBuf, ToCopy);
			pDstBuf += ToCopy;
			pSrcBuf += ToCopy;
			LeftToRead -= ToCopy;
			LeftToWrite -= ToCopy;
			m_SrcCopyPos += ToCopy;
			m_DstCopyPos += ToCopy;
		}
		else
		{
			// copy one channel
			// sample pair may get across buffer boundary,
			// we need to account this
			__int16 * pSrc = (__int16 *) pSrcBuf;
			__int16 * pDst = (__int16 *) pDstBuf;
			int i;
			// both are 2 channel
			if (((m_DstCopyPos - m_DstStart) & 2)
				!= m_DstChan * 2)
			{
				// skip this word
				pDst++;
				m_DstCopyPos += 2;
				LeftToWrite -= 2;
			}
			if (((m_SrcCopyPos - m_SrcStart) & 2)
				!= m_DstChan * 2)
			{
				// skip this word
				pSrc++;
				m_SrcCopyPos += 2;
				LeftToRead -= 2;
			}
			size_t ToCopy = __min(LeftToRead / (2 * sizeof(__int16)),
								LeftToWrite / (2 * sizeof(__int16)));
			for (i = 0; i < ToCopy; i++, pDst += 2, pSrc += 2)
			{
				pDst[0] = pSrc[0];
			}
			LeftToRead -= ToCopy * (2 * sizeof (__int16));
			m_SrcCopyPos += ToCopy * (2 * sizeof (__int16));
			LeftToWrite -= ToCopy * (2 * sizeof (__int16));
			m_DstCopyPos += ToCopy * (2 * sizeof (__int16));
			if (LeftToRead >= 2 && LeftToWrite >= 2)
			{
				pDst[0] = pSrc[0];
				pDst++;
				pSrc++;
				m_DstCopyPos += 2;
				LeftToWrite -= 2;
				m_SrcCopyPos += 2;
				LeftToRead -= 2;
			}

			pSrcBuf = (char *) pSrc;
			pDstBuf = (char *) pDst;
		}
		ASSERT(pDstBuf + LeftToWrite == WasLockedToWrite + (char*)pOriginalDstBuf);
		ASSERT(pSrcBuf + LeftToRead == WasRead + (char*)pOriginalSrcBuf);
		if (0 == LeftToWrite)
		{
			m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
											CDirectFile::ReturnBufferDirty);
			WasLockedToWrite = 0;
		}
		if (0 == LeftToRead)
		{
			m_DstFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);
			WasRead = 0;
		}
	}
	while (m_SrcCopyPos < m_SrcEnd
			&& timeGetTime() - dwStartTime < 200
			);
	if (0 != WasRead)
	{
		m_DstFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
	}
	if (0 != WasLockedToWrite)
	{
		m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
	}
	// notify the view
	int nSampleSize = m_DstFile.SampleSize();
	int nFirstSample = (dwOperationBegin - m_DstFile.GetDataChunk()->dwDataOffset)
						/ nSampleSize;
	int nLastSample = (m_DstCopyPos - m_DstFile.GetDataChunk()->dwDataOffset)
					/ nSampleSize;
	pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample);

	if (m_SrcEnd > m_SrcStart)
	{
		PercentCompleted = 100i64 * (m_SrcCopyPos - m_SrcStart) / (m_SrcEnd - m_SrcStart);
	}
	return TRUE;

}

BOOL CResizeContext::ExpandProc()
{
	// copying is done from end backward
	if (m_SrcCopyPos <= m_SrcStart)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	DWORD DstFlags = 0;
	if (m_DstChan == 2 // all data is copied
		&& m_SrcCopyPos - m_DstCopyPos >= 0x10000)
	{
		DstFlags = CDirectFile::GetBufferWriteOnly;
	}

	DWORD dwStartTime = timeGetTime();
	DWORD dwOperationBegin = m_DstCopyPos;

	LONG LeftToRead = 0;
	LONG LeftToWrite = 0;
	LONG WasRead = 0;
	LONG WasLockedToWrite = 0;
	void * pOriginalSrcBuf;
	char * pSrcBuf;
	void * pOriginalDstBuf;
	char * pDstBuf;
	if (m_SrcCopyPos < m_SrcStart
		|| m_DstCopyPos < m_DstStart)
	{
		return FALSE;
	}
	do
	{
		if (0 == LeftToRead)
		{
			LONG SizeToRead = m_SrcStart - m_SrcCopyPos;
			// SizeToRead < 0 - reading backward
			if (SizeToRead < -0x10000)
			{
				SizeToRead = -0x10000;
			}
			WasRead = m_DstFile.m_File.GetDataBuffer( & pOriginalSrcBuf,
													SizeToRead, m_SrcCopyPos, CDirectFile::GetBufferAndPrefetchNext);
			if (0 == WasRead)
			{
				if (0 != WasLockedToWrite)
				{
					m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
													CDirectFile::ReturnBufferDirty);
				}
				return FALSE;
			}
			pSrcBuf = (char *) pOriginalSrcBuf;
			LeftToRead = WasRead;
		}
		if (0 == LeftToWrite)
		{
			LONG SizeToWrite = m_DstStart - m_DstCopyPos;
			WasLockedToWrite = m_DstFile.m_File.GetDataBuffer( & pOriginalDstBuf,
																SizeToWrite, m_DstCopyPos, DstFlags);

			if (0 == WasLockedToWrite)
			{
				if (0 != WasRead)
				{
					m_DstFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
													CDirectFile::ReturnBufferDiscard);
				}
				return FALSE;
			}
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;
		}
		// copy whatever possible
		if (m_DstChan == 2)
		{
			size_t ToCopy = __min(-LeftToRead, -LeftToWrite);
			pDstBuf -= ToCopy;
			pSrcBuf -= ToCopy;
			memmove(pDstBuf, pSrcBuf, ToCopy);
			LeftToRead += ToCopy;
			LeftToWrite += ToCopy;
			m_SrcCopyPos -= ToCopy;
			m_DstCopyPos -= ToCopy;
		}
		else
		{
			// copy one channel
			// sample pair may get across buffer boundary,
			// we need to account this
			__int16 * pSrc = (__int16 *) pSrcBuf;
			__int16 * pDst = (__int16 *) pDstBuf;
			int i;
			// both are 2 channel
			if (((m_DstCopyPos - m_DstStart) & 2)
				!= m_DstChan * 2)
			{
				// skip this word
				pDst++;
				m_DstCopyPos += 2;
				LeftToWrite -= 2;
			}
			if (((m_SrcCopyPos - m_SrcStart) & 2)
				!= m_DstChan * 2)
			{
				// skip this word
				pSrc++;
				m_SrcCopyPos += 2;
				LeftToRead -= 2;
			}
			size_t ToCopy = __min(( -LeftToRead) / (2 * sizeof(__int16)),
								( -LeftToWrite) / (2 * sizeof(__int16)));
			for (i = 0; i < ToCopy; i++)
			{
				pDst -= 2;
				pSrc -= 2;
				pDst[0] = pSrc[0];
			}
			LeftToRead += ToCopy * (2 * sizeof (__int16));
			m_SrcCopyPos -= ToCopy * (2 * sizeof (__int16));
			LeftToWrite += ToCopy * (2 * sizeof (__int16));
			m_DstCopyPos -= ToCopy * (2 * sizeof (__int16));
			if (LeftToRead <= -2 && LeftToWrite <= -2)
			{
				pDst--;
				pSrc--;
				pDst[0] = pSrc[0];
				m_DstCopyPos -= 2;
				LeftToWrite += 2;
				m_SrcCopyPos -= 2;
				LeftToRead += 2;
			}

			pSrcBuf = (char *) pSrc;
			pDstBuf = (char *) pDst;
		}
		ASSERT(pDstBuf + LeftToWrite == WasLockedToWrite + (char*)pOriginalDstBuf);
		ASSERT(pSrcBuf + LeftToRead == WasRead + (char*)pOriginalSrcBuf);
		if (0 == LeftToRead)
		{
			m_DstFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);
			WasRead = 0;
		}
		if (0 == LeftToWrite)
		{
			m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
											CDirectFile::ReturnBufferDirty);
			WasLockedToWrite = 0;
		}
	}
	while (m_SrcCopyPos > m_SrcStart
			&& timeGetTime() - dwStartTime < 200
			);
	if (0 != WasRead)
	{
		m_DstFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
	}
	if (0 != WasLockedToWrite)
	{
		m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
	}
	// notify the view
	int nSampleSize = m_DstFile.SampleSize();
	// operation goes backward
	int nLastSample = (dwOperationBegin - m_DstFile.GetDataChunk()->dwDataOffset)
					/ nSampleSize;
	int nFirstSample = (m_DstCopyPos - m_DstFile.GetDataChunk()->dwDataOffset)
						/ nSampleSize;
	pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample);

	if (m_SrcEnd > m_SrcStart)
	{
		PercentCompleted = 100i64 * (m_SrcEnd - m_SrcCopyPos) / (m_SrcEnd - m_SrcStart);
	}
	return TRUE;

}

class CCutContext : public CCopyContext
{
	friend class CWaveSoapFrontDoc;
public:
	CCutContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString)
		: CCopyContext(pDoc, StatusString)
	{

	}
	~CCutContext() {}
	virtual BOOL OperationProc();
	virtual CString GetStatusString() { return sOp; }
};

void CWaveSoapFrontDoc::DoPaste(LONG Start, LONG End, LONG Channel, LPCTSTR FileName)
{
	// create a operation context
	CWaveFile * pSrcFile = & GetApp()->m_ClipboardFile;
	if ( ! pSrcFile->IsOpen())
	{
		return;
	}

	CCopyContext * pContext = new CCopyContext(this, _T("Inserting data from clipboard..."));

	pContext->m_Flags |= OperationContextClipboard | CopyExpandFile;

	if ( ! pContext->InitCopy(m_WavFile, m_CaretPosition, 0, m_SelectedChannel,
							* pSrcFile, 0, pSrcFile->NumberOfSamples(), 2))
	{
		delete pContext;
	}

	// set operation context to the queue
	SoundChanged(WaveFileID(), 0, 0, WaveFileSamples());
	QueueOperation(pContext);
}

void CWaveSoapFrontDoc::DoCut(LONG Start, LONG End, LONG Channel, LPCTSTR FileName)
{
}

void CWaveSoapFrontDoc::DoDelete(LONG Start, LONG End, LONG Channel)
{
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
	pCmdUI->Enable(m_OperationInProgress);
}

class CDecompressContext : public COperationContext
{
	friend class CWaveSoapFrontDoc;
	CWaveFile m_SrcFile;
	CWaveFile m_DstFile;
	// Start, End and position are in bytes
	DWORD m_SrcStart;
	DWORD m_SrcEnd;
	DWORD m_SrcPos;

	DWORD m_DstStart;
	DWORD m_DstPos;
	DWORD m_DstEnd;

	DWORD m_CurrentSamples;

	HACMSTREAM m_acmStr;
	size_t m_SrcBufSize;
	size_t m_DstBufSize;
	ACMSTREAMHEADER m_ash;
	DWORD m_ConvertFlags;


public:
	CString sOp;
	CDecompressContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString)
		: COperationContext(pDoc,
							// operation can be terminated by Close
							OperationContextDiskIntensive | OperationContextNonCritical),
		sOp(StatusString),
		m_SrcBufSize(0),
		m_DstBufSize(0),
		m_acmStr(NULL)
	{
		memset( & m_ash, 0, sizeof m_ash);
	}
	~CDecompressContext() {}
	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual BOOL DeInit();
	virtual CString GetStatusString()
	{
		return sOp;
	}
};

BOOL CDecompressContext::OperationProc()
{

	DWORD dwOperationBegin = m_DstPos;
	DWORD StartTime = GetTickCount();
	while (GetTickCount() - StartTime < 200)
	{
		// fill the source buffer
		size_t leftToRead = m_SrcEnd - m_SrcPos;
		size_t ToRead = m_SrcBufSize;
		if (ToRead > leftToRead)
		{
			ToRead = leftToRead;
		}
		if (ToRead != 0)
		{
			long read = m_SrcFile.m_File.ReadAt(m_ash.pbSrc,
												ToRead, m_SrcPos);
			m_ash.cbSrcLength = read;
		}
		else
		{
			m_ConvertFlags |= ACM_STREAMCONVERTF_END;
			m_ConvertFlags &= ~ACM_STREAMCONVERTF_BLOCKALIGN;
			m_ash.cbSrcLength = 0;
		}
		// do the conversion
		m_ash.cbSrcLengthUsed = 0;
		m_ash.cbDstLength = m_DstBufSize;
		m_ash.cbDstLengthUsed = 0;
		if (0 == acmStreamConvert(m_acmStr, & m_ash, m_ConvertFlags)
			&& (0 != m_ash.cbDstLengthUsed || 0 != m_ash.cbSrcLengthUsed))
		{
			// write the result
			if (0 != m_ash.cbDstLengthUsed)
			{
				long written = m_DstFile.m_File.WriteAt(m_ash.pbDst,
														m_ash.cbDstLengthUsed, m_DstPos);
				m_DstPos += written;
				if (written != m_ash.cbDstLengthUsed)
				{
					// error
				}
			}
			m_SrcPos += m_ash.cbSrcLengthUsed;
		}
		else
		{
			// error
		}
		m_ConvertFlags &= ~ACM_STREAMCONVERTF_START;
		if (0 == m_ash.cbSrcLengthUsed
			&& 0 == m_ash.cbDstLengthUsed)
		{
			m_Flags |= OperationContextFinished;
			m_CurrentSamples = 0;   // to force update file length
			break;
		}
	}
	// notify the view

	int nSampleSize = m_DstFile.SampleSize();
	int TotalSamples = -1;
	int nFirstSample = (dwOperationBegin - m_DstFile.GetDataChunk()->dwDataOffset)
						/ nSampleSize;
	int nLastSample = (m_DstPos - m_DstFile.GetDataChunk()->dwDataOffset)
					/ nSampleSize;
	// check if we need to change file size
	if (nLastSample > m_CurrentSamples)
	{
		// calculate new length
		TotalSamples = MulDiv(nLastSample, m_SrcEnd - m_SrcStart, m_SrcPos - m_SrcStart);
		LPMMCKINFO pck = m_DstFile.GetDataChunk();
		if (TotalSamples > 0x7FFFFFFF / nSampleSize)
		{
			TotalSamples = 0x7FFFFFFF / nSampleSize;
		}
		DWORD datasize = TotalSamples * nSampleSize;
		m_DstFile.m_File.SetFileLength(datasize + pck->dwDataOffset);
		m_CurrentSamples = TotalSamples;
		// update data chunk length
		pck->cksize = m_DstPos - m_DstStart;
		pDocument->AllocatePeakData(nLastSample);
	}
	pDocument->SoundChanged(m_DstFile.GetFileID(), nFirstSample, nLastSample, TotalSamples);

	if (m_SrcEnd > m_SrcStart)
	{
		PercentCompleted = 100i64 * (m_SrcPos - m_SrcStart) / (m_SrcEnd - m_SrcStart);
	}
	return TRUE;
}

BOOL CDecompressContext::Init()
{
	// Open codec, allocate buffers, ets
	WAVEFORMATEX * pSrcFormat = m_SrcFile.GetWaveFormat();
	WAVEFORMATEX wf =
	{
		WAVE_FORMAT_PCM,
		pSrcFormat->nChannels,
		0,  // nSamplesPerSec
		0,  // nAvgBytesPerSec
		0, // nBlockAlign
		16, // bits per sample
		0   // cbSize
	};
	acmFormatSuggest(NULL, pSrcFormat, & wf, sizeof wf,
					ACM_FORMATSUGGESTF_NCHANNELS
					| ACM_FORMATSUGGESTF_WBITSPERSAMPLE
					| ACM_FORMATSUGGESTF_WFORMATTAG);
	TRACE("acmFormatSuggest:nSamplesPerSec=%d, BytesPerSec=%d, nBlockAlign=%d\n",
		wf.nSamplesPerSec, wf.nAvgBytesPerSec, wf.nBlockAlign);

	m_ash.cbSrcLength = 0;
	m_ash.cbDstLength = 0x10000;  // 64K

	if (0 != acmStreamOpen( & m_acmStr, NULL, pSrcFormat, & wf, NULL, NULL, NULL,
							ACM_STREAMOPENF_NONREALTIME)
		|| 0 != acmStreamSize(m_acmStr, m_ash.cbDstLength, & m_ash.cbSrcLength,
							ACM_STREAMSIZEF_DESTINATION)
		|| 0 != acmStreamSize(m_acmStr, m_ash.cbSrcLength, & m_ash.cbDstLength,
							ACM_STREAMSIZEF_SOURCE))
	{
		if (m_acmStr != NULL)
		{
			acmStreamClose(m_acmStr, 0);
			m_acmStr = NULL;
		}
		return FALSE;
	}
	// allocate buffers
	m_SrcBufSize = m_ash.cbSrcLength;
	m_DstBufSize = m_ash.cbDstLength;
	m_ash.pbSrc = new BYTE[m_SrcBufSize];
	m_ash.pbDst = new BYTE[m_DstBufSize];

	m_ash.cbStruct = sizeof m_ash;
	if (NULL == m_ash.pbSrc
		|| NULL == m_ash.pbDst)
	{
		delete m_ash.pbSrc;
		m_ash.pbSrc = NULL;
		delete m_ash.pbDst;
		m_ash.pbDst = NULL;
		acmStreamClose(m_acmStr, 0);
		m_acmStr = NULL;
		return FALSE;
	}
	// prepare the buffer
	acmStreamPrepareHeader(m_acmStr, & m_ash, 0);
	m_ConvertFlags = ACM_STREAMCONVERTF_START;
	return TRUE;
}

BOOL CDecompressContext::DeInit()
{
	if (m_acmStr)
	{
		m_ash.cbDstLength = m_DstBufSize;
		m_ash.cbSrcLength = m_DstBufSize;
		acmStreamUnprepareHeader(m_acmStr, & m_ash, 0);
		delete[] m_ash.pbDst;
		delete[] m_ash.pbSrc;
		m_ash.pbDst = NULL;
		m_ash.pbSrc = NULL;
		acmStreamClose(m_acmStr, 0);
		m_acmStr = NULL;
	}
	return TRUE;
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
	m_bDirectMode = pApp->m_bDirectMode;
	m_bReadOnly = pApp->m_bReadOnly;
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
					m_WavFile.GetFileInformationByHandle(& WavFileInfo);
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
}


void CWaveSoapFrontDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(IsModified() && ! m_bReadOnly);
}

void CWaveSoapFrontDoc::SetModifiedFlag(BOOL bModified)
{
	BOOL OldModified = m_bModified;
	CDocument::SetModifiedFlag(bModified);
	if ((bModified != 0) != (OldModified != 0))
	{
		UpdateFrameCounts();        // will cause name change in views
	}
}

void CWaveSoapFrontDoc::OnEditSelectAll()
{
	long len = WaveFileSamples();
	SetSelection(0, len, 2, len);
}

void CWaveSoapFrontDoc::OnEditSelection()
{

}

void CWaveSoapFrontDoc::OnUpdateEditSelection(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here

}

class CSoundPlayContext : public COperationContext
{
public:
	CWaveOut m_WaveOut;
	long m_Begin;
	long m_End;
	long m_CurrentPlaybackPos;
	int m_Chan;
	int m_PlaybackDevice;
	int m_OldThreadPriority;
	CString m_ss;

public:
	CSoundPlayContext(class CWaveSoapFrontDoc * pDoc)
		: COperationContext(pDoc, 0), m_ss(_T("Playing..."))
	{
	}
	virtual ~CSoundPlayContext() {}
	virtual BOOL OperationProc();
	virtual BOOL Init();
	virtual BOOL DeInit();
	virtual CString GetStatusString() { return m_ss; }
};

BOOL CSoundPlayContext::Init()
{
	m_OldThreadPriority = GetThreadPriority(GetCurrentThread());
	MMRESULT mmres = m_WaveOut.Open(m_PlaybackDevice, pDocument->WaveFormat(), 0);
	if (MMSYSERR_NOERROR != mmres)
	{
		// TODO:
		// notify document
		return FALSE;
	}
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	return TRUE;
}

BOOL CSoundPlayContext::DeInit()
{
	SetThreadPriority(GetCurrentThread(), m_OldThreadPriority);
	m_WaveOut.Reset();
	return TRUE;
}

BOOL CSoundPlayContext::OperationProc()
{
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}
	if (m_CurrentPlaybackPos >= m_End)
	{
		// wait until all the buffer is finished
		if (FALSE == m_WaveOut.WaitForQueueEmpty(50))
		{
			return TRUE;    // not finished yet
		}
		m_Flags |= OperationContextFinished;
		PercentCompleted = 100;
		return TRUE;
	}
	char * pBuf;
	size_t size;
	int nBuf = m_WaveOut.GetBuffer( & pBuf, & size, FALSE);
	if (nBuf <= 0)
	{
		// buffer not available yet
		Sleep(50);
	}
	else
	{
		// fill the buffer
		int TotalInBuf = 0;
		while (size != 0 && m_CurrentPlaybackPos < m_End)
		{
			size_t len = m_End - m_CurrentPlaybackPos;
			if (len > size) len = size;
			void * pFileBuf = NULL;
			long lRead = pDocument->m_WavFile.m_File.GetDataBuffer( & pFileBuf,
							len, m_CurrentPlaybackPos, CDirectFile::GetBufferAndPrefetchNext);
			if (lRead <= 0)
			{
				m_WaveOut.ReturnBuffer(nBuf);
				return FALSE;
			}
			memcpy(pBuf, pFileBuf, lRead);
			pDocument->m_WavFile.m_File.ReturnDataBuffer(pFileBuf,
														lRead, CDirectFile::ReturnBufferDiscard);
			m_CurrentPlaybackPos += lRead;
			pBuf += lRead;
			size -= lRead;
			TotalInBuf += lRead;
		}
		m_WaveOut.Play(nBuf, TotalInBuf, 0);


	}
	// notify the document

	return TRUE;
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
	QueueOperation(pContext);
}

void CWaveSoapFrontDoc::OnUpdateSoundStop(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here

}

void CWaveSoapFrontDoc::OnSoundStop()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnSoundPause()
{
	// TODO: Add your command handler code here

}

void CWaveSoapFrontDoc::OnUpdateSoundPause(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here

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
