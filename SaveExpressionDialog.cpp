// SaveExpressionDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "SaveExpressionDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSaveExpressionDialog dialog


CSaveExpressionDialog::CSaveExpressionDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSaveExpressionDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSaveExpressionDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSaveExpressionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaveExpressionDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaveExpressionDialog, CDialog)
	//{{AFX_MSG_MAP(CSaveExpressionDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaveExpressionDialog message handlers
