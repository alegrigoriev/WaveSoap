#if !defined(AFX_REOPENCONVERTEDFILEDLG_H__2BDCD4B0_3E6D_499B_A233_70AB5D7EDBAC__INCLUDED_)
#define AFX_REOPENCONVERTEDFILEDLG_H__2BDCD4B0_3E6D_499B_A233_70AB5D7EDBAC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ReopenConvertedFileDlg.h : header file
//

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
	// NOTE: the ClassWizard will add data members here
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

#endif // !defined(AFX_REOPENCONVERTEDFILEDLG_H__2BDCD4B0_3E6D_499B_A233_70AB5D7EDBAC__INCLUDED_)
