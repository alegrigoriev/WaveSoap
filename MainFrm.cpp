// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "WaveSoapFront.h"

#include "MainFrm.h"
#include "GdiObjectSave.h"
#include "WaveSoapFrontDoc.h"
#include "resource.h"       // main symbols

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame
IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, BaseClass)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND_EX(ID_VIEW_STATUS_BAR, OnBarCheckStatusBar)
	ON_COMMAND_EX(ID_VIEW_TOOLBAR, OnBarCheckToolbar)
	ON_COMMAND_EX(ID_VIEW_REBAR, OnBarCheckRebar)
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, BaseClass::OnHelpFinder)
	ON_COMMAND(ID_HELP, BaseClass::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, BaseClass::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, BaseClass::OnHelpFinder)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_SIZE, OnUpdateIndicatorFileSize)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SAMPLE_RATE, OnUpdateIndicatorSampleRate)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SAMPLE_SIZE, OnUpdateIndicatorSampleSize)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CHANNELS, OnUpdateIndicatorChannels)

END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_SAMPLE_SIZE,
	ID_INDICATOR_SAMPLE_RATE,
	ID_INDICATOR_CHANNELS,
	ID_INDICATOR_FILE_SIZE,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (BaseClass::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	if (!m_wndToolBar2.CreateEx(this) ||
		!m_wndToolBar2.LoadToolBar(IDR_CHILDFRAME))
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
		|| !m_wndReBar.AddBar(&m_wndToolBar2)
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
									countof(indicators)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.EnableToolTips();

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
							CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar2.SetBarStyle(m_wndToolBar2.GetBarStyle() |
							CBRS_TOOLTIPS | CBRS_FLYBY);

	CThisApp * pApp = GetApp();
	if ( ! pApp->m_bShowStatusBar)
	{
		ShowControlBar( & m_wndStatusBar, FALSE, FALSE);
	}
	if ( ! pApp->m_bShowToolbar)
	{
		ShowControlBar( & m_wndToolBar, FALSE, FALSE);
		ShowControlBar( & m_wndToolBar2, FALSE, FALSE);
	}
	EnableToolTips();
	return 0;
}

BOOL CMainFrame::OnBarCheckStatusBar(UINT nID)
{
	if (BaseClass::OnBarCheck(nID))
	{
		GetApp()->m_bShowStatusBar = (0 != (GetControlBar(nID)->GetStyle() & WS_VISIBLE));
		return TRUE;
	}
	return FALSE;
}

BOOL CMainFrame::OnBarCheckToolbar(UINT nID)
{
	if (BaseClass::OnBarCheck(nID))
	{
		GetApp()->m_bShowToolbar = (0 != (GetControlBar(nID)->GetStyle() & WS_VISIBLE));
		return TRUE;
	}
	return FALSE;
}

BOOL CMainFrame::OnBarCheckRebar(UINT nID)
{
	if (BaseClass::OnBarCheck(nID))
	{
		GetApp()->m_bShowToolbar = (0 != (GetControlBar(nID)->GetStyle() & WS_VISIBLE));
		return TRUE;
	}
	return FALSE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !BaseClass::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}

void CMainFrame::GetMessageString(UINT nID, CString& rMessage) const
{
	// use either a status string from the topmost child,
	// or status string from application thread, or status string from one of other documents
	// find, starting with the topmost frame,
	// a document which performs an operation
	if (nID != AFX_IDS_IDLEMESSAGE)
	{
		if (! GetApp()->GetMessageString(nID, rMessage))
		{
			BaseClass::GetMessageString(nID, rMessage);
		}
		return;
	}

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
		BaseClass::GetMessageString(nID, rMessage);
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
	// size the pane to the max expected size and set empty string
	SetStatusString(pCmdUI, CString(), _T("00:00:00.000"), TRUE);
}

void CMainFrame::OnUpdateIndicatorSampleRate(CCmdUI* pCmdUI)
{
	// size the pane to the max expected size and set empty string
	SetStatusString(pCmdUI, CString(), _T("44,100"), TRUE);
}

void CMainFrame::OnUpdateIndicatorSampleSize(CCmdUI* pCmdUI)
{
	// size the pane to the max expected size and set empty string
	SetStatusString(pCmdUI, CString(), LoadCString(IDS_STATUS_STRING16BIT), TRUE);
}

void CMainFrame::OnUpdateIndicatorChannels(CCmdUI* pCmdUI)
{
	// size the pane to the max expected size and set empty string
	SetStatusString(pCmdUI, CString(), LoadCString(IDS_STATUS_STRING_STEREO), TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	BaseClass::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	BaseClass::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


