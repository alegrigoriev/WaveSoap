// NewFilePropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "NewFilePropertiesDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewFilePropertiesDlg dialog


CNewFilePropertiesDlg::CNewFilePropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewFilePropertiesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewFilePropertiesDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CNewFilePropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewFilePropertiesDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewFilePropertiesDlg, CDialog)
	//{{AFX_MSG_MAP(CNewFilePropertiesDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewFilePropertiesDlg message handlers
