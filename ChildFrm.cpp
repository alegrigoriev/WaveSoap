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
#include "SpectrumSectionView.h"
#include <afxpriv.h>
#include "MainFrm.h"

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
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_DUMMY,
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
	if( !CMDIChildWnd::PreCreateWindow(cs) )
		return FALSE;
	cs.style = WS_CHILD | WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU
				| FWS_ADDTOTITLE | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

	CMDIChildWnd * pActive = ((CMDIFrameWnd *)AfxGetMainWnd())->MDIGetActive();

	if ((pActive == NULL && GetApp()->m_bOpenChildMaximized)
		|| (pActive != NULL && (WS_MAXIMIZE & pActive->GetStyle())))
	{
		cs.style |= WS_MAXIMIZE;
	}

	return TRUE;
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
	m_bShowSpectrumSection(FALSE),
	m_bShowFft(FALSE)
{
	m_SpectrumSectionWidth = GetApp()->m_SpectrumSectionWidth;
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
	ON_UPDATE_COMMAND_UI(ID_VIEW_SPECTRUMSECTION, OnUpdateViewSpectrumsection)
	ON_COMMAND(ID_VIEW_SPECTRUMSECTION, OnViewSpectrumsection)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HIDE_SPECTRUMSECTION, OnUpdateViewHideSpectrumsection)
	ON_COMMAND(ID_VIEW_HIDE_SPECTRUMSECTION, OnViewHideSpectrumsection)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
	ON_MESSAGE(WM_SETTINGCHANGE, OnSettingChange)
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
	int FftRulerWidth = CFftRulerView::CalculateWidth();

	int cyhscroll = GetSystemMetrics(SM_CYHSCROLL);
	int OutlineHeight = 2 * cyhscroll;
	int SpectrumSectionWidth = m_SpectrumSectionWidth;
	int VerticalTrackerWidth = GetSystemMetrics(SM_CXSIZEFRAME);
	if ( ! m_bShowSpectrumSection)
	{
		SpectrumSectionWidth = 0;
		VerticalTrackerWidth = 0;
	}
	else if (SpectrumSectionWidth + VerticalTrackerWidth > cr.right - 2)
	{
		SpectrumSectionWidth = cr.right - 2 - VerticalTrackerWidth;
		if (SpectrumSectionWidth <= 0)
		{
			SpectrumSectionWidth = 0;
			VerticalTrackerWidth = 0;
		}
	}
	if ( ! m_bShowVerticalRuler)
	{
		RulerWidth = 0;
		FftRulerWidth = 0;
	}
	if ( ! m_bShowFft && ! m_bShowSpectrumSection)
	{
		FftRulerWidth = 0;
	}
	if (m_bShowFft)
	{
		RulerWidth = 0;
	}
	CWnd * pOutline = GetDlgItem(OutlineViewID);
	if (NULL != pOutline && m_bShowOutline)
	{
		r.left = 0;
		r.top = 0;
		r.bottom = OutlineHeight;
		r.right = cr.right;
		pOutline->ShowWindow(SW_SHOWNOACTIVATE);
		DeferClientPos(&layout, pOutline, r, FALSE);
	}
	else
	{
		OutlineHeight = 0;
		if (NULL != pOutline)
		{
			pOutline->ShowWindow(SW_HIDE);
		}
	}

	CWnd * pHorRuler = GetDlgItem(HorizontalRulerID);
	CWnd * pSpectrumSectionRuler = GetDlgItem(SpectrumSectionRulerID);

	if (m_bShowTimeRuler)
	{
		r.left = SpectrumSectionWidth + VerticalTrackerWidth + RulerWidth + FftRulerWidth;
		r.top = OutlineHeight;
		r.bottom = OutlineHeight + RulerHeight;
		r.right = cr.right;
		if (NULL != pHorRuler)
		{
			pHorRuler->ShowWindow(SW_SHOWNOACTIVATE);
			DeferClientPos(&layout, pHorRuler, r, FALSE);
		}
		r.left = FftRulerWidth;
		r.right = SpectrumSectionWidth + FftRulerWidth;
		if (NULL != pSpectrumSectionRuler)
		{
			pSpectrumSectionRuler->ShowWindow(SW_SHOWNOACTIVATE);
			DeferClientPos(&layout, pSpectrumSectionRuler, r, FALSE);
		}
	}
	else
	{
		RulerHeight = 0;
		if (NULL != pHorRuler)
		{
			pHorRuler->ShowWindow(SW_HIDE);
		}
		if (NULL != pSpectrumSectionRuler)
		{
			pSpectrumSectionRuler->ShowWindow(SW_HIDE);
		}
	}

	CWnd * pVertRuler = GetDlgItem(VerticalWaveRulerID);
	CWnd * pVertFftRuler = GetDlgItem(VerticalFftRulerID);
	if (m_bShowVerticalRuler)
	{
		if (NULL != pVertRuler)
		{
			r.left = SpectrumSectionWidth + FftRulerWidth + VerticalTrackerWidth;
			r.right = r.left + RulerWidth;
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
			r.right = FftRulerWidth;
			r.top = OutlineHeight + RulerHeight;
			r.bottom = cr.bottom - cyhscroll;
			if (m_bShowFft || m_bShowSpectrumSection)
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
		r.left = SpectrumSectionWidth + VerticalTrackerWidth + RulerWidth + FftRulerWidth;
		r.right = cr.right;
		r.top = cr.bottom - cyhscroll;
		r.bottom = cr.bottom;
		DeferClientPos(&layout, pScroll, r, TRUE);
	}

	CWnd * pSpectrumSection = GetDlgItem(SpectrumSectionViewID);
	if (pSpectrumSection)
	{
		r.left = FftRulerWidth;
		r.right = FftRulerWidth + SpectrumSectionWidth;
		r.top = OutlineHeight + RulerHeight;
		r.bottom = cr.bottom;
		if (0 != SpectrumSectionWidth)
		{
			pSpectrumSection->ShowWindow(SW_SHOWNOACTIVATE);
		}
		else
		{
			pSpectrumSection->ShowWindow(SW_HIDE);
		}
		DeferClientPos(&layout, pSpectrumSection, r, FALSE);
	}

	CWnd * pVerticalTracker = GetDlgItem(VerticalTrackerID);
	if (pVerticalTracker)
	{
		r.left = FftRulerWidth + SpectrumSectionWidth;
		r.right = r.left + VerticalTrackerWidth;
		r.top = OutlineHeight;
		r.bottom = cr.bottom;
		if (0 != VerticalTrackerWidth)
		{
			pVerticalTracker->ShowWindow(SW_SHOWNOACTIVATE);
		}
		else
		{
			pVerticalTracker->ShowWindow(SW_HIDE);
		}
		DeferClientPos(&layout, pVerticalTracker, r, FALSE);
	}

	r.left = FftRulerWidth + SpectrumSectionWidth + VerticalTrackerWidth;
	r.right = RulerWidth + r.left;
	r.top = cr.bottom - cyhscroll;
	r.bottom = cr.bottom;
	if (RulerWidth != 0)
	{
		m_AmplZoomBar.ShowWindow(SW_SHOWNOACTIVATE);
		DeferClientPos(&layout, & m_AmplZoomBar, r, FALSE);
	}
	else
	{
		m_AmplZoomBar.ShowWindow(SW_HIDE);
	}

	r.top = OutlineHeight;
	r.bottom = OutlineHeight + RulerHeight;
	if (RulerWidth != 0 && RulerHeight != 0)
	{
		m_TimeZoomBar.ShowWindow(SW_SHOWNOACTIVATE);
		DeferClientPos(&layout, & m_TimeZoomBar, r, FALSE);
	}
	else
	{
		m_TimeZoomBar.ShowWindow(SW_HIDE);
	}

	r.left = 0;
	r.right = FftRulerWidth;
	r.top = cr.bottom - cyhscroll;
	r.bottom = cr.bottom;
	if (r.right != r.left)
	{
		CRect r1(0, r.Width() / 2, 0, r.Height());
		CRect r2(0, r.Width() - r1.right, 0, r.Height());
		m_FftZoomBar.ShowWindow(SW_SHOWNOACTIVATE);
		DeferClientPos(&layout, & m_FftZoomBar, r, FALSE);
	}
	else
	{
		m_FftZoomBar.ShowWindow(SW_HIDE);
	}

	r.top = OutlineHeight;
	r.bottom = OutlineHeight + RulerHeight;
	if (r.right != 0 && r.bottom != 0)
	{
		if (0 != SpectrumSectionWidth)
		{
			m_SsZoomBar.ShowWindow(SW_SHOWNOACTIVATE);
			DeferClientPos(&layout, & m_SsZoomBar, r, FALSE);
		}
		else
		{
			m_SsZoomBar.ShowWindow(SW_HIDE);
			m_TimeZoomBar.ShowWindow(SW_SHOWNOACTIVATE);
			DeferClientPos(&layout, & m_SsZoomBar, r, FALSE);
		}
	}
	else
	{
		if (0 != SpectrumSectionWidth)
		{
			m_TimeZoomBar.ShowWindow(SW_HIDE);
		}
		m_SsZoomBar.ShowWindow(SW_HIDE);
	}

	r.left = SpectrumSectionWidth + VerticalTrackerWidth + RulerWidth + FftRulerWidth;
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

	if (NULL != pFftView
		&& NULL != pWaveView)
	{
		if (m_bShowWaveform)
		{
			pFftView->ShowWindow(SW_HIDE);
		}
		else
		{
			pWaveView->ShowWindow(SW_HIDE);
		}

		if (m_bShowFft)
		{
			pFftView->ShowWindow(SW_SHOW);
			GetParentFrame()->SetActiveView(DYNAMIC_DOWNCAST(CView, pFftView));
		}
		else
		{
			pWaveView->ShowWindow(SW_SHOW);
			GetParentFrame()->SetActiveView(DYNAMIC_DOWNCAST(CView, pWaveView));
		}
	}
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

		if (NULL != pDocument)
		{
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
		}

		// set title if changed, but don't remove completely
		AfxSetWindowText(m_hWnd, szText);
	}
}

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
	if (NULL == pExtra && NULL == pHandlerInfo)
		TRACE("CWaveMDIChildClient::OnCmdMsg nID=%X, nCode=%X\n", nID, nCode);
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

	pView = GetDlgItem(SpectrumSectionViewID);
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

	CWnd * pSpectrumSectionRuler = CreateView(RUNTIME_CLASS(CSpectrumSectionRuler),
											r, SpectrumSectionRulerID, pContext, FALSE);    // not visible

	CWnd * pOutlineView = CreateView(RUNTIME_CLASS(CWaveOutlineView),
									r, OutlineViewID, pContext, TRUE);    // visible

	m_FftZoomBar.Create(this, FftZoomBarID, r);
	m_FftZoomBar.AddButton(ID_BITMAP_ZOOMINVERT, ID_VIEW_SS_ZOOMINVERT);
	m_FftZoomBar.AddButton(ID_BITMAP_ZOOMOUTVERT, ID_VIEW_SS_ZOOMOUTVERT);

	m_SsZoomBar.Create(this, SsZoomBarID, r);
	m_SsZoomBar.AddButton(ID_BITMAP_ZOOMINHOR, ID_VIEW_SS_ZOOMINHOR2);
	m_SsZoomBar.AddButton(ID_BITMAP_ZOOMOUTHOR, ID_VIEW_SS_ZOOMOUTHOR2);

	m_AmplZoomBar.Create(this, AmplZoomBarID, r);
	m_AmplZoomBar.AddButton(ID_BITMAP_ZOOMINVERT, ID_VIEW_ZOOMINVERT);
	m_AmplZoomBar.AddButton(ID_BITMAP_ZOOMOUTVERT, ID_VIEW_ZOOMOUTVERT);

	m_TimeZoomBar.Create(this, TimeZoomBarID, r);
	m_TimeZoomBar.AddButton(ID_BITMAP_ZOOMINHOR, ID_VIEW_ZOOMINHOR2);
	m_TimeZoomBar.AddButton(ID_BITMAP_ZOOMOUTHOR, ID_VIEW_ZOOMOUTHOR2);

	// create scrollbar
	m_sb.Create(SBS_HORZ | WS_VISIBLE | WS_CHILD, r, this, AFX_IDW_HSCROLL_FIRST);
	wTracker.Create(AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW,
										AfxGetApp()->LoadStandardCursor(IDC_SIZEWE),
										HBRUSH(COLOR_ACTIVEBORDER + 1)), _T(""),
					WS_CHILD, r, this, VerticalTrackerID);

	CWnd * pFftView = CreateView(RUNTIME_CLASS(CWaveFftView),
								r10, FftViewID, pContext, FALSE); //do not show

	CWnd * pView = CreateView(RUNTIME_CLASS(CWaveSoapFrontView),
							r10, WaveViewID, pContext);

	CWnd * pTrackView = CreateView(RUNTIME_CLASS(CSpectrumSectionView),
									r10, SpectrumSectionViewID, pContext, FALSE);

	if (pView && pFftView)
	{
		(DYNAMIC_DOWNCAST(CScaledScrollView, pFftView))->SyncHorizontal
			(DYNAMIC_DOWNCAST(CScaledScrollView, pView));
	}

	if (pTrackView && pFftView)
	{
		(DYNAMIC_DOWNCAST(CScaledScrollView, pTrackView))->SyncVertical
			(DYNAMIC_DOWNCAST(CScaledScrollView, pFftView));
	}

	if (pTrackView && pSpectrumSectionRuler)
	{
		(DYNAMIC_DOWNCAST(CScaledScrollView, pSpectrumSectionRuler))->SyncHorizontal
			(DYNAMIC_DOWNCAST(CScaledScrollView, pTrackView));
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

	RecalcLayout();
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

	RecalcLayout();
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

#if 0
	if ( ! m_wndToolBar.CreateEx(this, WS_CHILD | WS_VISIBLE | CBRS_SIZE_FIXED | CBRS_TOP )
		|| ! m_wndToolBar.LoadToolBar(IDR_CHILDFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	// to avoid setting width to the string length,
	// the status bar is created with dummy IDs
	if (!m_wndReBar.Create(this, RBS_BANDBORDERS, WS_CHILD | WS_VISIBLE
							| WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM)
		|| !m_wndReBar.AddBar(&m_wndToolBar)
		)
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}
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
#endif

	if (!m_wndStatusBar.Create(this)
//        || !m_wndReBar.AddBar(&m_wndStatusBar)
		|| !m_wndStatusBar.SetIndicators(indicators,
										sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	int width;
	unsigned id, style;
	m_wndStatusBar.GetPaneInfo(1, id, style, width);
	m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_SCALE, style, width);
	m_wndStatusBar.SetPaneInfo(2, ID_INDICATOR_CURRENT_POS, style, width);
	m_wndStatusBar.SetPaneInfo(3, ID_INDICATOR_SELECTION_LENGTH, style, width);
	m_wndStatusBar.EnableToolTips();
	//EnableToolTips();
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

// CVerticalTrackerBar

CVerticalTrackerBar::CVerticalTrackerBar()
{
	m_bTracking = FALSE;
}

CVerticalTrackerBar::~CVerticalTrackerBar()
{
}


BEGIN_MESSAGE_MAP(CVerticalTrackerBar, CWnd)
	//{{AFX_MSG_MAP(CVerticalTrackerBar)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CAPTURECHANGED()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVerticalTrackerBar diagnostics

#ifdef _DEBUG
void CVerticalTrackerBar::AssertValid() const
{
	CWnd::AssertValid();
}

void CVerticalTrackerBar::Dump(CDumpContext& dc) const
{
	CWnd::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVerticalTrackerBar message handlers

void CWaveMDIChildClient::OnUpdateViewSpectrumsection(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bShowSpectrumSection);
}

void CWaveMDIChildClient::OnViewSpectrumsection()
{
	m_bShowSpectrumSection = ! m_bShowSpectrumSection;
	RecalcLayout();
}

/////////////////////////////////////////////////////////////////////////////
// CVerticalTrackerBar drawing

void CVerticalTrackerBar::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CRect rect;
	GetClientRect( &rect);
	dc.Draw3dRect(rect, GetSysColor(COLOR_3DHIGHLIGHT), GetSysColor(COLOR_3DSHADOW));
}

void CVerticalTrackerBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_bTracking = TRUE;
	SetCapture();
	m_ClickPointX = point.x;
	CWnd::OnLButtonDown(nFlags, point);
}

void CVerticalTrackerBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bTracking = FALSE;
	ReleaseCapture();
	CWnd::OnLButtonUp(nFlags, point);
}

void CVerticalTrackerBar::OnMouseMove(UINT nFlags, CPoint point)
{
	if(m_bTracking)
	{
		CWaveMDIChildClient * pParent = dynamic_cast<CWaveMDIChildClient *>(GetParent());
		if (NULL != pParent)
		{
			point.x -= m_ClickPointX;
			ClientToScreen( & point);
			CSpectrumSectionView * pSpectrumSection =
				dynamic_cast<CSpectrumSectionView *>
				(pParent->GetDlgItem(CWaveMDIChildClient::SpectrumSectionViewID));

			if (NULL != pSpectrumSection)
			{
				pSpectrumSection->ScreenToClient( & point);
				pParent->m_SpectrumSectionWidth = point.x;
				GetApp()->m_SpectrumSectionWidth = point.x;
				pParent->RecalcLayout();
			}
		}
	}
	CWnd::OnMouseMove(nFlags, point);
}

void CVerticalTrackerBar::OnCaptureChanged(CWnd *pWnd)
{
	if (pWnd != this)
	{
		m_bTracking = FALSE;
	}
	CWnd::OnCaptureChanged(pWnd);
}


void CWaveMDIChildClient::OnUpdateViewHideSpectrumsection(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bShowSpectrumSection);
}

void CWaveMDIChildClient::OnViewHideSpectrumsection()
{
	m_bShowSpectrumSection = FALSE;
	RecalcLayout();
}

LRESULT CWaveMDIChildClient::OnSettingChange(WPARAM uFlags, LPARAM lParam)
{
	RecalcLayout();
	CWnd::OnSettingChange(uFlags, (LPCTSTR) lParam);
	return 0;
}

LRESULT CWaveMDIChildClient::OnDisplayChange(WPARAM wParam, LPARAM lParam)
{
	RecalcLayout();
	return CWnd::OnDisplayChange(wParam, lParam);
}


void CChildFrame::OnDestroy()
{
	CThisApp * pApp = GetApp();
	pApp->m_bOpenChildMaximized = (0 != (GetStyle() & WS_MAXIMIZE));
	CMDIChildWnd::OnDestroy();
}

/////////////////////////////////////////////////////////////////////////////
// CMiniToolbar

CMiniToolbar::CMiniToolbar()
{
	m_ButtonClicked = 0;
	m_MouseCaptured = false;
	m_LButtonPressed = false;
	m_ButtonHilit = 0;
}

CMiniToolbar::~CMiniToolbar()
{
	for (unsigned i = 0; i < m_Buttons.size(); i++)
	{
		// delete our own bitmaps
		if (m_Buttons[i].bDeleteBitmap)
		{
			delete m_Buttons[i].pBitmap;
			m_Buttons[i].pBitmap = NULL;
		}
	}
}

IMPLEMENT_DYNAMIC(CMiniToolbar, CWnd)

BEGIN_MESSAGE_MAP(CMiniToolbar, CWnd)
ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
//{{AFX_MSG_MAP(CMiniToolbar)
ON_WM_CAPTURECHANGED()
ON_WM_ERASEBKGND()
ON_WM_LBUTTONDOWN()
ON_WM_LBUTTONUP()
ON_WM_MOUSEACTIVATE()
ON_WM_MOUSEMOVE()
ON_WM_PAINT()
ON_WM_CREATE()
//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMiniToolbar message handlers

void CMiniToolbar::OnCaptureChanged(CWnd *pWnd)
{
	if (pWnd != this)
	{
		m_LButtonPressed = false;
		m_ButtonClicked = 0;
		m_MouseCaptured = 0;
		if (m_ButtonHilit)
		{
			HiliteButton(m_ButtonHilit, false);
		}
	}
	CWnd::OnCaptureChanged(pWnd);
}

BOOL CMiniToolbar::OnEraseBkgnd(CDC* pDC)
{
	CBrush brush(GetSysColor(COLOR_3DFACE));
	CRect cr;
	GetClientRect( & cr);
	pDC->FillRect( & cr, & brush);

	return TRUE;
}

void CMiniToolbar::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_LButtonPressed = true;
	m_ButtonClicked = GetHitCode(point);
	if (0 != m_ButtonClicked)
	{
		HiliteButton(m_ButtonClicked, true);
	}
	CWnd::OnLButtonDown(nFlags, point);
}

void CMiniToolbar::OnLButtonUp(UINT nFlags, CPoint point)
{
	int nID = 0;
	if (m_ButtonClicked)
	{
		nID = GetHitCode(point);
		if (nID != m_ButtonClicked)
		{
			nID = 0;
		}
		HiliteButton(m_ButtonHilit, false);
	}
	if (m_MouseCaptured)
	{
		ReleaseCapture();
	}
	m_LButtonPressed = false;
	m_ButtonClicked = 0;
	CWnd::OnLButtonUp(nFlags, point);
	if (nID != 0)
	{
		GetParentFrame()->PostMessage(WM_COMMAND, nID | (BN_CLICKED << 16), LPARAM(m_hWnd));
	}
}

int CMiniToolbar::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	return MA_NOACTIVATE;
}

void CMiniToolbar::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_LButtonPressed)
	{
		if ( ! m_MouseCaptured)
		{
			SetCapture();
			m_MouseCaptured = true;
		}
		if (GetHitCode(point) != m_ButtonClicked)
		{
			HiliteButton(m_ButtonClicked, false);
		}
		else
		{
			HiliteButton(m_ButtonClicked, true);
		}
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CMiniToolbar::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CRect cr;
	GetClientRect( & cr);
	// draw the bitmaps
	CBrush br(COLORREF(0));
	for (unsigned i = 0; i < m_Buttons.size(); i++)
	{
		BITMAP bmp;
		m_Buttons[i].pBitmap->GetBitmap( & bmp);
		// center the bitmap
		int x = cr.Width() * i / m_Buttons.size()
				+ (cr.Width() / m_Buttons.size() - bmp.bmWidth) / 2;
		int y = (cr.Height() - bmp.bmHeight) / 2;
		int flags = DST_BITMAP | DSS_NORMAL | DSS_MONO;
		if (m_Buttons[i].bEnabled)
		{
			if (m_ButtonHilit == m_Buttons[i].nID)
			{
				x++;
				y++;
			}
		}
		else
		{
			flags = DST_BITMAP | DSS_DISABLED | DSS_MONO;
		}
		dc.DrawState(CPoint(x, y), CSize(bmp.bmWidth, bmp.bmHeight),
					m_Buttons[i].pBitmap, flags, & br);
	}
}

int CMiniToolbar::GetHitCode(POINT point) const
{
	CRect cr;
	GetClientRect( & cr);
	if (point.y < cr.top || point.y >= cr.bottom
		|| point.x < cr.left || point.x >= cr.right
		|| m_Buttons.size() == 0
		|| cr.Width() == 0)
	{
		return 0;
	}
	return m_Buttons[point.x * m_Buttons.size() / cr.Width()].nID;
}

void CMiniToolbar::GetItemRect(UINT nID, RECT & rect) const
{
	CRect cr;
	GetClientRect( & cr);
	for (unsigned i = 0; i < m_Buttons.size(); i++)
	{
		if (m_Buttons[i].nID == nID)
		{
			rect.top = cr.top;
			rect.bottom = cr.bottom;
			rect.left = cr.Width() * i / m_Buttons.size();
			rect.right = cr.Width() * (i + 1) / m_Buttons.size();
			return;
		}
	}
	rect.left = 0;
	rect.right = 0;
	rect.bottom = 0;
	rect.top = 0;
}

void CMiniToolbar::HiliteButton(UINT nID, bool Hilite)
{
	if (m_ButtonHilit != 0 && nID != m_ButtonHilit)
	{
		HiliteButton(m_ButtonHilit, false);
	}
	if (Hilite)
	{
		if (m_ButtonHilit == nID)
		{
			return;
		}
		m_ButtonHilit = nID;
	}
	else
	{
		if (m_ButtonHilit == 0)
		{
			return;
		}
		m_ButtonHilit = 0;
	}
	for (unsigned i = 0; i < m_Buttons.size(); i++)
	{
		if (m_Buttons[i].nID == nID)
		{
			break;
		}
	}
	if (i == m_Buttons.size())
	{
		m_ButtonHilit = 0;
		return;
	}
	RedrawButton(i);
}

void CMiniToolbar::RedrawButton(int Index)
{
	TRACE("CMiniToolbar::RedrawButton %d\n", Index);
	CRect cr;
	GetClientRect( & cr);
	CRect r = cr;
	r.left = cr.Width() * Index / m_Buttons.size();
	r.right = cr.Width() * (Index + 1) / m_Buttons.size();
	InvalidateRect( & r);
}

void CMiniToolbar::AddButton(CBitmap * pBitmap, UINT nID)
{
	// not checking for duplicate ID
	Button btn;
	btn.nID = nID;
	btn.pBitmap = pBitmap;
	btn.bEnabled = true;
	btn.bDeleteBitmap = false;
	m_Buttons.push_back(btn);
}

void CMiniToolbar::AddButton(UINT nBitmapID, UINT nID)
{
	// not checking for duplicate ID
	Button btn;
	btn.nID = nID;
	btn.pBitmap = new CBitmap;
	if ( ! btn.pBitmap->LoadBitmap(nBitmapID))
	{
		delete btn.pBitmap;
		return;
	}
	btn.bEnabled = true;
	btn.bDeleteBitmap = true;
	m_Buttons.push_back(btn);
}

void CMiniToolbar::EnableButton(unsigned Index, BOOL bEnable)
{
	if (Index >= m_Buttons.size())
	{
		return;
	}
	if (bEnable)
	{
		if (m_Buttons[Index].bEnabled)
		{
			return;
		}
		m_Buttons[Index].bEnabled = true;
	}
	else
	{
		if ( ! m_Buttons[Index].bEnabled)
		{
			return;
		}
		m_Buttons[Index].bEnabled = false;
	}
	RedrawButton(Index);
}

class CMiniToolbarCmdUI : public CCmdUI        // class private to this file !
{
public: // re-implementations only
	virtual void Enable(BOOL bOn);
	virtual void SetCheck(int nCheck);
	virtual void SetText(LPCTSTR lpszText);
};

void CMiniToolbarCmdUI::Enable(BOOL bOn)
{
	m_bEnableChanged = TRUE;
	CMiniToolbar* pToolBar = (CMiniToolbar*)m_pOther;
	ASSERT(pToolBar != NULL);
	ASSERT_KINDOF(CMiniToolbar, pToolBar);
	ASSERT(m_nIndex < m_nIndexMax);

	pToolBar->EnableButton(m_nIndex, bOn);
}

void CMiniToolbarCmdUI::SetCheck(int nCheck)
{
	// ignore it
}

void CMiniToolbarCmdUI::SetText(LPCTSTR)
{
	// ignore it
}

LRESULT CMiniToolbar::OnIdleUpdateCmdUI(WPARAM wParam, LPARAM)
{
	// the style must be visible
	if (GetStyle() & WS_VISIBLE)
	{
		CFrameWnd* pTarget = GetParentFrame();
		if (pTarget != NULL)
		{
			OnUpdateCmdUI(pTarget, (BOOL)wParam);
		}
	}
	return 0L;
}

void CMiniToolbar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	CMiniToolbarCmdUI state;
	state.m_pOther = this;

	state.m_nIndexMax = m_Buttons.size();
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; state.m_nIndex++)
	{
		state.m_nID = m_Buttons[state.m_nIndex].nID;
		state.DoUpdate(pTarget, bDisableIfNoHndler);
	}

}

BOOL CMiniToolbar::Create(CWnd * pParent, UINT nID, RECT const & rect,
						DWORD dwStyle)
{
	return CWnd::Create(AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW,
											AfxGetApp()->LoadStandardCursor(IDC_ARROW), NULL, NULL), "",
						dwStyle, rect, pParent, nID, NULL);
}

int CMiniToolbar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	EnableToolTips();
	return 0;
}

int CMiniToolbar::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	ASSERT_VALID(this);
	ASSERT(::IsWindow(m_hWnd));

	int nHit = GetHitCode(point);
	if (0 == nHit)
	{
		return -1;
	}
	RECT rect;
	GetItemRect(nHit, rect);
	if (pTI != NULL && pTI->cbSize >= offsetof(TOOLINFO, lParam))
	{
		pTI->uFlags |= TTF_ALWAYSTIP;
		pTI->hwnd = m_hWnd;
		pTI->rect = rect;
		pTI->uId = nHit;
		pTI->lpszText = LPSTR_TEXTCALLBACK;
	}
	// found matching rect, return the ID of the button
	return nHit;
}

BOOL CMiniToolbar::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
	return ((CMainFrame*)AfxGetMainWnd())->OnToolTipText(0, pNMHDR, pResult);
}

