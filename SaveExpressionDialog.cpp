// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
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


CSaveExpressionDialog::CSaveExpressionDialog(const ExprGroupVector & Exprs,
											CWnd* pParent /*=NULL*/)
	: BaseClass(IDD, pParent),
	m_Expressions(Exprs)
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
	BaseClass::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaveExpressionDialog)
	DDX_Control(pDX, IDC_COMBO_NAME, m_SavedExpressionCombo);
	DDX_Text(pDX, IDC_COMBO_NAME, m_Name);
	DDX_Control(pDX, IDC_EDIT_COMMENT, m_eComment);
	DDX_Text(pDX, IDC_EDIT_COMMENT, m_Comment);
	DDX_Control(pDX, IDC_COMBO_GROUP, m_ExpressionGroupCombo);
	DDX_Text(pDX, IDC_COMBO_GROUP, m_GroupName);
	//}}AFX_DATA_MAP
	if (pDX->m_bSaveAndValidate
		&& m_GroupName == LoadCString(IDS_ALL_EXPRESSIONS_GROUP))
	{
		AfxMessageBox(IDS_MSG_GROUP_CANT_BE_ALL_EXPRESSION);
		pDX->Fail();
	}
}

BEGIN_MESSAGE_MAP(CSaveExpressionDialog, BaseClass)
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
	BaseClass::OnInitDialog();

	BuildExpressionGroupCombobox(m_ExpressionGroupSelected, m_ExpressionSelected);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSaveExpressionDialog::BuildExpressionGroupCombobox(unsigned nGroupSelected, int nExprSelected)
{
	m_ExpressionGroupCombo.ResetContent();

	for (ExprGroupVector::const_iterator ii = m_Expressions.begin() + 1
		; ii != m_Expressions.end(); ii++)
	{
		m_ExpressionGroupCombo.AddString(ii->name);
	}

	if (nGroupSelected > m_Expressions.size() - 1)
	{
		nGroupSelected = 0;
	}
	m_ExpressionGroupSelected = nGroupSelected;
	m_CurrExpressionGroupSelected = nGroupSelected;
	m_ExpressionGroupCombo.SetCurSel(m_ExpressionGroupSelected);
	LoadExpressionCombobox(nGroupSelected, nExprSelected);
}

void CSaveExpressionDialog::LoadExpressionCombobox(unsigned nGroupSelected, unsigned nExprSelected)
{
	m_ExpressionGroupSelected = nGroupSelected;
	m_CurrExpressionGroupSelected = nGroupSelected;

	while(m_SavedExpressionCombo.DeleteString(0) > 0);

	if (nGroupSelected >= m_Expressions.size())
	{
		m_CurrExpressionGroupSelected = -1;
		m_SavedExpressionCombo.SetCurSel(-1);
		OnSelchangeComboName();
		return;
	}

	for (ExprVector::const_iterator jj = m_Expressions[nGroupSelected + 1].exprs.begin()
		; jj != m_Expressions[nGroupSelected + 1].exprs.end(); jj++)
	{
		m_SavedExpressionCombo.AddString(jj->name);
	}
	unsigned NumExpressions =  (unsigned)m_Expressions[nGroupSelected + 1].exprs.size();
	if (nExprSelected >= NumExpressions)
	{
		nExprSelected = NumExpressions - 1;
	}
	m_SavedExpressionCombo.SetCurSel(nExprSelected);
	OnSelchangeComboName();
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
		unsigned sel = m_SavedExpressionCombo.GetCurSel();

		ExprGroupVector::const_iterator ii = m_Expressions.begin() + m_ExpressionGroupSelected + 1;
		if (sel < ii->exprs.size())
		{
			m_eComment.SetWindowText(ii->exprs[sel].comment);
		}
		else
		{
			m_eComment.SetWindowText(_T(""));
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
