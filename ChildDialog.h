#if !defined(AFX_CHILDDIALOG_H__836B73F8_4242_4B0E_9336_BA710C7E5555__INCLUDED_)
#define AFX_CHILDDIALOG_H__836B73F8_4242_4B0E_9336_BA710C7E5555__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChildDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CChildDialog dialog

class CChildDialog : public CDialog
{
// Construction
public:
	CChildDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CChildDialog)
	enum { IDD = IDD_FUNCTIONS_TAB };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CChildDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDDIALOG_H__836B73F8_4242_4B0E_9336_BA710C7E5555__INCLUDED_)
