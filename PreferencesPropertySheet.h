#if !defined(AFX_PREFERENCESPROPERTYSHEET_H__4D2A4435_2BFB_4E5A_8D8C_49EE56304555__INCLUDED_)
#define AFX_PREFERENCESPROPERTYSHEET_H__4D2A4435_2BFB_4E5A_8D8C_49EE56304555__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PreferencesPropertySheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPreferencesPropertySheet

class CPreferencesPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CPreferencesPropertySheet)

// Construction
public:
	CPreferencesPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CPreferencesPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesPropertySheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPreferencesPropertySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPreferencesPropertySheet)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PREFERENCESPROPERTYSHEET_H__4D2A4435_2BFB_4E5A_8D8C_49EE56304555__INCLUDED_)
