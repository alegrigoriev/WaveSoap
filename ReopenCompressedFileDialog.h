// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#pragma once
// ReopenCompressedFileDialog.h : header file
//
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CReopenCompressedFileDialog dialog

class CReopenCompressedFileDialog : public CDialog
{
// Construction
public:
	CReopenCompressedFileDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CReopenCompressedFileDialog)
	enum { IDD = IDD_DIALOG_REOPEN_COMPRESSED };
	CString	m_Text;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReopenCompressedFileDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CReopenCompressedFileDialog)
	afx_msg void OnNo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

