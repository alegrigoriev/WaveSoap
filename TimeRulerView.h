#if !defined(AFX_TIMERULERVIEW_H__0755E980_3D17_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_TIMERULERVIEW_H__0755E980_3D17_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TimeRulerView.h : header file
//
#include "Ruler.h"
/////////////////////////////////////////////////////////////////////////////
// CTimeRulerView view

class CTimeRulerView : public CHorizontalRuler
{
protected:
	CTimeRulerView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CTimeRulerView)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimeRulerView)
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CTimeRulerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CTimeRulerView)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TIMERULERVIEW_H__0755E980_3D17_11D4_9ADD_00C0F0583C4B__INCLUDED_)
