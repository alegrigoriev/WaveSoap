// OperationDialogs.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "OperationDialogs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVolumeChangeDialog dialog


CVolumeChangeDialog::CVolumeChangeDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CVolumeChangeDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVolumeChangeDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CVolumeChangeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVolumeChangeDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVolumeChangeDialog, CDialog)
	//{{AFX_MSG_MAP(CVolumeChangeDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVolumeChangeDialog message handlers
