#if !defined(AFX_SAVEEXPRESSIONDIALOG_H__1FEC6434_5713_49ED_815E_6B35C28D8CD9__INCLUDED_)
#define AFX_SAVEEXPRESSIONDIALOG_H__1FEC6434_5713_49ED_815E_6B35C28D8CD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SaveExpressionDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSaveExpressionDialog dialog
#include "ChildDialog.h"

class CSaveExpressionDialog : public CDialog
{
	typedef Expressions::Expr Expr;
	typedef Expressions::ExprGroup ExprGroup;
// Construction
public:
	CSaveExpressionDialog(const vector<ExprGroup> & Exprs,
						CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSaveExpressionDialog)
	enum { IDD = IDD_DIALOG_SAVE_EXPRESSION };
	CComboBox	m_SavedExpressionCombo;
	CEdit	m_eComment;
	CComboBox	m_ExpressionGroupCombo;
	CString	m_GroupName;
	CString	m_Comment;
	CString	m_Name;
	//}}AFX_DATA
	const vector<ExprGroup> & m_Expressions;
	int m_ExpressionGroupSelected;
	int m_CurrExpressionGroupSelected;
	int m_ExpressionSelected;
	bool m_bCommentChanged;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSaveExpressionDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void BuildExpressionGroupCombobox(int nGroupSelected, int nExprSelected);
	void LoadExpressionCombobox(int nGroupSelected, int nExprSelected);

	// Generated message map functions
	//{{AFX_MSG(CSaveExpressionDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboGroup();
	afx_msg void OnSelchangeComboName();
	afx_msg void OnChangeEditComment();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAVEEXPRESSIONDIALOG_H__1FEC6434_5713_49ED_815E_6B35C28D8CD9__INCLUDED_)
