// WaveSoapFrontView.h : interface of the CWaveSoapFrontView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAVESOAPFRONTVIEW_H__FFA16C4E_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_WAVESOAPFRONTVIEW_H__FFA16C4E_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ScaledScrollView.h"
#include "WaveSoapFrontDoc.h"

class CWaveSoapFrontView : public CScaledScrollView
{
protected: // create from serialization only
	CWaveSoapFrontView();
	DECLARE_DYNCREATE(CWaveSoapFrontView)

// Attributes
public:
	friend class CAmplitudeRuler;
	CWaveSoapFrontDoc* GetDocument();

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
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE);
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
	// how many samples in the display point
	int m_HorizontalScale;
	// multiply the wave to get the additional magnification.
	// m_VerticalScale means all the range is shown, scale 2 means the wave
	// is magnified 2 times.
	double m_VerticalScale;
	enum {
		WAVE_OFFSET_CHANGED = 0x10000,
		WAVE_SCALE_CHANGED = 0x20000,
	};
	// additional vertical offset, to see a region of magnified wave
	double m_WaveOffsetY;
	virtual void AdjustNewScale(double OldScaleX, double OldScaleY,
								double & NewScaleX, double & NewScaleY);
	virtual BOOL ScrollBy(double dx, double dy, BOOL bDoScroll = TRUE);
	DWORD m_FirstSampleInBuffer;    // in 16-bit numbers
	__int16 * m_pWaveBuffer;
	size_t m_WaveBufferSize;    // in 16-bit samples
	size_t m_WaveDataSizeInBuffer;  // in 16-bit samples
	void GetWaveSamples(int Position, int NumOfSamples);
	void DrawHorizontalWithSelection(CDC * pDC,
									int left, int right, int Y,
									CPen * NormalPen, CPen * SelectedPen,
									int nChannel);
	void CreateAndShowCaret();
	DWORD ClientHitTest(CPoint p);
	virtual POINT GetZoomCenter();
	void MovePointIntoView(int nCaret);

	virtual void DrawPlaybackCursor(CDC * pDC, long Sample, int Channel);
	virtual void ShowPlaybackCursor(CDC * pDC = NULL);
	virtual void HidePlaybackCursor(CDC * pDC = NULL);
	void UpdatePlaybackCursor(long sample, int channel);
	BOOL PlaybackCursorVisible();
	int m_PlaybackChannel;  // -1 = not playing
	bool m_PlaybackCursorDrawn;
	long m_LastPlaybackCursorPos;
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
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define VSHT_BCKGND         0x10000     // in the middle area (selecting both channels
#define VSHT_LEFT_CHAN      0x08000     // in the upper half of the left chan
#define VSHT_RIGHT_CHAN     0x04000     // in the lower half of the right chan
#define VSHT_SEL_BOUNDARY_L 0x02000
#define VSHT_SEL_BOUNDARY_R 0x01000     // on the selection boundary
#define VSHT_NOWAVE         0x00800     // after the data end
#define VSHT_SELECTION      0x00400     // inside the selection
#define VSHT_WAVEFORM       0x00200     // on the waveform (allows drag)
#define VSHT_NONCLIENT      0x00100     // out of the client area
//#define VSHT_

#ifndef _DEBUG  // debug version in WaveSoapFrontView.cpp
inline CWaveSoapFrontDoc* CWaveSoapFrontView::GetDocument()
{ return (CWaveSoapFrontDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVESOAPFRONTVIEW_H__FFA16C4E_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
