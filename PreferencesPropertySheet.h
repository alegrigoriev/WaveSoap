// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_PREFERENCESPROPERTYSHEET_H__4D2A4435_2BFB_4E5A_8D8C_49EE56304555__INCLUDED_)
#define AFX_PREFERENCESPROPERTYSHEET_H__4D2A4435_2BFB_4E5A_8D8C_49EE56304555__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PreferencesPropertySheet.h : header file
//
#include "resource.h"       // main symbols
#include "ApplicationParameters.h"
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CFilePreferencesPage dialog

class CFilePreferencesPage : public CPropertyPage
{
	typedef CPropertyPage BaseClass;
	DECLARE_DYNCREATE(CFilePreferencesPage)

// Construction
public:
	CFilePreferencesPage();
	~CFilePreferencesPage();

// Dialog Data
	//{{AFX_DATA(CFilePreferencesPage)
	enum { IDD = IDD_PROPPAGE_FILE_PREFERENCES };
	CEdit	m_eTempFileLocation;

	CString	m_sTempFileLocation;
	UINT	m_MaxMemoryFileSize;
	int		m_DefaultFileOpenMode;
	int		m_bEnable4GbWavFile;
	UINT	m_MaxFileCache;
	int     m_FileTextEncoding;
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
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CSoundPreferencesPage dialog

class CSoundPreferencesPage : public CPropertyPage
{
	typedef CPropertyPage BaseClass;
	DECLARE_DYNCREATE(CSoundPreferencesPage)

// Construction
public:
	CSoundPreferencesPage();
	~CSoundPreferencesPage();

// Dialog Data
	//{{AFX_DATA(CSoundPreferencesPage)
	enum { IDD = IDD_PROPPAGE_SOUND_PREFERENCES };
	CSpinButtonCtrl	m_SpinRecordingBufs;
	CSpinButtonCtrl	m_SpinPlaybackBufs;
	CComboBox	m_RecordingDeviceCombo;
	CComboBox	m_PlaybackDeviceCombo;
	INT_PTR		m_PlaybackDevice;
	INT_PTR		m_RecordingDevice;
	UINT	m_NumPlaybackBuffers;
	UINT	m_NumRecordingBuffers;
	UINT	m_PlaybackBufferSize;
	UINT	m_RecordingBufferSize;
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
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CViewPreferencesPage dialog

class CViewPreferencesPage : public CPropertyPage
{
	typedef CPropertyPage BaseClass;
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
/////////////////////////////////////////////////
// CUndoPropertyPage dialog

class CUndoPropertyPage : public CPropertyPage, protected UndoRedoParameters
{
	typedef CPropertyPage BaseClass;
	DECLARE_DYNAMIC(CUndoPropertyPage)

public:
	CUndoPropertyPage(UndoRedoParameters const * pParams);
	virtual ~CUndoPropertyPage();
	UndoRedoParameters const * GetParams() const
	{
		return this;
	}
// Dialog Data
	enum { IDD = IDD_PROPPAGE_UNDO_PREFERENCES };
	CSpinButtonCtrl	m_SpinUndoLimit;
	CSpinButtonCtrl	m_SpinRedoLimit;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

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
	int GetLastSelectedPage() const
	{
		return m_PageSelected;
	}
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesPropertySheet)
	//}}AFX_VIRTUAL

public:
	virtual ~CPreferencesPropertySheet();

// Implementation
	CFilePreferencesPage m_FilePage;
	CUndoPropertyPage m_UndoPage;
	CSoundPreferencesPage m_SoundPage;
	CViewPreferencesPage m_ViewPage;
protected:
	int m_PageSelected;
	// Generated message map functions
	//{{AFX_MSG(CPreferencesPropertySheet)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PREFERENCESPROPERTYSHEET_H__4D2A4435_2BFB_4E5A_8D8C_49EE56304555__INCLUDED_)
