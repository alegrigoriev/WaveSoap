#if !defined(AFX_MAINPAGE_H__816570C0_2D78_11D2_BE02_8C377EC01214__INCLUDED_)
#define AFX_MAINPAGE_H__816570C0_2D78_11D2_BE02_8C377EC01214__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MainPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMainPage dialog

class CMainPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CMainPage)

// Construction
public:
	CMainPage();
	~CMainPage();

// Dialog Data
	//{{AFX_DATA(CMainPage)
	enum { IDD = IDD_PROPPAGE_MAIN };
	BOOL	m_bDoCameraNoiseReduction;
	BOOL	m_bDoDeclick;
	BOOL	m_bDoNoiseReduction;
	BOOL	m_bDoUlfNoiseReduction;
	CString	m_InFile;
	CString	m_OutFile;
	BOOL	m_HighpassFilter;
	//}}AFX_DATA

	void LoadValuesFromRegistry();
	void StoreValuesToRegistry();
	void ProcessSoundFile();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMainPage)
public:
	virtual void OnOK();
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMainPage)
	afx_msg void OnButtonBrowseSource();
	afx_msg void OnButtonBrowseTarget();
	afx_msg void OnChangeEditSourceFilename();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINPAGE_H__816570C0_2D78_11D2_BE02_8C377EC01214__INCLUDED_)
