// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
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
	m_Text = _T("");
	//}}AFX_DATA_INIT
}


void CReopenCompressedFileDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReopenCompressedFileDialog)
	DDX_Text(pDX, IDC_STATIC_PROMPT, m_Text);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReopenCompressedFileDialog, CDialog)
	//{{AFX_MSG_MAP(CReopenCompressedFileDialog)
	ON_BN_CLICKED(IDNO, OnNo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReopenCompressedFileDialog message handlers

void CReopenCompressedFileDialog::OnNo()
{
	EndDialog(IDNO);
}
