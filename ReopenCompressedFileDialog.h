// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_REOPENCOMPRESSEDFILEDIALOG_H__56082322_CB13_4D74_A4F1_2FFFA8355821__INCLUDED_)
#define AFX_REOPENCOMPRESSEDFILEDIALOG_H__56082322_CB13_4D74_A4F1_2FFFA8355821__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ReopenCompressedFileDialog.h : header file
//

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

#endif // !defined(AFX_REOPENCOMPRESSEDFILEDIALOG_H__56082322_CB13_4D74_A4F1_2FFFA8355821__INCLUDED_)
