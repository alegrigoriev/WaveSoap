// ChildDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "ChildDialog.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildDialog dialog


CChildDialog::CChildDialog(UINT id, CWnd* pParent /*=NULL*/)
	: CDialog(id, pParent)
{
	//{{AFX_DATA_INIT(CChildDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CChildDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChildDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChildDialog, CDialog)
	//{{AFX_MSG_MAP(CChildDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildDialog message handlers
BOOL CChildDialog::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
	//TRACE("CChildDialog::OnToolTipText\n");
	return ((CMainFrame*)AfxGetMainWnd())->OnToolTipText(0, pNMHDR, pResult);
}

/////////////////////////////////////////////////////////////////////////////
// CInsertExpressionDialog dialog


CInsertExpressionDialog::CInsertExpressionDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CInsertExpressionDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInsertExpressionDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_ExpressionGroupSelected = 0;
	m_ExpressionSelected = 0;
	m_CurrExpressionGroupSelected = -1;
	m_NumSavedExpressions = 0;
	m_NumExprGroups = 0;
}


void CInsertExpressionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInsertExpressionDialog)
	DDX_Control(pDX, IDC_COMBO_SAVED_EXPRESSION_GROUP, m_ExpressionGroupCombo);
	DDX_Control(pDX, IDC_STATIC_DESCRIPTION, m_Description);
	DDX_Control(pDX, IDC_COMBO_SAVED_EXPRESSIONS, m_SavedExpressionCombo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInsertExpressionDialog, CDialog)
	//{{AFX_MSG_MAP(CInsertExpressionDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_SAVED_EXPRESSION_GROUP, OnSelchangeComboSavedExpressionGroup)
	ON_CBN_SELCHANGE(IDC_COMBO_SAVED_EXPRESSIONS, OnSelchangeComboSavedExpressions)
	ON_BN_CLICKED(IDC_BUTTON_INSERT_EXPRESSION, OnButtonInsertExpression)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_EXPRESSION, OnButtonDeleteExpression)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInsertExpressionDialog message handlers

BOOL CInsertExpressionDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	// load expressions
	CThisApp * pApp = GetApp();

	for (int i = 0; i < pApp->m_NumOfExprGroups; i++)
	{
		m_ExpressionGroupCombo.AddString(pApp->m_ExpressionGroups[i]);
	}
	m_CurrExpressionGroupSelected = -1;
	m_ExpressionGroupCombo.SetCurSel(0);
	OnSelchangeComboSavedExpressionGroup();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInsertExpressionDialog::OnSelchangeComboSavedExpressionGroup()
{
	int sel = m_ExpressionGroupCombo.GetCurSel();
	CThisApp * pApp = GetApp();
	if (sel < 0
		|| sel >= pApp->m_NumOfExprGroups
		|| sel == m_CurrExpressionGroupSelected)
	{
		return;
	}
	m_SavedExpressionCombo.ResetContent();

	m_ExpressionGroupSelected = sel;
	m_CurrExpressionGroupSelected = sel;

	for (int i = 0; i < pApp->m_NumExpressions[sel]; i++)
	{
		m_SavedExpressionCombo.AddString(
										pApp->m_ExpressionNames[i + pApp->m_IndexOfGroupBegin[sel]]);
	}
	m_SavedExpressionCombo.SetCurSel(0);
	OnSelchangeComboSavedExpressions();
}

void CInsertExpressionDialog::OnSelchangeComboSavedExpressions()
{
	CThisApp * pApp = GetApp();
	int sel = m_SavedExpressionCombo.GetCurSel();
	if (sel >= 0 && sel < pApp->m_NumExpressions[m_ExpressionGroupSelected])
	{
		int ExprIndex = sel + pApp->m_IndexOfGroupBegin[m_ExpressionGroupSelected];
		CString s;
		s.Format("%s - %s", LPCTSTR(pApp->m_Expressions[ExprIndex]),
				LPCTSTR(pApp->m_ExpressionComments[ExprIndex]));
		m_Description.SetWindowText(s);
	}
}

void CInsertExpressionDialog::OnButtonInsertExpression()
{
	CEdit * pEdit = (CEdit *) GetParent()->GetDlgItem(IDC_EDIT_EXPRESSION);
	CThisApp * pApp = GetApp();
	if (NULL != pEdit)
	{
		int sel = m_SavedExpressionCombo.GetCurSel();
		if (sel >= 0 && sel < pApp->m_NumExpressions[m_ExpressionGroupSelected])
		{
			int ExprIndex = sel + pApp->m_IndexOfGroupBegin[m_ExpressionGroupSelected];
			pEdit->ReplaceSel(pApp->m_Expressions[ExprIndex], TRUE);
			pEdit->SetFocus();
		}
	}
}

void CInsertExpressionDialog::OnButtonDeleteExpression()
{
	CString s;
	int GroupSel = m_ExpressionGroupCombo.GetCurSel();
	int ExprSel = m_SavedExpressionCombo.GetCurSel();
	s.Format(IDS_DELETE_EXPRESSION, LPCTSTR(
	if (IDOK == AfxMessageBox(s, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2))
	{
	}
}
