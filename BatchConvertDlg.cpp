// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// BatchConvertDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wavesoapfront.h"
#include "BatchConvertDlg.h"
#include "BatchSaveTargetDlg.h"

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
	ON_BN_CLICKED(IDC_BUTTON_ADD_DESTINATION, OnButtonAddDestination)
	ON_BN_CLICKED(IDC_BUTTON_ADD_FILES, OnButtonAddFiles)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_DESTINATION, OnButtonDeleteDestination)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_FILES, OnButtonDeleteFiles)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_DESTINATION, OnButtonEditDestination)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_DOWN, OnButtonMoveDown)
	ON_BN_CLICKED(IDC_BUTTON_MOVE_UP, OnButtonMoveUp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBatchConvertDlg message handlers

void CBatchConvertDlg::OnButtonAddDestination()
{
	CBatchSaveTargetDlg dlg;
	if (IDOK != dlg.DoModal())
	{
		return;
	}
}

void CBatchConvertDlg::OnButtonAddFiles()
{
	// TODO Add your control notification handler code here

}

void CBatchConvertDlg::OnButtonDeleteDestination()
{
	// TODO Add your control notification handler code here

}

void CBatchConvertDlg::OnButtonDeleteFiles()
{
	// TODO Add your control notification handler code here

}

void CBatchConvertDlg::OnButtonEditDestination()
{
	// TODO: Add your control notification handler code here

}

void CBatchConvertDlg::OnButtonMoveDown()
{
	// TODO: Add your control notification handler code here

}

void CBatchConvertDlg::OnButtonMoveUp()
{
	// TODO: Add your control notification handler code here

}
