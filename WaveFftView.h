#if !defined(AFX_WAVEFFTVIEW_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_WAVEFFTVIEW_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WaveFftView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWaveFftView view

class CWaveFftView : public CWaveSoapFrontView
{
protected:
	CWaveFftView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWaveFftView)

// Attributes
public:
	friend class CFftRulerView;
	enum
	{
		FFT_OFFSET_CHANGED = 0x00300000,
		FFT_SCALE_CHANGED = 0x00400000,
		FFT_BANDS_CHANGED = 0x00500000,
	};
	static void FillLogPalette(LOGPALETTE * pal, int nEntries);
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveFftView)
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CWaveFftView();
	virtual BOOL ScrollBy(double dx, double dy, BOOL bDoScroll = TRUE);
	virtual UINT GetPopupMenuID(CPoint point);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	unsigned char * m_pFftResultArray;
	size_t m_FftArraySize;
	int m_FftResultArrayWidth;    // number of FFT sets
	int m_FftResultArrayHeight;   // number of frequencies
	int m_FftResultBegin;     // number of the first sample
	int m_FftResultEnd;     // number of the sample after the last
	//int m_FftSamplesCalculated;
	double m_FftLogRange;     // what dB zero value corresponds
	double m_FirstbandVisible;     // how much the chart is scrolled. 0 = DC is visible
	float * m_pFftWindow;
	int m_FftOrder;
	int m_FftSpacing;
	void MakeFftArray(int left, int right);
	void CalculateFftRange(int left, int right);
	static HBRUSH m_Brush;
	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveFftView)
	afx_msg void OnViewZoomvertNormal();
	afx_msg void OnViewZoomInVert();
	afx_msg void OnViewZoomOutVert();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnFftBands1024();
	afx_msg void OnUpdateFftBands1024(CCmdUI* pCmdUI);
	afx_msg void OnFftBands128();
	afx_msg void OnUpdateFftBands128(CCmdUI* pCmdUI);
	afx_msg void OnFftBands2048();
	afx_msg void OnUpdateFftBands2048(CCmdUI* pCmdUI);
	afx_msg void OnFftBands256();
	afx_msg void OnUpdateFftBands256(CCmdUI* pCmdUI);
	afx_msg void OnFftBands4096();
	afx_msg void OnUpdateFftBands4096(CCmdUI* pCmdUI);
	afx_msg void OnFftBands512();
	afx_msg void OnUpdateFftBands512(CCmdUI* pCmdUI);
	afx_msg void OnFftBands64();
	afx_msg void OnUpdateFftBands64(CCmdUI* pCmdUI);
	afx_msg void OnFftBands8192();
	afx_msg void OnUpdateFftBands8192(CCmdUI* pCmdUI);
	//}}AFX_MSG
	void OnUpdateBands(CCmdUI* pCmdUI, int number);
	void OnSetBands(int number);
	friend class CSpectrumSectionView;
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVEFFTVIEW_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_)
