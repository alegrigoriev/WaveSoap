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
protected:
	CAmplitudeRuler();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CAmplitudeRuler)

// Attributes
public:
	CWaveSoapFrontDoc* GetDocument();
	static int CalculateWidth();
// Operations
public:
	enum
	{
		SampleView = 0,
		PercentView = 1,
		DecibelView = 2,
	};

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAmplitudeRuler)
	virtual void OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint );
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateMaxExtents();
	virtual ~CAmplitudeRuler();
	virtual UINT GetPopupMenuID(CPoint) { return IDR_MENU_AMPLITUDE_RULER; }
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	int m_DrawMode;
	void DrawSamples(CDC * pDC);
	void DrawPercents(CDC * pDC);
	void DrawDecibels(CDC * pDC);

	// Generated message map functions
protected:
	//{{AFX_MSG(CAmplitudeRuler)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnViewAmplRulerSamples();
	afx_msg void OnViewAmplRulerPercent();
	afx_msg void OnViewAmplRulerDecibels();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
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

// Implementation
protected:
	void UpdateMaxExtents();
	virtual ~CSpectrumSectionRuler();
//    virtual UINT GetPopupMenuID(CPoint) { return IDR_MENU_SPECTRUM_SECTION_RULER; }
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//{{AFX_MSG(CSpectrumSectionRuler)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in AmplitudeRuler.cpp
inline CWaveSoapFrontDoc* CSpectrumSectionRuler::GetDocument()
{ return (CWaveSoapFrontDoc*)m_pDocument; }
#endif
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AMPLITUDERULER_H__7C3E5800_3F6E_11D4_9ADD_00C0F0583C4B__INCLUDED_)
