// ChildDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "ChildDialog.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildDialog dialog


CChildDialog::CChildDialog(UINT id, CWnd* pParent /*=NULL*/)
	: CDialog(id, pParent)
{
	//{{AFX_DATA_INIT(CChildDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CChildDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChildDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChildDialog, CDialog)
	//{{AFX_MSG_MAP(CChildDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildDialog message handlers
BOOL CChildDialog::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
	//TRACE("CChildDialog::OnToolTipText\n");
	return ((CMainFrame*)AfxGetMainWnd())->OnToolTipText(0, pNMHDR, pResult);
}

