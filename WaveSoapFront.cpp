// WaveSoapFront.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WaveSoapFront.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "WaveSoapFrontDoc.h"
#include "WaveSoapFrontView.h"
#include "WaveFftView.h"
#include "ShelLink.h"
#include <float.h>
#include <afxpriv.h>
#include <mmreg.h>
#include <msacm.h>
#define _countof(array) (sizeof(array)/sizeof(array[0]))

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontApp
class CWaveSoapDocTemplate : public CMultiDocTemplate
{
public:
	CWaveSoapDocTemplate( UINT nIDResource, CRuntimeClass* pDocClass,
						CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass )
		:CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass)
	{
	}
	~CWaveSoapDocTemplate() {}
	virtual CDocument* OpenDocumentFile( LPCTSTR lpszPathName, BOOL bMakeVisible = TRUE );

};

class CWaveSoapDocManager : public CDocManager
{
public:
	CWaveSoapDocManager()
		:m_bReadOnly(FALSE),
		m_bDirectMode(FALSE)
	{}
	~CWaveSoapDocManager() {}
	virtual void OnFileOpen();
	BOOL m_bReadOnly;
	BOOL m_bDirectMode;
};

class CWaveSoapFileDialog : public CFileDialog
{
public:
	CWaveSoapFileDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
						LPCTSTR lpszDefExt = NULL,
						LPCTSTR lpszFileName = NULL,
						DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						LPCTSTR lpszFilter = NULL,
						CWnd* pParentWnd = NULL)
		: CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags,
					lpszFilter, pParentWnd), m_bReadOnly(false), m_bDirectMode(false)
	{}
	~CWaveSoapFileDialog() {}

	CWaveFile m_WaveFile;
	CString GetNextPathName(POSITION& pos) const;

	bool m_bReadOnly;
	bool m_bDirectMode;

	int nChannels;
	int nBitsPerSample;
	int nSamplingRate;
	int nSamples;
	CString WaveFormat;

	virtual BOOL OnFileNameOK();
	//virtual void OnLBSelChangedNotify(UINT nIDBox, UINT iCurSel, UINT nCode);

	virtual void OnInitDone();
	virtual void OnFileNameChange();
	virtual void OnFolderChange();
	virtual void OnTypeChange();
	void ClearFileInfoDisplay();

	//{{AFX_MSG(CWaveSoapFileDialog)
	afx_msg void OnCheckReadOnly();
	afx_msg void OnCheckDirectMode();
	afx_msg void OnComboClosed();
	// NOTE - the ClassWizard will add and remove member functions here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
BEGIN_MESSAGE_MAP(CWaveSoapFileDialog, CFileDialog)
	//{{AFX_MSG_MAP(CWaveSoapFileDialog)
	ON_BN_CLICKED(IDC_CHECK_READONLY, OnCheckReadOnly)
	ON_BN_CLICKED(IDC_CHECK_DIRECT, OnCheckDirectMode)
	ON_CBN_CLOSEUP(IDC_COMBO_RECENT, OnComboClosed)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CWaveSoapFrontStatusBar, CStatusBar)
	//{{AFX_MSG_MAP(CWaveSoapFrontStatusBar)
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CWaveSoapFrontApp, CWinApp)
	//{{AFX_MSG_MAP(CWaveSoapFrontApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_EDIT_PASTE_NEW, OnEditPasteNew)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE_NEW, OnUpdateEditPasteNew)
	//}}AFX_MSG_MAP
	// if no documents, Paste will create a new file
	ON_COMMAND(ID_EDIT_PASTE, OnEditPasteNew)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPasteNew)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontApp construction

CWaveSoapFrontApp::CWaveSoapFrontApp()
	: m_FileCache(NULL),
	m_pFirstOp(NULL),
	m_pLastOp(NULL),
	m_Thread(ThreadProc, this),
	m_RunThread(false),
	m_DefaultPlaybackDevice(WAVE_MAPPER),
	m_NumPlaybackBuffers(4),
	m_SizePlaybackBuffers(0x10000),
	m_bReadOnly(false),
	m_bDirectMode(false),
	m_TimeSeparator(':'),
	m_DecimalPoint('.'),
	m_ThousandSeparator(','),
	m_bUseCountrySpecificNumberAndTime(false),
	m_bUndoEnabled(true),
	m_bRedoEnabled(true),
	m_VolumeDialogDbPercents(0),    // dB, 1 - percents
	m_dVolumeLeftDb(0.),
	m_dVolumeRightDb(0.),
	m_dVolumeLeftPercent(100.),
	m_dVolumeRightPercent(100.),
	m_SoundTimeFormat(SampleToString_HhMmSs | TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs),

	m_b5SecondsDC(TRUE),
	m_nDcOffset(0),
	m_DcSelectMode(0),
	m_pActiveDocument(NULL)
{
	// Place all significant initialization in InitInstance
	m_Thread.m_bAutoDelete = FALSE;
	m_pDocManager = new CWaveSoapDocManager;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWaveSoapFrontApp object

CWaveSoapFrontApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontApp initialization

BOOL CWaveSoapFrontApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("AleGr SoftWare"));

	Profile.AddItem(_T("Settings"), _T("CurrentDir"), m_CurrentDir, _T("."));
	if (m_CurrentDir != ".")
	{
		SetCurrentDirectory(m_CurrentDir);
	}

	Profile.AddItem(_T("Settings\\Colors"), _T("WaveBackground"), m_WaveBackground,
					RGB(0xFF, 0xFF, 0xFF));
	Profile.AddItem(_T("Settings\\Colors"), _T("SelectedWaveBackground"), m_SelectedWaveBackground,
					RGB(0x00, 0x00, 0x00));
	Profile.AddItem(_T("Settings\\Colors"), _T("WaveColor"), m_WaveColor,
					RGB(0x00, 72, 0x00));
	Profile.AddItem(_T("Settings\\Colors"), _T("SelectedWaveColor"), m_SelectedWaveColor,
					RGB(0x00, 192, 0x00));
	Profile.AddItem(_T("Settings\\Colors"), _T("ZeroLineColor"), m_ZeroLineColor,
					RGB(0x00, 0x00, 240));
	Profile.AddItem(_T("Settings\\Colors"), _T("SelectedZeroLineColor"), m_SelectedZeroLineColor,
					RGB(0x00, 0x00, 240));
	Profile.AddItem(_T("Settings\\Colors"), _T("6dBLineColor"), m_6dBLineColor,
					RGB(192, 192, 192));
	Profile.AddItem(_T("Settings\\Colors"), _T("Selected6dBLineColor"), m_Selected6dBLineColor,
					RGB(192, 192, 192));
	Profile.AddItem(_T("Settings\\Colors"), _T("ChannelSeparatorColor"), m_ChannelSeparatorColor,
					RGB(0x00, 0x00, 0x00));
	Profile.AddItem(_T("Settings\\Colors"), _T("SelectedChannelSeparatorColor"), m_SelectedChannelSeparatorColor,
					RGB(0x00, 0x00, 0x00));
	Profile.AddItem(_T("Settings\\Colors"), _T("InterpolatedColor"), m_InterpolatedColor,
					RGB(72, 0x00, 0x00));
	Profile.AddItem(_T("Settings\\Colors"), _T("SelectedInterpolatedColor"), m_SelectedInterpolatedColor,
					RGB(192, 0x00, 0x00));

	Profile.AddItem(_T("Settings\\Colors"), _T("MarkerColor"), m_MarkerColor,
					RGB(192, 0x00, 0x00));
	Profile.AddItem(_T("Settings\\Colors"), _T("SelectedMarkerColor"), m_SelectedMarkerColor,
					RGB(72, 0x00, 0x00));
	Profile.AddItem(_T("Settings\\Colors"), _T("RegionColor"), m_RegionColor,
					RGB(0, 0x00, 0x00));
	Profile.AddItem(_T("Settings\\Colors"), _T("SelectedRegionColor"), m_SelectedRegionColor,
					RGB(192, 192, 192));

	Profile.AddBoolItem(_T("Settings"), _T("UseCountrySpecificNumberAndTime"), m_bUseCountrySpecificNumberAndTime,
						FALSE);

	Profile.AddBoolItem(_T("Settings"), _T("OpenAsReadOnly"), m_bReadOnly, FALSE);
	Profile.AddBoolItem(_T("Settings"), _T("OpenInDirectMode"), m_bDirectMode, FALSE);

	Profile.AddBoolItem(_T("Settings"), _T("UndoEnabled"), m_bUndoEnabled, TRUE);
	Profile.AddBoolItem(_T("Settings"), _T("RedoEnabled"), m_bRedoEnabled, TRUE);
	Profile.AddItem(_T("Settings"), _T("MaxUndoDepth"), m_MaxUndoDepth, 100, 0, 1000);
	Profile.AddItem(_T("Settings"), _T("MaxRedoDepth"), m_MaxRedoDepth, 100, 0, 1000);
	Profile.AddItem(_T("Settings"), _T("MaxUndoSize"), m_MaxUndoSize, 0x40000000u,
					0u, 0xC0000000u);
	Profile.AddItem(_T("Settings"), _T("MaxRedoSize"), m_MaxRedoSize, 0x40000000u,
					0u, 0xC0000000u);

	Profile.AddBoolItem(_T("Settings"), _T("5SecondsDC"), m_b5SecondsDC, TRUE);
	Profile.AddItem(_T("Settings"), _T("DcOffsetSelectMode"), m_DcSelectMode, 0, 0, 1);
	Profile.AddItem(_T("Settings"), _T("DcOffset"), m_nDcOffset, 0, -32767, 32767);

	LoadStdProfileSettings(10);  // Load standard INI file options (including MRU)

	if (m_bUseCountrySpecificNumberAndTime)
	{
		TCHAR TimeSeparator[] = _T(": ");
		TCHAR DecimalPoint[] = _T(". ");
		TCHAR ThousandSeparator[] = _T(", ");
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, DecimalPoint, 2);
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, TimeSeparator, 2);
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, ThousandSeparator, 2);
		m_TimeSeparator = TimeSeparator[0];
		m_DecimalPoint = DecimalPoint[0];
		m_ThousandSeparator = ThousandSeparator[0];
	}

	m_FileCache = new CDirectFile::CDirectFileCache(0);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CWaveSoapDocTemplate* pDocTemplate;
	pDocTemplate = new CWaveSoapDocTemplate(
											IDR_WAVESOTYPE,
											RUNTIME_CLASS(CWaveSoapFrontDoc),
											RUNTIME_CLASS(CChildFrame), // custom MDI child frame
											RUNTIME_CLASS(CWaveSoapFrontView)
											);
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
	ParseCommandLine(cmdInfo);

	// start the processing thread
	m_hThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_RunThread = true;
	m_Thread.CreateThread(0, 0x10000);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	m_pMainWnd->DragAcceptFiles();
	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CWaveSoapFrontApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

double CWaveSoapFrontApp::GetProfileDouble(LPCTSTR Section, LPCTSTR ValueName,
											double Default, double MinVal, double MaxVal)
{
	CString s = GetProfileString(Section, ValueName, "");
	double val;
	TCHAR * endptr;

	if (s.IsEmpty()
		|| (val = _tcstod(s, & endptr), 0 != *endptr)
		|| _isnan(val)
		|| ! _finite(val))
	{
		val = Default;
	}

	if (val < MinVal) val = MinVal;
	if (val > MaxVal) val = MaxVal;

	return val;
}

void CWaveSoapFrontApp::WriteProfileDouble(LPCTSTR Section, LPCTSTR ValueName,
											double value)
{
	CString s;
	s.Format("%g", value);
	WriteProfileString(Section, ValueName, s);
}

CDocument* CWaveSoapDocTemplate::OpenDocumentFile(LPCTSTR lpszPathName,
												BOOL bMakeVisible)
{
	CDocument* pDocument = CreateNewDocument();
	if (pDocument == NULL)
	{
		TRACE0("CDocTemplate::CreateNewDocument returned NULL.\n");
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		return NULL;
	}
	ASSERT_VALID(pDocument);
	// modified version of original CMultiDocTemplate::OpenDocumentFile
	// Document is created and initialized first, then the frame is created
	if (lpszPathName == NULL)
	{
		// create a new document - with default document name
		SetDefaultTitle(pDocument);

		// avoid creating temporary compound file when starting up invisible
		if (!bMakeVisible)
			pDocument->m_bEmbedded = TRUE;

		if (!pDocument->OnNewDocument())
		{
			// user has be alerted to what failed in OnNewDocument
			TRACE0("CDocument::OnNewDocument returned FALSE.\n");
			//pFrame->DestroyWindow();
			delete pDocument;       // explicit delete on error
			return NULL;
		}

		// it worked, now bump untitled count
		m_nUntitledCount++;
	}
	else
	{
		// open an existing document
		CWaveSoapFrontApp * pApp = GetApp();
		CWaitCursor wait;
		BOOL ReadOnly = pApp->m_bReadOnly;
		BOOL DirectMode = pApp->m_bDirectMode;
		if (lpszPathName[0] <= 7)
		{
			pApp->m_bReadOnly = (0 != (lpszPathName[0] & 4));
			pApp->m_bDirectMode = (0 != (lpszPathName[0] & 2));
			lpszPathName++;
		}
		if (!pDocument->OnOpenDocument(lpszPathName))
		{
			pApp->m_bReadOnly = ReadOnly;
			pApp->m_bDirectMode = DirectMode;
			// user has be alerted to what failed in OnOpenDocument
			TRACE0("CDocument::OnOpenDocument returned FALSE.\n");
			//pFrame->DestroyWindow();
			delete pDocument;       // explicit delete on error
			return NULL;
		}
		pApp->m_bReadOnly = ReadOnly;
		pApp->m_bDirectMode = DirectMode;
		pDocument->SetPathName(lpszPathName);
	}

	BOOL bAutoDelete = pDocument->m_bAutoDelete;
	pDocument->m_bAutoDelete = FALSE;   // don't destroy if something goes wrong
	CFrameWnd* pFrame = CreateNewFrame(pDocument, NULL);
	pDocument->m_bAutoDelete = bAutoDelete;
	if (pFrame == NULL)
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		delete pDocument;       // explicit delete on error
		return NULL;
	}
	ASSERT_VALID(pFrame);

	InitialUpdateFrame(pFrame, pDocument, bMakeVisible);
	return pDocument;
}

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontApp message handlers


int CWaveSoapFrontApp::ExitInstance()
{
	// TODO: Add your specialized code here and/or call the base class
	if (m_Thread.m_hThread)
	{
		m_RunThread = false;
		SetEvent(m_hThreadEvent);
		if (WAIT_TIMEOUT == WaitForSingleObject(m_Thread.m_hThread, 5000))
		{
			TerminateThread(m_Thread.m_hThread, -1);
		}
	}
	CloseHandle(m_hThreadEvent);
	m_hThreadEvent = NULL;

	LPTSTR dirbuf = m_CurrentDir.GetBuffer(MAX_PATH+1);
	if (dirbuf)
	{
		GetCurrentDirectory(MAX_PATH+1, dirbuf);
		m_CurrentDir.ReleaseBuffer();
	}
	Profile.UnloadSection(_T("Settings"));
	Profile.UnloadSection(_T("Settings\\Colors"));
	// must close the file before deleting the cache
	m_ClipboardFile.Close();
	delete m_FileCache;
	m_FileCache = NULL;
	return CWinApp::ExitInstance();
}

void CWaveSoapFrontApp::QueueOperation(COperationContext * pContext)
{
	// add the operation to the tail
	CSimpleCriticalSectionLock lock(m_cs);
	pContext->pDocument->m_pQueuedOperation = pContext;
	pContext->pPrev = m_pLastOp;
	pContext->pNext = NULL;
	if (m_pLastOp != NULL)
	{
		m_pLastOp->pNext = pContext;
	}
	else
	{
		m_pFirstOp = pContext;
	}
	m_pLastOp = pContext;
	SetEvent(m_hThreadEvent);

}

unsigned CWaveSoapFrontApp::_ThreadProc()
{
	m_Thread.SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);

	bool NeedKickIdle = false;
	COperationContext * pLastContext = NULL;
	while (m_RunThread)
	{
		if (NeedKickIdle)
		{
			if (m_pMainWnd)
			{
				((CMainFrame *)m_pMainWnd)->ResetLastStatusMessage();
				if (::PostMessage(m_pMainWnd->m_hWnd, WM_KICKIDLE, 0, 0))
				{
					NeedKickIdle = false;    // otherwise keep bugging
				}
			}
		}
		COperationContext * pContext = NULL;
		if (NULL != m_pFirstOp)
		{
			CSimpleCriticalSectionLock lock(m_cs);
			// find if stop requested for any document
			pContext = m_pFirstOp;
			while (pContext)
			{
				if ((pContext->m_Flags & OperationContextStopRequested)
					|| pContext->pDocument->m_StopOperation)
				{
					break;
				}
				pContext = pContext->pNext;
			}
			if (NULL == pContext)
			{
				// Find if there is an operation for the active document
				pContext = m_pFirstOp;
				while (pContext)
				{
					if (pContext->pDocument == m_pActiveDocument)
					{
						break;
					}
					pContext = pContext->pNext;
				}
				// But if it is clipboard operation,
				// the first clipboard op will be executed instead
				if (pContext != NULL
					&& (pContext->m_Flags & OperationContextClipboard))
				{
					pContext = m_pFirstOp;
					while (pContext)
					{
						if (pContext->m_Flags & OperationContextClipboard)
						{
							break;
						}
						pContext = pContext->pNext;
					}
				}
				if (NULL == pContext)
				{
					pContext = m_pFirstOp;
				}
			}
		}
		if (pContext != pLastContext)
		{
			pLastContext = pContext;
			NeedKickIdle = true;
		}
		if (pContext != NULL)
		{
			// execute one step
			if (0 == (pContext->m_Flags & OperationContextInitialized))
			{
				if ( ! pContext->Init())
				{
					pContext->m_Flags |= OperationContextStop;
				}
				pContext->m_Flags |= OperationContextInitialized;
				NeedKickIdle = true;
			}

			if (pContext->pDocument->m_StopOperation)
			{
				pContext->m_Flags |= OperationContextStopRequested;
			}
			int LastPercent = pContext->PercentCompleted;
			if ( 0 == (pContext->m_Flags & (OperationContextStop | OperationContextFinished)))
			{
				if ( ! pContext->OperationProc())
				{
					pContext->m_Flags |= OperationContextStop;
				}
			}
			// signal for status update
			if (LastPercent != pContext->PercentCompleted)
			{
				NeedKickIdle = true;
			}
			if (pContext->m_Flags & (OperationContextStop | OperationContextFinished))
			{
				// remove the context from the list and delete the context
				CSimpleCriticalSectionLock lock(m_cs);
				if (pContext->pPrev != NULL)
				{
					pContext->pPrev->pNext = pContext->pNext;
				}
				else
				{
					m_pFirstOp = pContext->pNext;
				}
				if (pContext->pNext != NULL)
				{
					pContext->pNext->pPrev = pContext->pPrev;
				}
				else
				{
					m_pLastOp = pContext->pPrev;
				}
				ASSERT(pContext->pDocument->m_pQueuedOperation == pContext);
				pContext->pDocument->m_pQueuedOperation = NULL;
				// send a signal to the document, that the operation completed
				pContext->pDocument->m_CurrentStatusString =
					pContext->GetStatusString() + _T("Completed");
				pContext->DeInit();
				pContext->Retire();     // usually deletes it
				// send a signal to the document, that the operation completed
				NeedKickIdle = true;    // this will reenable all commands
			}
			else
			{
				if (NeedKickIdle)
				{
					pContext->pDocument->m_CurrentStatusString.Format(_T("%s%d%%"),
						(LPCTSTR)pContext->GetStatusString(), pContext->PercentCompleted);
				}
			}
			continue;
		}
		else
		{
		}
		WaitForSingleObject(m_hThreadEvent, 1000);
	}
	return 0;
}

static void _AfxAppendFilterSuffix(CString& filter, OPENFILENAME& ofn,
									CDocTemplate* pTemplate, CString* pstrDefaultExt)
{
	ASSERT_VALID(pTemplate);
	ASSERT_KINDOF(CDocTemplate, pTemplate);

	CString strFilterExt, strFilterName;
	if (pTemplate->GetDocString(strFilterExt, CDocTemplate::filterExt) &&
		!strFilterExt.IsEmpty() &&
		pTemplate->GetDocString(strFilterName, CDocTemplate::filterName) &&
		!strFilterName.IsEmpty())
	{
		// a file based document template - add to filter list
		ASSERT(strFilterExt[0] == '.');
		if (pstrDefaultExt != NULL)
		{
			// set the default extension
			*pstrDefaultExt = ((LPCTSTR)strFilterExt) + 1;  // skip the '.'
			ofn.lpstrDefExt = (LPTSTR)(LPCTSTR)(*pstrDefaultExt);
			ofn.nFilterIndex = ofn.nMaxCustFilter + 1;  // 1 based number
		}

		// add to filter
		filter += strFilterName;
		ASSERT(!filter.IsEmpty());  // must have a file type name
		filter += (TCHAR)'\0';  // next string please
		filter += (TCHAR)'*';
		filter += strFilterExt;
		filter += (TCHAR)'\0';  // next string please
		ofn.nMaxCustFilter++;
	}
}

void CWaveSoapDocManager::OnFileOpen()
{
	CWaveSoapFileDialog dlgFile(TRUE);

	CString title("Open");;
	CString fileNameBuf;
	//VERIFY(title.LoadString(nIDSTitle));

	dlgFile.m_ofn.Flags |=
		OFN_HIDEREADONLY
		| OFN_FILEMUSTEXIST
		| OFN_EXPLORER
		| OFN_ENABLESIZING
		| OFN_ENABLETEMPLATE
		| OFN_ALLOWMULTISELECT;
	dlgFile.m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG_OPEN_TEMPLATE);

	CString strFilter;
	CString strDefault;
	// do for all doc template
	POSITION pos = m_templateList.GetHeadPosition();
	BOOL bFirst = TRUE;
	while (pos != NULL)
	{
		CDocTemplate* pTemplate = (CDocTemplate*)m_templateList.GetNext(pos);
		_AfxAppendFilterSuffix(strFilter, dlgFile.m_ofn, pTemplate,
								bFirst ? &strDefault : NULL);
		bFirst = FALSE;
	}

	// append the "*.*" all files filter
	CString allFilter;
	VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));
	strFilter += allFilter;
	strFilter += (TCHAR)'\0';   // next string please
	strFilter += _T("*.*");
	strFilter += (TCHAR)'\0';   // last string
	dlgFile.m_ofn.nMaxCustFilter++;

	dlgFile.m_ofn.lpstrFilter = strFilter;
	dlgFile.m_ofn.lpstrTitle = title;
	dlgFile.m_ofn.lpstrFile = fileNameBuf.GetBuffer(0x2000);   // 8K
	dlgFile.m_ofn.nMaxFile = 0x2000-1;

	int nResult = dlgFile.DoModal();
	if (nResult != IDOK)
	{
		fileNameBuf.ReleaseBuffer(0x2000-1);
		return;
	}
	m_bReadOnly = dlgFile.m_bReadOnly;
	m_bDirectMode = dlgFile.m_bDirectMode;
	CWaveSoapFrontApp * pApp = GetApp();
	pApp->m_bReadOnly = m_bReadOnly;
	pApp->m_bDirectMode = m_bDirectMode;

	pos = dlgFile.GetStartPosition();
	while (pos != NULL)
	{
		// check how many names selected,
		// open all of selected files
		CString fileName = dlgFile.GetNextPathName(pos);
		TRACE("Opening file: %s\n", LPCTSTR(fileName));
		AfxGetApp()->OpenDocumentFile(fileName);
	}
	fileNameBuf.ReleaseBuffer();
}

void CWaveSoapFileDialog::OnComboClosed()
{
}

void CWaveSoapFileDialog::OnCheckReadOnly()
{
	CButton * pRO = (CButton *)GetDlgItem(IDC_CHECK_READONLY);
	CWnd * pDirect = GetDlgItem(IDC_CHECK_DIRECT);
	if (NULL != pRO)
	{
		m_bReadOnly = (0 != pRO->GetCheck());
		if (NULL != pDirect)
		{
			pDirect->EnableWindow( ! m_bReadOnly);
		}
	}
}

void CWaveSoapFileDialog::OnCheckDirectMode()
{
	CButton * pDirect = (CButton *)GetDlgItem(IDC_CHECK_DIRECT);
	if (NULL != pDirect)
	{
		m_bDirectMode = (0 != pDirect->GetCheck());
	}
}

BOOL CWaveSoapFileDialog::OnFileNameOK()
{
	m_WaveFile.Close();
	return CFileDialog::OnFileNameOK();
}

#if 0
void CWaveSoapFileDialog::OnLBSelChangedNotify(UINT nIDBox, UINT iCurSel, UINT nCode)
{
}
#endif

void CWaveSoapFileDialog::OnInitDone()
{
	CFileDialog::OnInitDone();
	ClearFileInfoDisplay();
	CWaveSoapFrontApp * pApp = GetApp();
	m_bReadOnly = pApp->m_bReadOnly != 0;
	m_bDirectMode = pApp->m_bDirectMode != 0;

	CButton * pRO = (CButton *)GetDlgItem(IDC_CHECK_READONLY);
	CButton * pDirect = (CButton *)GetDlgItem(IDC_CHECK_DIRECT);
	if (NULL != pRO)
	{
		pRO->SetCheck(m_bReadOnly);
	}
	if (NULL != pDirect)
	{
		pDirect->SetCheck(m_bDirectMode);
		pDirect->EnableWindow( ! m_bReadOnly);
	}
}

void CWaveSoapFileDialog::OnFileNameChange()
{
	// get the file info
	CString sName;
	TCHAR * pBuf = sName.GetBuffer(m_ofn.nMaxFile);
	if (NULL == pBuf)
	{
		ClearFileInfoDisplay();
		return;
	}
	if (GetParent()->SendMessage(CDM_GETFILEPATH, (WPARAM)m_ofn.nMaxFile,
								(LPARAM)pBuf) < 0)
	{
		ClearFileInfoDisplay();
		return;
	}
	TRACE("CWaveSoapFileDialog::OnFileNameChange=%s\n", LPCTSTR(sName));
	// if one file selected, its name will be in the buffer.
	// If multiple files selected, the buffer will contain directory name,
	// then file name surrounded with double quotes,
	// then other filenames surrounded with quotes and delimited with space
	TCHAR * tmp = pBuf;
	while (*tmp != 0)
	{
		if ('"' == *tmp)
		{
			// multiple selection
			ClearFileInfoDisplay();
			return;
		}
		tmp++;
	}
	// try to open the file for reading (it can be .LNK, though!)
	CString FileName = ResolveIfShellLink(pBuf);
	if ( ! FileName.IsEmpty()
		&& m_WaveFile.Open(FileName, MmioFileOpenExisting | MmioFileOpenReadOnly)
		&& m_WaveFile.LoadWaveformat()
		&& m_WaveFile.FindData())
	{
		WAVEFORMATEX * pWf = m_WaveFile.GetWaveFormat();
		if (pWf != NULL)
		{
			nChannels = pWf->nChannels;
			nBitsPerSample = pWf->wBitsPerSample;
			if (16 == nBitsPerSample
				&& WAVE_FORMAT_PCM == pWf->wFormatTag
				&& (nChannels == 1 || nChannels == 2))
			{
				// can open direct and readonly
				CWnd * pWnd = GetDlgItem(IDC_CHECK_READONLY);
				if (pWnd)
				{
					pWnd->EnableWindow(TRUE);
				}
				pWnd = GetDlgItem(IDC_CHECK_DIRECT);
				if (pWnd)
				{
					pWnd->EnableWindow(TRUE);
				}
			}
			else
			{
				// can't open direct and readonly
				CWnd * pWnd = GetDlgItem(IDC_CHECK_READONLY);
				if (pWnd)
				{
					pWnd->EnableWindow(FALSE);
				}
				pWnd = GetDlgItem(IDC_CHECK_DIRECT);
				if (pWnd)
				{
					pWnd->EnableWindow(FALSE);
				}
			}
			nSamplingRate = pWf->nSamplesPerSec;
			nSamples = m_WaveFile.NumberOfSamples();
			SetDlgItemText(IDC_STATIC_FILE_TYPE, _T("Microsoft RIFF Wave"));
			CString s;
			CString s2;
			// get format name
			ACMFORMATDETAILS afd;
			memset (& afd, 0, sizeof afd);
			afd.cbStruct = sizeof afd;
			afd.pwfx = pWf;
			afd.cbwfx = sizeof (WAVEFORMATEX) + pWf->cbSize;
			afd.dwFormatTag = pWf->wFormatTag;

			if (MMSYSERR_NOERROR == acmFormatDetails(NULL, & afd,
													ACM_FORMATDETAILSF_FORMAT))
			{
				SetDlgItemText(IDC_STATIC_ATTRIBUTES, afd.szFormat);
			}
			else
			{
				SetDlgItemText(IDC_STATIC_ATTRIBUTES, _T("Unknown"));
			}

			s2.Format(_T("%s (%s)"),
					LPCTSTR(TimeToHhMmSs(nSamples / double(nSamplingRate) * 1000)),
					LPCTSTR(LtoaCS(nSamples)));
			SetDlgItemText(IDC_STATIC_FILE_LENGTH, s2);
			ACMFORMATTAGDETAILS aft;
			memset (& aft, 0, sizeof aft);
			aft.cbStruct = sizeof aft;
			aft.dwFormatTag = afd.dwFormatTag;
			if (MMSYSERR_NOERROR == acmFormatTagDetails(NULL, & aft,
														ACM_FORMATTAGDETAILSF_FORMATTAG))
			{
				SetDlgItemText(IDC_STATIC_FILE_FORMAT, aft.szFormatTag);
			}
			else
			{
				SetDlgItemText(IDC_STATIC_FILE_FORMAT, _T("Unknown"));
			}
		}
		else
		{
			ClearFileInfoDisplay();
		}
	}
	else
	{
		ClearFileInfoDisplay();
	}
	sName.ReleaseBuffer();
	//POSITION pos = Get
}

void CWaveSoapFileDialog::ClearFileInfoDisplay()
{
	SetDlgItemText(IDC_STATIC_FILE_TYPE, _T("Unknown"));
	SetDlgItemText(IDC_STATIC_FILE_FORMAT, _T(""));
	SetDlgItemText(IDC_STATIC_FILE_LENGTH, _T(""));
	SetDlgItemText(IDC_STATIC_ATTRIBUTES, _T(""));
	SetDlgItemText(IDC_EDIT_FILE_COMMENTS, _T(""));
}

void CWaveSoapFileDialog::OnFolderChange()
{
}

void CWaveSoapFileDialog::OnTypeChange()
{
}

CString CWaveSoapFileDialog::GetNextPathName(POSITION& pos) const
{
	ASSERT(m_ofn.Flags & OFN_EXPLORER);

	LPTSTR lpsz = (LPTSTR)pos;
	if (lpsz == m_ofn.lpstrFile) // first time
	{
		if ((m_ofn.Flags & OFN_ALLOWMULTISELECT) == 0)
		{
			pos = NULL;
			return m_ofn.lpstrFile;
		}

		// find char pos after first Delimiter
		while(*lpsz != '\0')
			lpsz = _tcsinc(lpsz);
		lpsz = _tcsinc(lpsz);

		// if single selection then return only selection
		if (*lpsz == 0)
		{
			pos = NULL;
			return m_ofn.lpstrFile;
		}
	}

	CString strPath = m_ofn.lpstrFile;
	LPTSTR lpszFileName = lpsz;
	CString strFileName = lpsz;

	// find char pos at next Delimiter
	while(*lpsz != '\0')
		lpsz = _tcsinc(lpsz);

	lpsz = _tcsinc(lpsz);
	if (*lpsz == '\0') // if double terminated then done
		pos = NULL;
	else
		pos = (POSITION)lpsz;

	// check if the filename is already absolute
	if (strFileName[0] == '/' || strFileName[0] == '\\'
		|| (strFileName.GetLength() > 1 && strFileName[1] == ':'))
	{
		TCHAR * pTitle;
		GetFullPathName(strFileName,MAX_PATH,strPath.GetBuffer(MAX_PATH), & pTitle);
		strPath.ReleaseBuffer();
		return strPath;
	}
	else
	{
		// only add '\\' if it is needed
		if (!strPath.IsEmpty())
		{
			// check for last back-slash or forward slash (handles DBCS)
			LPCTSTR lpsz = _tcsrchr(strPath, '\\');
			if (lpsz == NULL)
				lpsz = _tcsrchr(strPath, '/');
			// if it is also the last character, then we don't need an extra
			if (lpsz != NULL &&
				(lpsz - (LPCTSTR)strPath) == strPath.GetLength()-1)
			{
				ASSERT(*lpsz == '\\' || *lpsz == '/');
				return strPath + strFileName;
			}
		}
		return strPath + '\\' + strFileName;
	}
}

// long to string, thousands separated by commas
CString LtoaCS(long num)
{
	char s[20];
	char s1[30];
	char * p = s;
	char * p1 = s1;
	TCHAR ThSep = GetApp()->m_ThousandSeparator;
	ltoa(num, s, 10);
	if (0 == ThSep)
	{
		return s;
	}
	if ('-' == p[0])
	{
		p1[0] = '-';
		p++;
		p1++;
	}
	int len = strlen(p);
	int first = len % 3;
	if (0 == first && len > 0)
	{
		first = 3;
	}
	strncpy(p1, p, first);
	p1 += first;
	p += first;
	len -= first;
	while (p[0])
	{
		p1[0] = ThSep;
		p1[1] = p[0];
		p1[2] = p[1];
		p1[3] = p[2];
		p1 += 4;
		p += 3;
	}
	*p1 = 0;
	return CString(s1);
}

CString TimeToHhMmSs(unsigned TimeMs, int Flags)
{
	int hh = TimeMs / 3600000;
	TimeMs -= hh * 3600000;
	int mm = TimeMs / 60000;
	TimeMs -= mm * 60000;
	int ss = TimeMs / 1000;
	TimeMs -= ss * 1000;
	int ms = TimeMs;
	CString s;
	TCHAR TimeSeparator = GetApp()->m_TimeSeparator;
	TCHAR DecimalPoint = GetApp()->m_DecimalPoint;

	if (Flags & TimeToHhMmSs_NeedsMs)
	{
		if (hh != 0 || (Flags & TimeToHhMmSs_NeedsHhMm))
		{
			s.Format(_T("%d%c%02d%c%02d%c%03d"),
					hh, TimeSeparator,
					mm, TimeSeparator,
					ss, DecimalPoint,
					ms);
		}
		else if (mm != 0)
		{
			s.Format(_T("%d%c%02d%c%03d"),
					mm, TimeSeparator,
					ss, DecimalPoint,
					ms);
		}
		else
		{
			s.Format(_T("%d%c%03d"),
					ss, DecimalPoint,
					ms);
		}
	}
	else
	{
		if (hh != 0 || (Flags & TimeToHhMmSs_NeedsHhMm))
		{
			s.Format(_T("%d%c%02d%c%02d"),
					hh, TimeSeparator,
					mm, TimeSeparator,
					ss);
		}
		else
		{
			s.Format(_T("%d%c%02d"),
					mm, TimeSeparator,
					ss);
		}
	}
	return s;
}

CString SampleToString(long Sample, long nSamplesPerSec, int Flags)
{
	switch (Flags & SampleToString_Mask)
	{
	case SampleToString_Sample:
		return LtoaCS(Sample);
		break;
	case SampleToString_Seconds:
	{
		CString s;
		unsigned ms = unsigned(Sample * 1000. / nSamplesPerSec);
		int sec = ms / 1000;
		ms = ms % 1000;
		TCHAR * pFormat = _T("%s%c0");
		if (Flags & TimeToHhMmSs_NeedsMs)
		{
			pFormat = _T("%s%c%03d");
		}
		s.Format(pFormat, LtoaCS(sec), GetApp()->m_DecimalPoint, ms);
		return s;
	}
		break;
	default:
	case SampleToString_HhMmSs:
		return TimeToHhMmSs(unsigned(Sample * 1000. / nSamplesPerSec), Flags);
		break;
	}
}

int CWaveSoapFrontStatusBar::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	for (int i = 1; i < m_nCount; i++)
	{
		CRect r;
		GetItemRect(i, & r);
		if (r.PtInRect(point))
		{
			int nHit = GetItemID(i);
			if (pTI != NULL && pTI->cbSize >= sizeof(TTTOOLINFOA_V1_SIZE))
			{
				pTI->hwnd = m_hWnd;
				pTI->rect = r;
				pTI->uId = nHit;
				pTI->lpszText = LPSTR_TEXTCALLBACK;
			}
			// found matching rect, return the ID
			return nHit != 0 ? nHit : -1;
		}
	}
	return -1;
}

void CWaveSoapFrontStatusBar::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// make sure window is active
	GetParentFrame()->ActivateFrame();
	CPoint pclient = point;
	ScreenToClient(& pclient);
	int nHit = -1;
	for (int i = 1; i < m_nCount; i++)
	{
		CRect r;
		GetItemRect(i, & r);
		if (r.PtInRect(pclient))
		{
			nHit = GetItemID(i);
			// found matching rect, return the ID
			break;
		}
	}
	UINT id;
	CMenu menu;
	switch(nHit)
	{
	case ID_INDICATOR_SAMPLE_RATE:
		id = IDR_MENU_POPUP_SAMPLE_RATE;
		break;
	case ID_INDICATOR_SAMPLE_SIZE:
		id = IDR_MENU_POPUP_SAMPLE_SIZE;
		break;
	case ID_INDICATOR_CHANNELS:
		id = IDR_MENU_POPUP_CHANNELS;
		break;
	case ID_INDICATOR_FILE_SIZE:
		id = IDR_MENU_FILE_LENGTH;
		break;
	case ID_INDICATOR_CURRENT_POS:
		id = IDR_MENU_CURRENT_POSITION;
		break;
	case ID_INDICATOR_SELECTION_LENGTH:
		id = IDR_MENU_SELECTION_LENGTH;
		break;
	default:
		Default();
		return;
	}
	menu.LoadMenu(id);
	CMenu* pPopup = menu.GetSubMenu(0);
	if(pPopup != NULL)
	{
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
								point.x, point.y,
								AfxGetMainWnd()); // use main window for cmds
	}
}

void SetStatusString(CCmdUI* pCmdUI, const CString & string,
					LPCTSTR MaxString, BOOL bForceSize)
{
	CStatusBar *pSB = DYNAMIC_DOWNCAST(CStatusBar,
										pCmdUI->m_pOther);
	if (NULL == MaxString)
	{
		MaxString = string;
	}
	if(pSB != NULL)
	{
		// Set pane size
		CSize size;
		{
			CWindowDC dc(pSB);
			CFont * pFont = pSB->GetFont();
			VERIFY(pFont);
			CFont *pOldFont = dc.SelectObject(pFont);
			VERIFY(::GetTextExtentPoint32(dc,
										MaxString, strlen(MaxString), & size));
			dc.SelectObject(pOldFont);
		}
		UINT nID, nStyle;
		int cxWidth;
		pSB->GetPaneInfo(pCmdUI->m_nIndex, nID, nStyle,
						cxWidth);
		if (bForceSize || size.cx > cxWidth)
		{
			pSB->SetPaneInfo(pCmdUI->m_nIndex, nID,
							nStyle, size.cx);
		}
	}
	pCmdUI->SetText(string);
}


void CWaveSoapFrontApp::OnEditPasteNew()
{
	if ( ! m_ClipboardFile.IsOpen())
	{
		return;
	}
	POSITION pos = m_pDocManager->GetFirstDocTemplatePosition();
	CDocTemplate* pTemplate = m_pDocManager->GetNextDocTemplate(pos);
	if (pTemplate != NULL)
	{
		m_NewTemplateFile = m_ClipboardFile;
		TRACE("New file channels=%d\n", m_NewTemplateFile.GetWaveFormat()->nChannels);

		CWaveSoapFrontDoc * pDoc = (CWaveSoapFrontDoc *)pTemplate->OpenDocumentFile(NULL);
		if (NULL != pDoc)
		{
			BOOL TmpUndo = pDoc->UndoEnabled();
			pDoc->EnableUndo(FALSE);
			pDoc->DoEditPaste();
			pDoc->EnableUndo(TmpUndo);
		}
		m_NewTemplateFile.Close();
	}
}

void CWaveSoapFrontApp::OnUpdateEditPasteNew(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_ClipboardFile.IsOpen());
}

class CWaveSoapFileList : public CRecentFileList
{
public:
	CWaveSoapFileList(UINT nStart, LPCTSTR lpszSection,
					LPCTSTR lpszEntryFormat, int nSize,
					int nMaxDispLen = AFX_ABBREV_FILENAME_LEN)
		: CRecentFileList(nStart, lpszSection,
						lpszEntryFormat, nSize,
						nMaxDispLen)
	{
	}
	BOOL GetDisplayName(CString& strName, int nIndex,
						LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName = TRUE) const;
	virtual void UpdateMenu(CCmdUI* pCmdUI);
	virtual ~CWaveSoapFileList() {}
};

BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
UINT AFXAPI AfxGetFileTitle(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);
UINT AFXAPI AfxGetFileName(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);

static void AFXAPI _AfxAbbreviateName(LPTSTR lpszCanon, int cchMax, BOOL bAtLeastName)
{
	int cchFullPath, cchFileName, cchVolName;
	const TCHAR* lpszCur;
	const TCHAR* lpszBase;
	const TCHAR* lpszFileName;

	lpszBase = lpszCanon;
	cchFullPath = lstrlen(lpszCanon);

	cchFileName = AfxGetFileName(lpszCanon, NULL, 0) - 1;
	lpszFileName = lpszBase + (cchFullPath-cchFileName);

	// If cchMax is more than enough to hold the full path name, we're done.
	// This is probably a pretty common case, so we'll put it first.
	if (cchMax >= cchFullPath)
		return;

	// If cchMax isn't enough to hold at least the basename, we're done
	if (cchMax < cchFileName)
	{
		lstrcpy(lpszCanon, (bAtLeastName) ? lpszFileName : &afxChNil);
		return;
	}

	// Calculate the length of the volume name.  Normally, this is two characters
	// (e.g., "C:", "D:", etc.), but for a UNC name, it could be more (e.g.,
	// "\\server\share").
	//
	// If cchMax isn't enough to hold at least <volume_name>\...\<base_name>, the
	// result is the base filename.

	lpszCur = lpszBase + 2;                 // Skip "C:" or leading "\\"

	if (lpszBase[0] == '\\' && lpszBase[1] == '\\') // UNC pathname
	{
		// First skip to the '\' between the server name and the share name,
		while (*lpszCur != '\\')
		{
			lpszCur = _tcsinc(lpszCur);
			ASSERT(*lpszCur != '\0');
		}
	}
	// if a UNC get the share name, if a drive get at least one directory
	ASSERT(*lpszCur == '\\');
	// make sure there is another directory, not just c:\filename.ext
	if (cchFullPath - cchFileName > 3)
	{
		lpszCur = _tcsinc(lpszCur);
		while (*lpszCur != '\\')
		{
			lpszCur = _tcsinc(lpszCur);
			ASSERT(*lpszCur != '\0');
		}
	}
	ASSERT(*lpszCur == '\\');

	cchVolName = lpszCur - lpszBase;
	if (cchMax < cchVolName + 5 + cchFileName)
	{
		lstrcpy(lpszCanon, lpszFileName);
		return;
	}

	// Now loop through the remaining directory components until something
	// of the form <volume_name>\...\<one_or_more_dirs>\<base_name> fits.
	//
	// Assert that the whole filename doesn't fit -- this should have been
	// handled earlier.

	ASSERT(cchVolName + (int)lstrlen(lpszCur) > cchMax);
	while (cchVolName + 4 + (int)lstrlen(lpszCur) > cchMax)
	{
		do
		{
			lpszCur = _tcsinc(lpszCur);
			ASSERT(*lpszCur != '\0');
		}
		while (*lpszCur != '\\');
	}

	// Form the resultant string and we're done.
	lpszCanon[cchVolName] = '\0';
	lstrcat(lpszCanon, _T("\\..."));
	lstrcat(lpszCanon, lpszCur);
}

void CWaveSoapFrontApp::LoadStdProfileSettings(UINT nMaxMRU)
{
	ASSERT_VALID(this);
	ASSERT(m_pRecentFileList == NULL);

	if (nMaxMRU != 0)
	{
		// create file MRU since nMaxMRU not zero
		m_pRecentFileList = new CWaveSoapFileList(0, _T("Recent File List"), _T("File%d"),
												nMaxMRU);
		m_pRecentFileList->ReadList();
	}
	// 0 by default means not set
	m_nNumPreviewPages = 0;//GetProfileInt(_T("Settings"), _T("PreviewPages"), 0);
}

BOOL CWaveSoapFileList::GetDisplayName(CString& strName, int nIndex,
										LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName) const
{
	ASSERT(lpszCurDir == NULL || AfxIsValidString(lpszCurDir, nCurDir));

	ASSERT(m_arrNames != NULL);
	ASSERT(nIndex < m_nSize);
	if (m_arrNames[nIndex].IsEmpty())
		return FALSE;

	LPTSTR lpch = strName.GetBuffer(_MAX_PATH+1);
	lpch[_MAX_PATH] = 0;
	CString suffix;
	TCHAR flags = m_arrNames[nIndex][0];
	if (flags <= 7)
	{
		if (flags & 4)
		{
			suffix = " (RO)";
		}
		else if (flags & 2)
		{
			suffix = " (D)";
		}
		lstrcpyn(lpch, 1 + LPCTSTR(m_arrNames[nIndex]), _MAX_PATH);
	}
	else
	{
		lstrcpyn(lpch, m_arrNames[nIndex], _MAX_PATH);
	}
	// nLenDir is the length of the directory part of the full path
	int nLenDir = lstrlen(lpch) - (AfxGetFileName(lpch, NULL, 0) - 1);
	BOOL bSameDir = FALSE;
	if (nLenDir == nCurDir)
	{
		TCHAR chSave = lpch[nLenDir];
		lpch[nCurDir] = 0;  // terminate at same location as current dir
		bSameDir = lstrcmpi(lpszCurDir, lpch) == 0;
		lpch[nLenDir] = chSave;
	}
	// copy the full path, otherwise abbreviate the name
	if (bSameDir)
	{
		// copy file name only since directories are same
		TCHAR szTemp[_MAX_PATH];
		AfxGetFileTitle(lpch+nCurDir, szTemp, _countof(szTemp));
		lstrcpyn(lpch, szTemp, _MAX_PATH);
	}
	else if (m_nMaxDisplayLength != -1)
	{
		// strip the extension if the system calls for it
		TCHAR szTemp[_MAX_PATH];
		AfxGetFileTitle(lpch+nLenDir, szTemp, _countof(szTemp));
		lstrcpyn(lpch+nLenDir, szTemp, _MAX_PATH-nLenDir);

		// abbreviate name based on what will fit in limited space
		_AfxAbbreviateName(lpch, m_nMaxDisplayLength, bAtLeastName);
	}
	strName.ReleaseBuffer();
	strName += suffix;
	return TRUE;
}

void CWaveSoapFileList::UpdateMenu(CCmdUI* pCmdUI)
{
	ASSERT(m_arrNames != NULL);

	CMenu* pMenu = pCmdUI->m_pMenu;
	if (m_strOriginal.IsEmpty() && pMenu != NULL)
		pMenu->GetMenuString(pCmdUI->m_nID, m_strOriginal, MF_BYCOMMAND);

	if (m_arrNames[0].IsEmpty())
	{
		// no MRU files
		if (!m_strOriginal.IsEmpty())
			pCmdUI->SetText(m_strOriginal);
		pCmdUI->Enable(FALSE);
		return;
	}

	if (pCmdUI->m_pMenu == NULL)
		return;

	for (int iMRU = 0; iMRU < m_nSize; iMRU++)
		pCmdUI->m_pMenu->DeleteMenu(pCmdUI->m_nID + iMRU, MF_BYCOMMAND);

	TCHAR szCurDir[_MAX_PATH];
	GetCurrentDirectory(_MAX_PATH, szCurDir);
	int nCurDir = lstrlen(szCurDir);
	ASSERT(nCurDir >= 0);
	szCurDir[nCurDir] = '\\';
	szCurDir[++nCurDir] = '\0';

	CString strName;
	CString strTemp;
	for (iMRU = 0; iMRU < m_nSize; iMRU++)
	{
		if (!GetDisplayName(strName, iMRU, szCurDir, nCurDir))
			break;

		// double up any '&' characters so they are not underlined
		LPCTSTR lpszSrc = strName;
		LPTSTR lpszDest = strTemp.GetBuffer(strName.GetLength()*2);
		while (*lpszSrc != 0)
		{
			if (*lpszSrc == '&')
				*lpszDest++ = '&';
			if (_istlead(*lpszSrc))
				*lpszDest++ = *lpszSrc++;
			*lpszDest++ = *lpszSrc++;
		}
		*lpszDest = 0;
		strTemp.ReleaseBuffer();

		// insert mnemonic + the file name
		TCHAR buf[10];
		wsprintf(buf, _T("&%d "), (iMRU+1+m_nStart) % 10);
		pCmdUI->m_pMenu->InsertMenu(pCmdUI->m_nIndex++,
									MF_STRING | MF_BYPOSITION, pCmdUI->m_nID++,
									CString(buf) + strTemp);
	}

	// update end menu count
	pCmdUI->m_nIndex--; // point to last menu added
	pCmdUI->m_nIndexMax = pCmdUI->m_pMenu->GetMenuItemCount();

	pCmdUI->m_bEnableChanged = TRUE;    // all the added items are enabled
}

