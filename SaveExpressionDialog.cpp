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
	m_Comment = _T("");
	m_Name = _T("");
	//}}AFX_DATA_INIT
}


void CSaveExpressionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaveExpressionDialog)
	DDX_Text(pDX, IDC_EDIT_COMMENT, m_Comment);
	DDX_CBString(pDX, IDC_COMBO_NAME, m_Name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaveExpressionDialog, CDialog)
	//{{AFX_MSG_MAP(CSaveExpressionDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_NAME, OnSelchangeComboName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaveExpressionDialog message handlers

void CSaveExpressionDialog::OnSelchangeComboName()
{
	// TODO: Add your control notification handler code here

}
