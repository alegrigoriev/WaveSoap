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
#include "TimeToStr.h"

class CTimeRulerView : public CHorizontalRuler
{
	typedef CHorizontalRuler BaseClass;
protected:
	CTimeRulerView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CTimeRulerView)

// Attributes
public:
	CWaveSoapFrontDoc* GetDocument() const;

	static int CalculateHeight();

// Operations
public:
	enum
	{
		ShowSamples = SampleToString_Sample,
		ShowHhMmSs = SampleToString_HhMmSs/* | TimeToHhMmSs_NeedsHhMm | TimeToHhMmSs_NeedsMs*/,
		ShowSeconds = SampleToString_Seconds/* | TimeToHhMmSs_NeedsMs*/,
		ShowHhMmSsFf = SampleToString_HhMmSsFf,
	};
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
	virtual UINT GetPopupMenuID(CPoint);
	virtual void OnContextMenu(CWnd* pWnd, CPoint point);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	unsigned HitTest(POINT p, RECT * pHitRect = NULL, int * OffsetX = NULL) const;
	void InvalidateMarkerRegion(WAVEREGIONINFO const * pInfo);
	virtual void HorizontalScrollByPixels(int Pixels);
	void HorizontalScrollTo(double first_sample_in_view);
	double WindowXToSample(int x) const
	{
		return x * m_HorizontalScale + m_FirstSampleInView;
	}
	int SampleToWindowXfloor(double sample) const
	{
		return (int)floor((sample - m_FirstSampleInView) / m_HorizontalScale);
	}
	int SampleToWindowXceil(double sample) const
	{
		return (int)ceil((sample - m_FirstSampleInView) / m_HorizontalScale);
	}
	enum
	{
		HitTestNone = 0x00000000,          // non-specific area of the ruler hit - no bits set
		HitTestRegionBegin = 0x40000000,    // mark of region begin hit
		HitTestRegionEnd = 0x20000000,      // mark of region begin hit
		HitTestMarker = 0x10000000,
		HitTestLowerHalf = 0x08000000,
		HitTestCueIndexMask = 0x000FFFF,    // these bits contain index of the region/marker hit
	};
	// Generated message map functions
protected:
	int m_CurrentDisplayMode;
	unsigned m_DraggedMarkerHitTest;
	int m_HitOffset;
	unsigned m_PopupMenuHitTest;
	WAVEREGIONINFO m_PopupMenuHit;

	double m_FirstSampleInView;
	double m_HorizontalScale;
	double m_TotalSamplesInView;
	double m_TotalSamplesInExtent;

	UINT_PTR m_AutoscrollTimerID;
	int m_MarkerHeight;

	void EndMarkerDrag();
	void BeginMarkerDrag();

	//{{AFX_MSG(CTimeRulerView)
	afx_msg void OnViewRulerHhmmss();
	afx_msg void OnUpdateViewRulerHhmmss(CCmdUI* pCmdUI);
	afx_msg void OnViewRulerSamples();
	afx_msg void OnUpdateViewRulerSamples(CCmdUI* pCmdUI);
	afx_msg void OnViewRulerSeconds();
	afx_msg void OnUpdateViewRulerSeconds(CCmdUI* pCmdUI);
	afx_msg void OnViewRulerHhmmssFf();
	afx_msg void OnUpdateViewRulerHhmmssFf(CCmdUI* pCmdUI);

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	virtual void OnInitialUpdate();
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	void OnToolTipText(UINT /*id*/, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnGotoMarker();
	afx_msg void OnDeleteMarker();
	afx_msg void OnUpdateDeleteMarker(CCmdUI *pCmdUI);
	afx_msg void OnMoveMarkerToCurrent();
	afx_msg void OnUpdateMoveMarkerToCurrent(CCmdUI *pCmdUI);
	afx_msg void OnEditMarker();
	afx_msg void OnUpdateEditMarker(CCmdUI *pCmdUI);
	afx_msg void OnSelectRegion();
protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg LRESULT OnUwmNotifyViews(WPARAM wParam, LPARAM lParam);
};

#ifndef _DEBUG  // debug version in TimeRulerView.cpp
inline CWaveSoapFrontDoc* CTimeRulerView::GetDocument() const
{ return (CWaveSoapFrontDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TIMERULERVIEW_H__0755E980_3D17_11D4_9ADD_00C0F0583C4B__INCLUDED_)
