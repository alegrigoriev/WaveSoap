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
	m_Prompt = _T("");
	//}}AFX_DATA_INIT
	m_bDisableDirect = FALSE;
}


void CReopenSavedFileCopyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReopenSavedFileCopyDlg)
	DDX_Text(pDX, IDC_STATIC_PROMPT, m_Prompt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReopenSavedFileCopyDlg, CDialog)
	//{{AFX_MSG_MAP(CReopenSavedFileCopyDlg)
	ON_BN_CLICKED(IDNO, OnNo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReopenSavedFileCopyDlg message handlers

void CReopenSavedFileCopyDlg::OnNo()
{
	EndDialog(IDNO);
}

BOOL CReopenSavedFileCopyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (m_bDisableDirect)
	{
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
