// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#pragma once
// FftRulerView.h : header file
//

#include "Ruler.h"
#include "resource.h"
/////////////////////////////////////////////////////////////////////////////
// CFftRulerView view

class CFftRulerView : public CVerticalRuler
{
protected:
	CFftRulerView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CFftRulerView)

// Attributes
public:
	CWaveSoapFrontDoc* GetDocument();
	static int CalculateWidth();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFftRulerView)
	virtual void OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint );
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL
	virtual void VerticalScrollByPixels(int Pixels);
	virtual void BeginMouseTracking();
	// Implementation
protected:
	virtual ~CFftRulerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	double m_VerticalScale;
	// how much the chart is scrolled.
	// 0 = DC is visible.
	// Whole frequency range is 1.
	double m_VisibleBottom;
	double m_BottomBeforeScroll;
	NotifyChannelHeightsData m_Heights;
	int m_MouseYOffsetForScroll;

	// Offset 1 corresponds to the whole frequency range
	double AdjustOffset(double offset) const;
	void SetNewFftOffset(double visible_bottom);

	// Generated message map functions
protected:
	//{{AFX_MSG(CFftRulerView)
	// NOTE - the ClassWizard will add and remove member functions here.
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint point);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnUwmNotifyViews(WPARAM wParam, LPARAM lParam);
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

#ifndef _DEBUG  // debug version in FftRulerView.cpp
inline CWaveSoapFrontDoc* CFftRulerView::GetDocument()
{ return (CWaveSoapFrontDoc*)m_pDocument; }
#endif
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
