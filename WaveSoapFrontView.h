// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveSoapFrontView.h : interface of the CWaveSoapFrontView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAVESOAPFRONTVIEW_H__FFA16C4E_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_WAVESOAPFRONTVIEW_H__FFA16C4E_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WaveSoapFrontDoc.h"
#include "DataSection.h"
#include <math.h>

class CWaveSoapFrontView : public CView
{
	typedef CView BaseClass;
protected: // create from serialization only
	typedef CWaveSoapFrontDoc ThisDoc;
	CWaveSoapFrontView();
	DECLARE_DYNCREATE(CWaveSoapFrontView);

// Attributes
public:
	friend class CAmplitudeRuler;
	ThisDoc * GetDocument() const;
	double GetHorizontalScale() const { return m_HorizontalScale; }
	int SampleToX(SAMPLE_INDEX sample) const;      // floor
	int SampleToXceil(SAMPLE_INDEX sample) const;
	SAMPLE_INDEX WindowXtoSample(int x) const;
	int SampleValueToY(double value, int ch) const;

	void SetHorizontalScale(double NewHorizontalScale, int ZoomCenter);    // if ZoomCenter MAX_INT, center the current selection
	void SetVerticalScale(double NewVerticalScale);

	virtual POINT GetZoomCenter();

	//virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll, RECT const * pScrollRect, RECT const * pClipRect);
	void HorizontalScrollBy(double samples);

	NUMBER_OF_SAMPLES WaveFileSamples() const
	{
		return GetDocument()->WaveFileSamples();
	}
	// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapFrontView)
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWaveSoapFrontView();
	virtual void UpdateCaretPosition();
	void InvalidateRect( LPCRECT lpRect, BOOL bErase = TRUE );

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
protected:
	void InvalidateMarkerRegion(WAVEREGIONINFO const * pInfo);

	virtual UINT GetPopupMenuID(CPoint point);
	// how many samples in the display point
	//virtual void AdjustNewScale(double OldScaleX, double OldScaleY, double & NewScaleX, double & NewScaleY);
	//virtual void AdjustNewOrigin(double & NewOrgX, double & NewOrgY);

	BOOL MasterScrollBy(double dx, double dy, BOOL bDoScroll);  // TODO: remove
	void DrawHorizontalWithSelection(CDC * pDC,
									int left, int right, int Y,
									CPen * NormalPen, CPen * SelectedPen,
									CHANNEL_MASK nChannel, LPCRECT ClipRect = NULL);

	void GetChannelRect(int Channel, RECT * r) const;
	void GetChannelClipRect(int Channel, RECT * pR) const;
	int GetChannelFromPoint(int y) const;

	void CreateAndShowCaret();
	DWORD ClientHitTest(CPoint p) const;

	void MovePointIntoView(SAMPLE_INDEX nCaret, BOOL Center = FALSE);
	void AdjustCaretVisibility(SAMPLE_INDEX CaretPos, SAMPLE_INDEX OldCaretPos,
								unsigned Flags);

	void UpdateHorizontalExtents(NUMBER_OF_SAMPLES Length, int WindowWidth);

	void SetFirstSampleInView(double sample);

	double m_HorizontalScale;   // number of samples per window pixel. Can be 0.5, .25, etc for additional stretch
	double m_PrevHorizontalScale;

	double m_FirstSampleInView; // first sample that maps to client.x=0; Can be fractional if m_HorizontalScale < 1. Always multiple of m_HorScale.
	// multiply the wave to get the additional magnification.
	// m_VerticalScale means all the range is shown, scale 2 means the wave
	// is magnified 2 times.
	double m_VerticalScale;     // full sweep divided by channel rectangle height (for full height channels only). 1 - full sweep equal rect height, > 1 - show magnified.
	// minimum height channels are shown with scale 1.

	double m_WaveOffsetY; // additional vertical offset, to see a region of magnified wave. Only full height channels are scrolled vertically. This is the sample value
	// of the center line of the channel clip rect

	CDataSection<float, CWaveSoapFrontView> m_WaveBuffer;

	virtual void DrawPlaybackCursor(CDC * pDC, SAMPLE_INDEX Sample, CHANNEL_MASK Channel);
	virtual void ShowPlaybackCursor(CDC * pDC = NULL);
	virtual void HidePlaybackCursor(CDC * pDC = NULL);
	void UpdatePlaybackCursor(SAMPLE_INDEX sample, CHANNEL_MASK channel);
	bool PlaybackCursorVisible() const;

	// cursor and selection:
	CHANNEL_MASK m_PlaybackCursorChannel;  // 0 = not playing
	bool m_PlaybackCursorDrawn;
	bool m_NewSelectionMade;
	UINT_PTR m_AutoscrollTimerID;
	SAMPLE_INDEX m_PlaybackCursorDrawnSamplePos;
	int m_WheelAccumulator;
	int nKeyPressed;
	bool bIsTrackingSelection;
	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveSoapFrontView)
	afx_msg void OnUpdateViewZoominhor(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewZoominhor2(CCmdUI* pCmdUI);
	afx_msg void OnViewZoomInVert();
	afx_msg void OnViewZoomOutVert();
	afx_msg void OnUpdateViewZoomInVert(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewZoomOutVert(CCmdUI* pCmdUI);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewZoomvertNormal();
	afx_msg void OnUpdateViewZoomvertNormal(CCmdUI* pCmdUI);
	afx_msg void OnViewZoominHorFull();
	afx_msg void OnUpdateViewZoominHorFull(CCmdUI* pCmdUI);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnUpdateViewZoomSelection(CCmdUI* pCmdUI);
	afx_msg void OnViewZoomSelection();
	afx_msg void OnUpdateIndicatorScale(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewHorScale(CCmdUI* pCmdUI);
	afx_msg void OnViewHorScale(UINT command);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg void OnViewZoominHor2();
	afx_msg void OnViewZoomOutHor2();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnViewZoomprevious();
	enum
	{
		VSHT_RIGHT_AUTOSCROLL  = 0x40000,    // autoscroll area
		VSHT_LEFT_AUTOSCROLL   = 0x20000,    // autoscroll area
		VSHT_BCKGND            = 0x10000,     // in the middle area (selecting both channels
		VSHT_LEFT_CHAN         = 0x08000,     // in the upper half of the left chan
		VSHT_RIGHT_CHAN        = 0x04000,     // in the lower half of the right chan
		VSHT_SEL_BOUNDARY_L    = 0x02000,
		VSHT_SEL_BOUNDARY_R    = 0x01000,     // on the selection boundary
		VSHT_NOWAVE            = 0x00800,     // after the data end
		VSHT_SELECTION         = 0x00400,     // inside the selection
		VSHT_WAVEFORM          = 0x00200,     // on the waveform (allows drag)
		VSHT_NONCLIENT         = 0x00100,     // out of the client area
		VSHT_CHANNEL_MASK      = 0x0001F,     // index of the hit channel
	};
protected:
	afx_msg LRESULT OnUwmNotifyViews(WPARAM wParam, LPARAM lParam);
};

//#define VSHT_

#ifndef _DEBUG  // debug version in WaveSoapFrontView.cpp
inline CWaveSoapFrontDoc* CWaveSoapFrontView::GetDocument() const
{
	return static_cast<ThisDoc *>(m_pDocument);
}
#endif

class WaveCalculate
{
public:
	WaveCalculate(double offset, double scale, int bottom, int top)
	{
		m_Scale = -((top - bottom) * scale) / 65536.;

		double MaxOffset = 65535.99 * (1. - 1. / scale);

		if (offset >= MaxOffset)
		{
			m_Offset = 65536. * (1. - 1. / scale);
		}
		else if (offset <= -MaxOffset)
		{
			m_Offset = -65536. * (1. - 1. / scale);
		}
		else
		{
			m_Offset = offset;
		}
		m_Offset = m_Offset * m_Scale + (bottom + top) / 2.;
	}

	int operator()(int w)
	{
		return (int)floor((w + 1) * m_Scale + m_Offset);
	}

	int ConvertToSample(int y)
	{
		return (int)floor((y - m_Offset) / m_Scale - 1);
	}

	double operator()(double w)
	{
		return (w + 1.) * m_Scale + m_Offset;
	}

	double ConvertToSample(double y)
	{
		return (y - m_Offset) / m_Scale - 1.;
	}

protected:
	double m_Offset;
	double m_Scale;
};
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVESOAPFRONTVIEW_H__FFA16C4E_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
