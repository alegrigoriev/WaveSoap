// OperationDialogs2.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "OperationDialogs2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInsertSilenceDialog dialog


CInsertSilenceDialog::CInsertSilenceDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CInsertSilenceDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInsertSilenceDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CInsertSilenceDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInsertSilenceDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInsertSilenceDialog, CDialog)
	//{{AFX_MSG_MAP(CInsertSilenceDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInsertSilenceDialog message handlers
