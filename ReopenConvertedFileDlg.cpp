// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
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
	m_Text = _T("");
	//}}AFX_DATA_INIT
}


void CReopenConvertedFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReopenConvertedFileDlg)
	DDX_Text(pDX, IDC_STATIC_PROMPT, m_Text);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReopenConvertedFileDlg, CDialog)
	//{{AFX_MSG_MAP(CReopenConvertedFileDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReopenConvertedFileDlg message handlers
