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
#include "ScaledScrollView.h"

class CSpectrumSectionView : public CScaledScrollView
{
	typedef CScaledScrollView BaseClass;
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

	struct FftGraphBand;

	BOOL AllocateFftArrays();
	int InitBandArray(ATL::CHeapPtr<FftGraphBand> & pBands, int rows, int FftOrder);
	void CalculateFftPowerSum(float * pFftSum, SAMPLE_INDEX FirstSample,
							int NumberOfSamplesAveraged, int FftOrder);

	void BuildBandArray(double PowerScaleCoeff, FftGraphBand * pBands, int NumBands,
						float const * pFftSum, int FftOrder);

	typedef POINT DoublePoint[2];

	void BuildPointArray(FftGraphBand * pBands, int NumBands, DoublePoint * ppArray, int nNumberOfPoints, int OffsetY);
	void DrawPointArray(CDC * pDC, DoublePoint * ppArray, int NumberOfPoints, int right);

	int m_FftOrder;     // frequencies in FFT conversions (window width=2*FFT order)

	float * m_pFftSum;
	float * m_pWindow;
	class NoiseReductionCore * m_pNoiseReduction;
	int m_NrFftOrder;

	int m_nFftSumSize;
	long m_PlaybackSample;

	double m_DbOffset;
	double m_DbRange;
	double m_DbRangeInView;
	double m_DbPerPixel;
	NotifyChannelHeightsData m_Heights;

	virtual ~CSpectrumSectionView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	virtual void RemoveSelectionRect();
	virtual void RestoreSelectionRect();
	LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
	//{{AFX_MSG(CSpectrumSectionView)
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
