#if !defined(AFX_PREFERENCESPROPERTYSHEET_H__4D2A4435_2BFB_4E5A_8D8C_49EE56304555__INCLUDED_)
#define AFX_PREFERENCESPROPERTYSHEET_H__4D2A4435_2BFB_4E5A_8D8C_49EE56304555__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PreferencesPropertySheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CFilePreferencesPage dialog

class CFilePreferencesPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CFilePreferencesPage)

// Construction
public:
	CFilePreferencesPage();
	~CFilePreferencesPage();

// Dialog Data
	//{{AFX_DATA(CFilePreferencesPage)
	enum { IDD = IDD_PROPPAGE_FILE_PREFERENCES };
	CSpinButtonCtrl	m_SpinUndoLimit;
	CSpinButtonCtrl	m_SpinRedoLimit;
	CEdit	m_eTempFileLocation;
	BOOL	m_bAllow4GbWav;
	BOOL	m_bEnableRedo;
	BOOL	m_bEnableUndo;
	BOOL	m_bLimitRedoDepth;
	BOOL	m_bLimitRedoSize;
	BOOL	m_bLimitUndoSize;
	BOOL	m_bLimitUndoDepth;
	BOOL	m_bRememberSelectionInUndo;
	BOOL	m_UseMemoryFiles;
	UINT	m_RedoDepthLimit;
	UINT	m_RedoSizeLimit;
	CString	m_sTempFileLocation;
	UINT	m_MaxMemoryFileSize;
	UINT	m_UndoDepthLimit;
	UINT	m_UndoSizeLimit;
	int		m_DefaultFileOpenMode;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFilePreferencesPage)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFilePreferencesPage)
	afx_msg void OnButtonBrowseTempFileLocation();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CSoundPreferencesPage dialog

class CSoundPreferencesPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSoundPreferencesPage)

// Construction
public:
	CSoundPreferencesPage();
	~CSoundPreferencesPage();

// Dialog Data
	//{{AFX_DATA(CSoundPreferencesPage)
	enum { IDD = IDD_PROPPAGE_SOUND_PREFERENCES };
	// NOTE - ClassWizard will add data members here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSoundPreferencesPage)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSoundPreferencesPage)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CViewPreferencesPage dialog

class CViewPreferencesPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CViewPreferencesPage)

// Construction
public:
	CViewPreferencesPage();
	~CViewPreferencesPage();

// Dialog Data
	//{{AFX_DATA(CViewPreferencesPage)
	enum { IDD = IDD_PROPPAGE_VIEW_PREFERENCES };
	BOOL	m_bSnapMouseSelection;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CViewPreferencesPage)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CViewPreferencesPage)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CPreferencesPropertySheet

class CPreferencesPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CPreferencesPropertySheet)

// Construction
public:
	CPreferencesPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CPreferencesPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesPropertySheet)
	//}}AFX_VIRTUAL

// Implementation
	CFilePreferencesPage m_FilePage;
	CSoundPreferencesPage m_SoundPage;
	CViewPreferencesPage m_ViewPage;

public:
	virtual ~CPreferencesPropertySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPreferencesPropertySheet)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PREFERENCESPROPERTYSHEET_H__4D2A4435_2BFB_4E5A_8D8C_49EE56304555__INCLUDED_)
