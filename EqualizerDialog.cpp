// EqualizerDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "EqualizerDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEqualizerDialog dialog


CEqualizerDialog::CEqualizerDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CEqualizerDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEqualizerDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CEqualizerDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEqualizerDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEqualizerDialog, CDialog)
	//{{AFX_MSG_MAP(CEqualizerDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEqualizerDialog message handlers
