// WaveSoap.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WaveSoap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapApp

BEGIN_MESSAGE_MAP(CWaveSoapApp, CWinApp)
	//{{AFX_MSG_MAP(CWaveSoapApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapApp construction

CWaveSoapApp::CWaveSoapApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWaveSoapApp object

CWaveSoapApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWaveSoapApp initialization

BOOL CWaveSoapApp::InitInstance()
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

	SetRegistryKey(_T("AleGr SoftWare"));
	CString CurrentDir;
	theApp.Profile.AddItem(_T("Settings"), _T("CurrentDir"), CurrentDir, _T("."));
	if (CurrentDir != ".")
	{
		SetCurrentDirectory(CurrentDir);
	}
	pDlg = new CWaveSoapSheet("AleGr Wave Soap");
	m_pMainWnd = pDlg;
	pDlg->LoadValuesFromRegistry();

	pDlg->DoModal();
	pDlg->StoreValuesToRegistry();
	LPTSTR dirbuf = CurrentDir.GetBuffer(MAX_PATH+1);
	if (dirbuf)
	{
		GetCurrentDirectory(MAX_PATH+1, dirbuf);
		CurrentDir.ReleaseBuffer();
		theApp.Profile.FlushItem(_T("Settings"), _T("CurrentDir"));
	}
	// CurrentDir is not valid anymore
	theApp.Profile.RemoveSection(_T("Settings"));

	return FALSE;
}

int CWaveSoapApp::ExitInstance()
{
	// TODO: Add your specialized code here and/or call the base class
	delete pDlg;
	m_pMainWnd = NULL;
	return CWinApp::ExitInstance();
}

double CWaveSoapApp::GetProfileDouble(LPCTSTR Section, LPCTSTR ValueName,
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

void CWaveSoapApp::WriteProfileDouble(LPCTSTR Section, LPCTSTR ValueName,
									double value)
{
	CString s;
	s.Format("%g", value);
	WriteProfileString(Section, ValueName, s);
}
