// PreferencesPropertySheet.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "PreferencesPropertySheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPreferencesPropertySheet

IMPLEMENT_DYNAMIC(CPreferencesPropertySheet, CPropertySheet)

CPreferencesPropertySheet::CPreferencesPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	AddPage( & m_FilePage);
	AddPage( & m_SoundPage);
	AddPage( & m_ViewPage);
}

CPreferencesPropertySheet::CPreferencesPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	AddPage( & m_FilePage);
	AddPage( & m_SoundPage);
	AddPage( & m_ViewPage);
}

CPreferencesPropertySheet::~CPreferencesPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CPreferencesPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CPreferencesPropertySheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPreferencesPropertySheet message handlers
/////////////////////////////////////////////////////////////////////////////
// CFilePreferencesPage property page

IMPLEMENT_DYNCREATE(CFilePreferencesPage, CPropertyPage)

CFilePreferencesPage::CFilePreferencesPage() : CPropertyPage(CFilePreferencesPage::IDD)
{
	//{{AFX_DATA_INIT(CFilePreferencesPage)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CFilePreferencesPage::~CFilePreferencesPage()
{
}

void CFilePreferencesPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFilePreferencesPage)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFilePreferencesPage, CPropertyPage)
	//{{AFX_MSG_MAP(CFilePreferencesPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilePreferencesPage message handlers
/////////////////////////////////////////////////////////////////////////////
// CSoundPreferencesPage property page

IMPLEMENT_DYNCREATE(CSoundPreferencesPage, CPropertyPage)

CSoundPreferencesPage::CSoundPreferencesPage() : CPropertyPage(CSoundPreferencesPage::IDD)
{
	//{{AFX_DATA_INIT(CSoundPreferencesPage)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSoundPreferencesPage::~CSoundPreferencesPage()
{
}

void CSoundPreferencesPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSoundPreferencesPage)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSoundPreferencesPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSoundPreferencesPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSoundPreferencesPage message handlers
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CViewPreferencesPage property page

IMPLEMENT_DYNCREATE(CViewPreferencesPage, CPropertyPage)

CViewPreferencesPage::CViewPreferencesPage() : CPropertyPage(CViewPreferencesPage::IDD)
{
	//{{AFX_DATA_INIT(CViewPreferencesPage)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CViewPreferencesPage::~CViewPreferencesPage()
{
}

void CViewPreferencesPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewPreferencesPage)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CViewPreferencesPage, CPropertyPage)
	//{{AFX_MSG_MAP(CViewPreferencesPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewPreferencesPage message handlers
