#if !defined(AFX_WAVEOUTLINEVIEW_H__049155A1_4B99_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_WAVEOUTLINEVIEW_H__049155A1_4B99_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WaveOutlineView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWaveOutlineView view

class CWaveOutlineView : public CView
{
protected:
	CWaveOutlineView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWaveOutlineView)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveOutlineView)
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CWaveOutlineView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveOutlineView)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVEOUTLINEVIEW_H__049155A1_4B99_11D4_9ADD_00C0F0583C4B__INCLUDED_)
