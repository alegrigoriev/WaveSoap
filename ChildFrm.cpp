// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "WaveSoapFrontView.h"

#include "ChildFrm.h"
#include "TimeRulerView.h"
#include "AmplitudeRuler.h"
#include "FftRulerView.h"
#include "WaveFftView.h"
#include "WaveOutlineView.h"
#include <afxpriv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CChildFrame)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_DUMMY,
	ID_INDICATOR_DUMMY,
};

CChildFrame::CChildFrame()
{
	// TODO: add member initialization code here

}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	TRACE("CChildFrame::PreCreateWindow()\n");

	if( !CMDIChildWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.style = WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU
				| FWS_ADDTOTITLE | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

	return TRUE;
}

void CChildFrame::ActivateFrame(int nCmdShow)
{
	// TODO: Modify this function to change how the frame is activated.

	TRACE("CChildFrame::ActivateFrame(%d)\n", nCmdShow);
	nCmdShow = SW_SHOWMAXIMIZED;
	CMDIChildWnd::ActivateFrame(nCmdShow);
}


/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	// TODO: Add your specialized code here and/or call the base class
	CRect r;
	GetClientRect( & r);

	//r1.top = dr.bottom;
	BOOL ret = m_wClient.Create(NULL, _T(""),
								WS_CHILD | WS_VISIBLE,
								r, this, AFX_IDW_PANE_FIRST, pContext);

	return ret;
}
/////////////////////////////////////////////////////////////////////////////
// CWaveMDIChildClient

CWaveMDIChildClient::CWaveMDIChildClient()
	: m_bShowWaveform(TRUE),
	m_bShowOutline(TRUE),
	m_bShowTimeRuler(TRUE),
	m_bShowVerticalRuler(TRUE),
	m_bShowFft(FALSE)
{
}

CWaveMDIChildClient::~CWaveMDIChildClient()
{
}


BEGIN_MESSAGE_MAP(CWaveMDIChildClient, CWnd)
	//{{AFX_MSG_MAP(CWaveMDIChildClient)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_COMMAND(IDC_VIEW_SHOW_FFT, OnViewShowFft)
	ON_UPDATE_COMMAND_UI(IDC_VIEW_SHOW_FFT, OnUpdateViewShowFft)
	ON_COMMAND(IDC_VIEW_WAVEFORM, OnViewWaveform)
	ON_UPDATE_COMMAND_UI(IDC_VIEW_WAVEFORM, OnUpdateViewWaveform)
	ON_WM_HSCROLL()
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTLINE, OnUpdateViewOutline)
	ON_COMMAND(ID_VIEW_OUTLINE, OnViewOutline)
	ON_COMMAND(ID_VIEW_TIME_RULER, OnViewTimeRuler)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TIME_RULER, OnUpdateViewTimeRuler)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VERTICAL_RULER, OnUpdateViewVerticalRuler)
	ON_COMMAND(ID_VIEW_VERTICAL_RULER, OnViewVerticalRuler)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CWnd * CWaveMDIChildClient::CreateView(CRuntimeClass* pViewClass,
										CRect rect, int nID, CCreateContext* pContext, BOOL bShow)
{
	ASSERT_VALID(this);
	ASSERT(pViewClass != NULL);
	ASSERT(pViewClass->IsDerivedFrom(RUNTIME_CLASS(CWnd)));
	ASSERT(AfxIsValidAddress(pViewClass, sizeof(CRuntimeClass), FALSE));


	//BOOL bSendInitialUpdate = FALSE;

	if (pContext == NULL)
	{
		return NULL;
	}

	CWnd* pWnd;
	TRY
	{
		pWnd = (CWnd*)pViewClass->CreateObject();
		if (pWnd == NULL)
			AfxThrowMemoryException();
	}
	CATCH_ALL(e)
	{
		TRACE("Out of memory creating a subview\n");
		// Note: DELETE_EXCEPTION(e) not required
		return NULL;
	}
	END_CATCH_ALL

	ASSERT_KINDOF(CWnd, pWnd);
	ASSERT(pWnd->m_hWnd == NULL);       // not yet created

	DWORD dwStyle = AFX_WS_DEFAULT_VIEW;

	dwStyle &= ~WS_BORDER;

	if (! bShow)
	{
		dwStyle &= ~WS_VISIBLE;
	}

	// Create with the right size (wrong position)
	if (!pWnd->Create(NULL, NULL, dwStyle,
					rect, this, nID, pContext))
	{
		TRACE("Warning: couldn't create client view\n");
		// pWnd will be cleaned up by PostNcDestroy
		return NULL;
	}
	// send initial notification message
	//if (bSendInitialUpdate)
	//pWnd->SendMessage(WM_INITIALUPDATE);

	return pWnd;
}


/////////////////////////////////////////////////////////////////////////////
// CWaveMDIChildClient message handlers

void CWaveMDIChildClient::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
// resize child windows
	RecalcLayout();
}

// repositions client area of specified window
// assumes everything has WS_BORDER or is inset like it does
//  (includes scroll bars)
static void DeferClientPos(AFX_SIZEPARENTPARAMS* lpLayout,
							CWnd* pWnd, RECT const& cr, BOOL bScrollBar)
{
	ASSERT(pWnd != NULL);
	ASSERT(pWnd->m_hWnd != NULL);

	CRect rect = cr;
	if (bScrollBar)
	{
		// if there is enough room, draw scroll bar without border
		// if there is not enough room, set the WS_BORDER bit so that
		//   we will at least get a proper border drawn
		BOOL bNeedBorder = (rect.Width() <= 1 || rect.Height() <= 1);
		pWnd->ModifyStyle(bNeedBorder ? 0 : WS_BORDER,
						bNeedBorder ? WS_BORDER : 0);
	}

	// adjust for 3d border (splitter windows have implied border)
	if ((pWnd->GetExStyle() & WS_EX_CLIENTEDGE) ||
		pWnd->IsKindOf(RUNTIME_CLASS(CSplitterWnd)))
		rect.InflateRect(2, 2);

	// first check if the new rectangle is the same as the current
	CRect rectOld;
	pWnd->GetWindowRect(rectOld);
	pWnd->GetParent()->ScreenToClient(&rectOld);
	if (rect != rectOld)
		AfxRepositionWindow(lpLayout, pWnd->m_hWnd, rect);
}

void CWaveMDIChildClient::RecalcLayout()
{
// resize child windows
	CRect cr;
	GetClientRect( & cr);

	CRect r;
	AFX_SIZEPARENTPARAMS layout;
	layout.hDWP = ::BeginDeferWindowPos(10);

	int RulerHeight = CTimeRulerView::CalculateHeight();
	// calculate horizontal ruler width
	int RulerWidth = CAmplitudeRuler::CalculateWidth();
	int cyhscroll = GetSystemMetrics(SM_CYHSCROLL);
	int OutlineHeight = 2 * cyhscroll;
	CWnd * pOutline = GetDlgItem(OutlineViewID);
	if ( ! m_bShowVerticalRuler)
	{
		RulerWidth = 0;
	}

	if (pOutline && m_bShowOutline)
	{
		r.left = RulerWidth;
		r.top = 0;
		r.bottom = OutlineHeight;
		r.right = cr.right;
		pOutline->ShowWindow(SW_SHOWNOACTIVATE);
		DeferClientPos(&layout, pOutline, r, FALSE);
	}
	else
	{
		OutlineHeight = 0;
		if (pOutline)
		{
			pOutline->ShowWindow(SW_HIDE);
		}
	}

	CWnd * pHorRuler = GetDlgItem(HorizontalRulerID);

	if (pHorRuler && m_bShowTimeRuler)
	{
		r.left = RulerWidth;
		r.top = OutlineHeight;
		r.bottom = OutlineHeight + RulerHeight;
		r.right = cr.right;
		pHorRuler->ShowWindow(SW_SHOWNOACTIVATE);
		DeferClientPos(&layout, pHorRuler, r, FALSE);
	}
	else
	{
		RulerHeight = 0;
		if (pHorRuler)
		{
			pHorRuler->ShowWindow(SW_HIDE);
		}
	}

	CWnd * pVertRuler = GetDlgItem(VerticalWaveRulerID);
	CWnd * pVertFftRuler = GetDlgItem(VerticalFftRulerID);
	if (m_bShowVerticalRuler)
	{
		if (pVertRuler)
		{
			r.left = 0;
			r.right = RulerWidth;
			r.top = OutlineHeight + RulerHeight;
			r.bottom = cr.bottom - cyhscroll;
			if (m_bShowWaveform)
			{
				pVertRuler->ShowWindow(SW_SHOWNOACTIVATE);
			}
			else
			{
				pVertRuler->ShowWindow(SW_HIDE);
			}
			DeferClientPos(&layout, pVertRuler, r, FALSE);
		}

		if (pVertFftRuler)
		{
			r.left = 0;
			r.right = RulerWidth;
			r.top = OutlineHeight + RulerHeight;
			r.bottom = cr.bottom - cyhscroll;
			if (m_bShowFft)
			{
				pVertFftRuler->ShowWindow(SW_SHOWNOACTIVATE);
			}
			else
			{
				pVertFftRuler->ShowWindow(SW_HIDE);
			}
			DeferClientPos(&layout, pVertFftRuler, r, FALSE);
		}
	}
	else
	{
		if (pVertRuler)
		{
			pVertRuler->ShowWindow(SW_HIDE);
		}
		if (pVertFftRuler)
		{
			pVertFftRuler->ShowWindow(SW_HIDE);
		}
		RulerWidth = 0;
	}

	CWnd * pScroll = GetDlgItem(AFX_IDW_HSCROLL_FIRST);
	if (pScroll)
	{
		r.left = RulerWidth;
		r.right = cr.right;
		r.top = cr.bottom - cyhscroll;
		r.bottom = cr.bottom;
		DeferClientPos(&layout, pScroll, r, TRUE);
	}

	r.left = 0;
	r.right = RulerWidth;
	r.top = cr.bottom - cyhscroll;
	r.bottom = cr.bottom;
	if (r.right != 0)
	{
		wStatic1.ShowWindow(SW_SHOWNOACTIVATE);
		DeferClientPos(&layout, & wStatic, r, FALSE);
	}
	else
	{
		wStatic1.ShowWindow(SW_HIDE);
	}

	r.top = 0;
	r.bottom = OutlineHeight + RulerHeight;
	if (r.right != 0 && r.bottom != 0)
	{
		wStatic.ShowWindow(SW_SHOWNOACTIVATE);
		DeferClientPos(&layout, & wStatic1, r, FALSE);
	}
	else
	{
		wStatic.ShowWindow(SW_HIDE);
	}

	r.left = RulerWidth;
	r.top = OutlineHeight + RulerHeight;
	r.bottom = cr.bottom - cyhscroll;
	r.right = cr.right;
	CWnd * pWaveView = GetDlgItem(WaveViewID);
	if (pWaveView)
	{
		DeferClientPos(&layout, pWaveView, r, FALSE);
	}

	CWnd * pFftView = GetDlgItem(FftViewID);
	if (pFftView)
	{
		DeferClientPos(&layout, pFftView, r, FALSE);
	}
	// move and resize all the windows at once!
	if (layout.hDWP == NULL || !::EndDeferWindowPos(layout.hDWP))
		TRACE0("Warning: DeferWindowPos failed - low system resources.\n");
}

void CChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	// update our parent window first
	GetMDIFrame()->OnUpdateFrameTitle(bAddToTitle);

	if ((GetStyle() & FWS_ADDTOTITLE) == 0)
		return;     // leave child window alone!

	CWaveSoapFrontDoc * pDocument = (CWaveSoapFrontDoc *)GetActiveDocument();
	if (bAddToTitle)
	{
		CString szText;
		if (pDocument == NULL)
		{
			szText = m_strTitle;
		}
		else
		{
			szText = pDocument->GetTitle();
		}
		if (m_nWindow > 0)
		{
			TCHAR sznum[20];
			wsprintf(sznum, _T(":%d"), m_nWindow);
			szText += sznum;
		}

		if (pDocument->m_bReadOnly)
		{
			szText += _T(" (Read Only)");
		}
		else if (pDocument->m_bDirectMode)
		{
			szText += _T(" (Direct)");
		}
		if (pDocument->IsModified())
		{
			szText += _T(" *");
		}

		// set title if changed, but don't remove completely
		AfxSetWindowText(m_hWnd, szText);
	}
}

class CPushRoutingFrame
{
protected:
	CFrameWnd* pOldRoutingFrame;
	_AFX_THREAD_STATE* pThreadState;

public:
	CPushRoutingFrame(CFrameWnd* pNewRoutingFrame)
	{
		pThreadState = AfxGetThreadState();
		pOldRoutingFrame = pThreadState->m_pRoutingFrame;
		pThreadState->m_pRoutingFrame = pNewRoutingFrame;
	}
	~CPushRoutingFrame()
	{ pThreadState->m_pRoutingFrame = pOldRoutingFrame; }
};

// the function overloaded to get views that never get focus a chance to process the command
BOOL CChildFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	CPushRoutingFrame push(this);

	// pump through current view FIRST
	CView* pActiveView = GetActiveView();
	if (pActiveView != NULL && pActiveView->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	if (m_wClient.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// then pump through frame
	if (CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// last but not least, pump through app
	CWinApp* pApp = AfxGetApp();
	if (pApp != NULL && pApp->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	return FALSE;
}

BOOL CWaveMDIChildClient::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// then pump its own handlers
	if (CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	CWnd * pRuler;
	pRuler = GetDlgItem(HorizontalRulerID);
	if (pRuler && pRuler->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	{
		return TRUE;
	}

	pRuler = GetDlgItem(VerticalWaveRulerID);
	if (pRuler && pRuler->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	{
		return TRUE;
	}

	pRuler = GetDlgItem(VerticalFftRulerID);
	if (pRuler && pRuler->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	{
		return TRUE;
	}

	pRuler = GetDlgItem(OutlineViewID);
	if (pRuler && pRuler->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	{
		return TRUE;
	}

	CWnd * pActiveView;
	pActiveView = GetParentFrame()->GetActiveView();
	CWnd * pView;
	pView = GetDlgItem(WaveViewID);
	if (pView != NULL && pView != pActiveView
		&& pView->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	{
		return TRUE;
	}
	pView = GetDlgItem(FftViewID);
	if (pView != NULL && pView != pActiveView
		&& pView->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
	{
		return TRUE;
	}

	return FALSE;
}

int CWaveMDIChildClient::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CCreateContext * pContext = (CCreateContext *) lpCreateStruct->lpCreateParams;
	CRect r(0, 0, 1, 1);
	CRect r10(0, 0, 10, 10);


	CWnd * pHorRuler = CreateView(RUNTIME_CLASS(CTimeRulerView),
								r, HorizontalRulerID, pContext);

	CWnd * pVertRuler = CreateView(RUNTIME_CLASS(CAmplitudeRuler),
									r, VerticalWaveRulerID, pContext);

	CWnd * pFftRuler = CreateView(RUNTIME_CLASS(CFftRulerView),
								r, VerticalFftRulerID, pContext, FALSE);    // not visible

	CWnd * pOutlineView = CreateView(RUNTIME_CLASS(CWaveOutlineView),
									r, OutlineViewID, pContext, TRUE);    // visible

	wStatic.Create("STATIC", "", WS_BORDER | WS_VISIBLE | WS_CHILD | SS_CENTER, r, this, ScaleStaticID, NULL);
	wStatic1.Create("STATIC", "", WS_BORDER | WS_VISIBLE | WS_CHILD | SS_CENTER, r, this, Static1ID, NULL);

	// create scrollbar
	m_sb.Create(SBS_HORZ | WS_VISIBLE | WS_CHILD, r, this, AFX_IDW_HSCROLL_FIRST);

	CWnd * pFftView = CreateView(RUNTIME_CLASS(CWaveFftView),
								r10, FftViewID, pContext, FALSE); //do not show

	CWnd * pView = CreateView(RUNTIME_CLASS(CWaveSoapFrontView),
							r10, WaveViewID, pContext);

	if (pView && pFftView)
	{
		(DYNAMIC_DOWNCAST(CScaledScrollView, pFftView))->SyncHorizontal
			(DYNAMIC_DOWNCAST(CScaledScrollView, pView));
	}

	GetParentFrame()->SetActiveView(DYNAMIC_DOWNCAST(CView, pView));

	CScaledScrollView * pRulerCast = DYNAMIC_DOWNCAST(CScaledScrollView, pHorRuler);
	if (pRulerCast) pRulerCast->SyncHorizontal
			(DYNAMIC_DOWNCAST(CScaledScrollView, pView));

	pRulerCast = DYNAMIC_DOWNCAST(CScaledScrollView, pVertRuler);
	if (pRulerCast) pRulerCast->SyncVertical
			(DYNAMIC_DOWNCAST(CScaledScrollView, pView));
	pRulerCast = DYNAMIC_DOWNCAST(CScaledScrollView, pFftRuler);
	if (pRulerCast) pRulerCast->SyncVertical
			(DYNAMIC_DOWNCAST(CScaledScrollView, pFftView));

	RecalcLayout();
	return 0;
}


void CWaveMDIChildClient::OnViewShowFft()
{
	if (m_bShowFft)
	{
		return;
	}

	m_bShowFft = TRUE;
	m_bShowWaveform = FALSE;

	CWnd * pView = GetDlgItem(FftViewID);
	if (pView != NULL)
	{
		pView->ShowWindow(SW_SHOW);
		((CFrameWnd*)GetParent())->SetActiveView((CView *)pView);
		pView = GetDlgItem(VerticalFftRulerID);
		if (NULL != pView)
		{
			pView->ShowWindow(SW_SHOWNOACTIVATE);
		}
		pView = GetDlgItem(WaveViewID);
		if (NULL != pView)
		{
			pView->ShowWindow(SW_HIDE);
		}
		pView = GetDlgItem(VerticalWaveRulerID);
		if (NULL != pView)
		{
			pView->ShowWindow(SW_HIDE);
		}
	}
}

void CWaveMDIChildClient::OnUpdateViewShowFft(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(m_bShowFft);
}

void CWaveMDIChildClient::OnViewWaveform()
{
	if (m_bShowWaveform)
	{
		return;
	}

	m_bShowFft = FALSE;
	m_bShowWaveform = TRUE;

	CWnd * pView = GetDlgItem(WaveViewID);
	if (pView != NULL)
	{
		pView->ShowWindow(SW_SHOW);
		((CFrameWnd*)GetParent())->SetActiveView((CView *)pView);
		pView = GetDlgItem(VerticalWaveRulerID);
		if (NULL != pView)
		{
			pView->ShowWindow(SW_SHOWNOACTIVATE);
		}
		pView = GetDlgItem(FftViewID);
		if (NULL != pView)
		{
			pView->ShowWindow(SW_HIDE);
		}
		pView = GetDlgItem(VerticalFftRulerID);
		if (NULL != pView)
		{
			pView->ShowWindow(SW_HIDE);
		}
	}
}

void CWaveMDIChildClient::OnUpdateViewWaveform(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(m_bShowWaveform);
}

void CWaveMDIChildClient::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	//	forward the message to all views
	CWnd * pView;
	pView = GetDlgItem(WaveViewID);
	if (pView != NULL)
	{
		pView->SendMessage(WM_HSCROLL,
							MAKELONG(nSBCode, nPos), (LPARAM)pScrollBar->m_hWnd);
	}
	pView = GetDlgItem(FftViewID);
	if (pView != NULL)
	{
		pView->SendMessage(WM_HSCROLL,
							MAKELONG(nSBCode, nPos), (LPARAM)pScrollBar->m_hWnd);
	}

}

int CChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
									sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	int width;
	unsigned id, style;
	// to avoid setting width to the string length,
	// the status bar is created with dummy IDs
	m_wndStatusBar.GetPaneInfo(1, id, style, width);
	m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_CURRENT_POS, style, width);
	m_wndStatusBar.SetPaneInfo(2, ID_INDICATOR_SELECTION_LENGTH, style, width);
	m_wndStatusBar.EnableToolTips();

	return 0;
}

void CWaveMDIChildClient::OnUpdateViewOutline(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bShowOutline);
}

void CWaveMDIChildClient::OnViewOutline()
{
	m_bShowOutline = ! m_bShowOutline;
	RecalcLayout();
}

void CWaveMDIChildClient::OnUpdateViewTimeRuler(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bShowTimeRuler);
}

void CWaveMDIChildClient::OnViewTimeRuler()
{
	m_bShowTimeRuler = ! m_bShowTimeRuler;
	RecalcLayout();
}

void CWaveMDIChildClient::OnUpdateViewVerticalRuler(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bShowVerticalRuler);
}

void CWaveMDIChildClient::OnViewVerticalRuler()
{
	m_bShowVerticalRuler = ! m_bShowVerticalRuler;
	RecalcLayout();
}
