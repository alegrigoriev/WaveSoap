// BatchConvertDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wavesoapfront.h"
#include "BatchConvertDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBatchConvertDlg dialog


CBatchConvertDlg::CBatchConvertDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBatchConvertDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBatchConvertDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CBatchConvertDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBatchConvertDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBatchConvertDlg, CDialog)
	//{{AFX_MSG_MAP(CBatchConvertDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBatchConvertDlg message handlers
