#if !defined(AFX_FILESUBDIALOG_H__CF626021_5310_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_FILESUBDIALOG_H__CF626021_5310_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FileSubDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFileSubDialog dialog

class CFileSubDialog : public CDialog
{
// Construction
public:
	CFileSubDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFileSubDialog)
	enum { IDD = IDD_DIALOG_OPEN_TEMPLATE };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFileSubDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFileSubDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILESUBDIALOG_H__CF626021_5310_11D4_9ADD_00C0F0583C4B__INCLUDED_)
