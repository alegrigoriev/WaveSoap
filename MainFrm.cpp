// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "WaveSoapFront.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_PALETTECHANGED()
	ON_WM_QUERYNEWPALETTE()
	ON_COMMAND_EX(ID_VIEW_STATUS_BAR, OnBarCheckStatusBar)
	ON_COMMAND_EX(ID_VIEW_TOOLBAR, OnBarCheckToolbar)
	ON_COMMAND_EX(ID_VIEW_REBAR, OnBarCheckRebar)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CMDIFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CMDIFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CMDIFrameWnd::OnHelpFinder)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_SIZE, OnUpdateIndicatorFileSize)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SAMPLE_RATE, OnUpdateIndicatorSampleRate)
	//ON_UPDATE_COMMAND_UI(ID_INDICATOR_SAMPLE_SIZE, OnUpdateIndicatorSampleSize)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CHANNELS, OnUpdateIndicatorChannels)
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
	ON_MESSAGE(WM_SETTINGCHANGE, OnSettingChange)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_SAMPLE_RATE,
//    ID_INDICATOR_SAMPLE_SIZE,
	ID_INDICATOR_CHANNELS,
	ID_INDICATOR_FILE_SIZE,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_nRotateChildIndex = 0;

}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CFrameWnd::OnSettingChange(uFlags, lpszSection);
	RecalcLayout();
}

LRESULT CMainFrame::OnDisplayChange(LPARAM lParam, WPARAM wParam)
{
	LRESULT result = CFrameWnd::OnDisplayChange(lParam, wParam);
	RecalcLayout();
	return result;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
#if 0
	if (!m_wndDlgBar.Create(this, IDR_MAINFRAME,
							CBRS_ALIGN_TOP, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
#endif
	if (!m_wndReBar.Create(this)
		|| !m_wndReBar.AddBar(&m_wndToolBar)
#if 0
		|| !m_wndReBar.AddBar(&m_wndDlgBar)
#endif
		)
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
									sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.EnableToolTips();

	// TODO: Remove this if you don't want tool tips
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
							CBRS_TOOLTIPS | CBRS_FLYBY);

	CThisApp * pApp = GetApp();
	if ( ! pApp->m_bShowStatusBar)
	{
		ShowControlBar( & m_wndStatusBar, FALSE, FALSE);
	}
	if ( ! pApp->m_bShowToolbar)
	{
		ShowControlBar( & m_wndToolBar, FALSE, FALSE);
	}

	return 0;
}

BOOL CMainFrame::OnBarCheckStatusBar(UINT nID)
{
	if (CFrameWnd::OnBarCheck(nID))
	{
		GetApp()->m_bShowStatusBar = (0 != (GetControlBar(nID)->GetStyle() & WS_VISIBLE));
		return TRUE;
	}
	return FALSE;
}

BOOL CMainFrame::OnBarCheckToolbar(UINT nID)
{
	if (CFrameWnd::OnBarCheck(nID))
	{
		GetApp()->m_bShowToolbar = (0 != (GetControlBar(nID)->GetStyle() & WS_VISIBLE));
		return TRUE;
	}
	return FALSE;
}

BOOL CMainFrame::OnBarCheckRebar(UINT nID)
{
	if (CFrameWnd::OnBarCheck(nID))
	{
		GetApp()->m_bShowToolbar = (0 != (GetControlBar(nID)->GetStyle() & WS_VISIBLE));
		return TRUE;
	}
	return FALSE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

void CMainFrame::GetMessageString(UINT nID, CString& rMessage) const
{
	// use either a status string from the topmost child,
	// or status string from application thread, or status string from one of other documents
	// find, starting with the topmost frame,
	// a document which performs an operation
	CFrameWnd * pFrameWnd = const_cast<CMainFrame *>(this)->GetActiveFrame();
	CString str;
	CWaveSoapFrontDoc * pDoc = NULL;
	CThisApp * pApp = GetApp();

	BOOL Topmost = TRUE;
	if (pFrameWnd)
	{
		pDoc = dynamic_cast<CWaveSoapFrontDoc *>(pFrameWnd->GetActiveDocument());
		if (NULL != pDoc)
		{
			pDoc->GetCurrentStatusString(str);
			if ( ! str.IsEmpty() && 0 == pDoc->m_OperationInProgress)
			{
				pDoc->SetCurrentStatusString(CString());
			}
		}
		else
		{
			TRACE("CMainFrame::GetMessageString: NULL == GetActiveDocument()\n");
		}
	}
	else
	{
		TRACE("CMainFrame::GetMessageString: NULL == GetActiveFrame()\n");
	}
	if (str.IsEmpty())
	{
		CWaveSoapFrontDoc * pTmpDoc = NULL;
		pApp->GetStatusStringAndDoc(str, & pTmpDoc);
		if (pTmpDoc != pDoc)
		{
			Topmost = FALSE;
		}
		pDoc = pTmpDoc;
		if ( ! str.IsEmpty() && 0 == pDoc->m_OperationInProgress)
		{
			pApp->SetStatusStringAndDoc(CString(), NULL);
		}
	}

	if (NULL != pFrameWnd && str.IsEmpty())
	{
		Topmost = FALSE;
		while (NULL != (pFrameWnd = dynamic_cast<CFrameWnd *>(pFrameWnd->GetWindow(GW_HWNDNEXT))))
		{
			pDoc = dynamic_cast<CWaveSoapFrontDoc *>(pFrameWnd->GetActiveDocument());
			if (NULL != pDoc)
			{
				pDoc->GetCurrentStatusString(str);
				if ( ! str.IsEmpty() && 0 == pDoc->m_OperationInProgress)
				{
					pDoc->SetCurrentStatusString(CString());
				}
			}
			else
			{
				TRACE("CMainFrame::GetMessageString: NULL == pFrameWnd->GetActiveDocument()\n");
			}
		}
	}

	if ( ! str.IsEmpty())
	{
		if (Topmost)
		{
			rMessage = str;
		}
		else
		{
			rMessage.Format(_T("%s: %s"), LPCTSTR(pDoc->GetTitle()),
							LPCTSTR(str));
		}
		return;
	}
	else
	{
		CMDIFrameWnd::GetMessageString(nID, rMessage);
	}
}

void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if ((GetStyle() & FWS_ADDTOTITLE) == 0)
		return;     // leave it alone!

#if 0//ndef _AFX_NO_OLE_SUPPORT
	// allow hook to set the title (used for OLE support)
	if (m_pNotifyHook != NULL && m_pNotifyHook->OnUpdateFrameTitle())
		return;
#endif

	CMDIChildWnd* pActiveChild = NULL;
	CDocument* pDocument = GetActiveDocument();
	if (bAddToTitle &&
		(pActiveChild = MDIGetActive()) != NULL &&
		(pActiveChild->GetStyle() & WS_MAXIMIZE) == 0 &&
		(pDocument != NULL ||
			(pDocument = pActiveChild->GetActiveDocument()) != NULL))
		// don't show file name for non-maximized window
		UpdateFrameTitleForDocument(NULL /* pDocument->GetTitle() */);
	else
	{
		LPCTSTR lpstrTitle = NULL;
		CString strTitle;

		if (pActiveChild != NULL)
		{
			strTitle = pActiveChild->GetTitle();
			if (!strTitle.IsEmpty())
				lpstrTitle = strTitle;
		}
		UpdateFrameTitleForDocument(lpstrTitle);
	}
}

void CMainFrame::OnUpdateIndicatorFileSize(CCmdUI* pCmdUI)
{
	SetStatusString(pCmdUI, _T(""), _T("00:00:00.000"), TRUE);
}

void CMainFrame::OnUpdateIndicatorSampleRate(CCmdUI* pCmdUI)
{
	SetStatusString(pCmdUI, _T(""), _T("44,100"), TRUE);
}

void CMainFrame::OnUpdateIndicatorSampleSize(CCmdUI* pCmdUI)
{
	SetStatusString(pCmdUI, _T(""), _T("16-bit"), TRUE);
}

void CMainFrame::OnUpdateIndicatorChannels(CCmdUI* pCmdUI)
{
	SetStatusString(pCmdUI, _T(""), _T("Stereo"), TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


void CMainFrame::OnPaletteChanged(CWnd* pFocusWnd)
{
	TRACE("CMainFrame::OnPaletteChanged\n");
	if (pFocusWnd != this)
	{
		OnQueryNewPalette();
	}
}

BOOL CMainFrame::OnQueryNewPalette()
{
	TRACE("CMainFrame::OnQueryNewPalette\n");
	CDC * dc = GetDC();
	CPalette* hOldPal = dc->SelectPalette(GetApp()->GetPalette(), FALSE);
	int redraw = dc->RealizePalette();
	if (redraw)
	{
		GetApp()->BroadcastUpdate();
	}
	dc->SelectPalette(hOldPal, FALSE);
	ReleaseDC(dc);
	//CMDIFrameWnd::OnQueryNewPalette();
	return TRUE;
}


BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	// catch Ctrl key down and up
	if (WM_KEYDOWN == pMsg->message)
	{
		if (VK_CONTROL == pMsg->wParam
			&& 0 == (0x40000000 & pMsg->lParam))
		{
			TRACE("Ctrl key was just pressed\n");
			m_nRotateChildIndex = 0;
		}
		else
		{
			if ((VK_TAB == pMsg->wParam || VK_F6 == pMsg->wParam)
				&& (0x8000 & GetKeyState(VK_CONTROL)))
			{
				CMDIChildWnd * pActive = MDIGetActive();
				if (NULL == pActive)
				{
					return TRUE;
				}
				CWnd * pBottom = pActive->GetWindow(GW_HWNDLAST);

				if (pBottom != pActive)
				{
					CWnd * pPlaceWnd = pActive;
					CWnd * pFrameToActivate;
					if (0x8000 & GetKeyState(VK_SHIFT))
					{
						if (m_nRotateChildIndex > 0)
						{
							for (int i = 0; i < m_nRotateChildIndex - 1; i++)
							{
								pPlaceWnd = pPlaceWnd->GetWindow(GW_HWNDNEXT);
								if (pPlaceWnd == pBottom)
								{
									break;
								}
							}
							m_nRotateChildIndex = i;
							if (pPlaceWnd == pBottom)
							{
								pFrameToActivate = pBottom;
								pPlaceWnd = pBottom->GetWindow(GW_HWNDPREV);
							}
							else
							{
								pFrameToActivate = pPlaceWnd->GetWindow(GW_HWNDNEXT);
							}
						}
						else
						{
							pFrameToActivate = pBottom;
							pPlaceWnd = pFrameToActivate;
							m_nRotateChildIndex = 1000;  // arbitrary big
						}
					}
					else
					{
						for (int i = 0; i < m_nRotateChildIndex; i++)
						{
							pPlaceWnd = pPlaceWnd->GetWindow(GW_HWNDNEXT);
							if (pPlaceWnd == pBottom)
							{
								break;
							}
						}
						m_nRotateChildIndex = i + 1;

						if (pPlaceWnd == pBottom)
						{
							pFrameToActivate = pActive->GetWindow(GW_HWNDNEXT);
							m_nRotateChildIndex = 0;
						}
						else
						{
							pFrameToActivate = pPlaceWnd->GetWindow(GW_HWNDNEXT);
						}
					}

					if (0) TRACE("m_nRotateChildIndex=%d, prev active=%X, pFrameToActivate=%X, pPlaceWnd=%X\n",
								m_nRotateChildIndex, pActive, pFrameToActivate, pPlaceWnd);

					// first activate new frame
					((CMDIChildWnd *) pFrameToActivate)->MDIActivate();
					// then move previously active window under pPlaceWnd
					pActive->SetWindowPos(pPlaceWnd, 0, 0, 0, 0,
										SWP_NOACTIVATE
										| SWP_NOMOVE
										| SWP_NOOWNERZORDER
										| SWP_NOSIZE);
				}
				return TRUE;  // message eaten
			}
		}
	}
	else if (WM_KEYUP == pMsg->message
			&& VK_CONTROL == pMsg->wParam)
	{
		m_nRotateChildIndex = 0;
	}

	return CMDIFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OnDestroy()
{
	GetApp()->m_bOpenMaximized = (0 != (GetStyle() & WS_MAXIMIZE));
	CMDIFrameWnd::OnDestroy();
}
