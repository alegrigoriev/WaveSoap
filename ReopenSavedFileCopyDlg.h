// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_REOPENSAVEDFILECOPYDLG_H__1F750370_C22A_4BE2_8537_189311ADCB35__INCLUDED_)
#define AFX_REOPENSAVEDFILECOPYDLG_H__1F750370_C22A_4BE2_8537_189311ADCB35__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ReopenSavedFileCopyDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CReopenSavedFileCopyDlg dialog

class CReopenSavedFileCopyDlg : public CDialog
{
// Construction
public:
	CReopenSavedFileCopyDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CReopenSavedFileCopyDlg)
	enum { IDD = IDD_DIALOG_OPEN_AFTER_SAVE_COPY };
	CString	m_Prompt;
	//}}AFX_DATA

	BOOL m_bDisableDirect;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReopenSavedFileCopyDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CReopenSavedFileCopyDlg)
	afx_msg void OnNo();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REOPENSAVEDFILECOPYDLG_H__1F750370_C22A_4BE2_8537_189311ADCB35__INCLUDED_)
