// WaveSoapFront.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WaveSoapFront.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "WaveSoapFrontDoc.h"
#include "WaveSoapFrontView.h"
#include <float.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontApp

BEGIN_MESSAGE_MAP(CWaveSoapFrontApp, CWinApp)
	//{{AFX_MSG_MAP(CWaveSoapFrontApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontApp construction

CWaveSoapFrontApp::CWaveSoapFrontApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
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
					RGB(0x00, 193, 0x00));
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
	Profile.AddItem(_T("Settings\\Colors"), _T("m_SelectedChannelSeparatorColor"), m_SelectedChannelSeparatorColor,
					RGB(0x00, 0x00, 0x00));
	Profile.AddItem(_T("Settings\\Colors"), _T("InterpolatedColor"), m_InterpolatedColor,
					RGB(0x00, 0x00, 0x00));
	Profile.AddItem(_T("Settings\\Colors"), _T("SelectedInterpolatedColor"), m_SelectedInterpolatedColor,
					RGB(0x00, 0x00, 0x00));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CWaveSoapDocTemplate* pDocTemplate;
	pDocTemplate = new CWaveSoapDocTemplate(
											IDR_WAVESOTYPE,
											RUNTIME_CLASS(CWaveSoapFrontDoc),
											RUNTIME_CLASS(CChildFrame), // custom MDI child frame
											RUNTIME_CLASS(CWaveSoapFrontView));
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

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

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
		CWaitCursor wait;
		if (!pDocument->OnOpenDocument(lpszPathName))
		{
			// user has be alerted to what failed in OnOpenDocument
			TRACE0("CDocument::OnOpenDocument returned FALSE.\n");
			//pFrame->DestroyWindow();
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
	LPTSTR dirbuf = m_CurrentDir.GetBuffer(MAX_PATH+1);
	if (dirbuf)
	{
		GetCurrentDirectory(MAX_PATH+1, dirbuf);
		m_CurrentDir.ReleaseBuffer();
	}
	Profile.UnloadSection(_T("Settings"));
	Profile.UnloadSection(_T("Settings\\Colors"));

	return CWinApp::ExitInstance();
}
