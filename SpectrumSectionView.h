#if !defined(AFX_SPECTRUMSECTIONVIEW_H__6686D1B7_327B_4095_B596_07E91D2CB8FB__INCLUDED_)
#define AFX_SPECTRUMSECTIONVIEW_H__6686D1B7_327B_4095_B596_07E91D2CB8FB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpectrumSectionView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSpectrumSectionView view

class CSpectrumSectionView : public CScaledScrollView
{
protected:
	CSpectrumSectionView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CSpectrumSectionView)

// Attributes
public:
	CWaveSoapFrontDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpectrumSectionView)
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_FftOrder;
	double * m_pFftSum;
	double * m_pWindow;
	int m_nFftSumSize;
	long m_FftPosition;
	virtual ~CSpectrumSectionView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CSpectrumSectionView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in WaveSoapFrontView.cpp
inline CWaveSoapFrontDoc* CSpectrumSectionView::GetDocument()
{ return (CWaveSoapFrontDoc*)m_pDocument; }
#endif
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPECTRUMSECTIONVIEW_H__6686D1B7_327B_4095_B596_07E91D2CB8FB__INCLUDED_)
