// PreferencesPropertySheet.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "PreferencesPropertySheet.h"
#include "FolderDialog.h"

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
	m_bAllow4GbWav = FALSE;
	m_bEnableRedo = FALSE;
	m_bEnableUndo = FALSE;
	m_bLimitRedoDepth = FALSE;
	m_bLimitRedoSize = FALSE;
	m_bLimitUndoSize = FALSE;
	m_bLimitUndoDepth = FALSE;
	m_bRememberSelectionInUndo = FALSE;
	m_UseMemoryFiles = FALSE;
	m_RedoDepthLimit = 0;
	m_RedoSizeLimit = 0;
	m_sTempFileLocation = _T("");
	m_MaxMemoryFileSize = 0;
	m_UndoDepthLimit = 0;
	m_UndoSizeLimit = 0;
	m_DefaultFileOpenMode = -1;
	//}}AFX_DATA_INIT
}

CFilePreferencesPage::~CFilePreferencesPage()
{
}

void CFilePreferencesPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFilePreferencesPage)
	DDX_Control(pDX, IDC_SPIN_UNDO_DEPTH, m_SpinUndoLimit);
	DDX_Control(pDX, IDC_SPIN_REDO_LIMIT, m_SpinRedoLimit);
	DDX_Control(pDX, IDC_EDIT_TEMP_FILE_LOCATION, m_eTempFileLocation);
	DDX_Check(pDX, IDC_CHECK_ALLOW_4GB_WAV, m_bAllow4GbWav);
	DDX_Check(pDX, IDC_CHECK_ENABLE_REDO, m_bEnableRedo);
	DDX_Check(pDX, IDC_CHECK_ENABLE_UNDO, m_bEnableUndo);
	DDX_Check(pDX, IDC_CHECK_LIMIT_REDO_DEPTH, m_bLimitRedoDepth);
	DDX_Check(pDX, IDC_CHECK_LIMIT_REDO_SIZE, m_bLimitRedoSize);
	DDX_Check(pDX, IDC_CHECK_LIMIT_UNDO, m_bLimitUndoSize);
	DDX_Check(pDX, IDC_CHECK_LIMIT_UNDO_DEPTH, m_bLimitUndoDepth);
	DDX_Check(pDX, IDC_CHECK_REMEMBER_SELECTION_IN_UNDO, m_bRememberSelectionInUndo);
	DDX_Check(pDX, IDC_CHECK_TEMP_MEMORY_FILES, m_UseMemoryFiles);
	DDX_Text(pDX, IDC_EDIT_REDO_DEPTH_LIMIT, m_RedoDepthLimit);
	DDV_MinMaxUInt(pDX, m_RedoDepthLimit, 1, 200);
	DDX_Text(pDX, IDC_EDIT_REDO_SIZE_LIMIT, m_RedoSizeLimit);
	DDV_MinMaxUInt(pDX, m_RedoSizeLimit, 1, 2047);
	DDX_Text(pDX, IDC_EDIT_TEMP_FILE_LOCATION, m_sTempFileLocation);
	DDX_Text(pDX, IDC_EDIT_TEMP_MEMORY_FILE_LIMIT, m_MaxMemoryFileSize);
	DDV_MinMaxUInt(pDX, m_MaxMemoryFileSize, 1, 4096);
	DDX_Text(pDX, IDC_EDIT_UNDO_DEPTH_LIMIT, m_UndoDepthLimit);
	DDV_MinMaxUInt(pDX, m_UndoDepthLimit, 1, 200);
	DDX_Text(pDX, IDC_EDIT_UNDO_LIMIT, m_UndoSizeLimit);
	DDV_MinMaxUInt(pDX, m_UndoSizeLimit, 1, 2047);
	DDX_Radio(pDX, IDC_RADIO_OPEN_FILE_MODE, m_DefaultFileOpenMode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFilePreferencesPage, CPropertyPage)
	//{{AFX_MSG_MAP(CFilePreferencesPage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_TEMP_FILE_LOCATION, OnButtonBrowseTempFileLocation)
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
	m_bSnapMouseSelection = FALSE;
	//}}AFX_DATA_INIT
}

CViewPreferencesPage::~CViewPreferencesPage()
{
}

void CViewPreferencesPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewPreferencesPage)
	DDX_Check(pDX, IDC_CHECK_SNAP_MOUSE_SELECTION, m_bSnapMouseSelection);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CViewPreferencesPage, CPropertyPage)
	//{{AFX_MSG_MAP(CViewPreferencesPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewPreferencesPage message handlers

void CFilePreferencesPage::OnButtonBrowseTempFileLocation()
{
	m_eTempFileLocation.GetWindowText(m_sTempFileLocation);
	CFolderDialog dlg("Temporary File Folder",
					m_sTempFileLocation,
					TRUE);
	if (IDOK == dlg.DoModal())
	{
		// TODO: check permissiong in callback
		m_eTempFileLocation.SetWindowText(m_sTempFileLocation);
	}
}

BOOL CFilePreferencesPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_SpinUndoLimit.SetRange(1, 200);
	m_SpinRedoLimit.SetRange(1, 200);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
