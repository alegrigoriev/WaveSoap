#if !defined(AFX_RULER_H__0AB440C1_3E09_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_RULER_H__0AB440C1_3E09_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Ruler.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHorizontalRuler view

class CHorizontalRuler : public CView
{
protected:
	CHorizontalRuler();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CHorizontalRuler)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHorizontalRuler)
protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CHorizontalRuler();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CHorizontalRuler)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULER_H__0AB440C1_3E09_11D4_9ADD_00C0F0583C4B__INCLUDED_)
