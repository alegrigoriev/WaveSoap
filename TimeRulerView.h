// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
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
	CWaveSoapFrontDoc* GetDocument();

// Operations
public:
	enum {ShowSamples, ShowHhMmSs, ShowSeconds };
	int m_CurrentDisplayMode;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimeRulerView)
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CTimeRulerView();
	virtual UINT GetPopupMenuID(CPoint) { return IDR_MENU_TIME_RULER; }
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	void DrawRulerSamples(CDC* pDC);
	void DrawRulerHhMmSs(CDC* pDC);
	void DrawRulerSeconds(CDC* pDC);

	// Generated message map functions
protected:
	//{{AFX_MSG(CTimeRulerView)
	afx_msg void OnViewRulerHhmmss();
	afx_msg void OnUpdateViewRulerHhmmss(CCmdUI* pCmdUI);
	afx_msg void OnViewRulerSamples();
	afx_msg void OnUpdateViewRulerSamples(CCmdUI* pCmdUI);
	afx_msg void OnViewRulerSeconds();
	afx_msg void OnUpdateViewRulerSeconds(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in TimeRulerView.cpp
inline CWaveSoapFrontDoc* CTimeRulerView::GetDocument()
{ return (CWaveSoapFrontDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TIMERULERVIEW_H__0755E980_3D17_11D4_9ADD_00C0F0583C4B__INCLUDED_)
