// ReopenCompressedFileDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "ReopenCompressedFileDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReopenCompressedFileDialog dialog


CReopenCompressedFileDialog::CReopenCompressedFileDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CReopenCompressedFileDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CReopenCompressedFileDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CReopenCompressedFileDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReopenCompressedFileDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReopenCompressedFileDialog, CDialog)
	//{{AFX_MSG_MAP(CReopenCompressedFileDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReopenCompressedFileDialog message handlers
