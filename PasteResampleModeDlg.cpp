// PasteResampleModeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wavesoapfront.h"
#include "PasteResampleModeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPasteResampleModeDlg dialog


CPasteResampleModeDlg::CPasteResampleModeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPasteResampleModeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPasteResampleModeDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPasteResampleModeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPasteResampleModeDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPasteResampleModeDlg, CDialog)
	//{{AFX_MSG_MAP(CPasteResampleModeDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPasteResampleModeDlg message handlers
