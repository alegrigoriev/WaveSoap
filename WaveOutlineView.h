// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#pragma once
// WaveOutlineView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView view

class CWaveOutlineView : public CView
{
	typedef CView BaseClass;
protected:
	CWaveOutlineView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWaveOutlineView)

// Attributes
public:
	CWaveSoapFrontDoc* GetDocument() const;

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
	SAMPLE_INDEX m_PlaybackCursorPosition;
	SAMPLE_INDEX m_LeftViewBoundary;
	SAMPLE_INDEX m_RightViewBoundary;
	WAVE_PEAK m_LastMaxAmplitude;
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
	unsigned HitTest(POINT p, RECT * pHitRect = NULL /*, int * OffsetX = NULL*/) const;

	enum
	{
		HitTestNone = 0x00000000,          // non-specific area of the ruler hit - no bits set
		HitTestRegionBegin = 0x40000000,    // mark of region begin hit
		HitTestRegionEnd = 0x20000000,      // mark of region begin hit
		HitTestMarker = 0x10000000,
		HitTestCueIndexMask = 0x000FFFF,    // these bits contain index of the region/marker hit
	};
	//{{AFX_MSG(CWaveOutlineView)
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	virtual void OnInitialUpdate();
	//}}AFX_MSG
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	void OnToolTipText(UINT /*id*/, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnUwmNotifyViews(WPARAM wParam, LPARAM lParam);
public:
protected:
};

/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUG  // debug version in WaveSoapFrontView.cpp
inline CWaveSoapFrontDoc* CWaveOutlineView::GetDocument() const
{ return (CWaveSoapFrontDoc*)m_pDocument; }
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

