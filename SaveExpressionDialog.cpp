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
	m_GroupName = _T("");
	m_Name = _T("");
	m_Comment = _T("");
	//}}AFX_DATA_INIT
	m_ExpressionGroupSelected = 0;
	m_ExpressionSelected = 0;
	m_CurrExpressionGroupSelected = -1;
	m_bCommentChanged = false;
}


void CSaveExpressionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaveExpressionDialog)
	DDX_Control(pDX, IDC_COMBO_NAME, m_SavedExpressionCombo);
	DDX_Text(pDX, IDC_COMBO_NAME, m_Name);
	DDX_Control(pDX, IDC_EDIT_COMMENT, m_eComment);
	DDX_Text(pDX, IDC_EDIT_COMMENT, m_Comment);
	DDX_Control(pDX, IDC_COMBO_GROUP, m_ExpressionGroupCombo);
	DDX_Text(pDX, IDC_COMBO_GROUP, m_GroupName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaveExpressionDialog, CDialog)
	//{{AFX_MSG_MAP(CSaveExpressionDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_GROUP, OnSelchangeComboGroup)
	ON_CBN_SELCHANGE(IDC_COMBO_NAME, OnSelchangeComboName)
	ON_EN_CHANGE(IDC_EDIT_COMMENT, OnChangeEditComment)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaveExpressionDialog message handlers

BOOL CSaveExpressionDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	BuildExpressionGroupCombobox(m_ExpressionGroupSelected, m_ExpressionSelected);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSaveExpressionDialog::BuildExpressionGroupCombobox(int nGroupSelected, int nExprSelected)
{
	CThisApp * pApp = GetApp();
	m_ExpressionGroupCombo.ResetContent();
	for (int i = 0; i < pApp->m_NumOfExprGroups; i++)
	{
		m_ExpressionGroupCombo.AddString(pApp->m_ExpressionGroups[i]);
	}
	if (nGroupSelected >= pApp->m_NumOfExprGroups)
	{
		nGroupSelected = pApp->m_NumOfExprGroups - 1;
	}
	m_ExpressionGroupSelected = nGroupSelected;
	m_CurrExpressionGroupSelected = nGroupSelected;
	m_ExpressionGroupCombo.SetCurSel(m_ExpressionGroupSelected);
	LoadExpressionCombobox(nGroupSelected, nExprSelected);
}

void CSaveExpressionDialog::LoadExpressionCombobox(int nGroupSelected, int nExprSelected)
{
	CThisApp * pApp = GetApp();
	m_ExpressionGroupSelected = nGroupSelected;
	m_CurrExpressionGroupSelected = nGroupSelected;

	while(m_SavedExpressionCombo.DeleteString(0) > 0);

	if (nGroupSelected < 0
		|| nGroupSelected >= pApp->m_NumOfExprGroups)
	{
		m_CurrExpressionGroupSelected = -1;
		m_SavedExpressionCombo.SetCurSel(-1);
		OnSelchangeComboName();
		return;
	}

	for (int i = 0; i < pApp->m_NumExpressions[nGroupSelected]; i++)
	{
		m_SavedExpressionCombo.AddString(
										pApp->m_ExpressionNames[i + pApp->m_IndexOfGroupBegin[nGroupSelected]]);
	}
	if (nExprSelected >= i)
	{
		nExprSelected = i - 1;
	}
}

void CSaveExpressionDialog::OnSelchangeComboGroup()
{
	int sel = m_ExpressionGroupCombo.GetCurSel();
	if (sel == m_CurrExpressionGroupSelected)
	{
		return;
	}
	LoadExpressionCombobox(sel, 0);
}

void CSaveExpressionDialog::OnSelchangeComboName()
{
	// if name selected, set comment edit box
	if ( ! m_bCommentChanged)
	{
		CThisApp * pApp = GetApp();
		int sel = m_SavedExpressionCombo.GetCurSel();
		if (sel >= 0 && sel < pApp->m_NumExpressions[m_ExpressionGroupSelected])
		{
			int ExprIndex = sel + pApp->m_IndexOfGroupBegin[m_ExpressionGroupSelected];
			m_eComment.SetWindowText(pApp->m_ExpressionComments[ExprIndex]);
		}
		else
		{
			m_eComment.SetWindowText("");
		}

		m_bCommentChanged = false;
	}
}

void CSaveExpressionDialog::OnChangeEditComment()
{
	if (m_eComment.m_hWnd != NULL)
	{
		if (m_eComment.GetWindowTextLength() != 0)
		{
			m_bCommentChanged = true;
		}
		else
		{
			m_bCommentChanged = false;
		}
	}
}
