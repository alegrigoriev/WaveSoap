#if !defined(AFX_RESIZABLEDIALOG_H__1E590ECD_BEE6_4C1C_9E9C_A8F58A8FA54A__INCLUDED_)
#define AFX_RESIZABLEDIALOG_H__1E590ECD_BEE6_4C1C_9E9C_A8F58A8FA54A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ResizableDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CResizableDialog dialog

class CResizableDialog : public CDialog
{
// Construction
protected:
	CResizableDialog(UINT id, CWnd* pParent = NULL);   // standard constructor
public:

// Dialog Data
	//{{AFX_DATA(CResizableDialog)
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResizableDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CResizableDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESIZABLEDIALOG_H__1E590ECD_BEE6_4C1C_9E9C_A8F58A8FA54A__INCLUDED_)
