// FilterDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "FilterDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog dialog


CFilterDialog::CFilterDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CFilterDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFilterDialog)
	m_bUndo = FALSE;
	m_bZeroPhase = FALSE;
	//}}AFX_DATA_INIT
}


void CFilterDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFilterDialog)
	DDX_Check(pDX, IDC_CHECK_UNDO, m_bUndo);
	DDX_Check(pDX, IDC_CHECK_ZERO_PHASE, m_bZeroPhase);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFilterDialog, CDialog)
	//{{AFX_MSG_MAP(CFilterDialog)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, OnButtonLoad)
	ON_BN_CLICKED(IDC_BUTTON_RESET_BANDS, OnButtonResetBands)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_AS, OnButtonSaveAs)
	ON_BN_CLICKED(IDC_BUTTON_SELECTION, OnButtonSelection)
	ON_BN_CLICKED(IDC_CHECK_ZERO_PHASE, OnCheckZeroPhase)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog message handlers

void CFilterDialog::OnButtonLoad()
{
	// TODO: Add your control notification handler code here

}

void CFilterDialog::OnButtonResetBands()
{
	// TODO: Add your control notification handler code here

}

void CFilterDialog::OnButtonSaveAs()
{
	// TODO: Add your control notification handler code here

}

void CFilterDialog::OnButtonSelection()
{
	// TODO: Add your control notification handler code here

}

void CFilterDialog::OnCheckZeroPhase()
{
	// TODO: Add your control notification handler code here

}
