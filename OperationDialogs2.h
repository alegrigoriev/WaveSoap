#if !defined(AFX_OPERATIONDIALOGS2_H__908708EA_C03D_430C_A6BA_E92753C9E84F__INCLUDED_)
#define AFX_OPERATIONDIALOGS2_H__908708EA_C03D_430C_A6BA_E92753C9E84F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OperationDialogs2.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInsertSilenceDialog dialog

class CInsertSilenceDialog : public CDialog
{
// Construction
public:
	CInsertSilenceDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CInsertSilenceDialog)
	enum { IDD = IDD_DIALOG_INSERT_SILENCE };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInsertSilenceDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInsertSilenceDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPERATIONDIALOGS2_H__908708EA_C03D_430C_A6BA_E92753C9E84F__INCLUDED_)
