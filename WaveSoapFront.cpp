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

#define _countof(array) (sizeof(array)/sizeof(array[0]))

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
BOOL AFXAPI AfxResolveShortcut(CWnd* pWnd, LPCTSTR pszShortcutFile,
								LPTSTR pszPath, int cchPath);
// CWaveSoapFrontApp
class CWaveSoapDocTemplate : public CMultiDocTemplate
{
public:
	CWaveSoapDocTemplate( UINT nIDResource, UINT nIDStringResource,
						CRuntimeClass* pDocClass,
						CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass )
		:CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass),
		m_OpenDocumentFlags(0)
	{
		if ( ! m_strDocStrings.LoadString(nIDStringResource))
		{
			m_strDocStrings.LoadString(m_nIDResource);
		}
	}
	~CWaveSoapDocTemplate() {}
	virtual CDocument* OpenDocumentFile( LPCTSTR lpszPathName, int flags = 1
											//BOOL bMakeVisible = TRUE
										);

	DWORD m_OpenDocumentFlags;
	virtual void OnIdle();
	void BroadcastUpdate(UINT lHint);
	BOOL IsAnyDocumentModified();
	BOOL CanSaveAnyDocument();
	void SaveAll();
};

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

BEGIN_MESSAGE_MAP(CWaveSoapFrontStatusBar, CStatusBar)
	//{{AFX_MSG_MAP(CWaveSoapFrontStatusBar)
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CWaveSoapFrontApp, CWinApp)
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
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
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

	m_TimeSeparator(':'),
	m_DecimalPoint('.'),
	m_ThousandSeparator(','),
	m_bUseCountrySpecificNumberAndTime(false),

	m_bUndoEnabled(true),
	m_bRedoEnabled(true),
	m_bRememberSelectionInUndo(false),
	m_bEnableUndoLimit(true),
	m_bEnableRedoLimit(true),
	m_bEnableUndoDepthLimit(false),
	m_bEnableRedoDepthLimit(false),
	m_bUseMemoryFiles(true),
	m_MaxMemoryFileSize(64),

	m_bShowToolbar(true),
	m_bShowStatusBar(true),
	m_bOpenMaximized(true),
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

	m_DontShowMediaPlayerWarning(FALSE),

	m_bSnapMouseSelectionToMax(TRUE),

	m_hWMVCORE_DLL_Handle(NULL),

	m_bShowNewFormatDialogWhenShiftOnly(false),
	m_NewFileLength(10),

	m_SpectrumSectionWidth(100),
	m_FftBandsOrder(9),
	m_FftWindowType(0),

	m_OpenFileDialogFilter(1)
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
#ifdef _DEBUG
	LoadLibrary("thrdtime.dll");
#endif
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	m_VersionInfo.dwOSVersionInfoSize = sizeof m_VersionInfo;
	GetVersionEx( & m_VersionInfo);
#ifdef _DEBUG
	SupportsV5FileDialog();
#endif
	m_hWMVCORE_DLL_Handle = LoadLibrary(_T("WMVCORE.DLL"));

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
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

	Profile.AddBoolItem(_T("Settings"), _T("UndoEnabled"), m_bUndoEnabled, TRUE);
	Profile.AddBoolItem(_T("Settings"), _T("RedoEnabled"), m_bRedoEnabled, TRUE);
	Profile.AddBoolItem(_T("Settings"), _T("EnableUndoLimit"), m_bEnableUndoLimit, FALSE);
	Profile.AddBoolItem(_T("Settings"), _T("EnableRedoLimit"), m_bEnableRedoLimit, FALSE);
	Profile.AddBoolItem(_T("Settings"), _T("EnableUndoDepthLimit"), m_bEnableUndoDepthLimit, FALSE);
	Profile.AddBoolItem(_T("Settings"), _T("EnableRedoDepthLimit"), m_bEnableRedoDepthLimit, FALSE);
	Profile.AddBoolItem(_T("Settings"), _T("RememberSelectionInUndo"), m_bRememberSelectionInUndo, FALSE);
	Profile.AddItem(_T("Settings"), _T("MaxUndoDepth"), m_MaxUndoDepth, 100, 0, 1000);
	Profile.AddItem(_T("Settings"), _T("MaxRedoDepth"), m_MaxRedoDepth, 100, 0, 1000);
	Profile.AddItem(_T("Settings"), _T("MaxUndoSize"), m_MaxUndoSize, 0x40000000u,
					0u, 0xC0000000u);
	Profile.AddItem(_T("Settings"), _T("MaxRedoSize"), m_MaxRedoSize, 0x40000000u,
					0u, 0xC0000000u);

	Profile.AddBoolItem(_T("Settings"), _T("DontShowMediaPlayerWarning"), m_DontShowMediaPlayerWarning, FALSE);

	Profile.AddBoolItem(_T("Settings"), _T("SnapMouseSelectionToMax"), m_bSnapMouseSelectionToMax, TRUE);

	Profile.AddItem(_T("Settings"), _T("ShowToolbar"), m_bShowToolbar, true);
	Profile.AddItem(_T("Settings"), _T("ShowStatusBar"), m_bShowStatusBar, true);
	Profile.AddItem(_T("Settings"), _T("OpenChildMaximized"), m_bOpenChildMaximized, true);
	Profile.AddItem(_T("Settings"), _T("OpenMaximized"), m_bOpenMaximized, true);
	Profile.AddItem(_T("Settings"), _T("ShowNewFormatDialogWhenShiftOnly"), m_bShowNewFormatDialogWhenShiftOnly, false);
	Profile.AddItem(_T("Settings"), _T("Allow4GbWavFile"), m_bAllow4GbWavFile, false);

	Profile.AddItem(_T("Settings"), _T("NewFileLength"), m_NewFileLength, 10, 0, 4800);
	Profile.AddItem(_T("Settings"), _T("NewFileSampleRate"), m_NewFileFormat.nSamplesPerSec, 44100, 1, 1000000);
	Profile.AddItem(_T("Settings"), _T("NewFileChannels"), m_NewFileChannels, 2, 1, 2);
	m_NewFileFormat.nChannels = m_NewFileChannels;

	Profile.AddItem(_T("Settings"), _T("SpectrumSectionWidth"), m_SpectrumSectionWidth, 100, 1, 1000);
	Profile.AddItem(_T("Settings"), _T("FftBandsOrder"), m_FftBandsOrder, 9, 6, 13);
	Profile.AddItem(_T("Settings"), _T("FftWindowType"), m_FftWindowType, 0, 0, 2);
	Profile.AddItem(_T("Settings"), _T("DefaultOpenMode"), m_DefaultOpenMode, DefaultOpenBuffered, 0, 2);

	Profile.AddBoolItem(_T("Settings"), _T("UseMemoryFiles"), m_bUseMemoryFiles, TRUE);
	Profile.AddItem(_T("Settings"), _T("MaxMemoryFileSize"), m_MaxMemoryFileSize, 64, 1, 1024);
	Profile.AddItem(_T("Settings"), _T("Allow4GbWavFile"), m_bAllow4GbWavFile, FALSE);

	Profile.AddItem(_T("Settings"), _T("PlaybackDevice"), m_DefaultPlaybackDevice, WAVE_MAPPER, WAVE_MAPPER, 64);
	Profile.AddItem(_T("Settings"), _T("NumPlaybackBuffers"), m_NumPlaybackBuffers, 4, 2, 32);
	Profile.AddItem(_T("Settings"), _T("SizePlaybackBuffers"), m_SizePlaybackBuffers, 0x10000, 0x1000, 0x40000);

	Profile.AddItem(_T("Settings"), _T("RecordingDevice"), m_DefaultRecordDevice, WAVE_MAPPER, WAVE_MAPPER, 64);
	Profile.AddItem(_T("Settings"), _T("NumRecordBuffers"), m_NumRecordBuffers, 8, 2, 32);
	Profile.AddItem(_T("Settings"), _T("SizeRecordBuffers"), m_SizeRecordBuffers, 0x10000, 0x1000, 0x40000);
	Profile.AddItem(_T("Settings"), _T("MaxFileCache"), m_MaxFileCache, 64, 1, 512);

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

	m_FileCache = new CDirectFile::CDirectFileCache(m_MaxFileCache * 0x100000);

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
												RUNTIME_CLASS(CWaveSoapFrontView)
												);
	AddDocTemplate(m_pWavTypeTemplate);

	// MP3 and WMA are registered even when the decoder is not available
	// register MP3 document
	m_pMP3TypeTemplate = new CWaveSoapDocTemplate(
												IDR_WAVESOTYPE, IDR_MP3TYPE,
												RUNTIME_CLASS(CWaveSoapFrontDoc),
												RUNTIME_CLASS(CChildFrame), // custom MDI child frame
												RUNTIME_CLASS(CWaveSoapFrontView)
												);
	m_pMP3TypeTemplate->m_OpenDocumentFlags = OpenDocumentMp3File;
	AddDocTemplate(m_pMP3TypeTemplate);

	// register WMA document
	m_pWmaTypeTemplate = new CWaveSoapDocTemplate(
												IDR_WAVESOTYPE, IDR_WMATYPE,
												RUNTIME_CLASS(CWaveSoapFrontDoc),
												RUNTIME_CLASS(CChildFrame), // custom MDI child frame
												RUNTIME_CLASS(CWaveSoapFrontView)
												);
	m_pWmaTypeTemplate->m_OpenDocumentFlags = OpenDocumentWmaFile;
	AddDocTemplate(m_pWmaTypeTemplate);

	// register WMA document
	m_pRawTypeTemplate = new CWaveSoapDocTemplate(
												IDR_WAVESOTYPE, IDR_RAWTYPE,
												RUNTIME_CLASS(CWaveSoapFrontDoc),
												RUNTIME_CLASS(CChildFrame), // custom MDI child frame
												RUNTIME_CLASS(CWaveSoapFrontView)
												);
	m_pRawTypeTemplate->m_OpenDocumentFlags = OpenDocumentRawFile;
	AddDocTemplate(m_pRawTypeTemplate);

	// register All types document
	m_pAllTypesTemplate = new CWaveSoapDocTemplate(
													IDR_WAVESOTYPE, IDR_ALLTYPES,
													RUNTIME_CLASS(CWaveSoapFrontDoc),
													RUNTIME_CLASS(CChildFrame), // custom MDI child frame
													RUNTIME_CLASS(CWaveSoapFrontView)
													);
	m_pAllTypesTemplate->m_OpenDocumentFlags = OpenDocumentRawFile;

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
	int nCmdShow = SW_SHOWDEFAULT;
	if (m_bOpenMaximized)
	{
		nCmdShow = SW_SHOWMAXIMIZED;
	}
	pMainFrame->ShowWindow(nCmdShow);
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
	memset( & shex, 0, sizeof shex);
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

CDocument* CWaveSoapDocTemplate::OpenDocumentFile(LPCTSTR lpszPathName,
												int flags/* BOOL bMakeVisible */)
{
	flags |= m_OpenDocumentFlags;
	CDocument* pJustDocument = CreateNewDocument();
	BOOL bMakeVisible = flags & 1;
	WAVEFORMATEX * pWfx = NULL;
	if (flags & OpenDocumentCreateNewWithWaveformat)
	{
		pWfx = (WAVEFORMATEX *) lpszPathName;
		lpszPathName = NULL;
		flags &= ~ OpenDocumentCreateNewWithWaveformat;
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
		// create a new document - with default document name
		CThisApp * pApp = GetApp();
		SetDefaultTitle(pDocument);
		if (flags & OpenDocumentCreateNewQueryFormat
			&& NULL != pWfx)
		{
			if (! pApp->m_bShowNewFormatDialogWhenShiftOnly
				|| (0x8000 & GetKeyState(VK_SHIFT)))
			{
				CNewFilePropertiesDlg dlg;
				dlg.m_bShowOnlyWhenShift = pApp->m_bShowNewFormatDialogWhenShiftOnly;
				dlg.m_nSamplingRate = pWfx->nSamplesPerSec;
				dlg.m_MonoStereo = (pWfx->nChannels == 2);
				dlg.m_Length = pApp->m_NewFileLength;   // in seconds
				if (IDOK != dlg.DoModal())
				{
					delete pDocument;
					return NULL;
				}

				pApp->m_NewFileLength = dlg.m_Length;   // in seconds
				pApp->m_bShowNewFormatDialogWhenShiftOnly = (dlg.m_bShowOnlyWhenShift != 0);
				pWfx->nSamplesPerSec = dlg.m_nSamplingRate;
				if (dlg.m_MonoStereo)
				{
					pWfx->nChannels = 2;
				}
				else
				{
					pWfx->nChannels = 1;
				}
			}
		}
		// avoid creating temporary compound file when starting up invisible
		if (!bMakeVisible)
			pDocument->m_bEmbedded = TRUE;
		long length = pApp->m_NewFileLength;
		if (flags & OpenNewDocumentZeroLength)
		{
			length = 0;
		}
		if (!pDocument->OnNewDocument(pWfx, length))
		{
			// user has be alerted to what failed in OnNewDocument
			TRACE0("CDocument::OnNewDocument returned FALSE.\n");
			delete pDocument;       // explicit delete on error
			return NULL;
		}

		// it worked, now bump untitled count
		m_nUntitledCount++;
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
		// TODO: Check if the document is already open
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
		CThisApp * pApp = GetApp();
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
	// TODO: Add your specialized code here and/or call the base class
	if (m_Thread.m_hThread)
	{
		m_RunThread = false;
#ifdef _DEBUG
		DWORD Time = timeGetTime();
		TRACE("Signalled App thread stop\n");
#endif
		SetEvent(m_hThreadEvent);
		if (WAIT_TIMEOUT == WaitForSingleObject(m_Thread.m_hThread, 5000))
		{
			TRACE("Terminating App Thread\n");
			TerminateThread(m_Thread.m_hThread, -1);
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
	for (int i = 0; i < sizeof AppColors / sizeof AppColors[0]; i++)
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
	delete m_pAllTypesTemplate;

	// those are deleted by doc manager:
	//delete m_pMP3TypeTemplate;
	//delete m_pWavTypeTemplate;
	//delete m_pWmaTypeTemplate;
	//delete m_pRawTypeTemplate;
	m_Palette.DeleteObject();
	return CWinApp::ExitInstance();
}

void CWaveSoapFrontApp::QueueOperation(COperationContext * pContext)
{
	{
		// add the operation to the tail
		CSimpleCriticalSectionLock lock(m_cs);
		m_OpList.InsertTail(pContext);
	}
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
			CSimpleCriticalSectionLock lock(m_cs);
			// find if stop requested for any document
			pContext = m_OpList.Next();
			while (pContext != m_OpList.Head())
			{
				if ((pContext->m_Flags & OperationContextStopRequested)
					|| pContext->pDocument->m_StopOperation)
				{
					break;
				}
				pContext = pContext->Next();
			}
			if (pContext == m_OpList.Head())
			{
				// Find if there is an operation for the active document
				pContext = m_OpList.Next();
				while (pContext != m_OpList.Head())
				{
					if (pContext->pDocument == m_pActiveDocument)
					{
						break;
					}
					pContext = pContext->Next();
				}
				// But if it is clipboard operation,
				// the first clipboard op will be executed instead
				if (pContext != m_OpList.Head()
					&& (pContext->m_Flags & OperationContextClipboard))
				{
					pContext = m_OpList.Next();
					while (pContext != m_OpList.Head())
					{
						if (pContext->m_Flags & OperationContextClipboard)
						{
							break;
						}
						pContext = pContext->Next();
					}
				}
				if (pContext == m_OpList.Head())
				{
					pContext = m_OpList.Next();
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
					pContext->m_Flags |= OperationContextInitFailed | OperationContextStop;
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
				{
					CSimpleCriticalSectionLock lock(m_cs);
					pContext->RemoveFromList();
				}

				// send a signal to the document, that the operation completed
				SetStatusStringAndDoc(pContext->GetStatusString() + _T("Completed"),
									pContext->pDocument);
				pContext->DeInit();

				pContext->Retire();     // usually deletes it
				// send a signal to the document, that the operation completed
				NeedKickIdle = true;    // this will reenable all commands
			}
			else
			{
				if (NeedKickIdle)
				{
					CString s;
					s.Format(_T("%s%d%%"),
							(LPCTSTR)pContext->GetStatusString(), pContext->PercentCompleted);
					SetStatusStringAndDoc(s, pContext->pDocument);
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
	CWaveSoapFileOpenDialog dlgFile(TRUE);

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

	if (CThisApp::SupportsV5FileDialog())
	{
		dlgFile.m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG_OPEN_TEMPLATE_V5);
	}
	else
	{
		dlgFile.m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG_OPEN_TEMPLATE_V4);
	}

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
		TRACE("Opening file: %s\n", LPCTSTR(fileName));
		pApp->OpenDocumentFile(fileName, flags);
	}
	fileNameBuf.ReleaseBuffer();
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

void CWaveSoapFrontApp::OnFileNew()
{
	POSITION pos = m_pDocManager->GetFirstDocTemplatePosition();
	CDocTemplate* pTemplate = m_pDocManager->GetNextDocTemplate(pos);
	if (pTemplate != NULL)
	{
		//CWaveSoapFrontDoc * pDoc =
		(CWaveSoapFrontDoc *)pTemplate->OpenDocumentFile(
														(LPCTSTR) & (GetApp()->m_NewFileFormat),
														OpenDocumentCreateNewWithWaveformat | OpenDocumentCreateNewQueryFormat);
		m_NewFileChannels = m_NewFileFormat.nChannels;
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
		WAVEFORMATEX * pWfx = m_ClipboardFile.GetWaveFormat();

		CWaveSoapFrontDoc * pDoc =
			(CWaveSoapFrontDoc *)pTemplate->OpenDocumentFile((LPCTSTR) pWfx,
															OpenDocumentCreateNewWithWaveformat | OpenNewDocumentZeroLength);

		if (NULL != pDoc)
		{
			BOOL TmpUndo = pDoc->UndoEnabled();
			pDoc->EnableUndo(FALSE);
			pDoc->DoEditPaste();
			pDoc->EnableUndo(TmpUndo);
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
	}
	BOOL GetDisplayName(CString& strName, int nIndex,
						LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName = TRUE) const;
	virtual void UpdateMenu(CCmdUI* pCmdUI);
	virtual ~CWaveSoapFileList() {}
	virtual void Add(LPCTSTR lpszPathName);
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
	//TRACE("First byte of name #%d = %02x\n", nIndex, flags);
	if (flags <= 0x1F)
	{
		if (flags & OpenDocumentReadOnly)
		{
			suffix = " (RO)";
		}
		else if (flags & OpenDocumentDirectMode)
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

	TCHAR szCurDir[_MAX_PATH + 2];
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

void CWaveSoapFileList::Add(LPCTSTR lpszPathName)
{
	ASSERT(m_arrNames != NULL);
	ASSERT(lpszPathName != NULL);
	ASSERT(AfxIsValidString(lpszPathName));

	// fully qualify the path name
	TCHAR szTemp[_MAX_PATH];
	szTemp[0] = lpszPathName[0];
	AfxFullPath(szTemp+1, lpszPathName+1);

	// update the MRU list, if an existing MRU string matches file name
	for (int iMRU = 0; iMRU < m_nSize-1; iMRU++)
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
	ASSERT(lstrlen(lpszFileName) < _countof(szPath));
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
		lstrcpy(szPath, szLinkName);

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
	AfxMessageBox(GetApp()->m_NotEnoughMemoryMsg, MB_ICONEXCLAMATION | MB_OK);
}

void NotEnoughDiskSpaceMessageBox()
{
	AfxMessageBox(IDS_NOT_ENOUGH_DISK_SPACE);
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
		FormatId = IDS_OVERWRITE_SHARING_VIOLATION;
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
	CWaveFftView::FillLogPalette( & p.lp, sizeof p.pe / sizeof p.pe[0]);
	// add wave color entries and make negative entries
	PALETTEENTRY SysColors[20];
	CDC * pDC = AfxGetMainWnd()->GetDC();
	TRACE("Number of system colors = %d\n", GetSystemPaletteEntries(*pDC, 0, 40, NULL));
	GetSystemPaletteEntries(*pDC, 0, 10, SysColors);
	GetSystemPaletteEntries(*pDC, 246, 10, &SysColors[10]);
	AfxGetMainWnd()->ReleaseDC(pDC);
	for (int k = 0, n = 256 / 2; k < sizeof AppColors / sizeof AppColors[0]; k++)
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
	for (i = 0; i < 10; i++)
	{
		p.lp.palPalEntry[i].peFlags = 0;
		p.lp.palPalEntry[i].peRed = SysColors[i].peRed;
		p.lp.palPalEntry[i].peGreen = SysColors[i].peGreen;
		p.lp.palPalEntry[i].peBlue = SysColors[i].peBlue;
	}
	p.lp.palNumEntries = 246;
	for (i = 0; i < 20; i++)
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

#include <devioctl.h>
#include <ntddcdrm.h>
#include <winioctl.h>

void CWaveSoapFrontApp::OnToolsCdgrab()
{
#ifdef _DEBUG
	CCdGrabbingDialog dlg;
	dlg.DoModal();
	return;
	HANDLE hCD = NULL;
	hCD = CreateFile(
					"G:\\TRACK01.CDA",
					//"R:",
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);
	if (INVALID_HANDLE_VALUE == hCD || NULL == hCD)
	{
		TRACE("Couldn't open CD,error=%d\n",GetLastError());
		return;
	}
	TRACE("Handle %x opened, device type=%d\n", hCD, GetFileType(hCD));
	CDROM_TOC toc;
	DWORD dwReturned;
	BOOL res = DeviceIoControl(hCD, IOCTL_CDROM_READ_TOC,
								NULL, 0,
								& toc, sizeof toc,
								& dwReturned,
								NULL);
	TRACE("Get TOC IoControl returned %x, bytes: %d, First track %d, last track: %d\n",
		res, dwReturned, toc.FirstTrack, toc.LastTrack);

	STORAGE_DEVICE_NUMBER devnum;
	res = DeviceIoControl(hCD, IOCTL_STORAGE_GET_DEVICE_NUMBER,
						NULL, 0,
						& devnum, sizeof devnum,
						& dwReturned,
						NULL);
	TRACE("Get device number IoControl returned %x, bytes: %d, devtype=%d, DeviceNumber=%d, partitionNumber=%d\n",
		res, dwReturned, devnum.DeviceType, devnum.DeviceNumber, devnum.PartitionNumber);

#if 0
	CDROM_AUDIO_CONTROL cac;
	res = DeviceIoControl(hCD, IOCTL_CDROM_GET_CONTROL,
						NULL, 0,
						& cac, sizeof cac,
						& dwReturned,
						NULL);
	TRACE("Get control IoControl returned %x, bytes: %d, Format=%d, Bloskc per second=%d\n",
		res, dwReturned, cac.LbaFormat, cac.LogicalBlocksPerSecond);
	DISK_GEOMETRY dg;
	res = DeviceIoControl(hCD, IOCTL_CDROM_GET_DRIVE_GEOMETRY,
						NULL, 0,
						& dg, sizeof dg,
						& dwReturned,
						NULL);
	TRACE("Get geometry IoControl returned %x, bytes: %d, cylinders=%d, media type=%d"
		"tracks per cyl=%d, spt=%d, sector size=%d\n",
		res, dwReturned, long(dg.Cylinders.QuadPart), dg.MediaType, dg.TracksPerCylinder,
		dg.SectorsPerTrack, dg.BytesPerSector);
#endif

	// can't read more than 65536 bytes
	const int SectorSize = 2352;
	const int NumberOfSectors = 0x10000 / SectorSize;
	void * data1 = VirtualAlloc(NULL, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	RAW_READ_INFO rri = {{0, 0}, NumberOfSectors, CDDA};
	rri.DiskOffset.QuadPart = SectorSize*75*30;
	BYTE * data = (BYTE *)data1;
	res = DeviceIoControl(hCD, IOCTL_CDROM_RAW_READ,
						& rri, sizeof rri,
						data1, SectorSize*NumberOfSectors,
						& dwReturned,
						NULL);
	TRACE("Raw read IoControl returned %x, bytes: %d, last error=%d, data=%02X%02X%02X%02X\n",
		res, dwReturned, GetLastError(), data[3], data[2], data[1], data[0]);
	VirtualFree(data1, 0, MEM_RELEASE);
	CloseHandle(hCD);
#endif
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

void CWaveSoapFrontApp::GetStatusStringAndDoc(CString & str, CWaveSoapFrontDoc ** ppDoc)
{
	m_StatusStringLock.Lock();

	str = m_CurrentStatusString;
	*ppDoc = m_pLastStatusDocument;

	m_StatusStringLock.Unlock();
}

void CWaveSoapFrontApp::SetStatusStringAndDoc(const CString & str, CWaveSoapFrontDoc * pDoc)
{
	m_StatusStringLock.Lock();

	m_CurrentStatusString = str;
	m_pLastStatusDocument = pDoc;

	m_StatusStringLock.Unlock();
}

void AddStringToHistory(const CString & str, CString history[], int NumItems, bool CaseSensitive)
{
	// remove those that match the currently selected dirs
	int i, j;
	for (i = 0, j = 0; i < NumItems; i++)
	{
		if (CaseSensitive)
		{
			if (0 == str.Compare(history[i]))
			{
				continue;
			}
		}
		else
		{
			if (0 == str.CompareNoCase(history[i]))
			{
				continue;
			}
		}
		if (i != j)
		{
			history[j] = history[i];
		}
		j++;
	}
	// remove last dir from the list
	for (i = NumItems - 1; i >= 1; i--)
	{
		history[i] = history[i - 1];
	}
	history[0] = str;
}

OSVERSIONINFO CWaveSoapFrontApp::m_VersionInfo;

bool CWaveSoapFrontApp::SupportsV5FileDialog()
{
	TRACE("Major version=%d, minor version=%d, build=%X\n",
		m_VersionInfo.dwMajorVersion,
		m_VersionInfo.dwMinorVersion,
		m_VersionInfo.dwBuildNumber);
	switch (m_VersionInfo.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_NT:
		return m_VersionInfo.dwMajorVersion >= 5;
		break;
	case VER_PLATFORM_WIN32_WINDOWS:
		return m_VersionInfo.dwMajorVersion > 4
				|| (m_VersionInfo.dwMajorVersion == 4
					&& m_VersionInfo.dwMinorVersion >= 90);
		break;
	default:
		return FALSE;

	}
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

BOOL CanExpandWaveFile(const CWaveFile & WaveFile, long NumOfSamplesToAdd)
{
	if (NumOfSamplesToAdd <= 0)
	{
		return TRUE;
	}
	LONGLONG NewLength = WaveFile.GetLength() + LONGLONG(NumOfSamplesToAdd) * WaveFile.SampleSize();
	LONGLONG MaxLength = 0x7FFFFFFEi64;
	if (GetApp()->m_bAllow4GbWavFile)
	{
		MaxLength = 0xFFFFFFFEi64;
	}
	return NewLength <= MaxLength;
}

BOOL CanExpandWaveFileDlg(const CWaveFile & WaveFile, long NumOfSamplesToAdd)
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
	CPreferencesPropertySheet dlg(IDS_OPTIONS_CAPTION);
	dlg.m_FilePage.m_sTempFileLocation = m_sTempDir;
	dlg.m_FilePage.m_bEnableUndo = m_bUndoEnabled;
	dlg.m_FilePage.m_bEnableRedo = m_bRedoEnabled;
	dlg.m_FilePage.m_UndoDepthLimit = m_MaxUndoDepth;
	dlg.m_FilePage.m_RedoDepthLimit = m_MaxRedoDepth;
	dlg.m_FilePage.m_UndoSizeLimit = m_MaxUndoSize / 0x100000;
	dlg.m_FilePage.m_RedoSizeLimit = m_MaxRedoSize / 0x100000;
	dlg.m_FilePage.m_bLimitUndoSize = m_bEnableUndoLimit;
	dlg.m_FilePage.m_bLimitRedoSize = m_bEnableRedoLimit;
	dlg.m_FilePage.m_bLimitUndoDepth = m_bEnableUndoDepthLimit;
	dlg.m_FilePage.m_bLimitRedoDepth = m_bEnableRedoDepthLimit;
	dlg.m_FilePage.m_bRememberSelectionInUndo = m_bRememberSelectionInUndo;
	dlg.m_FilePage.m_DefaultFileOpenMode = m_DefaultOpenMode;
	dlg.m_FilePage.m_bEnable4GbWavFile = m_bAllow4GbWavFile;
	dlg.m_FilePage.m_UseMemoryFiles = m_bUseMemoryFiles;
	dlg.m_FilePage.m_MaxMemoryFileSize = m_MaxMemoryFileSize;
	dlg.m_FilePage.m_MaxFileCache = m_MaxFileCache;

	dlg.m_SoundPage.m_PlaybackDevice = m_DefaultPlaybackDevice;
	dlg.m_SoundPage.m_NumPlaybackBuffers = m_NumPlaybackBuffers;
	dlg.m_SoundPage.m_PlaybackBufferSize = m_SizePlaybackBuffers / 1024;
	dlg.m_SoundPage.m_RecordingDevice = m_DefaultRecordDevice;
	dlg.m_SoundPage.m_NumRecordingBuffers = m_NumRecordBuffers;
	dlg.m_SoundPage.m_RecordingBufferSize = m_SizeRecordBuffers / 1024;


#if 0
	dlg.m_ViewPage.m_bSnapMouseSelection = m_bSnapMouseSelectionToMax;
#endif

	if (IDOK == dlg.DoModal())
	{
		m_sTempDir = dlg.m_FilePage.m_sTempFileLocation;
		m_bUndoEnabled = dlg.m_FilePage.m_bEnableUndo;
		m_bRedoEnabled = dlg.m_FilePage.m_bEnableRedo;
		m_MaxUndoDepth = dlg.m_FilePage.m_UndoDepthLimit;
		m_MaxRedoDepth = dlg.m_FilePage.m_RedoDepthLimit;
		m_MaxUndoSize = dlg.m_FilePage.m_UndoSizeLimit * 0x100000;
		m_MaxRedoSize = dlg.m_FilePage.m_RedoSizeLimit * 0x100000;
		m_bEnableUndoLimit = dlg.m_FilePage.m_bLimitUndoSize;
		m_bEnableRedoLimit = dlg.m_FilePage.m_bLimitRedoSize;
		m_bEnableUndoDepthLimit = dlg.m_FilePage.m_bLimitUndoDepth;
		m_bEnableRedoDepthLimit = dlg.m_FilePage.m_bLimitRedoDepth;
		m_bRememberSelectionInUndo = dlg.m_FilePage.m_bRememberSelectionInUndo;
		m_DefaultOpenMode = dlg.m_FilePage.m_DefaultFileOpenMode;
		m_bAllow4GbWavFile = (0 != dlg.m_FilePage.m_bEnable4GbWavFile);
		m_bUseMemoryFiles = dlg.m_FilePage.m_UseMemoryFiles;
		m_MaxMemoryFileSize = dlg.m_FilePage.m_MaxMemoryFileSize;
		m_MaxFileCache = dlg.m_FilePage.m_MaxFileCache;

		m_DefaultPlaybackDevice = dlg.m_SoundPage.m_PlaybackDevice;
		m_NumPlaybackBuffers = dlg.m_SoundPage.m_NumPlaybackBuffers;
		m_SizePlaybackBuffers = dlg.m_SoundPage.m_PlaybackBufferSize * 1024;

		m_DefaultRecordDevice = dlg.m_SoundPage.m_RecordingDevice;
		m_NumRecordBuffers = dlg.m_SoundPage.m_NumRecordingBuffers;
		m_SizeRecordBuffers = dlg.m_SoundPage.m_RecordingBufferSize * 1024;

	}
}

CString GetSelectionText(long Start, long End, int Chan,
						int nChannels, BOOL bLockChannels,
						long nSamplesPerSec, int TimeFormat)
{
	CString s;
	if (nChannels > 1)
	{
		LPCTSTR sChans = _T("Stereo");
		if (! bLockChannels)
		{
			if (0 == Chan)
			{
				sChans = _T("Left");
			}
			else if (1 == Chan)
			{
				sChans = _T("Right");
			}
		}
		s.Format("Selection : %s to %s (%s)\n"
				"Channels: %s",
				SampleToString(Start, nSamplesPerSec, TimeFormat),
				SampleToString(End, nSamplesPerSec, TimeFormat),
				SampleToString(End - Start, nSamplesPerSec, TimeFormat),
				sChans);
	}
	else
	{
		s.Format("Selection : %s to %s (%s)",
				SampleToString(Start, nSamplesPerSec, TimeFormat),
				SampleToString(End, nSamplesPerSec, TimeFormat),
				SampleToString(End - Start, nSamplesPerSec, TimeFormat));
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
