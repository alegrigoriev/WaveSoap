// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "WaveSoapFrontView.h"

#include "ChildFrm.h"
#include "TimeRulerView.h"

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
	BOOL ret = m_wClient.Create(NULL, _T(""),
								WS_CHILD | WS_VISIBLE,
								r, this, 1, NULL);
	if (ret)
	{
		CRect r1 = r;
		int height = GetSystemMetrics(SM_CYMENU);
		r1.bottom = height;
		m_wClient.CreateView(RUNTIME_CLASS(CTimeRulerView),
							r1, 2, pContext);
		CRect rect = r;
		rect.top = height;
		CWnd * pView = m_wClient.CreateView(RUNTIME_CLASS(CWaveSoapFrontView),
											rect, 1, pContext);
		SetActiveView(DYNAMIC_DOWNCAST(CView, pView));
	}

	return ret;
}
/////////////////////////////////////////////////////////////////////////////
// CWaveMDIChildClient

CWaveMDIChildClient::CWaveMDIChildClient()
{
}

CWaveMDIChildClient::~CWaveMDIChildClient()
{
}


BEGIN_MESSAGE_MAP(CWaveMDIChildClient, CWnd)
	//{{AFX_MSG_MAP(CWaveMDIChildClient)
	ON_WM_SIZE()
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CWnd * CWaveMDIChildClient::CreateView(CRuntimeClass* pViewClass,
										CRect rect, int nID, CCreateContext* pContext)
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
	int height = GetSystemMetrics(SM_CYMENU);
	CRect r1(0, height, cx, cy);
	CRect r2(0, 0, cx, height);
	CWnd * pWnd = GetDlgItem(2);
	if (pWnd) pWnd->MoveWindow( & r2);
	pWnd = GetDlgItem(1);
	if (pWnd) pWnd->MoveWindow( & r1);

}

void CChildFrame::RecalcLayout(BOOL bNotify)
{
	// TODO: Add your specialized code here and/or call the base class
	CRect r;
	GetClientRect( & r);
	CWnd * pWnd = GetDlgItem(1);
	if (pWnd) pWnd->MoveWindow( & r);
}
