#if !defined(AFX_TIMEEDIT_H__8CFD06E0_6A44_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_TIMEEDIT_H__8CFD06E0_6A44_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TimeEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTimeEdit window

class CTimeEdit : public CEdit
{
// Construction
public:
	CTimeEdit();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimeEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTimeEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTimeEdit)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TIMEEDIT_H__8CFD06E0_6A44_11D4_9ADD_00C0F0583C4B__INCLUDED_)
