// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__FFA16C48_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_MAINFRM_H__FFA16C48_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "MessageBoxSynch.h"

class CMainFrame : public DialogProxyWnd<CMDIFrameWnd>
{
	typedef DialogProxyWnd<CMDIFrameWnd> BaseClass;
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Attributes
public:
	void GetMessageString(UINT nID, CString& rMessage) const;
// Operations
public:
	void ResetLastStatusMessage()
	{
		m_nIDLastMessage = 0;
	}
	using CMDIFrameWnd::OnToolTipText;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);

protected:  // control bar embedded members
	CWaveSoapFrontStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CToolBar    m_wndToolBar2;
	CReBar      m_wndReBar;
	CDialogBar      m_wndDlgBar;
	int m_nRotateChildIndex;  // used for Ctrl+Tab handling

	afx_msg LRESULT OnDisplayChange(WPARAM, LPARAM);
	afx_msg LRESULT OnSettingChange(WPARAM, LPARAM);
// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg BOOL OnQueryNewPalette();
	afx_msg BOOL OnBarCheckStatusBar(UINT nID);
	afx_msg BOOL OnBarCheckToolbar(UINT nID);
	afx_msg BOOL OnBarCheckRebar(UINT nID);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnUpdateIndicatorFileSize(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorSampleRate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorSampleSize(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorChannels(CCmdUI* pCmdUI);
	LRESULT OnRunModalSync(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
	// synchronously runs the dialog from a worker thread in the main thread context
	INT_PTR OnMessageBoxSync(LPCTSTR );
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__FFA16C48_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
