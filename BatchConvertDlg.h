// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#pragma once
// BatchConvertDlg.h : header file
//
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CBatchConvertDlg dialog

class CBatchConvertDlg : public CDialog
{
// Construction
public:
	CBatchConvertDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CBatchConvertDlg)
	enum { IDD = IDD_DIALOG_BATCH_CONVERSION };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBatchConvertDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBatchConvertDlg)
	afx_msg void OnButtonAddDestination();
	afx_msg void OnButtonAddFiles();
	afx_msg void OnButtonDeleteDestination();
	afx_msg void OnButtonDeleteFiles();
	afx_msg void OnButtonEditDestination();
	afx_msg void OnButtonMoveDown();
	afx_msg void OnButtonMoveUp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

