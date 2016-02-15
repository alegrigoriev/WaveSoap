// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#if !defined(AFX_WAVEFFTVIEW_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_WAVEFFTVIEW_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WaveFftView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWaveFftView view
#include "WaveSoapFrontView.h"

class CWaveFftView : public CWaveSoapViewBase
{
	typedef CWaveSoapViewBase BaseClass;
protected:
	CWaveFftView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWaveFftView)

// Attributes
public:
	static void FillLogPalette(LOGPALETTE * pal, int nEntries);
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveFftView)
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnInitialUpdate();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL
	enum { MaxDrawColumnPerOnDraw = 128};
// Implementation
protected:
	virtual ~CWaveFftView();
	virtual UINT GetPopupMenuID(CPoint point);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	DWORD ClientHitTest(CPoint p) const;
	// FFT array is stored as circular buffer of FFT columns.
	// each column is a row of the array of m_FftResultArrayHeight bytes
	// m_IndexOfFftBegin is number of row with m_FftResultBegin
	// FFT array is stored as a ring array of 'm_FftResultArrayWidth' columns
	// m_IndexOfFftBegin is offset in the array of the beginning column.
	// m_FftResultBegin holds the median sample index of the first column.
	// m_FftResultEnd holds the median sample index of the last column.
	float * m_pFftResultArray;
	unsigned m_FftArraySize;
	int m_FftResultArrayWidth;    // number of FFT sets
	int m_FftResultArrayHeight;   // m_FftOrder * pDoc->WaveChannels() + 1;
	int m_IndexOfFftBegin;

	long         m_FirstFftColumn;      // at m_IndexOfFftBegin

	double m_FftLogRange;     // what dB zero value corresponds
	double m_FirstbandVisible;     // how much the chart is scrolled. 0 = DC is visible
	double m_VerticalScale;

	ATL::CHeapPtr<float> m_pFftWindow;
	bool m_FftWindowValid;
	typedef double DATA;
	ATL::CHeapPtr<DATA> m_pFftBuf;  // for calculations, sized  m_FftOrder * 2 + 2
	enum
	{
		WindowTypeSquaredSine = 0,
		WindowTypeHalfSine = 1,
		WindowTypeHamming = 2,
		WindowTypeNuttall = 3,
	};

	int m_FftWindowType;
	int m_FftOrder;     // frequencies in FFT conversions (window width=2*FFT order)
	int m_FftSpacing;   // samples between FFT columns. Minimum m_FftOrder, or m_HorizontalScale

	void OnSetWindowType(int window);

	void AllocateFftArray(SAMPLE_INDEX SampleLeft, SAMPLE_INDEX SampleRight);

	float const * GetFftResult(SAMPLE_INDEX sample, unsigned channel);

	// to which FFT displayed column the sample falls in (samples from N*m_FftSpacing +/- m_FftSpacing/2).
	// Use for display conversion only
	long SampleToFftColumn(SAMPLE_INDEX sample);
	// which leftmost FFT column the sample affects:
	long SampleToFftColumnLowerBound(SAMPLE_INDEX sample);
	// which rightmost FFT column the sample affects:
	long SampleToFftColumnUpperBound(SAMPLE_INDEX sample);

	SAMPLE_INDEX FftColumnToDisplaySample(long Column);
	SAMPLE_INDEX SampleToFftBaseSample(SAMPLE_INDEX sample);
	SAMPLE_INDEX DisplaySampleToFftBaseSample(SAMPLE_INDEX sample);
	void InvalidateFftColumnRange(long first_column, long last_column);  // including last
	void SetVerticalScale(double NewVerticalScale);

	double AdjustOffset(double offset) const;
	void SetNewFftOffset(double first_band);

	void RedrawSelectionRect(CDC * pDC, SAMPLE_INDEX OldSelectionStart, SAMPLE_INDEX OldSelectionEnd, CHANNEL_MASK OldSelectedChannel,
							SAMPLE_INDEX NewSelectionStart, SAMPLE_INDEX NewSelectionEnd, CHANNEL_MASK NewSelectedChannel);
	int BuildSelectionRegion(CRgn * NormalRgn, CRgn* MinimizedRgn,
							SAMPLE_INDEX SelectionStart, SAMPLE_INDEX SelectionEnd, CHANNEL_MASK selected);

	void RemoveSelectionRect();
	void ShowSelectionRect();

	SAMPLE_INDEX m_PrevSelectionStart;
	SAMPLE_INDEX m_PrevSelectionEnd;
	CHANNEL_MASK m_PrevSelectedChannel;
	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveFftView)
	afx_msg void OnViewZoomvertNormal();
	afx_msg void OnViewZoomInVert();
	afx_msg void OnViewZoomOutVert();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnFftBands(UINT id);
	afx_msg void OnUpdateFftBands(CCmdUI* pCmdUI);
	afx_msg void OnFftWindowSquaredSine();
	afx_msg void OnUpdateFftWindowSquaredSine(CCmdUI* pCmdUI);
	afx_msg void OnFftWindowSine();
	afx_msg void OnUpdateFftWindowSine(CCmdUI* pCmdUI);
	afx_msg void OnFftWindowHamming();
	afx_msg void OnUpdateFftWindowHamming(CCmdUI* pCmdUI);
	afx_msg void OnFftWindowNuttall();
	afx_msg void OnUpdateFftWindowNuttall(CCmdUI* pCmdUI);
	afx_msg void OnViewDecreaseFftBands();
	afx_msg void OnViewIncreaseFftBands();
	afx_msg void OnUpdateViewZoomInVert(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewZoomOutVert(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewZoomvertNormal(CCmdUI* pCmdUI);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	void OnSetBands(int order);

	friend class CSpectrumSectionView;
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnUwmNotifyViews(WPARAM wParam, LPARAM lParam);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVEFFTVIEW_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_)
