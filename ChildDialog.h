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
	CChildDialog(UINT id = 0, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CChildDialog)
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
	BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);

	// Generated message map functions
	//{{AFX_MSG(CChildDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CInsertExpressionDialog dialog

class CInsertExpressionDialog : public CDialog
{
// Construction
public:
	CInsertExpressionDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CInsertExpressionDialog)
	enum { IDD = IDD_SAVED_EXPRESSIONS_TAB };
	CComboBox	m_ExpressionGroupCombo;
	CStatic	m_Description;
	CComboBox	m_SavedExpressionCombo;
	//}}AFX_DATA
	int m_ExpressionGroupSelected;
	int m_CurrExpressionGroupSelected;
	int m_ExpressionSelected;
	int m_NumSavedExpressions;
	int m_NumExprGroups;
	//CString m_Expressions[CThisApp::MaxSavedTotalExpressions];
	//CString m_ExpressionComments[CThisApp::MaxSavedTotalExpressions];
	//CString m_ExpressionNames[CThisApp::MaxSavedTotalExpressions];

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInsertExpressionDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInsertExpressionDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboSavedExpressionGroup();
	afx_msg void OnSelchangeComboSavedExpressions();
	afx_msg void OnButtonInsertExpression();
	afx_msg void OnButtonDeleteExpression();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDDIALOG_H__836B73F8_4242_4B0E_9336_BA710C7E5555__INCLUDED_)
