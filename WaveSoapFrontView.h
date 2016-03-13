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
enum
{
	MovePointIntoView_MakeCenter = SetSelection_MoveCaretToCenter,
	MovePointIntoView_KeepAutoscrollWidth = SetSelection_Autoscroll,
	MovePointIntoView_DontAdjustView = SetSelection_DontAdjustView,
};
class CWaveSoapViewBase : public CView
{
	typedef CView BaseClass;
protected: // create from serialization only
	CWaveSoapViewBase();
	typedef CWaveSoapFrontDoc ThisDoc;

	DECLARE_DYNAMIC(CWaveSoapViewBase);

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	double GetHorizontalScale() const { return m_HorizontalScale; }
	int SampleToX(SAMPLE_INDEX sample) const;      // floor
	int SampleToXceil(SAMPLE_INDEX sample) const;
	double WindowXtoSample(int x) const;
	void SetHorizontalScale(double NewHorizontalScale, int ZoomCenter);    // if ZoomCenter MAX_INT, center the current selection

	// scroll_offset < 0 - image moves to the right, first pixel in view decremented
	// scroll_offset > 0 - image moves to the left, first pixel in view incremented
	void HorizontalScrollBy(double samples);
	void HorizontalScrollByPixels(int pixels);	// Pixels >0 - picture moves to the right, pixels <0 - picture moves to the left

	NUMBER_OF_SAMPLES WaveFileSamples() const
	{
		return GetDocument()->WaveFileSamples();
	}
public:

	ThisDoc* GetDocument() const;

	void GetChannelRect(int Channel, RECT * r) const;
	void GetChannelClipRect(int Channel, RECT * pR) const;
	int GetChannelFromPoint(int y) const;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapViewBase)
public:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

	void InvalidateMarkerRegion(WAVEREGIONINFO const * pInfo);

	void RecalculateChannelHeight(int cy);
	virtual DWORD ClientHitTest(CPoint p) const = 0;

	void SetFirstSampleInView(double sample);
	void MovePointIntoView(SAMPLE_INDEX nCaret, int Flags = 0);
	void AdjustCaretVisibility(SAMPLE_INDEX CaretPos, SAMPLE_INDEX OldCaretPos,
								unsigned Flags, SAMPLE_INDEX SelectionBegin = 0, SAMPLE_INDEX SelectionEnd = 0,
								SAMPLE_INDEX OldSelectionBegin = 0, SAMPLE_INDEX OldSelectionEnd = 0);

	void UpdateHorizontalExtents(NUMBER_OF_SAMPLES Length, int WindowWidth);

	double m_HorizontalScale;   // number of samples per window pixel. Can be 0.5, .25, etc for additional stretch
	double m_PrevHorizontalScale;
	virtual void UpdateCaretPosition();
	void CreateCursorBitmap(CBitmap & bmp, CHANNEL_MASK Channel);
	virtual POINT GetZoomCenter();

	// playback cursor:
	virtual void DrawPlaybackCursor(CDC * pDC, SAMPLE_INDEX Sample, CHANNEL_MASK Channel);
	virtual void ShowPlaybackCursor(CDC * pDC = NULL);
	virtual void HidePlaybackCursor(CDC * pDC = NULL);
	void UpdatePlaybackCursor(SAMPLE_INDEX sample, CHANNEL_MASK channel, SAMPLE_INDEX last_sample);
	bool PlaybackCursorVisible() const;
	CBitmap m_PlaybackCursorBitmap;
	CHANNEL_MASK m_PlaybackCursorChannel;  // 0 = not playing
	bool m_PlaybackCursorDrawn;
	SAMPLE_INDEX m_PlaybackCursorDrawnSamplePos;

	// cursor and selection:
	void CreateAndShowCaret(bool ForceCreateCaret = false);
	void InvalidateMarkerLabels(int dy = 0);

	bool m_NewSelectionMade;             // during playback, the caret is hidden unless a different selection were made
	UINT_PTR m_AutoscrollTimerID;
	CBitmap m_CursorBitmap;
	int m_WheelAccumulator;
	int nKeyPressed;
	bool bIsTrackingSelection;
	bool m_HasFocus;
	bool m_CaretShown;
	bool m_CaretCreated;
	CHANNEL_MASK m_CurrentCaretChannels;

	double m_FirstSampleInView; // first sample that maps to client.x=0; Can be fractional if m_HorizontalScale < 1. Always multiple of m_HorScale.
	NotifyChannelHeightsData m_Heights;
	int m_InvalidAreaTop[MAX_NUMBER_OF_CHANNELS];
	int m_InvalidAreaBottom[MAX_NUMBER_OF_CHANNELS];

	CDataSection<float, CWaveSoapViewBase> m_WaveBuffer;

	enum
	{
		VSHT_CHANNEL_MINIMIZED = 0x80000,    // the channel is shown minimized
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
	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveSoapViewBase)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnUwmNotifyViews(WPARAM wParam, LPARAM lParam);
};

class CWaveSoapFrontView : public CWaveSoapViewBase
{
	typedef CWaveSoapViewBase BaseClass;
	CWaveSoapFrontView();
	DECLARE_DYNCREATE(CWaveSoapFrontView);

	// Attributes
public:
	friend class CAmplitudeRuler;
	int SampleValueToY(double value, int ch) const;

	void SetVerticalScaleIndex(int NewVerticalScaleIndex);

	//virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll, RECT const * pScrollRect, RECT const * pClipRect);
	// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapFrontView)
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void OnInitialUpdate();
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CWaveSoapFrontView();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
protected:

	// how many samples in the display point

	void DrawHorizontalWithSelection(CDC * pDC,
									int SelectionLeft, int SelectionRight, int Y,
									CPen * NormalPen, CPen * SelectedPen,
									CHANNEL_MASK nChannel, LPCRECT ClipRect);

	virtual DWORD ClientHitTest(CPoint p) const;

	// multiply the wave to get the additional magnification.
	// m_VerticalScale means all the range is shown, scale 2 means the wave
	// is magnified 2 times.
	double m_VerticalScale;     // full sweep divided by channel rectangle height (for full height channels only). 1 - full sweep equal rect height, > 1 - show magnified.
	// minimum height channels are shown with scale 1.
	int m_VerticalScaleIndex;
	double m_WaveOffsetY; // additional vertical offset, to see a region of magnified wave. Only full height channels are scrolled vertically.
	//	This is the sample value of the center line of the channel clip rect. Full range is from -1. to + 1. float
	double m_MaxAmplitudeRange;		// 1. for integer data, sqrt(10) for float data
	void SetNewAmplitudeOffset(double offset);
	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveSoapFrontView)
	afx_msg void OnViewZoominHor2();
	afx_msg void OnViewZoomOutHor2();
	afx_msg void OnUpdateViewZoominhor2(CCmdUI* pCmdUI);
	afx_msg void OnViewZoomInVert();
	afx_msg void OnViewZoomOutVert();
	afx_msg void OnUpdateViewZoomInVert(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewZoomOutVert(CCmdUI* pCmdUI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewZoomvertNormal();
	afx_msg void OnUpdateViewZoomvertNormal(CCmdUI* pCmdUI);
	afx_msg void OnViewZoominHorFull();
	afx_msg void OnUpdateViewZoominHorFull(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewZoomSelection(CCmdUI* pCmdUI);
	afx_msg void OnViewZoomSelection();
	afx_msg void OnUpdateIndicatorScale(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewHorScale(CCmdUI* pCmdUI);
	afx_msg void OnViewHorScale(UINT command);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnViewMinimize0(UINT id);
	afx_msg void OnViewMaximize0(UINT id);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnViewZoomprevious();
protected:
	afx_msg LRESULT OnUwmNotifyViews(WPARAM wParam, LPARAM lParam);
public:
};

//#define VSHT_

#ifndef _DEBUG  // debug version in WaveSoapFrontView.cpp
inline CWaveSoapFrontDoc* CWaveSoapViewBase::GetDocument() const
{
	return static_cast<ThisDoc *>(m_pDocument);
}
#endif

class WaveCalculate
{
public:
	WaveCalculate(double offset, double scale, int top, int bottom);
	// full range is from -1. to 1.
	long operator()(double w)
	{
		return m_Offset - (long)floor((w)* m_Scale + 0.5);
	}
	long operator()(int w)
	{
		return m_Offset - (long)floor(((w + 0.5) / 32768.)* m_Scale + 0.5);
	}

	// integer sample in range -32768 to + 32768
	int ConvertToSample(int y)
	{
		return int((32768 * (m_Offset - y - 0.5) / m_Scale) - 0.5);
	}

	double ConvertToDoubleSample(int y)
	{
		return (m_Offset - y - 0.5) / m_Scale;
	}
	double AdjustOffset(double offset, double MinRange = -1., double MaxRange = 1.);

protected:
	double m_Scale;		//
	long m_Offset;		// Y coordinate of the zero line
	int m_ViewHeight;	// height of actual view
	int m_Height;		// height of full range from -1. to 1. Float samples can take larger range
};
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVESOAPFRONTVIEW_H__FFA16C4E_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
