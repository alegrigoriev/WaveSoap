#if !defined(AFX_FILTERDIALOG_H__E4CA69D8_E2BD_4C89_AB70_2F3F8C35FF3B__INCLUDED_)
#define AFX_FILTERDIALOG_H__E4CA69D8_E2BD_4C89_AB70_2F3F8C35FF3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FilterDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog dialog

class CFilterDialog : public CDialog
{
// Construction
public:
	CFilterDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFilterDialog)
	enum { IDD = IDD_DIALOG_FILTER };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFilterDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFilterDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILTERDIALOG_H__E4CA69D8_E2BD_4C89_AB70_2F3F8C35FF3B__INCLUDED_)
