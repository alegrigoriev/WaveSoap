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
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
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
	m_SelectedChannel(2)
{
	// TODO: add one-time construction code here
	TRACE("CWaveSoapFrontDoc::CWaveSoapFrontDoc()\n");
	memset(& WavFileInfo, 0, sizeof WavFileInfo);

}

CWaveSoapFrontDoc::~CWaveSoapFrontDoc()
{
	TRACE("CWaveSoapFrontDoc::~CWaveSoapFrontDoc()\n");
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
		szWaveTitle = fdlg.GetFileTitle();
		if (FALSE == OpenWaveFile())
		{
			return FALSE;
		}
		SetTitle(szWaveTitle);
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

	m_WavFile.Seek(m_WavFile.m_datack.dwDataOffset);
	DWORD CurrPos = 0;
	// wave file peak granule is smaller than the buffer
	int PeakIndex = 0;
	while (CurrPos != m_WavFile.m_datack.cksize)
	{
		DWORD SizeToRead = m_WavFile.m_datack.cksize - CurrPos;
		if (SizeToRead > BufSize)
		{
			SizeToRead = BufSize;
		}
		if (SizeToRead != m_WavFile.Read(m_pWaveBuffer, SizeToRead))
		{
			break;
		}

		DWORD GranuleSize =
			m_WavFile.m_pWf->nChannels * m_PeakDataGranularity * sizeof(__int16);
		DWORD DataToProcess = SizeToRead;
		__int16 * pWaveData = m_pWaveBuffer;
		if (2 == m_WavFile.m_pWf->nChannels)
		{
			while (0 != DataToProcess)
			{
				int wpl_l = 0x7FFF;
				int wpl_h = -0x8000;
				int wpr_l = 0x7FFF;
				int wpr_h = -0x8000;
				DWORD DataForGranule = GranuleSize;
				if (DataForGranule > DataToProcess)
				{
					DataForGranule = DataToProcess;
				}
				for (i = 0; i < DataForGranule / sizeof(__int16); i+=2)
				{
					if (pWaveData[i] < wpl_l)
					{
						wpl_l = pWaveData[i];
					}
					if (pWaveData[i] > wpl_h)
					{
						wpl_h = pWaveData[i];
					}
					if (pWaveData[i + 1] < wpr_l)
					{
						wpr_l = pWaveData[i + 1];
					}
					if (pWaveData[i + 1] > wpr_h)
					{
						wpr_h = pWaveData[i + 1];
					}
				}
				pWaveData += DataForGranule / sizeof(__int16);
				ASSERT(PeakIndex < m_WavePeakSize);
				m_pPeaks[PeakIndex].low = wpl_l;
				m_pPeaks[PeakIndex].high = wpl_h;
				m_pPeaks[PeakIndex + 1].low = wpr_l;
				m_pPeaks[PeakIndex + 1].high = wpr_h;
				PeakIndex+=2;
				DataToProcess -= DataForGranule;
			}
		}
		else
		{
			while (0 != DataToProcess)
			{
				int wpl = 0x7FFF;
				int wph = -0x8000;
				DWORD DataForGranule = GranuleSize;
				if (DataForGranule > DataToProcess)
				{
					DataForGranule = DataToProcess;
				}
				for (i = 0; i < DataForGranule / sizeof(__int16); i++)
				{
					if (pWaveData[i] < wpl)
					{
						wpl = pWaveData[i];
					}
					if (pWaveData[i] > wph)
					{
						wph = pWaveData[i];
					}
				}
				pWaveData += DataForGranule / sizeof(__int16);
				ASSERT(PeakIndex < m_WavePeakSize);
				m_pPeaks[PeakIndex].low = wpl;
				m_pPeaks[PeakIndex].high = wph;
				PeakIndex++;
				DataToProcess -= DataForGranule;
			}
		}

		CurrPos += SizeToRead;
	}
	ASSERT(PeakIndex == m_WavePeakSize);
	SavePeakInfo();
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
	if (m_WavFile.Open(szWaveFilename, 0)
		&& m_WavFile.LoadWaveformat()
		&& m_WavFile.FindData())
	{
		m_SizeOfWaveData = m_WavFile.m_datack.cksize;
		GetFileInformationByHandle(m_WavFile.m_hFile, & WavFileInfo);
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
