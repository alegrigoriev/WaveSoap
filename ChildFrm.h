// ChildFrm.h : interface of the CChildFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHILDFRM_H__FFA16C4A_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_CHILDFRM_H__FFA16C4A_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "resource.h"
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CWaveMDIChildClient window

class CWaveMDIChildClient : public CWnd
{
// Construction
public:
	CWaveMDIChildClient();

// Attributes
public:

// Operations
public:
	CWnd * CreateView(CRuntimeClass* pViewClass,
					CRect rect, int nID, CCreateContext* pContext, BOOL bShow = TRUE);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveMDIChildClient)
public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL
	enum { HorizontalRulerID = 1,
		VerticalWaveRulerID,
		VerticalFftRulerID,
		WaveViewID,
		FftViewID,
		ScaleStaticID,
		Static1ID,
		OutlineViewID};

	CWnd wStatic;
	CWnd wStatic1;
	CScrollBar m_sb;

// Implementation
	BOOL m_bShowWaveform;
	BOOL m_bShowFft;
public:
	virtual ~CWaveMDIChildClient();
	void RecalcLayout();
	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveMDIChildClient)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewShowFft();
	afx_msg void OnUpdateViewShowFft(CCmdUI* pCmdUI);
	afx_msg void OnViewWaveform();
	afx_msg void OnUpdateViewWaveform(CCmdUI* pCmdUI);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


class CChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CChildFrame)
public:
	CChildFrame();

// Attributes
public:
	CWaveMDIChildClient m_wClient;
// Operations
public:
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildFrame)
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void ActivateFrame(int nCmdShow);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
	CWaveSoapFrontStatusBar  m_wndStatusBar;
public:
	virtual ~CChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CChildFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDFRM_H__FFA16C4A_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
