// BatchSaveTargetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wavesoapfront.h"
#include "BatchSaveTargetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBatchSaveTargetDlg dialog


CBatchSaveTargetDlg::CBatchSaveTargetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBatchSaveTargetDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBatchSaveTargetDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CBatchSaveTargetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBatchSaveTargetDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBatchSaveTargetDlg, CDialog)
	//{{AFX_MSG_MAP(CBatchSaveTargetDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBatchSaveTargetDlg message handlers
