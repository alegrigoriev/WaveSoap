// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "WaveSoapFrontView.h"

#include "ChildFrm.h"
#include "TimeRulerView.h"
#include "AmplitudeRuler.h"
#include "WaveFftView.h"
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

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
	CRect tr = r;
	CRect r1 = r;
	//BOOL ret = m_dBar.Create(IDD_DIALOGBAR_MDI_CHILD, this);

	//CRect dr;
	//m_dBar.GetClientRect( & dr);

	//r1.top = dr.bottom;
	BOOL ret = m_wClient.Create(NULL, _T(""),
								WS_CHILD | WS_VISIBLE,
								r1, this, 1, pContext);

	if (ret)
	{
#if 0
		m_Tab.SetFont(CFont::FromHandle((HFONT)GetStockObject(ANSI_VAR_FONT)), FALSE);
		m_Tab.InsertItem(0, _T("Source"));
		m_Tab.InsertItem(1, _T("De-humming"));
		m_Tab.InsertItem(2, _T("De-clicking"));
		m_Tab.InsertItem(3, _T("Noise reduction"));
#endif
	}

	return ret;
}
/////////////////////////////////////////////////////////////////////////////
// CWaveMDIChildClient

CWaveMDIChildClient::CWaveMDIChildClient()
	: m_bShowWaveform(TRUE),
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
	RecalcLayout(cx, cy);


}

void CWaveMDIChildClient::RecalcLayout(int cx, int cy)
{
// resize child windows

	CRect r;
	int RulerHeight = CTimeRulerView::CalculateHeight();
	// calculate horizontal ruler width
	int RulerWidth = CAmplitudeRuler::CalculateWidth();

	CWnd * pHorRuler = GetDlgItem(HorizontalRulerID);

	if (pHorRuler)
	{
		r.left = RulerWidth;
		r.top = 0;
		r.bottom = RulerHeight;
		r.right = cx;
		pHorRuler->MoveWindow( & r);
	}
	CWnd * pVertRuler = GetDlgItem(VerticalRulerID);
	if (pVertRuler)
	{
		r.left = 0;
		r.right = RulerWidth;
		r.top = RulerHeight;
		r.bottom = cy - GetSystemMetrics(SM_CYHSCROLL);
		pVertRuler->MoveWindow( & r);
	}

	CWnd * pStatic = GetDlgItem(ScaleStaticID);
	if (pStatic)
	{
		r.left = 0;
		r.right = RulerWidth;
		r.top = cy - GetSystemMetrics(SM_CYHSCROLL);
		r.bottom = cy;
		pStatic->MoveWindow( & r);
	}

	r.left = RulerWidth;
	r.top = RulerHeight;
	r.bottom = cy;
	r.right = cx;
	CWnd * pWaveView = GetDlgItem(WaveViewID);
	if (pWaveView)
	{
		pWaveView->MoveWindow( & r);
	}

	CWnd * pFftView = GetDlgItem(FftViewID);
	if (pFftView)
	{
		pFftView->MoveWindow( & r);
	}

}

void CChildFrame::RecalcLayout(BOOL bNotify)
{
	// TODO: Add your specialized code here and/or call the base class
	CRect r, r1, dr;
	GetClientRect( & r);
	//m_dBar.GetWindowRect( & dr);
	//r1 = r;
	//r.bottom = dr.Height();
	//r1.top = r.bottom;
	//m_dBar.MoveWindow( & r);
	m_wClient.MoveWindow( & r);
}
/////////////////////////////////////////////////////////////////////////////
// CChildViewDialogBar dialog


CChildViewDialogBar::CChildViewDialogBar(CWnd* pParent /*=NULL*/)
	: CDialog(CChildViewDialogBar::IDD, pParent)
{
	//{{AFX_DATA_INIT(CChildViewDialogBar)
	//}}AFX_DATA_INIT
}


void CChildViewDialogBar::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChildViewDialogBar)
	DDX_Control(pDX, IDC_CHECK_DECLICKING, m_ClickRemoval);
	DDX_Control(pDX, IDC_CHECK_DEHUMMING, m_HumReduction);
	DDX_Control(pDX, IDC_CHECK_NOISE_REDUCTION, m_NoiseReduction);
	DDX_Control(pDX, IDC_TAB_VIEW_SWITCH, m_TabViewSwitch);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChildViewDialogBar, CDialog)
	//{{AFX_MSG_MAP(CChildViewDialogBar)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_VIEW_SWITCH, OnSelchangeTabViewSwitch)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_SWITCH_VIEW_MODE, OnSelchangeTabSwitchViewMode)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildViewDialogBar message handlers

void CChildViewDialogBar::OnSelchangeTabViewSwitch(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here

	*pResult = 0;
}

void CChildViewDialogBar::OnSelchangeTabSwitchViewMode(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here

	*pResult = 0;
}

BOOL CChildViewDialogBar::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_TabViewSwitch.InsertItem(0, _T("Source"));
	m_TabViewSwitch.InsertItem(1, _T("Result"));
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
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

	pRuler = GetDlgItem(VerticalRulerID);
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


	CWnd * pHorRuler = CreateView(RUNTIME_CLASS(CTimeRulerView),
								r, HorizontalRulerID, pContext);

	CWnd * pVertRuler = CreateView(RUNTIME_CLASS(CAmplitudeRuler),
									r, VerticalRulerID, pContext);


	wStatic.Create("STATIC", "", WS_VISIBLE | WS_CHILD | SS_CENTER, r, this, ScaleStaticID, NULL);

	CWnd * pFftView = CreateView(RUNTIME_CLASS(CWaveFftView),
								r, FftViewID, pContext, FALSE); //do not show

	CWnd * pView = CreateView(RUNTIME_CLASS(CWaveSoapFrontView),
							r, WaveViewID, pContext);

	if (0 && pView && pFftView)
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

	GetClientRect( & r);
	RecalcLayout(r.right, r.bottom);
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
		pView = GetDlgItem(WaveViewID);
		if (NULL != pView)
		{
			pView->ShowWindow(SW_HIDE);
		}
	}
}

void CWaveMDIChildClient::OnUpdateViewShowFft(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bShowFft);
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
		pView = GetDlgItem(FftViewID);
		if (NULL != pView)
		{
			pView->ShowWindow(SW_HIDE);
		}
	}
}

void CWaveMDIChildClient::OnUpdateViewWaveform(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bShowWaveform);
}
