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

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveFftView)
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CWaveFftView();
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
	int m_FirstbandVisible;     // how much the chart is scrolled. 0 = DC is visible
	float * m_pFftWindow;
	int m_FftOrder;
	int m_FftSpacing;
	void MakeFftArray(int left, int right);
	void CalculateFftRange(int left, int right);
	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveFftView)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVEFFTVIEW_H__165978E0_39A0_11D4_9ADD_00C0F0583C4B__INCLUDED_)
