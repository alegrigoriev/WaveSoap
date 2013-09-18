// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_RULER_H__0AB440C1_3E09_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_RULER_H__0AB440C1_3E09_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Ruler.h : header file
//
#include "ScaledScrollView.h"

/////////////////////////////////////////////////////////////////////////////
// CHorizontalRuler view

class CHorizontalRuler : public CScaledScrollView
{
	typedef CScaledScrollView BaseClass;
protected:
	CHorizontalRuler();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CHorizontalRuler)

// Attributes
public:
	static int CalculateHeight();
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHorizontalRuler)
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CHorizontalRuler();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	int PrevMouseX;
	int ButtonPressed;

	// Generated message map functions
protected:
	//{{AFX_MSG(CHorizontalRuler)
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVerticalRuler view

class CVerticalRuler : public CScaledScrollView
{
	typedef CScaledScrollView BaseClass;
protected:
	CVerticalRuler();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CVerticalRuler)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVerticalRuler)
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CVerticalRuler();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	int PrevMouseY;
	int ButtonPressed;

	// Generated message map functions
protected:
	//{{AFX_MSG(CVerticalRuler)
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULER_H__0AB440C1_3E09_11D4_9ADD_00C0F0583C4B__INCLUDED_)
