// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_WAVEOUTLINEVIEW_H__049155A1_4B99_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_WAVEOUTLINEVIEW_H__049155A1_4B99_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WaveOutlineView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView view

class CWaveOutlineView : public CView
{
protected:
	CWaveOutlineView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWaveOutlineView)

// Attributes
public:
	CWaveSoapFrontDoc* GetDocument();

// Operations
public:
	void NotifyViewExtents(SAMPLE_INDEX left, SAMPLE_INDEX right);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveOutlineView)
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_PlaybackCursorPosition;
	int m_LeftViewBoundary;
	int m_RightViewBoundary;
	int m_LastMaxAmplitude;
	bool bIsTrackingSelection;
	int nKeyPressed;
	virtual ~CWaveOutlineView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	BOOL EraseBkgnd(CDC* pDC);
	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveOutlineView)
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG  // debug version in WaveSoapFrontView.cpp
inline CWaveSoapFrontDoc* CWaveOutlineView::GetDocument()
{ return (CWaveSoapFrontDoc*)m_pDocument; }
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVEOUTLINEVIEW_H__049155A1_4B99_11D4_9ADD_00C0F0583C4B__INCLUDED_)
