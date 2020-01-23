// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#pragma once
// ChildDialog.h : header file
//
#include "NumEdit.h"
#include "resource.h"       // main symbols
#include <vector>
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

class COperandsDialog : public CChildDialog
{
// Construction
public:
	COperandsDialog(UINT id = 0, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(COperandsDialog)
	//}}AFX_DATA
	CNumEdit m_eFrequency;
	CNumEdit m_eFrequency1;
	CNumEdit m_eFrequency2;
	CNumEdit m_eFrequency3;
	double m_dFrequency;
	double m_dFrequency1;
	double m_dFrequency2;
	double m_dFrequency3;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COperandsDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COperandsDialog)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CInsertExpressionDialog dialog
namespace Expressions
{
struct Expr
{
	CString name;
	CString expr;
	CString comment;
};
struct ExprGroup
{
	CString name;
	std::vector<Expr> exprs;
};
};

class CInsertExpressionDialog : public CDialog
{
	typedef Expressions::Expr Expr;
	typedef Expressions::ExprGroup ExprGroup;

	typedef std::vector<ExprGroup> ExprGroupVector;
	typedef ExprGroupVector::iterator ExprGroupIterator;
	typedef std::vector<Expr> ExprVector;
	typedef ExprVector::iterator ExprIterator;
	typedef CDialog BaseClass;
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
	bool m_ExpressionsChanged;

	static void LoadExpressions(ExprGroupVector & Expressions,
								LPCTSTR ProfileName = NULL);
	void UnloadExpressions(LPCTSTR ProfileName = NULL);
	void RebuildAllExpressionsList();
	void SaveExpressionAs(const CString & expr);
	BOOL SaveExpression(const CString & expr, const CString & Name,
						const CString & Group, const CString & Comment, bool bEnableCancel);
	BOOL FindExpression(LPCTSTR Group, LPCTSTR Name, int * nGroup, int * nExpr);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInsertExpressionDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	ExprGroupVector m_Expressions;

	void BuildExpressionGroupCombobox(unsigned nGroupSelected, unsigned nExprSelected);
	void LoadExpressionCombobox(unsigned nGroupSelected, unsigned nExprSelected);
	void OnUpdateDeleteExpression(CCmdUI* pCmdUI);
	void OnUpdateInsertExpression(CCmdUI* pCmdUI);

	// Generated message map functions
	//{{AFX_MSG(CInsertExpressionDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboSavedExpressionGroup();
	afx_msg void OnSelchangeComboSavedExpressions();
	afx_msg void OnButtonInsertExpression();
	afx_msg void OnButtonDeleteExpression();
	afx_msg void OnButtonExportExpressions();
	afx_msg void OnButtonImportExpressions();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
