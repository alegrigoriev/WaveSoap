// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_SPECTRUMSECTIONVIEW_H__6686D1B7_327B_4095_B596_07E91D2CB8FB__INCLUDED_)
#define AFX_SPECTRUMSECTIONVIEW_H__6686D1B7_327B_4095_B596_07E91D2CB8FB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpectrumSectionView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView view

class CSpectrumSectionView : public CView
{
	typedef CView BaseClass;
protected:
	CSpectrumSectionView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CSpectrumSectionView)

// Attributes
public:
	CWaveSoapFrontDoc* GetDocument() const;
	virtual UINT GetPopupMenuID(CPoint point);

	BOOL m_bShowNoiseThreshold;

	double m_dNoiseThresholdLow;
	double m_dNoiseThresholdHigh;
	int m_nBeginFrequency;
	bool m_bShowCrossHair;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpectrumSectionView)
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnInitialUpdate();
	//}}AFX_VIRTUAL

// Implementation
protected:
	CPoint m_PrevCrossHair;
	bool m_bCrossHairDrawn;
	bool m_bTrackingMouseRect;

	void ShowCrossHair(POINT point, CDC * pDC = NULL);
	void HideCrossHair(CDC * pDC = NULL);
	void DrawCrossHair(POINT point, CDC * pDC = NULL);
	void GetChannelRect(int Channel, RECT * pR) const;
	void GetChannelClipRect(int Channel, RECT * pR) const;

	BOOL AllocateFftArrays();
	void CalculateFftPowerSum(float * pFftSum, SAMPLE_INDEX FirstSample,
							int NumberOfSamplesAveraged, int FftOrder);

	int m_FftOrder;     // frequencies in FFT conversions (window width=2*FFT order)

	float * m_pFftSum;
	ATL::CHeapPtr<float> m_pFftWindow;
	bool m_FftWindowValid;

	class NoiseReductionCore * m_pNoiseReduction;
	int m_NrFftOrder;
	int m_FftWindowType;
	int m_nFftSumSize;
	long m_PlaybackSample;

	double m_DbOffset;	// dB for the rightmost pixel
	double m_DbMin;
	double m_DbMax;
	double m_DbPerPixel;
	double m_Scale;		// DbRange / m_DbRangeInView, max 256, min 1
	int m_XOrigin;

	double WindowXToDb(int x) const;
	int DbToWindowX(double db) const;

	NotifyChannelHeightsData m_Heights;

	int m_InvalidAreaTop[MAX_NUMBER_OF_CHANNELS];
	int m_InvalidAreaBottom[MAX_NUMBER_OF_CHANNELS];

	double m_FirstbandVisible;     // how much the chart is scrolled. 0 = DC is visible
	double m_VerticalScale;

	bool m_bIsTrackingSelection;
	virtual ~CSpectrumSectionView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	virtual void RemoveSelectionRect();
	virtual void RestoreSelectionRect();
	void SetNewFftOffset(double first_band);
	double AdjustOffset(double offset) const;
	LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
	//{{AFX_MSG(CSpectrumSectionView)
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnUpdateViewSsShowNoiseThreshold(CCmdUI* pCmdUI);
	afx_msg void OnViewSsShowNoiseThreshold();
	afx_msg void OnViewSsZoominhor2();
	afx_msg void OnUpdateViewSsZoominhor2(CCmdUI* pCmdUI);
	afx_msg void OnViewSsZoomouthor2();
	afx_msg void OnUpdateViewSsZoomouthor2(CCmdUI* pCmdUI);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnUpdateViewShowCrosshair(CCmdUI* pCmdUI);
	afx_msg void OnViewShowCrosshair();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnUwmNotifyViews(WPARAM wParam, LPARAM lParam);
};

#ifndef _DEBUG  // debug version in WaveSoapFrontView.cpp
inline CWaveSoapFrontDoc* CSpectrumSectionView::GetDocument() const
{ return (CWaveSoapFrontDoc*)m_pDocument; }
#endif
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECTRUMSECTIONVIEW_H__6686D1B7_327B_4095_B596_07E91D2CB8FB__INCLUDED_)
