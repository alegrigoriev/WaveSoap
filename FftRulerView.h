#if !defined(AFX_FFTRULERVIEW_H__66E60720_517D_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_FFTRULERVIEW_H__66E60720_517D_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FftRulerView.h : header file
//

#include "Ruler.h"
#include "resource.h"
/////////////////////////////////////////////////////////////////////////////
// CFftRulerView view

class CFftRulerView : public CVerticalRuler
{
protected:
	CFftRulerView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CFftRulerView)

// Attributes
public:
	CWaveSoapFrontDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFftRulerView)
	void OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint );
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CFftRulerView();
	virtual UINT GetPopupMenuID() { return IDR_MENU_FFT_RULER; }
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CFftRulerView)
	// NOTE - the ClassWizard will add and remove member functions here.
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in FftRulerView.cpp
inline CWaveSoapFrontDoc* CFftRulerView::GetDocument()
{ return (CWaveSoapFrontDoc*)m_pDocument; }
#endif
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FFTRULERVIEW_H__66E60720_517D_11D4_9ADD_00C0F0583C4B__INCLUDED_)
