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
#include "GdiObjectSave.h"
#include "MainFrameEx.h"

typedef FrameExParameters<MainFrameRememberMaximized
						| MainFrameNeatCtrlTab | MainFrameRecalcLayoutOnDisplayChange
						| MainFrameRecalcLayoutOnSettingChange
						| MainFrameHandlePaletteChange,
						DialogProxyWnd<CMDIFrameWnd> >

	MainFrameExParameters;

class CMainFrame : public CMainFrameExT<MainFrameExParameters>
{
	typedef CMainFrameExT<MainFrameExParameters> BaseClass;
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Attributes
public:
	void GetMessageString(UINT nID, CString& rMessage) const;
// Operations
public:
	using CMDIFrameWnd::OnToolTipText;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
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

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnBarCheckStatusBar(UINT nID);
	afx_msg BOOL OnBarCheckToolbar(UINT nID);
	afx_msg BOOL OnBarCheckRebar(UINT nID);
	//}}AFX_MSG
	afx_msg void OnUpdateIndicatorFileSize(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorSampleRate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorSampleSize(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorChannels(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()
	LRESULT OnResetLastStatusMessage(WPARAM, LPARAM)
	{
		m_nIDLastMessage = 0;
		return 0;
	}
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__FFA16C48_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
