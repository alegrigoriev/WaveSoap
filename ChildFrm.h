// ChildFrm.h : interface of the CChildFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHILDFRM_H__FFA16C4A_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_CHILDFRM_H__FFA16C4A_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "resource.h"
#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CVerticalTrackerBar view

class CVerticalTrackerBar : public CWnd
{
public:
	CVerticalTrackerBar();           // protected constructor used by dynamic creation
	virtual ~CVerticalTrackerBar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVerticalTrackerBar)
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL m_bTracking;
	int m_ClickPointX;
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CVerticalTrackerBar)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CMiniToolbar window

class CMiniToolbar : public CWnd
{
// Construction
public:
	CMiniToolbar();

// Attributes
public:

// Operations
public:
	void AddButton(CBitmap * pBitmap, int nID);

	struct Button
	{
		CBitmap * pBitmap;
		int nID;
		bool bEnabled;
	};
	vector<Button> m_Buttons;
	int GetHitCode(POINT point);
	void HiliteButton(int nID, bool Hilite);

	int m_ButtonClicked;
	bool m_MouseCaptured;
	bool m_LButtonPressed;
	int m_ButtonHilit;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMiniToolbar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMiniToolbar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMiniToolbar)
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

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
		SpectrumSectionViewID,
		VerticalTrackerID,
		ScaleStaticID,
		Static1ID,
		FftStaticLID,
		FftStaticUID,
		OutlineViewID,
		SpectrumSectionRulerID,
		WaveViewID = AFX_IDW_PANE_FIRST,
		FftViewID = AFX_IDW_PANE_FIRST + 16,
	};


	CWnd wStatic;
	CWnd wStatic1;
	CWnd wStaticFftU;
	CWnd wStaticFftL;

	CButton m_btZoomInVert;
	CButton m_btZoomInHor;
	CButton m_btZoomOutVert;
	CButton m_btZoomOutHor;
	CButton m_btZoomInVertFft;
	CButton m_btZoomInHorSpSec;
	CButton m_btZoomOutVertFft;
	CButton m_btZoomOutHorSpSec;

	CBitmap m_bmZoomInVert;
	CBitmap m_bmZoomInHor;
	CBitmap m_bmZoomOutVert;
	CBitmap m_bmZoomOutHor;
	CBitmap m_bmZoomInVertFft;
	CBitmap m_bmZoomInHorSpSec;
	CBitmap m_bmZoomOutVertFft;
	CBitmap m_bmZoomOutHorSpSec;

	CMiniToolbar m_FftZoomBar;

	CVerticalTrackerBar wTracker;
	CScrollBar m_sb;

// Implementation
	BOOL m_bShowWaveform;
	BOOL m_bShowFft;
	BOOL m_bShowOutline;
	BOOL m_bShowTimeRuler;
	BOOL m_bShowVerticalRuler;
	BOOL m_bShowSpectrumSection;
	int m_SpectrumSectionWidth;

public:
	virtual ~CWaveMDIChildClient();
	void RecalcLayout();
	// Generated message map functions
protected:
	afx_msg LRESULT OnDisplayChange(WPARAM, LPARAM);
	LRESULT OnSettingChange(WPARAM uFlags, LPARAM);
	//{{AFX_MSG(CWaveMDIChildClient)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewShowFft();
	afx_msg void OnUpdateViewShowFft(CCmdUI* pCmdUI);
	afx_msg void OnViewWaveform();
	afx_msg void OnUpdateViewWaveform(CCmdUI* pCmdUI);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnUpdateViewOutline(CCmdUI* pCmdUI);
	afx_msg void OnViewOutline();
	afx_msg void OnViewTimeRuler();
	afx_msg void OnUpdateViewTimeRuler(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewVerticalRuler(CCmdUI* pCmdUI);
	afx_msg void OnViewVerticalRuler();
	afx_msg void OnUpdateViewSpectrumsection(CCmdUI* pCmdUI);
	afx_msg void OnViewSpectrumsection();
	afx_msg void OnUpdateViewHideSpectrumsection(CCmdUI* pCmdUI);
	afx_msg void OnViewHideSpectrumsection();
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
// Operations
public:
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildFrame)
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
	CReBar      m_wndReBar;
	CToolBar    m_wndToolBar;
	CWaveSoapFrontStatusBar  m_wndStatusBar;
public:
	CWaveMDIChildClient m_wClient;
	virtual ~CChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CChildFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDFRM_H__FFA16C4A_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
