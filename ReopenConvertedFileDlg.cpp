// ReopenConvertedFileDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "ReopenConvertedFileDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReopenConvertedFileDlg dialog


CReopenConvertedFileDlg::CReopenConvertedFileDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CReopenConvertedFileDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CReopenConvertedFileDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CReopenConvertedFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReopenConvertedFileDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReopenConvertedFileDlg, CDialog)
	//{{AFX_MSG_MAP(CReopenConvertedFileDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReopenConvertedFileDlg message handlers
