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
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CMDIFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CMDIFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CMDIFrameWnd::OnHelpFinder)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILE_SIZE, OnUpdateIndicatorFileSize)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SAMPLE_RATE, OnUpdateIndicatorSampleRate)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SAMPLE_SIZE, OnUpdateIndicatorSampleSize)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CHANNELS, OnUpdateIndicatorChannels)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_SAMPLE_RATE,
	ID_INDICATOR_SAMPLE_SIZE,
	ID_INDICATOR_CHANNELS,
	ID_INDICATOR_FILE_SIZE,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here

}

CMainFrame::~CMainFrame()
{
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
	if (!m_wndDlgBar.Create(this, IDR_MAINFRAME,
							CBRS_ALIGN_TOP, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}

	if (!m_wndReBar.Create(this) ||
		!m_wndReBar.AddBar(&m_wndToolBar) ||
		!m_wndReBar.AddBar(&m_wndDlgBar))
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

	return 0;
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
	// find, starting with the topmost frame,
	// a document which performs an operation
	CFrameWnd * pWnd = const_cast<CMainFrame *>(this)->GetActiveFrame();
	CWaveSoapFrontDoc * pDoc = NULL;
	BOOL Topmost = TRUE;
	while (NULL != pWnd)
	{
		CView * pView = pWnd->GetActiveView();
		if (NULL != pView)
		{
			pDoc = DYNAMIC_DOWNCAST(CWaveSoapFrontDoc,
									pView->GetDocument());
			if ( ! pDoc->m_CurrentStatusString.IsEmpty())
			{
				if (Topmost)
				{
					rMessage = pDoc->m_CurrentStatusString;
				}
				else
				{
					rMessage.Format(_T("%s: %s"), LPCTSTR(pDoc->GetTitle()),
									LPCTSTR(pDoc->m_CurrentStatusString));
				}
				if ( ! pDoc->m_OperationInProgress)
				{
					pDoc->m_CurrentStatusString.Empty();
				}
				return;
			}
		}
		Topmost = false;
		pWnd = DYNAMIC_DOWNCAST(CFrameWnd, pWnd->GetWindow(GW_HWNDNEXT));
	}
	CMDIFrameWnd::GetMessageString(nID, rMessage);
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

