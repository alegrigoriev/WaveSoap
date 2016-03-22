// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_AMPLITUDERULER_H__7C3E5800_3F6E_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_AMPLITUDERULER_H__7C3E5800_3F6E_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AmplitudeRuler.h : header file
//
#include "Ruler.h"
#include "resource.h"
/////////////////////////////////////////////////////////////////////////////
// CAmplitudeRuler view

class CAmplitudeRuler : public CVerticalRuler
{
	typedef CVerticalRuler BaseClass;
protected:
	CAmplitudeRuler();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAmplitudeRuler)

// Attributes
public:
	CWaveSoapFrontDoc* GetDocument();
	static int CalculateWidth();
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAmplitudeRuler)
	virtual void OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint );
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL
	virtual void VerticalScrollByPixels(int Pixels);
	virtual void BeginMouseTracking();

// Implementation
protected:
	virtual ~CAmplitudeRuler();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
protected:
	double m_VerticalScale;    // full sweep divided by channel rectangle height (for full height channels only). 1 - full sweep equal rect height, > 1 - show magnified.
	// minimum height channels are shown with scale 1.
	double m_WaveOffsetY;      // additional vertical offset, to see a region of magnified wave. Only full height channels are scrolled vertically. This is the sample value
	// of the center line of the channel clip rect. Full range is from -1. to + 1. float
	double m_MaxAmplitudeRange;		// 1. for integer data, sqrt(10) for float data
	double m_WaveOffsetBeforeScroll;
	int m_MouseYOffsetForScroll;

	NotifyChannelHeightsData m_Heights;

	void SetNewAmplitudeOffset(double offset);
	enum eDrawMode
	{
		SampleView = 0,
		PercentView = 1,
		DecibelView = 2,
	};
	eDrawMode m_DrawMode;

	// ChannelRect - rectangle occupied by the waveform
	// ClipRect - includes ChannelRect, and also the optional separator line.
	void DrawChannelSamples(CDC * pDC, CRect const & ChannelRect, CRect const & ClipRect);
	void DrawChannelPercents(CDC * pDC, CRect const & ChannelRect, CRect const & ClipRect);
	void DrawChannelDecibels(CDC * pDC, CRect const & ChannelRect, CRect const & ClipRect);

	// Generated message map functions
	//{{AFX_MSG(CAmplitudeRuler)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnViewAmplRulerSamples();
	afx_msg void OnViewAmplRulerPercent();
	afx_msg void OnViewAmplRulerDecibels();
	afx_msg void OnUpdateAmplRulerSamples(CCmdUI * pCmdUI);
	afx_msg void OnUpdateAmplRulerPercent(CCmdUI * pCmdUI);
	afx_msg void OnUpdateAmplRulerDecibels(CCmdUI * pCmdUI);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnUwmNotifyViews(WPARAM wParam, LPARAM lParam);
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

#ifndef _DEBUG  // debug version in AmplitudeRuler.cpp
inline CWaveSoapFrontDoc* CAmplitudeRuler::GetDocument()
{ return (CWaveSoapFrontDoc*)m_pDocument; }
#endif
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionRuler view

class CSpectrumSectionRuler : public CHorizontalRuler
{
	typedef CHorizontalRuler BaseClass;
protected:
	CSpectrumSectionRuler();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CSpectrumSectionRuler)

// Attributes
public:
	CWaveSoapFrontDoc* GetDocument();
// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpectrumSectionRuler)
	virtual void OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint );
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL
	virtual void HorizontalScrollByPixels(int Pixels);	// Pixels >0 - picture moves to the right, pixels <0 - picture moves to the left
	virtual void BeginMouseTracking();

	double WindowXToDb(int x) const;
	int DbToWindowX(double db) const;

	// Implementation
protected:
	virtual ~CSpectrumSectionRuler();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	double m_DbOffset;	// dB level for the pixel at m_XOrigin, 0 or negative. For floating point source, overshoot is allowed
	double m_DbPerPixel;
	double m_Scale;
	int m_XOrigin;		// usually the window width
	int m_MouseXOffsetForScroll;
	double m_DbOffsetBeforeScroll;
	// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//{{AFX_MSG(CSpectrumSectionRuler)
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnUwmNotifyViews(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

#ifndef _DEBUG  // debug version in AmplitudeRuler.cpp
inline CWaveSoapFrontDoc* CSpectrumSectionRuler::GetDocument()
{ return (CWaveSoapFrontDoc*)m_pDocument; }
#endif
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AMPLITUDERULER_H__7C3E5800_3F6E_11D4_9ADD_00C0F0583C4B__INCLUDED_)
