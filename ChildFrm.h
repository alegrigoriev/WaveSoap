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
// CChildViewDialogBar dialog

class CChildViewDialogBar : public CDialog
{
// Construction
public:
	CChildViewDialogBar(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CChildViewDialogBar)
	enum { IDD = IDD_DIALOGBAR_MDI_CHILD };
	CButton	m_ClickRemoval;
	CButton	m_HumReduction;
	CButton	m_NoiseReduction;
	CTabCtrl	m_TabViewSwitch;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildViewDialogBar)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CChildViewDialogBar)
	afx_msg void OnSelchangeTabViewSwitch(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeTabSwitchViewMode(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
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
		VerticalRulerID,
		WaveViewID,
		FftViewID,
		ScaleStaticID,
		OutlineViewID};

	CWnd wStatic;
// Implementation
	BOOL m_bShowWaveform;
	BOOL m_bShowFft;
public:
	virtual ~CWaveMDIChildClient();
	void RecalcLayout(int cx, int cy);
	// Generated message map functions
protected:
	//{{AFX_MSG(CWaveMDIChildClient)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewShowFft();
	afx_msg void OnUpdateViewShowFft(CCmdUI* pCmdUI);
	afx_msg void OnViewWaveform();
	afx_msg void OnUpdateViewWaveform(CCmdUI* pCmdUI);
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
	CChildViewDialogBar m_dBar;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildFrame)
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void ActivateFrame(int nCmdShow);
	virtual void RecalcLayout(BOOL bNotify = TRUE);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CChildFrame)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHILDFRM_H__FFA16C4A_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
