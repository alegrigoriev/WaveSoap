// ChildDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "ChildDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildDialog dialog


CChildDialog::CChildDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CChildDialog::IDD, pParent)
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
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildDialog message handlers
