// WaveSoapFrontDoc.cpp : implementation of the CWaveSoapFrontDoc class
//

#include "stdafx.h"
#include "WaveSoapFront.h"

#include "WaveSoapFrontDoc.h"

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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontDoc construction/destruction

CWaveSoapFrontDoc::CWaveSoapFrontDoc()
	:m_pPeaks(NULL), m_WavePeakSize(0), m_PeakDataGranularity(512),
	m_pWaveBuffer(NULL), m_WaveBufferSize(0),
	m_CaretPosition(0),
	m_TimeSelectionMode(TRUE),
	m_SelectionStart(0),
	m_SelectionEnd(0),
	m_hThreadEvent(NULL),
	m_bRunThread(false),
	m_bReadOnly(true),
	m_pCurrentContext(NULL),
	m_pQueuedOperation(NULL),
	m_pUndoList(NULL),
	m_pRedoList(NULL),
	m_OperationInProgress(false),
	m_Thread(ThreadProc, this),
	m_bUndoAvailable(false),
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

	if (m_pWaveBuffer)
	{
		delete[] m_pWaveBuffer;
		m_pWaveBuffer = NULL;
	}
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
	// Query for WAV filename and build peaks
	CString filter;
	filter.LoadString(IDS_WAV_FILTER);

	m_CaretPosition = 0;
	m_SelectionStart = 0;
	m_SelectionEnd = 0;
	m_SelectedChannel = 2;  // both channels
	m_TimeSelectionMode = TRUE;

	// allow multiple selection for batch processing
	CFileDialog fdlg(TRUE, "wav", szWaveFilename,
					OFN_EXPLORER
					| OFN_FILEMUSTEXIST
					| OFN_ENABLESIZING
					| OFN_HIDEREADONLY,
					filter);
	fdlg.m_ofn.lpstrInitialDir = _T(".");

	if (fdlg.DoModal() == IDOK)
	{
		szWaveFilename = fdlg.GetPathName();
		CString szWaveTitle = fdlg.GetFileTitle();
		if (FALSE == OpenWaveFile())
		{
			return FALSE;
		}
		// create peak file name
		// remove WAV extension and add ".wspk" extension
		// we need the full pathname.
		szPeakFilename = szWaveFilename;
		if (0 == szPeakFilename.Right(4).CompareNoCase(_T(".WAV")))
		{
			szPeakFilename.Delete(szPeakFilename.GetLength() - 4, 4);
		}
		szPeakFilename += _T(".wspk");
		LoadPeakFile();
	}
	else
	{
		return FALSE;
	}
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
			&& 0 == memcmp(& pfh.wfFormat, m_WavFile.m_pWf, sizeof pfh.wfFormat)
			&& pfh.PeakInfoSize
			== CalculatePeakInfoSize() * sizeof (WavePeak)
			)
		{
			// allocate data and read it
			delete[] m_pPeaks;
			m_pPeaks = NULL;
			m_WavePeakSize = pfh.PeakInfoSize / sizeof (WavePeak);
			m_pPeaks = new WavePeak[m_WavePeakSize];
			if (NULL == m_pPeaks)
			{
				TRACE("Unable to allocate peak info buffer\n");
				m_WavePeakSize = 0;
				return;
			}

			if (pfh.PeakInfoSize == PeakFile.Read(m_pPeaks, pfh.PeakInfoSize))
			{
				return;
			}
			TRACE("Unable to read peak data\n");
			delete[] m_pPeaks;
			m_pPeaks = NULL;
			m_WavePeakSize = 0;
			// rebuild the info from the WAV file
		}
		PeakFile.Close();
	}
	BuildPeakInfo();
}

void CWaveSoapFrontDoc::BuildPeakInfo()
{
	// read the DATA chunk of wavefile
	const int BufSize = 0x10000;
	if ( ! AllocateWaveBuffer(BufSize))    // 64K
	{
		return;
	}
	size_t RequiredPeakInfoSize = CalculatePeakInfoSize();
	if (m_WavePeakSize < RequiredPeakInfoSize)
	{
		delete[] m_pPeaks;
		m_pPeaks = NULL;
		m_WavePeakSize = 0;
	}
	if (NULL == m_pPeaks)
	{
		m_pPeaks = new WavePeak[CalculatePeakInfoSize()];
		if (NULL == m_pPeaks)
		{
			return;
		}
		m_WavePeakSize = CalculatePeakInfoSize();
	}
	int i;
	for (i = 0; i < m_WavePeakSize; i++)
	{
		m_pPeaks[i].low = 0x7FFF;
		m_pPeaks[i].high = -0x8000;
	}

	CScanPeaksContext * pContext = new CScanPeaksContext(this);
	if (NULL == pContext)
	{
		return;
	}
	pContext->m_Start = m_WavFile.m_datack.dwDataOffset;
	pContext->m_Position = pContext->m_Start;
	pContext->m_End = pContext->m_Start + m_WavFile.m_datack.cksize;
	pContext->m_GranuleSize =
		m_WavFile.m_pWf->nChannels * m_PeakDataGranularity * sizeof(__int16);
	if (pContext->m_End <= pContext->m_Start)
	{
		delete pContext;
		return;
	}
	QueueOperation(pContext);
}

BOOL CScanPeaksContext::OperationProc()
{
	// wave file peak granule is smaller than the buffer
	if (m_Position < m_End)
	{
		DWORD dwStartTime = timeGetTime();
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
				DWORD DataOffset = m_Position - pDocument->m_WavFile.m_datack.dwDataOffset;
				unsigned DataForGranule = m_GranuleSize - DataOffset % m_GranuleSize;

				if (2 == pDocument->m_WavFile.m_pWf->nChannels)
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
				return FALSE;
			}
		}
		while (m_Position < m_End
				&& timeGetTime() - dwStartTime < 200);
		if (m_End > m_Start)
		{
			PercentCompleted = 100i64 * (m_Position - m_Start) / (m_End - m_Start);
		}
		return TRUE;
	}
	else
	{
		pDocument->SavePeakInfo();
		Flags |= OperationContextFinished;
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
		pfh.dwWaveFileSize = m_WavFile.m_dwSize;
		pfh.Granularity = m_PeakDataGranularity;
		pfh.PeakInfoSize = m_WavePeakSize * sizeof (WavePeak);
		pfh.WaveFileTime = WavFileInfo.ftLastWriteTime;
		pfh.wfFormat = *(LPWAVEFORMATEX) m_WavFile.m_pWf;

		PeakFile.Write( & pfh, sizeof pfh);
		PeakFile.Write(m_pPeaks, pfh.PeakInfoSize);

		PeakFile.Close();
	}
}

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

BOOL CWaveSoapFrontDoc::OpenWaveFile()
{
	if (m_WavFile.Open(szWaveFilename, MmioFileOpenReadOnly)
		&& m_WavFile.LoadWaveformat()
		&& m_WavFile.FindData())
	{
		m_SizeOfWaveData = m_WavFile.m_datack.cksize;
		m_WavFile.GetFileInformationByHandle(& WavFileInfo);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontDoc serialization

void CWaveSoapFrontDoc::Serialize(CArchive& ar)
{
	TRACE("CWaveSoapFrontDoc::Serialize\n");
	if (ar.IsStoring())
	{
		// TODO: add storing code here
		ar << szWaveFilename;
		ar << szPeakFilename;
	}
	else
	{
		// TODO: add loading code here
		ar >> szWaveFilename;
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

void CWaveSoapFrontDoc::SetSelection(int begin, int end, int channel, int caret)
{
	CSelectionUpdateInfo ui;
	ui.SelBegin = m_SelectionStart;
	m_SelectionStart = begin;
	ui.SelEnd = m_SelectionEnd;
	m_SelectionEnd = end;
	ui.SelChannel = m_SelectedChannel;
	m_SelectedChannel = channel;
	ui.CaretPos = m_CaretPosition;
	m_CaretPosition = caret;
	UpdateAllViews(NULL, UpdateSelectionChanged, & ui);
}

void CWaveSoapFrontDoc::SoundChanged(int begin, int end)
{
	CSoundUpdateInfo ui;
	ui.Begin = begin;
	ui.End = end;
	ui.Length = 0;
	UpdateAllViews(NULL, UpdateSoundChanged, & ui);
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
}

void CWaveSoapFrontDoc::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! (m_OperationInProgress || m_bReadOnly));
}

void CWaveSoapFrontDoc::OnEditUndo()
{
	if (m_OperationInProgress)
	{
		return;
	}
}

void CWaveSoapFrontDoc::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( m_bUndoAvailable && ! (m_OperationInProgress || m_bReadOnly));
}

UINT CWaveSoapFrontDoc::_ThreadProc(void)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	while (m_bRunThread)
	{
		if (m_pCurrentContext)
		{
			while (0 == (m_pCurrentContext->Flags & OperationContextFinished))
			{
				if (! m_pCurrentContext->OperationProc())
				{
					break;
				}
			}

		}
		WaitForSingleObject(m_hThreadEvent, INFINITE);
	}
	return 0;
}

class CCopyContext : public COperationContext
{
	enum {
		CopyExpandFile = 0x80000000,
		CopyShrinkFile = 0x40000000,
	};
	friend class CWaveSoapFrontDoc;
	CWaveFile m_SrcFile;
	CWaveFile m_DstFile;
	// Start, End and position are in 'samples', not in bytes
	DWORD m_SrcStart;
	DWORD m_SrcEnd;
	int m_SrcChan;
	DWORD m_SrcCopyPos;

	DWORD m_DstStart;
	DWORD m_DstCopyPos;
	DWORD m_DstEnd;
	int m_DstChan;

public:
	CString sOp;
	CCopyContext(CWaveSoapFrontDoc * pDoc, LPCTSTR StatusString)
		: COperationContext(pDoc, OperationContextDiskIntensive),
		sOp(StatusString)
	{

	}
	~CCopyContext();
	BOOL InitCopy(CWaveFile & DstFile,
				LONG DstStartSample, LONG DstLength, LONG DstChannel,
				CWaveFile & SrcFile,
				LONG SrcStartSample, LONG SrcLength, LONG SrcChannel
				);
	virtual BOOL OperationProc();
	virtual CString GetStatusString() { return sOp; }
};

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
	m_OperationInProgress = TRUE;
	if (pContext->Flags & (OperationContextDiskIntensive | OperationContextClipboard))
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
	m_DstFile.LoadWaveformat();
	m_DstFile.FindData();
	m_SrcFile = SrcFile;
	m_SrcFile.LoadWaveformat();
	m_SrcFile.FindData();

	m_SrcChan = SrcChannel;

	DWORD SrcSampleSize = m_SrcFile.Channels() *
						m_SrcFile.m_pWf->wBitsPerSample / 8;
	m_SrcStart = m_SrcFile.m_datack.dwDataOffset +
				SrcStartSample * SrcSampleSize;
	m_SrcCopyPos = m_SrcStart;
	m_SrcEnd = m_SrcFile.m_datack.dwDataOffset +
				(SrcStartSample + SrcLength) * SrcSampleSize;

	DWORD DstSampleSize = m_DstFile.Channels() *
						m_DstFile.m_pWf->wBitsPerSample / 8;

	m_DstCopyPos = m_DstFile.m_datack.dwDataOffset +
					DstStartSample * DstSampleSize;

	m_DstEnd = m_DstFile.m_datack.dwDataOffset +
				(DstStartSample + DstLength) * DstSampleSize;
	m_DstChan = DstChannel;
	if (SrcLength > DstLength)
	{
		Flags |= CopyExpandFile;
	}
	else if (SrcLength < DstLength)
	{
		Flags |= CopyShrinkFile;
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
	pContext->Flags |= OperationContextClipboard;

	pContext->InitCopy(DstFile, 0, m_SelectionEnd - m_SelectionStart, 2,
						m_WavFile, m_SelectionStart, m_SelectionEnd - m_SelectionStart,
						m_SelectedChannel);

	GetApp()->m_ClipboardFile = DstFile;
	// set operation context to the queue
	QueueOperation(pContext);
}

CCopyContext::~CCopyContext()
{
}

BOOL CCopyContext::OperationProc()
{
	// get buffers from source file and copy them to m_CopyFile
	if (m_SrcCopyPos >= m_SrcEnd)
	{
		Flags |= OperationContextFinished;
		return TRUE;
	}
	if (Flags & (CopyExpandFile | CopyShrinkFile))
	{
		return FALSE;   // not implemented yet
	}
	DWORD dwStartTime = timeGetTime();

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
			DWORD SizeToWrite = m_DstEnd - m_DstCopyPos;
			WasLockedToWrite = m_DstFile.m_File.GetDataBuffer( & pOriginalDstBuf,
																SizeToWrite, m_DstCopyPos, CDirectFile::GetBufferWriteOnly);

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

		}
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
	if (m_SrcEnd > m_SrcStart)
	{
		PercentCompleted = 100i64 * (m_SrcCopyPos - m_SrcStart) / (m_SrcEnd - m_SrcStart);
	}
	return TRUE;

}

void CWaveSoapFrontDoc::DoPaste(LONG Start, LONG End, LONG Channel, LPCTSTR FileName)
{
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

BOOL CWaveSoapFrontDoc::OnOpenDocument(LPCTSTR lpszPathName)
{

	// TODO: Add your specialized creation code here
	TRACE("CWaveSoapFrontDoc::OnOpenDocument(%s)\n", lpszPathName);

	m_CaretPosition = 0;
	m_SelectionStart = 0;
	m_SelectionEnd = 0;
	m_SelectedChannel = 2;  // both channels
	m_TimeSelectionMode = TRUE;


	szWaveFilename = lpszPathName;
	if (FALSE == OpenWaveFile())
	{
		return FALSE;
	}
	// create peak file name
	// remove WAV extension and add ".wspk" extension
	// we need the full pathname.
	SetModifiedFlag(FALSE);     // start off with unmodified
	szPeakFilename = szWaveFilename;
	if (0 == szPeakFilename.Right(4).CompareNoCase(_T(".WAV")))
	{
		szPeakFilename.Delete(szPeakFilename.GetLength() - 4, 4);
	}
	szPeakFilename += _T(".wspk");
	LoadPeakFile();
	return TRUE;
}

BOOL CWaveSoapFrontDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	// TODO: Add your specialized code here and/or call the base class

	return TRUE;
}
