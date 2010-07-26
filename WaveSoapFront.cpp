// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveSoapFront.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WaveSoapFront.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "WaveSoapFrontDoc.h"
#include "WaveSoapFrontView.h"
#include "WaveFftView.h"
#include <float.h>
#include <afxpriv.h>
#include <mmreg.h>
#include <msacm.h>
#include "OperationDialogs2.h"
#include <Dlgs.h>
#include "NewFilePropertiesDlg.h"
#include "FileDialogWithHistory.h"
#include "PreferencesPropertySheet.h"
#include "WaveSoapFileDialogs.h"
#include "BatchConvertDlg.h"
#include <imagehlp.h>
#include <shlwapi.h>
#include "GdiObjectSave.h"
#include "OperationContext.h"
#include "OperationContext2.h"
#include "ApplicationParameters.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
BOOL AFXAPI AfxResolveShortcut(CWnd* pWnd, LPCTSTR pszShortcutFile,
								LPTSTR pszPath, int cchPath);
// CWaveSoapFrontApp
class CWaveSoapDocManager : public CDocManager
{
public:
	CWaveSoapDocManager()
	{}
	~CWaveSoapDocManager() {}
	virtual void OnFileOpen();
	CDocument* OpenDocumentFile(LPCTSTR lpszFileName, int flags);
	void BroadcastUpdate(UINT lHint);
	BOOL IsAnyDocumentModified();
	BOOL CanSaveAnyDocument();
	void SaveAll();
};

void CWaveSoapDocTemplate::OnIdle()
{
	CMultiDocTemplate::OnIdle();
	// now, delete all dead documents
	while (1)
	{
		POSITION pos = GetFirstDocPosition();
		while (pos != NULL)
		{
			CWaveSoapFrontDoc * pDoc =
				dynamic_cast<CWaveSoapFrontDoc *>(GetNextDoc(pos));
			if (NULL != pDoc)
			{
				ASSERT_VALID(pDoc);
				if (pDoc->m_bCloseThisDocumentNow)
				{
					pDoc->OnCloseDocument();
					break;  // iterate the document list once again
				}
			}
		}
		if (NULL == pos)
		{
			break;
		}
	}
}

void CWaveSoapFrontStatusBar::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// make sure window is active
	int nHit = -1;
	for (int i = 1; i < m_nCount; i++)
	{
		CRect r;
		GetItemRect(i, & r);
		if (r.PtInRect(point))
		{
			nHit = GetItemID(i);
			// found matching rect
			break;
		}
	}

	if (nHit == ID_INDICATOR_SCALE)
	{
		// toggle previous scale
		AfxGetMainWnd()->SendMessage(WM_COMMAND, ID_VIEW_ZOOMPREVIOUS);
	}

	CStatusBar::OnLButtonDblClk(nFlags, point);
}

BEGIN_MESSAGE_MAP(CWaveSoapFrontStatusBar, CStatusBar)
	//{{AFX_MSG_MAP(CWaveSoapFrontStatusBar)
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CWaveSoapFrontApp, BaseClass)
	//{{AFX_MSG_MAP(CWaveSoapFrontApp)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_EDIT_PASTE_NEW, OnEditPasteNew)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE_NEW, OnUpdateEditPasteNew)
	ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, OnOpenRecentFile)
	ON_COMMAND(ID_TOOLS_CDGRAB, OnToolsCdgrab)
	ON_COMMAND(ID_FILE_SAVE_ALL, OnFileSaveAll)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_ALL, OnUpdateFileSaveAll)
	ON_COMMAND(ID_TOOLS_OPTIONS, OnToolsOptions)
	ON_COMMAND(ID_FILE_BATCHCONVERSION, OnFileBatchconversion)
	//}}AFX_MSG_MAP
	// if no documents, Paste will create a new file
	ON_COMMAND(ID_EDIT_PASTE, OnEditPasteNew)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPasteNew)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_OPEN, BaseClass::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, BaseClass::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontApp construction

CWaveSoapFrontApp::CWaveSoapFrontApp()
	: m_FileCache(NULL),
	m_Thread(ThreadProc, this),
	m_RunThread(false),

	m_DefaultPlaybackDevice(WAVE_MAPPER),
	m_NumPlaybackBuffers(4),
	m_SizePlaybackBuffers(0x10000),

	m_DefaultRecordDevice(WAVE_MAPPER),
	m_NumRecordBuffers(8),
	m_SizeRecordBuffers(0x10000),

	m_bReadOnly(false),
	m_bDirectMode(false),

	m_bUseCountrySpecificNumberAndTime(false),

	m_bShowToolbar(true),
	m_bShowStatusBar(true),
	m_bOpenChildMaximized(true),

	m_bAllow4GbWavFile(false),
	m_DefaultOpenMode(DefaultOpenBuffered),

	m_SoundTimeFormat(SampleToString_HhMmSs | TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs),

	m_pActiveDocument(NULL),
	m_pLastStatusDocument(NULL),

	m_pAllTypesTemplate(NULL),
	m_pMP3TypeTemplate(NULL),
	m_pWavTypeTemplate(NULL),
	m_pRawTypeTemplate(NULL),
	m_pWmaTypeTemplate(NULL),
	m_pAviTypeTemplate(NULL),
	m_pAllWmTypeTemplate(NULL),

	m_DontShowMediaPlayerWarning(FALSE),

	m_bSnapMouseSelectionToMax(TRUE),

	m_hWMVCORE_DLL_Handle(NULL),

	m_bShowNewFormatDialogWhenShiftOnly(false),
	m_NewFileLength(10),

	m_SpectrumSectionWidth(100),
	m_FftBandsOrder(9),
	m_FftWindowType(0),

	m_OpenFileDialogFilter(1)

	, m_PasteResampleMode(0)
	, m_UseFadeInOut(false)
	, m_FadeInOutLengthMs(100)
	, m_FadeInEnvelope(FadeInSinSquared)
	, m_FadeOutEnvelope(FadeOutSinSquared)

	, m_LastPrefsPropertyPageSelected(0)
{
	// Place all significant initialization in InitInstance
	m_NewFileFormat.wFormatTag = WAVE_FORMAT_PCM;
	m_NewFileFormat.nChannels = 2;
	m_NewFileFormat.nSamplesPerSec = 44100;
	m_NewFileFormat.nAvgBytesPerSec = 44100 * 4;
	m_NewFileFormat.wBitsPerSample = 16;
	m_NewFileFormat.nBlockAlign = 4;
	m_NewFileFormat.cbSize = 0;

	m_Thread.m_bAutoDelete = FALSE;
	m_pDocManager = new CWaveSoapDocManager;

	SetAppID(_T("AlegrSoft.WaveSoap.WaveSoap.0000"));
	EnableHtmlHelp();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWaveSoapFrontApp object

CWaveSoapFrontApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontApp initialization
class CMyCommandLineInfo : public CCommandLineInfo
{
	//plain char* version on UNICODE for source-code backwards compatibility
	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);
	void ParseParamFlag(const TCHAR* pszParam);
	void ParseParamNotFlag(const TCHAR* pszParam);
	void ParseLast(BOOL bLast);
};

void CMyCommandLineInfo::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	if (bFlag)
	{
		ParseParamFlag(pszParam);
	}
	else
		ParseParamNotFlag(pszParam);

	ParseLast(bLast);
}

void CMyCommandLineInfo::ParseParamFlag(LPCTSTR pszParam)
{
	// OLE command switches are case insensitive, while
	// shell command switches are case sensitive

	if (lstrcmp(pszParam, _T("pt")) == 0)
	{
		m_nShellCommand = FilePrintTo;
	}
	else if (lstrcmp(pszParam, _T("p")) == 0)
	{
		m_nShellCommand = FilePrint;
	}
	else if (lstrcmpi(pszParam, _T("Unregister")) == 0 ||
			lstrcmpi(pszParam, _T("Unregserver")) == 0)
	{
		m_nShellCommand = AppUnregister;
	}
	else if (lstrcmp(pszParam, _T("dde")) == 0)
	{
		AfxOleSetUserCtrl(FALSE);
		m_nShellCommand = FileDDE;
	}
	else if (lstrcmpi(pszParam, _T("Embedding")) == 0)
	{
		AfxOleSetUserCtrl(FALSE);
		m_bRunEmbedded = TRUE;
		m_bShowSplash = FALSE;
	}
	else if (lstrcmpi(pszParam, _T("Automation")) == 0)
	{
		AfxOleSetUserCtrl(FALSE);
		m_bRunAutomated = TRUE;
		m_bShowSplash = FALSE;
	}
	else if (lstrcmpi(pszParam, _T("Nologo")) == 0)
	{
		m_bShowSplash = FALSE;
	}
	else if (lstrcmpi(pszParam, _T("N")) == 0)
	{
		m_bShowSplash = FALSE;
		m_nShellCommand = FileNew;
	}
}

void CMyCommandLineInfo::ParseParamNotFlag(const TCHAR* pszParam)
{
	if (m_strFileName.IsEmpty())
		m_strFileName = pszParam;
	else if (m_nShellCommand == FilePrintTo && m_strPrinterName.IsEmpty())
		m_strPrinterName = pszParam;
	else if (m_nShellCommand == FilePrintTo && m_strDriverName.IsEmpty())
		m_strDriverName = pszParam;
	else if (m_nShellCommand == FilePrintTo && m_strPortName.IsEmpty())
		m_strPortName = pszParam;
	else if (m_nShellCommand == FileNothing
			|| m_nShellCommand == FileOpen
			|| m_nShellCommand == FileNew)
	{
		m_strFileName += ';';
		m_strFileName += pszParam;
	}
}

void CMyCommandLineInfo::ParseLast(BOOL bLast)
{
	if (bLast)
	{
		if ((m_nShellCommand == FileNew || m_nShellCommand == FileNothing)
			&& !m_strFileName.IsEmpty())
		{
			m_nShellCommand = FileOpen;
		}
		m_bShowSplash = m_bShowSplash && !m_bRunEmbedded && !m_bRunAutomated;
	}
}

BOOL CWaveSoapFrontApp::InitInstance()
{
	InitCommonControls();
	BaseClass::InitInstance();
#ifdef _DEBUG
	LoadLibrary(_T("thrdtime.dll"));
#endif
	EnableTaskbarInteraction();

	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	m_hWMVCORE_DLL_Handle = LoadLibrary(_T("WMVCORE.DLL"));

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("AleGr SoftWare"));

	// this must be the first item to add. It will become the last item in the list
	Profile.AddItem(_T("Settings"), _T("ProductKey"), m_UserKey, _T("Unregistered"));
	Profile.AddItem(_T("Settings"), _T("CurrentDir"), m_CurrentDir, _T("."));
	if (0) if (m_CurrentDir != ".")
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

	PersistentUndoRedo::LoadData(Profile);
	PersistentFileParameters::LoadData(Profile);

	Profile.AddBoolItem(_T("Settings"), _T("DontShowMediaPlayerWarning"), m_DontShowMediaPlayerWarning, FALSE);

	Profile.AddBoolItem(_T("Settings"), _T("SnapMouseSelectionToMax"), m_bSnapMouseSelectionToMax, TRUE);

	Profile.AddItem(_T("Settings"), _T("ShowToolbar"), m_bShowToolbar, true);
	Profile.AddItem(_T("Settings"), _T("ShowStatusBar"), m_bShowStatusBar, true);
	Profile.AddItem(_T("Settings"), _T("OpenChildMaximized"), m_bOpenChildMaximized, true);
	Profile.AddItem(_T("Settings"), _T("ShowNewFormatDialogWhenShiftOnly"), m_bShowNewFormatDialogWhenShiftOnly, false);
	Profile.AddItem(_T("Settings"), _T("Allow4GbWavFile"), m_bAllow4GbWavFile, false);

	Profile.AddItem(_T("Settings"), _T("NewFileLength"), m_NewFileLength, 10, 0, 4800);
	Profile.AddItem(_T("Settings"), _T("NewFileSampleRate"), m_NewFileFormat.nSamplesPerSec, 44100, 1, 1000000);

	Profile.AddItem(_T("Settings"), _T("SpectrumSectionWidth"), m_SpectrumSectionWidth, 100, 1, 1000);
	Profile.AddItem(_T("Settings"), _T("FftBandsOrder"), m_FftBandsOrder, 9, 6, 13);
	Profile.AddItem(_T("Settings"), _T("FftWindowType"), m_FftWindowType, 0, 0, 2);
	Profile.AddItem(_T("Settings"), _T("DefaultOpenMode"), m_DefaultOpenMode, DefaultOpenBuffered, 0, 2);

	Profile.AddItem(_T("Settings"), _T("MaxMemoryFileSize"), m_MaxMemoryFileSize, 64, 1, 1024);
	Profile.AddItem(_T("Settings"), _T("Allow4GbWavFile"), m_bAllow4GbWavFile, FALSE);
	Profile.AddItem(_T("Settings"), _T("MetaTextEncoding"), CMmioFile::m_TextEncodingInFiles, 0, 0, 2);

	Profile.AddItem(_T("Settings"), _T("PlaybackDevice"), m_DefaultPlaybackDevice, WAVE_MAPPER, WAVE_MAPPER, 64);
	Profile.AddItem(_T("Settings"), _T("NumPlaybackBuffers"), m_NumPlaybackBuffers, 4, 2, 32);
	Profile.AddItem(_T("Settings"), _T("SizePlaybackBuffers"), m_SizePlaybackBuffers, 0x10000, 0x1000, 0x40000);

	Profile.AddItem(_T("Settings"), _T("RecordingDevice"), m_DefaultRecordDevice, WAVE_MAPPER, WAVE_MAPPER, 64);
	Profile.AddItem(_T("Settings"), _T("NumRecordBuffers"), m_NumRecordBuffers, 8, 2, 32);
	Profile.AddItem(_T("Settings"), _T("SizeRecordBuffers"), m_SizeRecordBuffers, 0x10000, 0x1000, 0x40000);
	Profile.AddItem(_T("Settings"), _T("MaxFileCache"), m_MaxFileCache, 64, 1, 512);

	Profile.AddItem(_T("Settings"), _T("PasteMode"), m_DefaultPasteMode, 0, 0, 2);
	Profile.AddItem(_T("Settings"), _T("PasteResampleMode"), m_PasteResampleMode, 0, 0, 1);

	Profile.AddBoolItem(_T("Settings"), _T("UseFadeInOut"), m_UseFadeInOut, FALSE);
	Profile.AddItem(_T("Settings"), _T("FadeInOutLengthMs"), m_FadeInOutLengthMs, 1, 1, 2000);
	Profile.AddItem(_T("Settings"), _T("FadeInOutType"), m_FadeInEnvelope, 2, 1, 3);

	LoadStdProfileSettings(10);  // Load standard INI file options (including MRU)

	if (m_bUseCountrySpecificNumberAndTime)
	{
		LoadLocaleParameters();
	}

	m_FileCache = new CDirectFileCacheProxy(m_MaxFileCache * 0x100000);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	// Only WAV template is added in the list and actually used.
	// All others are used only to hold file type strings.
	// register WAV document
	m_pWavTypeTemplate = new CWaveSoapDocTemplate(
												// second ID - template string
												IDR_WAVESOTYPE, IDR_WAVESOTYPE,
												RUNTIME_CLASS(CWaveSoapFrontDoc),
												RUNTIME_CLASS(CChildFrame), // custom MDI child frame
												RUNTIME_CLASS(CWaveSoapFrontView),
												0);
	AddDocTemplate(m_pWavTypeTemplate);

	// MP3 and WMA are registered even when the decoder is not available
	// register MP3 document
	m_pMP3TypeTemplate = new CWaveSoapDocTemplate(
												IDR_WAVESOTYPE, IDR_MP3TYPE,
												RUNTIME_CLASS(CWaveSoapFrontDoc),
												RUNTIME_CLASS(CChildFrame), // custom MDI child frame
												RUNTIME_CLASS(CWaveSoapFrontView),
												OpenDocumentMp3File);

	AddDocTemplate(m_pMP3TypeTemplate);

	// register WMA document
	m_pWmaTypeTemplate = new CWaveSoapDocTemplate(
												IDR_WAVESOTYPE, IDR_WMATYPE,
												RUNTIME_CLASS(CWaveSoapFrontDoc),
												RUNTIME_CLASS(CChildFrame), // custom MDI child frame
												RUNTIME_CLASS(CWaveSoapFrontView),
												OpenDocumentWmaFile);

	AddDocTemplate(m_pWmaTypeTemplate);

	// register AVI document
	m_pAviTypeTemplate = new CWaveSoapDocTemplate(
												IDR_WAVESOTYPE, IDR_AVITYPE,
												RUNTIME_CLASS(CWaveSoapFrontDoc),
												RUNTIME_CLASS(CChildFrame), // custom MDI child frame
												RUNTIME_CLASS(CWaveSoapFrontView),
												OpenDocumentAviFile | DocumentFlagOpenOnly);

	AddDocTemplate(m_pAviTypeTemplate);

	// register all Windows media documents
	m_pAllWmTypeTemplate = new CWaveSoapDocTemplate(
													IDR_WAVESOTYPE, IDR_WMTYPES,
													RUNTIME_CLASS(CWaveSoapFrontDoc),
													RUNTIME_CLASS(CChildFrame), // custom MDI child frame
													RUNTIME_CLASS(CWaveSoapFrontView),
													OpenDocumentWmaFile | DocumentFlagOpenOnly);

	AddDocTemplate(m_pAllWmTypeTemplate);

	// register RAW document
	m_pRawTypeTemplate = new CWaveSoapDocTemplate(
												IDR_WAVESOTYPE, IDR_RAWTYPE,
												RUNTIME_CLASS(CWaveSoapFrontDoc),
												RUNTIME_CLASS(CChildFrame), // custom MDI child frame
												RUNTIME_CLASS(CWaveSoapFrontView),
												OpenDocumentRawFile);

	AddDocTemplate(m_pRawTypeTemplate);

	// register All types document, don't add it to Doc template list!
	m_pAllTypesTemplate = new CWaveSoapDocTemplate(
													IDR_WAVESOTYPE, IDR_ALLTYPES,
													RUNTIME_CLASS(CWaveSoapFrontDoc),
													RUNTIME_CLASS(CChildFrame), // custom MDI child frame
													RUNTIME_CLASS(CWaveSoapFrontView),
													OpenDocumentRawFile | DocumentFlagOpenOnly);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Parse command line for standard shell commands, DDE, file open
	CMyCommandLineInfo cmdInfo;
	cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
	ParseCommandLine(cmdInfo);

	// start the processing thread
	m_hThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_RunThread = true;
	m_Thread.CreateThread(0, 0x10000);

	m_pMainWnd->DragAcceptFiles();
	// The main window has been initialized, so show and update it.

	pMainFrame->InitialShowWindow(SW_SHOWDEFAULT);
	pMainFrame->UpdateWindow();

	m_NotEnoughMemoryMsg.LoadString(IDS_NOT_ENOUGH_MEMORY);


	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
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
	afx_msg void OnButtonMailto();
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
	ON_BN_CLICKED(IDC_BUTTON_MAILTO, OnButtonMailto)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CAboutDlg::OnButtonMailto()
{
	SHELLEXECUTEINFO shex;
	memzero(shex);
	shex.cbSize = sizeof shex;
	shex.hwnd = NULL;//AfxGetMainWnd()->m_hWnd;

	CString Subj;
	CWnd * pWnd = GetDlgItem(IDC_STATIC_VERSION);
	if (NULL != pWnd)
	{
		pWnd->GetWindowText(Subj);
	}
	else
	{
		Subj = _T("WaveSoap");
	}
	CString file(_T("mailto:alegr@earthlink.net?Subject="));
	file += Subj;
	shex.lpFile = file;
	shex.nShow = SW_SHOWDEFAULT;
	ShellExecuteEx( & shex);
	EndDialog(IDOK);
}

// App command to run the dialog
void CWaveSoapFrontApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

CDocument* CWaveSoapFrontApp::OpenDocumentFile(LPCTSTR lpszPathName, int flags)
{
	ASSERT(m_pDocManager != NULL);
	CWaveSoapDocManager * pMyDocMan =
		dynamic_cast<CWaveSoapDocManager *>(m_pDocManager);
	if (NULL != pMyDocMan)
	{
		TRACE("Using CWaveSoapDocManager::OpenDocumentFile(lpszPathName)\n");
		// lpszPathName may contain several names separated by ';'
		if (NULL == lpszPathName)
		{
			return pMyDocMan->OpenDocumentFile(NULL, flags);
		}
		CDocument * pDoc = NULL;
		CDocument * pTmpDoc;
		while (*lpszPathName != 0)
		{
			int length;
			for (length = 0; lpszPathName[length] != 0 && lpszPathName[length] != ';'; length++)
			{}
			CString name(lpszPathName, length);
			pTmpDoc = pMyDocMan->OpenDocumentFile(name, flags);
			if (NULL != pTmpDoc)
			{
				pDoc = pTmpDoc;
			}
			lpszPathName += length;
			if (*lpszPathName == ';')
			{
				lpszPathName++;
			}
		}
		return pDoc;
	}
	else
	{
		TRACE("Using m_pDocManager->OpenDocumentFile(lpszPathName)\n");
		return m_pDocManager->OpenDocumentFile(lpszPathName);
	}
}

CDocument* CWaveSoapFrontApp::OpenDocumentFile(LPCTSTR lpszPathName)
{
	int flags = 1;
	if (DefaultOpenReadOnly == m_DefaultOpenMode)
	{
		flags |= OpenDocumentReadOnly;
	}
	else if (DefaultOpenDirect == m_DefaultOpenMode)
	{
		flags |= OpenDocumentDirectMode;
	}
	return OpenDocumentFile(lpszPathName, flags);
}

CDocTemplate::Confidence CWaveSoapDocTemplate::MatchDocType(LPCTSTR lpszPathName,
															CDocument*& rpDocMatch)
{
	Confidence conf = BaseClass::MatchDocType(lpszPathName, rpDocMatch);
	if (yesAlreadyOpen == conf)
	{
		return conf;
	}

	// allow multiple suffixes
	// see if it matches our default suffixes
	CString strFilterAllExts;
	if (GetDocString(strFilterAllExts, CDocTemplate::filterExt) &&
		! strFilterAllExts.IsEmpty())
	{
		// see if extension matches
		LPCTSTR lpszDot = ::PathFindExtension(lpszPathName);
		LPCTSTR pExt = strFilterAllExts;
		if (NULL != lpszDot)
		{
			while(0 != pExt[0])
			{
				unsigned i;
				if ('*' == pExt[0])
				{
					pExt++;
					continue;
				}
				ASSERT('.' == pExt[0]);
				for (i = 0; pExt[i] != ';' && pExt[i] != ',' && pExt[i] != 0; i++)
				{
				}
				if (0 == _tcsnicmp(lpszDot, pExt, i))
				{
					return yesAttemptNative; // extension matches, looks like ours
				}
				pExt += i;
				if (pExt[0] != 0)
				{
					pExt++;
				}
			}
		}
	}

	// otherwise we will guess it may work
	return yesAttemptForeign;
}

CDocument* CWaveSoapDocTemplate::OpenDocumentFile(LPCTSTR lpszPathName,
												int flags/* BOOL bMakeVisible */)
{
	flags |= m_OpenDocumentFlags;
	CDocument* pJustDocument = CreateNewDocument();
	BOOL bMakeVisible = flags & 1;
	WAVEFORMATEX * pWfx = NULL;

	NewFileParameters * pParams = NULL;

	if (flags & OpenDocumentCreateNewWithParameters)
	{
		pParams = (NewFileParameters *) lpszPathName;
		pWfx = pParams->pWf;
		lpszPathName = NULL;
		flags &= ~ OpenDocumentCreateNewWithParameters;
	}
	if (pJustDocument == NULL)
	{
		TRACE0("CDocTemplate::CreateNewDocument returned NULL.\n");
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		return NULL;
	}
	CWaveSoapFrontDoc * pDocument =
		dynamic_cast<CWaveSoapFrontDoc *>(pJustDocument);
	if (NULL == pDocument)
	{
		TRACE("CWaveSoapDocTemplate::OpenDocumentFile: Wrong document class created\n");
		delete pJustDocument;
		return NULL;
	}
	ASSERT_VALID(pDocument);
	// modified version of original CMultiDocTemplate::OpenDocumentFile
	// Document is created and initialized first, then the frame is created
	if (lpszPathName == NULL)
	{
#if 0
		// avoid creating temporary compound file when starting up invisible
		if (!bMakeVisible)
			pDocument->m_bEmbedded = TRUE;
#endif
		if ( ! pDocument->OnNewDocument(pParams))
		{
			// user has be alerted to what failed in OnNewDocument
			TRACE0("CDocument::OnNewDocument returned FALSE.\n");
			delete pDocument;       // explicit delete on error
			return NULL;
		}

		// create a new document - with default document name
		if (NULL != pParams->m_pInitialName)
		{
			pDocument->SetPathName(pParams->m_pInitialName, TRUE);
		}
		else
		{
			SetDefaultTitle(pDocument);
			m_nUntitledCount++;
		}

		// it worked, now bump untitled count
	}
	else
	{
		// convert the name to full long pathname
		TCHAR LongPath[512];
		TCHAR FullPath[512];
		LPTSTR pFilePart;
		if (GetFullPathName(lpszPathName, 512, FullPath, &pFilePart)
			&& GetLongPathName(FullPath, LongPath, 512))
		{
			lpszPathName = LongPath;
		}
		// Check if the document is already open
		POSITION pos = GetFirstDocPosition();
		while (pos != NULL)
		{
			CDocument * pTmpDoc = GetNextDoc(pos);
			if (0 == pTmpDoc->GetPathName().CompareNoCase(lpszPathName))
			{
				POSITION viewpos = pTmpDoc->GetFirstViewPosition();
				if (viewpos)
				{
					CView * pView = pTmpDoc->GetNextView(viewpos);
					((CMDIChildWnd*)pView->GetParentFrame())->MDIActivate();
				}
				delete pDocument;   // don't need it anymore
				return pTmpDoc;
			}
		}

		// open an existing document
		CWaitCursor wait;

		if (!pDocument->OnOpenDocument(lpszPathName, flags))
		{
			// user has be alerted to what failed in OnOpenDocument
			TRACE0("CDocument::OnOpenDocument returned FALSE.\n");
			delete pDocument;       // explicit delete on error
			return NULL;
		}
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
	if (m_Thread.m_hThread)
	{
		m_RunThread = false;
#ifdef _DEBUG
		DWORD Time = timeGetTime();
		TRACE("Signalled App thread stop\n");
#endif
		SetEvent(m_hThreadEvent);
		if (WAIT_TIMEOUT == WaitForSingleObjectAcceptSends(m_Thread.m_hThread, 20000))
		{
			TRACE("Terminating App Thread\n");
			TerminateThread(m_Thread.m_hThread, ~0UL);
		}
#ifdef _DEBUG
		TRACE("App Thread finished in %d ms\n",
			timeGetTime() - Time);
#endif
	}
	CloseHandle(m_hThreadEvent);
	m_hThreadEvent = NULL;

	LPTSTR dirbuf = m_CurrentDir.GetBuffer(MAX_PATH+1);
	if (dirbuf)
	{
		GetCurrentDirectory(MAX_PATH+1, dirbuf);
		m_CurrentDir.ReleaseBuffer();
	}
	for (int i = 0; i < countof(AppColors); i++)
	{
		AppColors[i] &= 0x00FFFFFF;
	}

	Profile.UnloadAll();
	// must close the file before deleting the cache
	m_ClipboardFile.Close();
	delete m_FileCache;
	m_FileCache = NULL;

	if (NULL != m_hWMVCORE_DLL_Handle)
	{
		FreeLibrary(m_hWMVCORE_DLL_Handle);
		m_hWMVCORE_DLL_Handle = NULL;
	}

	delete m_pAllTypesTemplate; // need to delete, because it's not added to list of templates

	m_Palette.DeleteObject();
	return BaseClass::ExitInstance();
}

void CWaveSoapFrontApp::QueueOperation(COperationContext * pContext)
{
	m_OpList.InsertTail(pContext);
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
		if ( ! m_OpList.IsEmpty())
		{
			CSimpleCriticalSectionLock lock(m_OpList);
			// find if stop requested for any document

			for (pContext = m_OpList.First();
				m_OpList.NotEnd(pContext); pContext = m_OpList.Next(pContext))
			{
				if ((pContext->m_Flags & OperationContextStopRequested)
					|| pContext->m_pDocument->m_StopOperation)
				{
					break;
				}
			}

			if (m_OpList.IsEnd(pContext))
			{
				// Find if there is an operation for the active document
				for (pContext = m_OpList.First();
					m_OpList.NotEnd(pContext); pContext = m_OpList.Next(pContext))
				{
					if (pContext->m_pDocument == m_pActiveDocument)
					{
						break;
					}
				}
				// But if it is clipboard operation,
				// the first clipboard op will be executed instead
				if (m_OpList.NotEnd(pContext)
					&& (pContext->m_Flags & OperationContextClipboard))
				{
					for (pContext = m_OpList.First();
						m_OpList.NotEnd(pContext); pContext = m_OpList.Next(pContext))
					{
						if (pContext->m_Flags & OperationContextClipboard)
						{
							break;
						}
					}
				}
				if (m_OpList.IsEnd(pContext))
				{
					pContext = m_OpList.First();
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
				pContext->m_Flags |= OperationContextInitialized;
				NeedKickIdle = true;
			}

			if (pContext->m_pDocument->m_StopOperation)
			{
				pContext->m_Flags |= OperationContextStopRequested;
			}

			int LastPercent = pContext->PercentCompleted();
			if ( 0 == (pContext->m_Flags & (OperationContextStop | OperationContextFinished)))
			{
				if ( ! pContext->OperationProc())
				{
					pContext->m_Flags |= OperationContextStop;
				}
			}

			int NewPercent = pContext->PercentCompleted();
			// signal for status update
			if (LastPercent != NewPercent)
			{
				NeedKickIdle = true;
			}

			if (pContext->m_Flags & (OperationContextStop | OperationContextFinished))
			{
				// remove the context from the list and delete the context
				m_OpList.RemoveEntry(pContext);

				bool ClipboardCreationAborted = 0 == (pContext->m_Flags & OperationContextFinished)
												&& 0 != (pContext->m_Flags & OperationContextWriteToClipboard);

				// send a signal to the document, that the operation completed
				SetStatusStringAndDoc(pContext->GetCompletedStatusString(),
									pContext->m_pDocument);

				pContext->DeInit();

				pContext->Retire();     // puts it in the document queue
				// send a signal to the document, that the operation completed
				NeedKickIdle = true;    // this will reenable all commands

				if (ClipboardCreationAborted)
				{
					// remove all operations that use the clipboard, to the next clipboard create operation
					for (pContext = m_OpList.First();
						m_OpList.NotEnd(pContext); )
					{
						COperationContext * pNext = m_OpList.Next(pContext);
						if (pContext->m_Flags & OperationContextWriteToClipboard)
						{
							break;
						}

						if (pContext->m_Flags & OperationContextClipboard)
						{
							m_OpList.RemoveEntry(pContext);
							pContext->Retire();
						}

						pContext = pNext;
					}
				}
			}
			else
			{
				if (NeedKickIdle)
				{
					CString s;
					s.Format(_T("%s %d%%"),
							(LPCTSTR)pContext->GetStatusString(), NewPercent);
					SetStatusStringAndDoc(s, pContext->m_pDocument);
				}
			}
			continue;
		}
		WaitForSingleObject(m_hThreadEvent, 1000);
	}
	return 0;
}

void _AfxAppendFilterSuffix(CString& filter, OPENFILENAME& ofn,
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
		if (pstrDefaultExt != NULL)
			pstrDefaultExt->Empty();

		// add to filter
		filter += strFilterName;
		ASSERT(!filter.IsEmpty());  // must have a file type name
		filter += (TCHAR)'\0';  // next string please

		CString strExtension;

		for (int iStart = 0; strExtension = strFilterExt.Tokenize( _T( ";" ), iStart ), iStart != -1; )
		{

			// a file based document template - add to filter list

			// If you hit the following ASSERT, your document template
			// string is formatted incorrectly.  The section of your
			// document template string that specifies the allowable file
			// extensions should be formatted as follows:
			//    .<ext1>;.<ext2>;.<ext3>
			// Extensions may contain wildcards (e.g. '?', '*'), but must
			// begin with a '.' and be separated from one another by a ';'.
			// Example:
			//    .jpg;.jpeg
			ASSERT(strExtension[0] == '.');
			if ((pstrDefaultExt != NULL) && pstrDefaultExt->IsEmpty())
			{
				// set the default extension
				*pstrDefaultExt = strExtension.Mid( 1 );  // skip the '.'
				ofn.lpstrDefExt = const_cast< LPTSTR >((LPCTSTR)(*pstrDefaultExt));
				ofn.nFilterIndex = ofn.nMaxCustFilter + 1;  // 1 based number
			}

			filter += (TCHAR)'*';
			filter += strExtension;
			filter += (TCHAR)';';  // Always append a ';'.  The last ';' will get replaced with a '\0' later.
		}

		filter.SetAt( filter.GetLength()-1, '\0' );;  // Replace the last ';' with a '\0'
		ofn.nMaxCustFilter++;
	}
}

void CWaveSoapDocManager::OnFileOpen()
{
	CWaveSoapFileOpenDialog dlgFile;

	CString title("Open");
	CString fileNameBuf;
	//VERIFY(title.LoadString(nIDSTitle));

	dlgFile.m_ofn.Flags |=
		OFN_HIDEREADONLY
		| OFN_FILEMUSTEXIST
		| OFN_EXPLORER
		| OFN_ENABLESIZING
		| OFN_ENABLETEMPLATE
		| OFN_ALLOWMULTISELECT;


	CString strFilter;
	CString strDefault;
	// do for all doc template
	CThisApp * pApp = GetApp();
	if (pApp->m_pAllTypesTemplate)
	{
		_AfxAppendFilterSuffix(strFilter, dlgFile.m_ofn, pApp->m_pAllTypesTemplate, & strDefault);
	}
	if (pApp->m_pWavTypeTemplate)
	{
		_AfxAppendFilterSuffix(strFilter, dlgFile.m_ofn, pApp->m_pWavTypeTemplate, NULL);
	}

	dlgFile.m_MinWmaFilter = dlgFile.m_ofn.nMaxCustFilter;
	if (pApp->m_pMP3TypeTemplate)
	{
		_AfxAppendFilterSuffix(strFilter, dlgFile.m_ofn, pApp->m_pMP3TypeTemplate, NULL);
	}
	if (pApp->m_pWmaTypeTemplate)
	{
		_AfxAppendFilterSuffix(strFilter, dlgFile.m_ofn, pApp->m_pWmaTypeTemplate, NULL);
	}
	if (pApp->m_pAviTypeTemplate)
	{
		_AfxAppendFilterSuffix(strFilter, dlgFile.m_ofn, pApp->m_pAviTypeTemplate, NULL);
	}
	if (pApp->m_pAllWmTypeTemplate)
	{
		_AfxAppendFilterSuffix(strFilter, dlgFile.m_ofn, pApp->m_pAllWmTypeTemplate, NULL);
	}
	dlgFile.m_MaxWmaFilter = dlgFile.m_ofn.nMaxCustFilter;

	if (pApp->m_pRawTypeTemplate)
	{
		_AfxAppendFilterSuffix(strFilter, dlgFile.m_ofn, pApp->m_pRawTypeTemplate, NULL);
	}

	// append the "*.*" all files filter
	CString allFilter;
	VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));
	strFilter += allFilter;
	strFilter += (TCHAR)'\0';   // next string please
	strFilter += _T("*.*");
	strFilter += (TCHAR)'\0';   // last string
	dlgFile.m_ofn.nMaxCustFilter++;

	dlgFile.m_bReadOnly = (pApp->m_bReadOnly != 0);
	dlgFile.m_bDirectMode = (pApp->m_bDirectMode != 0);
	dlgFile.m_ofn.lpstrFilter = strFilter;
	dlgFile.m_ofn.lpstrTitle = title;
	dlgFile.m_ofn.lpstrFile = fileNameBuf.GetBuffer(0x2000);   // 8K
	dlgFile.m_ofn.nMaxFile = 0x2000-1;

	dlgFile.m_ofn.nFilterIndex = pApp->m_OpenFileDialogFilter;
	if (dlgFile.m_ofn.nFilterIndex > dlgFile.m_ofn.nMaxCustFilter)
	{
		dlgFile.m_ofn.nFilterIndex = 1;
	}

	int nResult = dlgFile.DoModal();
	if (nResult != IDOK)
	{
		fileNameBuf.ReleaseBuffer(0x2000-1);
		return;
	}

	pApp->m_OpenFileDialogFilter = dlgFile.m_ofn.nFilterIndex;

	pApp->m_bReadOnly = dlgFile.m_bReadOnly;
	pApp->m_bDirectMode = dlgFile.m_bDirectMode;
	int flags = 1;
	if (dlgFile.m_bReadOnly)
	{
		flags |= OpenDocumentReadOnly;
	}
	if (dlgFile.m_bDirectMode)
	{
		flags |= OpenDocumentDirectMode;
	}

	POSITION pos = dlgFile.GetStartPosition();
	while (pos != NULL)
	{
		// check how many names selected,
		// open all of selected files
		CString fileName = dlgFile.GetNextPathName(pos);
		TRACE(_T("Opening file: %s\n"), LPCTSTR(fileName));
		pApp->OpenDocumentFile(fileName, flags);
	}
	fileNameBuf.ReleaseBuffer();
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

void CWaveSoapFrontStatusBar::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
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

	case ID_INDICATOR_SCALE:
	{
		CFrameWnd * pFrame = GetParentFrame();
		if (NULL != pFrame)
		{
			CWnd * pClient = pFrame->GetDlgItem(AFX_IDW_PANE_FIRST);
			if (NULL != pClient)
			{
				CWaveSoapFrontView * pView =
					dynamic_cast<CWaveSoapFrontView *>(pClient->GetDlgItem(CWaveMDIChildClient::WaveViewID));
				if (NULL != pView)
				{
					switch (pView->GetHorizontalScale())
					{
					case 1:
						id = IDR_MENU_POPUP_HOR_SCALE1;
						break;
					case 2:
						id = IDR_MENU_POPUP_HOR_SCALE2;
						break;
					case 4:
						id = IDR_MENU_POPUP_HOR_SCALE4;
						break;
					case 8:
						id = IDR_MENU_POPUP_HOR_SCALE8;
						break;
					case 16:
						id = IDR_MENU_POPUP_HOR_SCALE16;
						break;
					case 32:
						id = IDR_MENU_POPUP_HOR_SCALE32;
						break;
					case 64:
						id = IDR_MENU_POPUP_HOR_SCALE64;
						break;
					case 128:
						id = IDR_MENU_POPUP_HOR_SCALE128;
						break;
					case 256:
						id = IDR_MENU_POPUP_HOR_SCALE256;
						break;
					case 512:
						id = IDR_MENU_POPUP_HOR_SCALE512;
						break;
					case 1024:
						id = IDR_MENU_POPUP_HOR_SCALE1024;
						break;
					case 2048:
						id = IDR_MENU_POPUP_HOR_SCALE2048;
						break;
					case 4096:
						id = IDR_MENU_POPUP_HOR_SCALE4096;
						break;
					default:
					case 8192:
						id = IDR_MENU_POPUP_HOR_SCALE8192;
						break;
					}
					break;
				}
			}
		}
	}
		Default();
		return;
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

void SetStatusString(CCmdUI* pCmdUI, UINT id,
					LPCTSTR MaxString, BOOL bForceSize)
{
	SetStatusString(pCmdUI, LoadCString(id), MaxString, bForceSize);
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
			CGdiObjectSaveT<CFont> OldFont(dc, dc.SelectObject(pFont));

			VERIFY(::GetTextExtentPoint32(dc,
										MaxString, _tcslen(MaxString), & size));
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

void CWaveSoapFrontApp::OnFileNew()
{
	POSITION pos = m_pDocManager->GetFirstDocTemplatePosition();
	CDocTemplate* pTemplate = m_pDocManager->GetNextDocTemplate(pos);

	if (pTemplate != NULL)
	{

		if (! m_bShowNewFormatDialogWhenShiftOnly
			|| (0x8000 & GetKeyState(VK_SHIFT)))
		{
			CNewFilePropertiesDlg dlg(m_NewFileFormat.nSamplesPerSec,
									m_NewFileFormat.nChannels, m_NewFileLength,
									m_bShowNewFormatDialogWhenShiftOnly);

			if (IDOK != dlg.DoModal())
			{
				return;
			}

			m_NewFileLength = dlg.GetLengthSeconds();   // in seconds

			m_bShowNewFormatDialogWhenShiftOnly = dlg.ShowWhenShiftOnly();

			m_NewFileFormat.nSamplesPerSec = dlg.GetSamplingRate();

			m_NewFileFormat.nChannels = dlg.NumberOfChannels();
		}

		CWaveFormat wf;
		wf.InitFormat(WAVE_FORMAT_PCM, m_NewFileFormat.nSamplesPerSec,
					m_NewFileFormat.nChannels);

		NewFileParameters Params(wf,
								m_NewFileLength * m_NewFileFormat.nSamplesPerSec);

		pTemplate->OpenDocumentFile((LPCTSTR) & Params,
									OpenDocumentCreateNewWithParameters);

	}
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
		NewFileParameters Params(m_ClipboardFile.GetWaveFormat());

		CWaveSoapFrontDoc * pDoc =
			(CWaveSoapFrontDoc *)pTemplate->OpenDocumentFile(
															(LPCTSTR) & Params,
															OpenDocumentCreateNewWithParameters);

		if (NULL != pDoc)
		{
			BOOL TmpUndo = pDoc->UndoEnabled();
			pDoc->EnableUndo(FALSE);

			if ( ! pDoc->DoPaste(0, 0, ALL_CHANNELS, 0))
			{
				pDoc->OnCloseDocument();
			}
			else
			{
				pDoc->EnableUndo(TmpUndo);
			}
		}
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
		m_ReadOnlyFileSuffix.LoadString(IDS_MRU_LIST_RO_SUFFIX);
		m_DirectFileSuffix.LoadString(IDS_MRU_LIST_DIRECT_SUFFIX);
	}
	BOOL GetDisplayName(CString& strName, int nIndex,
						LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName = TRUE) const;
	virtual void UpdateMenu(CCmdUI* pCmdUI);
	virtual ~CWaveSoapFileList() {}
	virtual void Add(LPCTSTR lpszPathName);
	virtual void Add(LPCTSTR lpszPathName, LPCTSTR lpszAppID)
	{
		Add(lpszPathName);
	}
protected:

	CString m_ReadOnlyFileSuffix;
	CString m_DirectFileSuffix;
};

BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
UINT AFXAPI AfxGetFileTitle(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);
UINT AFXAPI AfxGetFileName(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax);
BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);

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
		lstrcpy(lpszCanon, (bAtLeastName) ? lpszFileName : _T(""));
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
												nMaxMRU, 45);
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

	CThisApp * pApp = GetApp();

	LPTSTR lpch = strName.GetBuffer(_MAX_PATH+1);
	lpch[_MAX_PATH] = 0;

	LPCTSTR suffix = _T("");

	LPCTSTR src = m_arrNames[nIndex];
	TCHAR flags = src[0];
	//TRACE("First byte of name #%d = %02x\n", nIndex, flags);
	if (flags <= 0x1F)
	{
		if ((flags & OpenDocumentModeFlagsMask) == OpenDocumentDefaultMode)
		{
			if (pApp->m_bReadOnly)
			{
				suffix = m_ReadOnlyFileSuffix;
			}
			else if (pApp->m_bDirectMode)
			{
				suffix = m_DirectFileSuffix;
			}
		}
		else
		{
			if (0 != (flags & OpenDocumentReadOnly))
			{
				suffix = m_ReadOnlyFileSuffix;
			}
			else if (flags & OpenDocumentDirectMode)
			{
				suffix = m_DirectFileSuffix;
			}
		}
		src++;
	}

	strName = src;
	CPath dir(strName);

	// copy the full path, otherwise abbreviate the name
	if (dir.RemoveFileSpec()
		&& 0 == dir.m_strPath.CompareNoCase(lpszCurDir))
	{
		dir = src;
		dir.StripPath();
		strName = LPCTSTR(dir);
	}
	else if (m_nMaxDisplayLength != -1)
	{
		// abbreviate name based on what will fit in limited space
		_AfxAbbreviateName(strName.GetBuffer(MAX_PATH), m_nMaxDisplayLength, bAtLeastName);
		strName.ReleaseBuffer();
	}
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
	{
		pCmdUI->m_pMenu->DeleteMenu(pCmdUI->m_nID + iMRU, MF_BYCOMMAND);
	}

	TCHAR szCurDir[_MAX_PATH + 2];
	GetCurrentDirectory(_MAX_PATH, szCurDir);
	int nCurDir = lstrlen(szCurDir);
	ASSERT(nCurDir >= 0);
	szCurDir[nCurDir] = '\\';
	szCurDir[++nCurDir] = '\0';

	CString strName;
	CString strTemp;
	for (int iMRU = 0; iMRU < m_nSize; iMRU++)
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

void CWaveSoapFileList::Add(LPCTSTR lpszPathName)
{
	ASSERT(m_arrNames != NULL);
	ASSERT(lpszPathName != NULL);
	ASSERT(AfxIsValidString(lpszPathName));

	// fully qualify the path name
	TCHAR szTemp[_MAX_PATH+1];
	szTemp[0] = lpszPathName[0];
	AfxFullPath(szTemp+1, lpszPathName+1);

	// update the MRU list, if an existing MRU string matches file name
	int iMRU;
	for (iMRU = 0; iMRU < m_nSize-1; iMRU++)
	{
		CString MruName = m_arrNames[iMRU];
		if (MruName.GetLength() > 1
			&& szTemp[0] == MruName[0]
			&& AfxComparePath(LPCTSTR(MruName)+1, szTemp+1))
			break;      // iMRU will point to matching entry
	}
	// move MRU strings before this one down
	for (; iMRU > 0; iMRU--)
	{
		ASSERT(iMRU > 0);
		ASSERT(iMRU < m_nSize);
		m_arrNames[iMRU] = m_arrNames[iMRU-1];
	}
	// place this one at the beginning
	m_arrNames[0] = szTemp;
}

/////////////////////////////////////////////////////////////////////////////
// MRU file list default implementation

BOOL CWaveSoapFrontApp::OnOpenRecentFile(UINT nID)
{
	ASSERT_VALID(this);
	ASSERT(m_pRecentFileList != NULL);

	ASSERT(nID >= ID_FILE_MRU_FILE1);
	ASSERT(nID < ID_FILE_MRU_FILE1 + (UINT)m_pRecentFileList->GetSize());
	int nIndex = nID - ID_FILE_MRU_FILE1;
	CString name = (*m_pRecentFileList)[nIndex];
	ASSERT(name.GetLength() > 1);
	if (name.GetLength() <= 1)
	{
		m_pRecentFileList->Remove(nIndex);
		return FALSE;
	}

	TRACE2("MRU: open file (%d) '%s'.\n", (nIndex) + 1,
			1 + (LPCTSTR)(name));
	int flags = 1 | name[0];
	if (OpenDocumentFile(1 + LPCTSTR(name), flags) == NULL)
	{
		m_pRecentFileList->Remove(nIndex);
	}

	return TRUE;
}

CDocument* CWaveSoapDocManager::OpenDocumentFile(LPCTSTR lpszFileName, int flags)
{
	// find the highest confidence
	POSITION pos = m_templateList.GetHeadPosition();
	CDocTemplate::Confidence bestMatch = CDocTemplate::noAttempt;
	CDocTemplate* pBestTemplate = NULL;
	CDocument* pOpenDocument = NULL;

	TCHAR szPath[_MAX_PATH];
	ASSERT(lstrlen(lpszFileName) < countof(szPath));
	TCHAR szTemp[_MAX_PATH];
	if (lpszFileName[0] == '\"')
		++lpszFileName;
	lstrcpyn(szTemp, lpszFileName, _MAX_PATH);
	LPTSTR lpszLast = _tcsrchr(szTemp, '\"');
	if (lpszLast != NULL)
		*lpszLast = 0;
	AfxFullPath(szPath, szTemp);
	TCHAR szLinkName[_MAX_PATH];
	if (AfxResolveShortcut(AfxGetMainWnd(), szPath, szLinkName, _MAX_PATH))
	{
		lstrcpy(szPath, szLinkName);
	}

	while (pos != NULL)
	{
		CDocTemplate* pTemplate = (CDocTemplate*)m_templateList.GetNext(pos);
		ASSERT_KINDOF(CDocTemplate, pTemplate);
		CDocTemplate::Confidence match;

		ASSERT(pOpenDocument == NULL);
		match = pTemplate->MatchDocType(szPath, pOpenDocument);
		if (match > bestMatch)
		{
			bestMatch = match;
			pBestTemplate = pTemplate;
		}
		if (match == CDocTemplate::yesAlreadyOpen)
			break;      // stop here
	}

	if (pOpenDocument != NULL)
	{
		POSITION pos = pOpenDocument->GetFirstViewPosition();
		if (pos != NULL)
		{
			CView* pView = pOpenDocument->GetNextView(pos); // get first one
			ASSERT_VALID(pView);
			CFrameWnd* pFrame = pView->GetParentFrame();
			if (pFrame != NULL)
				pFrame->ActivateFrame();
			else
				TRACE0("Error: Can not find a frame for document to activate.\n");
			CFrameWnd* pAppFrame;
			if (pFrame != (pAppFrame = (CFrameWnd*)AfxGetApp()->m_pMainWnd))
			{
				ASSERT_KINDOF(CFrameWnd, pAppFrame);
				pAppFrame->ActivateFrame();
			}
		}
		else
		{
			TRACE0("Error: Can not find a view for document to activate.\n");
		}
		return pOpenDocument;
	}

	if (pBestTemplate == NULL)
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_OPEN_DOC);
		return NULL;
	}
	if (bestMatch != CDocTemplate::yesAttemptNative)
	{
		flags |= OpenDocumentRawFile;
	}
	return pBestTemplate->OpenDocumentFile(szPath, flags);
}

void NotEnoughMemoryMessageBox()
{
	MessageBoxSync(GetApp()->m_NotEnoughMemoryMsg, MB_ICONEXCLAMATION | MB_OK);
}

void NotEnoughDiskSpaceMessageBox()
{
	MessageBoxSync(IDS_NOT_ENOUGH_DISK_SPACE);
}

void NotEnoughUndoSpaceMessageBox()
{
	AfxMessageBox(IDS_NOT_ENOUGH_UNDO_SPACE);
}

void FileCreationErrorMessageBox(LPCTSTR name)
{
	DWORD error = GetLastError();
	CString s;
	UINT FormatId = 0;
	if (error == ERROR_DISK_FULL)
	{
		FormatId = IDS_NOT_ENOUGH_DISK_SPACE;
	}
	else if (error == ERROR_SHARING_VIOLATION)
	{
		FormatId = IDS_OVERWRITE_SHARING_VIOLATION;
	}
	else if (error == ERROR_ACCESS_DENIED
			|| error == ERROR_FILE_READ_ONLY)
	{
		FormatId = IDS_OVERWRITE_ACCESS_DENIED;
		if (NULL == name)
		{
			FormatId = IDS_OVERWRITE_ACCESS_DENIED_TEMP;
		}
	}
	else
	{
		FormatId = IDS_UNKNOWN_FILE_CREATION_ERROR;
		if (NULL == name)
		{
			FormatId = IDS_UNKNOWN_FILE_CREATION_ERROR_TEMP;
		}
	}
	s.Format(FormatId, name);
	AfxMessageBox(s);
}

void CWaveSoapFrontApp::CreatePalette()
{
	struct {
		LOGPALETTE lp;
		PALETTEENTRY pe[255];
	} p = {0x300, 0};
	TRACE("CMainFrame::CreateFftPalette\n");
	CWaveFftView::FillLogPalette( & p.lp, countof(p.pe));
	// add wave color entries and make negative entries
	PALETTEENTRY SysColors[20];
	CDC * pDC = AfxGetMainWnd()->GetDC();
	TRACE("Number of system colors = %d\n", GetSystemPaletteEntries(*pDC, 0, 40, NULL));
	GetSystemPaletteEntries(*pDC, 0, 10, SysColors);
	GetSystemPaletteEntries(*pDC, 246, 10, &SysColors[10]);
	AfxGetMainWnd()->ReleaseDC(pDC);
	for (int k = 0, n = 256 / 2; k < countof(AppColors); k++)
	{
		// if the color is in system palette, don't put it to the application palette
		AppColors[k] &= 0x00FFFFFF;
		int i;
		for (i = 0; i < 20; i++)
		{
			if ((reinterpret_cast<DWORD &>(SysColors[i]) & 0x00FFFFFF)
				== (AppColors[k] & 0x00FFFFFF))
			{
				break;
			}
		}
		if (i == 20)
		{
			n--;
			p.lp.palPalEntry[n] = reinterpret_cast<PALETTEENTRY &>(AppColors[k]);
			p.lp.palPalEntry[n].peFlags = PC_NOCOLLAPSE;
			AppColors[k] |= 0x02000000;
			TRACE("Application color %d (%06X) NOT in system palette\n", k, AppColors[k]);
		}
		else
		{
			AppColors[k] &= ~0x02000000;
			TRACE("Application color %d (%06X) is in system palette\n", k, AppColors[k]);
		}
	}
	// total 236 entries, each half is 118
	for (int i = 10; i < 128; i++)
	{
		p.lp.palPalEntry[255-i].peFlags = p.lp.palPalEntry[i].peFlags;
		p.lp.palPalEntry[255-i].peRed = ~p.lp.palPalEntry[i].peRed;
		p.lp.palPalEntry[255-i].peGreen = ~p.lp.palPalEntry[i].peGreen;
		p.lp.palPalEntry[255-i].peBlue = ~p.lp.palPalEntry[i].peBlue;
	}

	for (int i = 0; i < 10; i++)
	{
		p.lp.palPalEntry[i].peFlags = 0;
		p.lp.palPalEntry[i].peRed = SysColors[i].peRed;
		p.lp.palPalEntry[i].peGreen = SysColors[i].peGreen;
		p.lp.palPalEntry[i].peBlue = SysColors[i].peBlue;
	}
	p.lp.palNumEntries = 246;
	for (int i = 0; i < 20; i++)
	{
		TRACE("System color %d=%08X\n", i, SysColors[i]);
	}
	m_Palette.CreatePalette( & p.lp);
}

CPalette * CWaveSoapFrontApp::GetPalette()
{

	if (NULL == HPALETTE(m_Palette))
	{
		CreatePalette();
	}
	return & m_Palette;
}

void CWaveSoapFrontApp::BroadcastUpdate(UINT lHint)
{
	CWaveSoapDocManager * pDocManager = dynamic_cast<CWaveSoapDocManager *>(m_pDocManager);
	if (pDocManager != NULL)
	{
		pDocManager->BroadcastUpdate(lHint);
	}
}

void CWaveSoapDocManager::BroadcastUpdate(UINT lHint)
{
	POSITION pos = m_templateList.GetHeadPosition();
	while (pos != NULL)
	{
		CWaveSoapDocTemplate* pTemplate = dynamic_cast<CWaveSoapDocTemplate*>((CDocTemplate*)m_templateList.GetNext(pos));
		if (NULL != pTemplate)
		{
			pTemplate->BroadcastUpdate(lHint);
		}
	}
}

void CWaveSoapDocManager::SaveAll()
{
	POSITION pos = m_templateList.GetHeadPosition();
	while (pos != NULL)
	{
		CWaveSoapDocTemplate* pTemplate = dynamic_cast<CWaveSoapDocTemplate*>((CDocTemplate*)m_templateList.GetNext(pos));
		if (NULL != pTemplate)
		{
			pTemplate->SaveAll();
		}
	}
}

void CWaveSoapDocTemplate::BroadcastUpdate(UINT lHint)
{
	POSITION pos = GetFirstDocPosition();
	while (pos != NULL)
	{
		CDocument* pDoc = GetNextDoc(pos);
		pDoc->UpdateAllViews(NULL, lHint);
	}
}

void CWaveSoapDocTemplate::SaveAll()
{
	POSITION pos = GetFirstDocPosition();
	while (pos != NULL)
	{
		CWaveSoapFrontDoc * pDoc = dynamic_cast<CWaveSoapFrontDoc *> (GetNextDoc(pos));
		if (NULL != pDoc
			&& pDoc->IsModified()
			&& ! pDoc->IsBusy())
		{
			pDoc->DoFileSave();
		}
	}
}

//#include <C:\WINDDK\6001.18000\inc\api\devioctl.h>
//#include <ntddcdrm.h>
//#include <winioctl.h>

void CWaveSoapFrontApp::OnToolsCdgrab()
{
	CCdGrabbingDialog dlg;

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	const int MaxTracks = 99;

	CCdReadingContext * pContexts[MaxTracks];
	CCdReadingContext * pContext;
	memzero(pContexts);

	unsigned t, n;
	for (t = 0, n = 0; t < dlg.m_Tracks.size() && t < MaxTracks; t++)
	{
		// create a new document
		CdTrackInfo * pTrack = & dlg.m_Tracks[t];
		if ( ! pTrack->Checked)
		{
			continue;
		}

		// allocate a context
		CString s;
		s.Format(IDS_READING_CD_TRACK_STATUS_PROMPT, t + 1);

		pContext = new CCdReadingContext(NULL, s, LoadCString(IDS_READING_CD_TRACK_OPERATION_NAME));

		// TODO: exception safe operation
		if (n > 0)
		{
			pContexts[n - 1]->m_pNextTrackContext = pContext;
		}

		pContext->m_RequiredReadSpeed = dlg.m_SelectedReadSpeed;
		pContext->m_OriginalReadSpeed = dlg.m_CurrentReadSpeed;
		pContexts[n] = pContext;
		n++;
		if ( ! pContext->InitTrackInformation(dlg.m_pCdDrive,
											pTrack, dlg.m_FileTypeFlags, dlg.m_Wf))
		{
			delete pContexts[0];    // will delete all of them
			return;
		}
		pContext->m_bSaveImmediately = ! dlg.m_RadioOpenInEditor;
	}

	if (n > 0)
	{
		pContexts[0]->Execute();
	}
	return;
}

void CWaveSoapFrontApp::OnActivateDocument(CWaveSoapFrontDoc *pDocument, BOOL bActivate)
{
	if (bActivate)
	{
		if (pDocument != m_pActiveDocument)
		{
			m_pActiveDocument = pDocument;
			pDocument->OnActivateDocument(TRUE);
		}
	}
	else
	{
		if (pDocument == m_pActiveDocument)
		{
			m_pActiveDocument = NULL;
			pDocument->OnActivateDocument(FALSE);
		}
	}
}

BOOL CWaveSoapFrontApp::GetMessageString(UINT nID, CString& rMessage)
{
	if (nID < ID_FILE_MRU_FILE1
		|| nID > ID_FILE_MRU_FILE16)
	{
		return FALSE;
	}
	// form the string from the recent file name
	nID -= ID_FILE_MRU_FILE1;
	if (nID >= UINT(m_pRecentFileList->GetSize()))
	{
		return FALSE;
	}

	LPCTSTR file = m_pRecentFileList->m_arrNames[nID];

	CString suffix;

	TCHAR flags = file[0];

	if (flags <= 0x1F)
	{
		if ((flags & OpenDocumentModeFlagsMask) == OpenDocumentDefaultMode)
		{
			if (m_bReadOnly)
			{
				suffix.LoadString(IDS_STRING_READ_ONLY_PROMPT);
			}
			else if (m_bDirectMode)
			{
				suffix.LoadString(IDS_STRING_DIRECT_MODE_PROMPT);
			}
		}
		else
		{
			if (0 != (flags & OpenDocumentReadOnly))
			{
				suffix.LoadString(IDS_STRING_READ_ONLY_PROMPT);
			}
			else if (flags & OpenDocumentDirectMode)
			{
				suffix.LoadString(IDS_STRING_DIRECT_MODE_PROMPT);
			}
		}
		file++;
	}

	rMessage.Format(IDS_OPEN_RECENT_FILE_PROMPT_FORMAT, file, LPCTSTR(suffix));
	return TRUE;
}

void CWaveSoapFrontApp::GetStatusStringAndDoc(CString & str, CWaveSoapFrontDoc ** ppDoc)
{
	CSimpleCriticalSectionLock lock(m_StatusStringLock);

	str = m_CurrentStatusString;
	*ppDoc = m_pLastStatusDocument;
}

void CWaveSoapFrontApp::SetStatusStringAndDoc(const CString & str, CWaveSoapFrontDoc * pDoc)
{
	CSimpleCriticalSectionLock lock(m_StatusStringLock);

	m_CurrentStatusString = str;
	m_pLastStatusDocument = pDoc;
}


void CWaveSoapFrontApp::OnFileSaveAll()
{
	CWaveSoapDocManager * pDocManager = dynamic_cast<CWaveSoapDocManager *>(m_pDocManager);
	if (pDocManager != NULL)
	{
		pDocManager->SaveAll();
	}
}

void CWaveSoapFrontApp::OnUpdateFileSaveAll(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CanSaveAnyDocument());
}

BOOL CWaveSoapDocManager::IsAnyDocumentModified()
{
	POSITION pos = m_templateList.GetHeadPosition();
	while (pos != NULL)
	{
		CWaveSoapDocTemplate* pTemplate = dynamic_cast<CWaveSoapDocTemplate*>((CDocTemplate*)m_templateList.GetNext(pos));
		if (NULL != pTemplate
			&& pTemplate->IsAnyDocumentModified())
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CWaveSoapDocTemplate::IsAnyDocumentModified()
{
	POSITION pos = GetFirstDocPosition();
	while (pos != NULL)
	{
		CDocument * pDoc = GetNextDoc(pos);
		if (pDoc->IsModified())
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CWaveSoapFrontApp::IsAnyDocumentModified()
{
	CWaveSoapDocManager * pDocManager = dynamic_cast<CWaveSoapDocManager *>(m_pDocManager);
	if (pDocManager != NULL)
	{
		return pDocManager->IsAnyDocumentModified();
	}
	else
	{
		return FALSE;
	}
}

BOOL CWaveSoapDocManager::CanSaveAnyDocument()
{
	POSITION pos = m_templateList.GetHeadPosition();
	while (pos != NULL)
	{
		CWaveSoapDocTemplate* pTemplate = dynamic_cast<CWaveSoapDocTemplate*>((CDocTemplate*)m_templateList.GetNext(pos));
		if (NULL != pTemplate
			&& pTemplate->CanSaveAnyDocument())
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CWaveSoapDocTemplate::CanSaveAnyDocument()
{
	POSITION pos = GetFirstDocPosition();
	while (pos != NULL)
	{
		CWaveSoapFrontDoc * pDoc = dynamic_cast<CWaveSoapFrontDoc *> (GetNextDoc(pos));
		if (NULL != pDoc
			&& pDoc->IsModified()
			&& ! pDoc->IsBusy())
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CWaveSoapFrontApp::CanSaveAnyDocument()
{
	CWaveSoapDocManager * pDocManager = dynamic_cast<CWaveSoapDocManager *>(m_pDocManager);
	if (pDocManager != NULL)
	{
		return pDocManager->CanSaveAnyDocument();
	}
	else
	{
		return FALSE;
	}
}

BOOL CanAllocateWaveFileSamples(const WAVEFORMATEX * pWf, LONGLONG NumOfSamples)
{
	int SampleSize = pWf->wBitsPerSample * pWf->nChannels / 8;
	LONGLONG NewSize = NumOfSamples * SampleSize;
	// reserve 1 megabyte of overhead
	LONGLONG MaxLength = 0x7FFFFFFEi64 - 0x100000;
	if (GetApp()->m_bAllow4GbWavFile)
	{
		MaxLength = 0xFFFFFFFEi64 - 0x100000;
	}
	return NewSize <= MaxLength;
}

BOOL CanAllocateWaveFileSamplesDlg(const WAVEFORMATEX * pWf, LONGLONG NumOfSamples)
{
	if (CanAllocateWaveFileSamples(pWf, NumOfSamples))
	{
		return TRUE;
	}
	AfxMessageBox(IDS_FILE_MAY_GET_TOO_BIG, MB_OK | MB_ICONSTOP);
	return FALSE;
}

BOOL CanExpandWaveFile(const CWaveFile & WaveFile, NUMBER_OF_SAMPLES NumOfSamplesToAdd)
{
	if (NumOfSamplesToAdd <= 0)
	{
		return TRUE;
	}
	LONGLONG NewLength = WaveFile.GetLength() + LONGLONG(NumOfSamplesToAdd) * WaveFile.SampleSize();
	LONGLONG MaxLength = 0x7FFFFFFEi64;
	if (GetApp()->m_bAllow4GbWavFile)
	{
		MaxLength = 0xFFFFFFFEi64 - 0x100000;
	}
	return NewLength <= MaxLength;
}

BOOL CanExpandWaveFileDlg(const CWaveFile & WaveFile, NUMBER_OF_SAMPLES NumOfSamplesToAdd)
{
	if (CanExpandWaveFile(WaveFile, NumOfSamplesToAdd))
	{
		return TRUE;
	}
	AfxMessageBox(IDS_FILE_MAY_GET_TOO_BIG, MB_OK | MB_ICONSTOP);
	return FALSE;
}

void CWaveSoapFrontApp::OnToolsOptions()
{
	CPreferencesPropertySheet dlg(IDS_OPTIONS_CAPTION, NULL, m_LastPrefsPropertyPageSelected);
	dlg.m_FilePage.m_sTempFileLocation = m_sTempDir;
	dlg.m_FilePage.m_DefaultFileOpenMode = m_DefaultOpenMode;
	dlg.m_FilePage.m_bEnable4GbWavFile = m_bAllow4GbWavFile;

	dlg.m_FilePage.m_MaxMemoryFileSize = m_MaxMemoryFileSize;
	dlg.m_FilePage.m_MaxFileCache = m_MaxFileCache;
	dlg.m_FilePage.m_FileTextEncoding = CMmioFile::m_TextEncodingInFiles;

	dlg.m_SoundPage.m_PlaybackDevice = m_DefaultPlaybackDevice + 1;
	dlg.m_SoundPage.m_NumPlaybackBuffers = m_NumPlaybackBuffers;
	dlg.m_SoundPage.m_PlaybackBufferSize = m_SizePlaybackBuffers / 1024;

	dlg.m_SoundPage.m_RecordingDevice = m_DefaultRecordDevice + 1;
	dlg.m_SoundPage.m_NumRecordingBuffers = m_NumRecordBuffers;
	dlg.m_SoundPage.m_RecordingBufferSize = m_SizeRecordBuffers / 1024;

	dlg.m_ViewPage.m_bSnapMouseSelection = m_bSnapMouseSelectionToMax;

	if (IDOK == dlg.DoModal())
	{
		m_sTempDir = dlg.m_FilePage.m_sTempFileLocation;

		m_DefaultOpenMode = dlg.m_FilePage.m_DefaultFileOpenMode;

		m_bAllow4GbWavFile = (0 != dlg.m_FilePage.m_bEnable4GbWavFile);
		m_MaxMemoryFileSize = dlg.m_FilePage.m_MaxMemoryFileSize;
		m_MaxFileCache = dlg.m_FilePage.m_MaxFileCache;
		CMmioFile::m_TextEncodingInFiles = dlg.m_FilePage.m_FileTextEncoding;

		PersistentUndoRedo::SaveData(dlg.m_UndoPage.GetParams());

		m_DefaultPlaybackDevice = dlg.m_SoundPage.m_PlaybackDevice - 1;
		m_NumPlaybackBuffers = dlg.m_SoundPage.m_NumPlaybackBuffers;
		m_SizePlaybackBuffers = dlg.m_SoundPage.m_PlaybackBufferSize * 1024;

		m_DefaultRecordDevice = dlg.m_SoundPage.m_RecordingDevice - 1;
		m_NumRecordBuffers = dlg.m_SoundPage.m_NumRecordingBuffers;
		m_SizeRecordBuffers = dlg.m_SoundPage.m_RecordingBufferSize * 1024;

		m_LastPrefsPropertyPageSelected = dlg.GetLastSelectedPage();

		m_bSnapMouseSelectionToMax = dlg.m_ViewPage.m_bSnapMouseSelection;
	}
}

CString GetSelectionText(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Chan,
						NUMBER_OF_CHANNELS nChannels, BOOL bLockChannels,
						int nSamplesPerSec, int TimeFormat)
{
	CString s;
	if (nChannels > 1)
	{
		CHANNEL_MASK mask = ~(~0L << nChannels);

		CString sChans;
		sChans.LoadString(IDS_STEREO);

		if (! bLockChannels
			&& mask != (Chan & mask))
		{
			if (Chan & (1 << 0))
			{
				sChans.LoadString(IDS_LEFT);
			}
			else if (Chan & (1 << 1))
			{
				sChans.LoadString(IDS_RIGHT);
			}
		}
		s.Format(IDS_SELECTION_STRING_FORMAT_STEREO,
				LPCTSTR(SampleToString(Start, nSamplesPerSec, TimeFormat)),
				LPCTSTR(SampleToString(End, nSamplesPerSec, TimeFormat)),
				LPCTSTR(SampleToString(End - Start, nSamplesPerSec, TimeFormat)),
				LPCTSTR(sChans));
	}
	else
	{
		s.Format(IDS_CHANNELS_STRING_FORMAT_MONO,
				LPCTSTR(SampleToString(Start, nSamplesPerSec, TimeFormat)),
				LPCTSTR(SampleToString(End, nSamplesPerSec, TimeFormat)),
				LPCTSTR(SampleToString(End - Start, nSamplesPerSec, TimeFormat)));
	}
	return s;
}


void CWaveSoapFrontApp::OnFileBatchconversion()
{
	CBatchConvertDlg dlg;
	if (IDOK != dlg.DoModal())
	{
		return;
	}
}

BOOL VerifyCreateDirectory(LPCTSTR pszPath)
{
	// try to use SHCreateDirectoryEx
	static BOOL (_stdcall * SHCreateDirectoryEx)(HWND, LPCTSTR, SECURITY_ATTRIBUTES *)
		= (BOOL (_stdcall * )(HWND, LPCTSTR, SECURITY_ATTRIBUTES *))
			GetProcAddress(GetModuleHandle(_T("Shell32.dll")),
#ifdef _UNICODE
							"SHCreateDirectoryExW"
#else
							"SHCreateDirectoryExA"
#endif
							);

	CString s;
	CString DirPath(pszPath);
	DirPath.TrimRight(_T("\\/"));
	DWORD attr = GetFileAttributes(DirPath);
	if (INVALID_FILE_ATTRIBUTES == attr)
	{
		DWORD error = GetLastError();
		if (ERROR_ACCESS_DENIED == error)
		{
			s.Format(IDS_DIRECTORY_CREATE_ACCESS_DENIED, pszPath);
			AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
			return FALSE;
		}
		s.Format(IDS_DIRECTORY_CREATE_PROMPT, pszPath);
		if (IDYES != AfxMessageBox(s, MB_YESNO | MB_ICONQUESTION))
		{
			return FALSE;
		}
		CString DirPathWithSlash(DirPath + '\\');
		if (NULL != SHCreateDirectoryEx)
		{
			SHCreateDirectoryEx(AfxGetMainWnd()->GetSafeHwnd(),
								DirPath, NULL);
		}
		else
		{
			// SHCreateDirectoryEx is supported on all UNICODE platforms
			MakeSureDirectoryPathExists(CStringA(DirPathWithSlash));
			error = GetLastError();
			if (ERROR_ACCESS_DENIED == error)
			{
				s.Format(IDS_UNABLE_TO_CREATE_DIRECTORY_ACCESSDENIED, pszPath);
				AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}
		}

		if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(DirPath))
		{
			return TRUE;
		}
		UINT format = IDS_UNABLE_TO_CREATE_DIRECTORY;
		if (ERROR_ACCESS_DENIED == error)
		{
			format = IDS_UNABLE_TO_CREATE_DIRECTORY_ACCESSDENIED;
		}
		AfxMessageBox(IDS_UNABLE_TO_CREATE_DIRECTORY, MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}
	else
	{
		if (FILE_ATTRIBUTE_DIRECTORY & attr)
		{
			return TRUE;
		}
		s.Format(IDS_WRONG_DIRECTORY_NAME, pszPath);
		AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}
}

CString LoadCString(UINT id)
{
	return CString(MAKEINTRESOURCE(id));
}

CDocumentPopup::CDocumentPopup(CDocument * pDoc)
{
	m_pPopupFrame = NULL;
	m_pFrameAbove = NULL;

	CMDIChildWnd * pActive = ((CMDIFrameWnd*)AfxGetMainWnd())->MDIGetActive();
	if (NULL == pActive)
	{
		return;
	}
	POSITION pos = pDoc->GetFirstViewPosition();
	CView * pView;
	// check if the top level frame is associated with the document
	while (NULL != (pView = pDoc->GetNextView(pos)))
	{
		CFrameWnd * pFrame = pView->GetParentFrame();
		if (pFrame == pActive)
		{
			return;
		}
	}

	pos = pDoc->GetFirstViewPosition();
	while (NULL != (pView = pDoc->GetNextView(pos)))
	{
		m_pPopupFrame = pView->GetParentFrame();
		if (NULL != m_pPopupFrame)
		{
			m_pFrameAbove = (CFrameWnd *)m_pPopupFrame->GetWindow(GW_HWNDPREV);
			static_cast<CMDIChildWnd*>(m_pPopupFrame)->MDIActivate();
			return;
		}
	}
}

CDocumentPopup::~CDocumentPopup()
{
	// move previously active window under m_pFrameAbove
	if (NULL != m_pFrameAbove
		&& NULL != m_pPopupFrame)
	{
		m_pPopupFrame->SetWindowPos(m_pFrameAbove, 0, 0, 0, 0,
									SWP_NOACTIVATE
									| SWP_NOMOVE
									| SWP_NOOWNERZORDER
									| SWP_NOSIZE);

		CWnd * pTop = m_pPopupFrame->GetWindow(GW_HWNDFIRST);
		static_cast<CMDIChildWnd*>(pTop)->MDIActivate();
	}
}

