// ReopenSavedFileCopyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "ReopenSavedFileCopyDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReopenSavedFileCopyDlg dialog


CReopenSavedFileCopyDlg::CReopenSavedFileCopyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CReopenSavedFileCopyDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CReopenSavedFileCopyDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CReopenSavedFileCopyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReopenSavedFileCopyDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReopenSavedFileCopyDlg, CDialog)
	//{{AFX_MSG_MAP(CReopenSavedFileCopyDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReopenSavedFileCopyDlg message handlers
