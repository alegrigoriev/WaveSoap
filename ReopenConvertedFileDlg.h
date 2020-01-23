// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#pragma once
// ReopenConvertedFileDlg.h : header file
//
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CReopenConvertedFileDlg dialog

class CReopenConvertedFileDlg : public CDialog
{
// Construction
public:
	CReopenConvertedFileDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CReopenConvertedFileDlg)
	enum { IDD = IDD_DIALOG_SHOULD_RELOAD_CONVERTED };
	CString	m_Text;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReopenConvertedFileDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CReopenConvertedFileDlg)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

