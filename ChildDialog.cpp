// ChildDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "ChildDialog.h"
#include "MainFrm.h"
#include "SaveExpressionDialog.h"

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

COperandsDialog::COperandsDialog(UINT id, CWnd* pParent /*=NULL*/)
	: CChildDialog(id, pParent)
{
	//{{AFX_DATA_INIT(COperandsDialog)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_dFrequency = 0.;
}


void COperandsDialog::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COperandsDialog)
	DDX_Control(pDX, IDC_EDIT_FREQUENCY, m_eFrequency);
	//}}AFX_DATA_MAP
	m_eFrequency.ExchangeData(pDX, m_dFrequency,
							"Frequency argument", "", 0., 1000000.);
}

BEGIN_MESSAGE_MAP(COperandsDialog, CChildDialog)
	//{{AFX_MSG_MAP(COperandsDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
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
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_DELETE_EXPRESSION, OnUpdateDeleteExpression)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_INSERT_EXPRESSION, OnUpdateInsertExpression)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInsertExpressionDialog message handlers

BOOL CInsertExpressionDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	// load expressions
	CThisApp * pApp = GetApp();
	BuildExpressionGroupCombobox(m_ExpressionGroupSelected, m_ExpressionSelected);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInsertExpressionDialog::BuildExpressionGroupCombobox(int nGroupSelected, int nExprSelected)
{
	CThisApp * pApp = GetApp();
	m_ExpressionGroupCombo.ResetContent();
	if (pApp->m_NumOfExprGroups > 0)
	{
		m_ExpressionGroupCombo.AddString("All Expressions");
	}
	for (int i = 0; i < pApp->m_NumOfExprGroups; i++)
	{
		m_ExpressionGroupCombo.AddString(pApp->m_ExpressionGroups[i]);
	}
	if (nGroupSelected > pApp->m_NumOfExprGroups)
	{
		nGroupSelected = 0;
	}
	m_ExpressionGroupSelected = nGroupSelected;
	m_CurrExpressionGroupSelected = nGroupSelected;
	m_ExpressionGroupCombo.SetCurSel(m_ExpressionGroupSelected);
	LoadExpressionCombobox(nGroupSelected, nExprSelected);
}

void CInsertExpressionDialog::LoadExpressionCombobox(int nGroupSelected, int nExprSelected)
{
	CThisApp * pApp = GetApp();
	m_ExpressionGroupSelected = nGroupSelected;
	m_CurrExpressionGroupSelected = nGroupSelected;

	m_SavedExpressionCombo.ResetContent();
	if (nGroupSelected < 0
		|| pApp->m_NumOfExprGroups <= 0
		|| nGroupSelected > pApp->m_NumOfExprGroups)
	{
		m_CurrExpressionGroupSelected = -1;
		m_SavedExpressionCombo.SetCurSel(-1);
		OnSelchangeComboSavedExpressions();
		return;
	}
	int NumExpressions;
	int GroupBegin;
	if (0 == nGroupSelected)
	{
		GroupBegin = 0;
		NumExpressions = pApp->m_NumExpressions[pApp->m_NumOfExprGroups - 1] +
						pApp->m_IndexOfGroupBegin[pApp->m_NumOfExprGroups - 1];
	}
	else
	{
		GroupBegin = pApp->m_IndexOfGroupBegin[nGroupSelected - 1];
		NumExpressions = pApp->m_NumExpressions[nGroupSelected - 1];
	}
	for (int i = 0; i < NumExpressions; i++)
	{
		m_SavedExpressionCombo.AddString(pApp->m_ExpressionNames[i + GroupBegin]);
	}
	if (nExprSelected >= i)
	{
		nExprSelected = i - 1;
	}
	m_SavedExpressionCombo.SetCurSel(nExprSelected);
	OnSelchangeComboSavedExpressions();
}

void CInsertExpressionDialog::OnSelchangeComboSavedExpressionGroup()
{
	int sel = m_ExpressionGroupCombo.GetCurSel();
	if (sel == m_CurrExpressionGroupSelected)
	{
		return;
	}
	LoadExpressionCombobox(sel, 0);
}

void CInsertExpressionDialog::OnSelchangeComboSavedExpressions()
{
	CThisApp * pApp = GetApp();
	int sel = m_SavedExpressionCombo.GetCurSel();
	int NumExpressions;
	int GroupBegin;
	if (0 == m_ExpressionGroupSelected)
	{
		GroupBegin = 0;
		NumExpressions = pApp->m_NumExpressions[pApp->m_NumOfExprGroups - 1] +
						pApp->m_IndexOfGroupBegin[pApp->m_NumOfExprGroups - 1];
	}
	else
	{
		GroupBegin = pApp->m_IndexOfGroupBegin[m_ExpressionGroupSelected - 1];
		NumExpressions = pApp->m_NumExpressions[m_ExpressionGroupSelected - 1];
	}
	if (sel >= 0 && sel < NumExpressions)
	{
		int ExprIndex = sel + GroupBegin;
		CString s;
		s.Format("%s - %s", LPCTSTR(pApp->m_Expressions[ExprIndex]),
				LPCTSTR(pApp->m_ExpressionComments[ExprIndex]));
		m_Description.SetWindowText(s);
	}
	else
	{
		m_Description.SetWindowText("");
	}
}

void CInsertExpressionDialog::OnButtonInsertExpression()
{
	CEdit * pEdit = (CEdit *) GetParent()->GetDlgItem(IDC_EDIT_EXPRESSION);
	CThisApp * pApp = GetApp();
	if (NULL != pEdit)
	{
		int sel = m_SavedExpressionCombo.GetCurSel();
		int NumExpressions;
		int GroupBegin;
		if (0 == m_ExpressionGroupSelected)
		{
			GroupBegin = 0;
			NumExpressions = pApp->m_NumExpressions[pApp->m_NumOfExprGroups - 1] +
							pApp->m_IndexOfGroupBegin[pApp->m_NumOfExprGroups - 1];
		}
		else
		{
			GroupBegin = pApp->m_IndexOfGroupBegin[m_ExpressionGroupSelected - 1];
			NumExpressions = pApp->m_NumExpressions[m_ExpressionGroupSelected - 1];
		}
		if (sel >= 0 && sel < NumExpressions)
		{
			int ExprIndex = sel + GroupBegin;
			pEdit->ReplaceSel(pApp->m_Expressions[ExprIndex], TRUE);
			pEdit->SetFocus();
		}
	}
}

void CInsertExpressionDialog::OnButtonDeleteExpression()
{
	CString s;
	CThisApp * pApp = GetApp();
	int ExprSel = m_SavedExpressionCombo.GetCurSel();
	if (m_ExpressionGroupSelected < 0
		|| m_ExpressionGroupSelected >= pApp->m_NumOfExprGroups
		|| ExprSel < 0
		|| ExprSel >= pApp->m_NumExpressions[m_ExpressionGroupSelected])
	{
		return;
	}
	int ExprIndex = ExprSel + pApp->m_IndexOfGroupBegin[m_ExpressionGroupSelected];
	s.Format(IDS_DELETE_EXPRESSION, LPCTSTR(pApp->m_ExpressionNames[ExprIndex]),
			LPCTSTR(pApp->m_ExpressionGroups[m_ExpressionGroupSelected]));
	if (IDYES == AfxMessageBox(s, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2))
	{
		int i;
		// delete this line from the list
		for (i = ExprIndex; i < CThisApp::MaxSavedTotalExpressions - 1; i++)
		{
			pApp->m_Expressions[i] = pApp->m_Expressions[i + 1];
			pApp->m_ExpressionComments[i] = pApp->m_ExpressionComments[i + 1];
			pApp->m_ExpressionNames[i] = pApp->m_ExpressionNames[i + 1];
		}

		pApp->m_Expressions[CThisApp::MaxSavedTotalExpressions - 1].Empty();
		pApp->m_ExpressionComments[CThisApp::MaxSavedTotalExpressions - 1].Empty();
		pApp->m_ExpressionNames[CThisApp::MaxSavedTotalExpressions - 1].Empty();

		pApp->m_NumExpressions[m_ExpressionGroupSelected]--;
		if (0 == pApp->m_NumExpressions[m_ExpressionGroupSelected])
		{
			// remove group
			for (i = m_ExpressionGroupSelected; i < pApp->m_NumOfExprGroups - 1; i++)
			{
				pApp->m_IndexOfGroupBegin[i] = pApp->m_IndexOfGroupBegin[i + 1] - 1;
				pApp->m_NumExpressions[i] = pApp->m_NumExpressions[i + 1];
				pApp->m_ExpressionGroups[i] = pApp->m_ExpressionGroups[i + 1];
			}
			pApp->m_NumOfExprGroups--;
			pApp->m_ExpressionGroups[pApp->m_NumOfExprGroups].Empty();
			// rebuild combobox
			BuildExpressionGroupCombobox(m_CurrExpressionGroupSelected, 0);
		}
		else
		{
			for (i = m_ExpressionGroupSelected + 1; i < pApp->m_NumOfExprGroups; i++)
			{
				pApp->m_IndexOfGroupBegin[i]--;
			}
			LoadExpressionCombobox(m_CurrExpressionGroupSelected, ExprSel);
		}
	}
}

void CInsertExpressionDialog::SaveExpressionAs(const CString & expr)
{
	// query expression name and comment
	CThisApp * pApp = GetApp();
	CSaveExpressionDialog dlg;
	if (IDOK == dlg.DoModal())
	{
		// find if there is such group
		int i, j;
		for (i = 0; i < pApp->m_NumOfExprGroups; i++)
		{
			if (pApp->m_ExpressionGroups[i] == dlg.m_GroupName)
			{
				break;
			}
		}
		if (i >= pApp->m_NumOfExprGroups)
		{
			// add a new group
			if (pApp->m_NumOfExprGroups >= pApp->MaxSavedExpressionGroups)
			{
				// too many groups
				AfxMessageBox("Can't add a new group, there are already too many");
				// return for now
				return;
			}
			pApp->m_ExpressionGroups[i] = dlg.m_GroupName;
			if (i > 0)
			{
				pApp->m_IndexOfGroupBegin[i] = pApp->m_IndexOfGroupBegin[i - 1]
												+ pApp->m_NumExpressions[i - 1];
			}
			else
			{
				pApp->m_IndexOfGroupBegin[i] = 0;
			}
			pApp->m_NumExpressions[i] = 0;
		}
		// check if there is already an expression with the same name
		for (j = 0; j < pApp->m_NumExpressions[i]; j++)
		{
			if (pApp->m_ExpressionNames[j + pApp->m_IndexOfGroupBegin[i]] == dlg.m_Name)
			{
				break;
			}
		}
		if (j < pApp->m_NumExpressions[i])
		{
			CString s;
			s.Format(IDS_REPLACE_EXPRESSION, LPCTSTR(dlg.m_Name), LPCTSTR(dlg.m_GroupName));
			if (IDYES != AfxMessageBox(s, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2))
			{
				return;
			}
			pApp->m_Expressions[j + pApp->m_IndexOfGroupBegin[i]] = expr;
			pApp->m_ExpressionComments[j + pApp->m_IndexOfGroupBegin[i]] =
				dlg.m_Comment;
		}
		else
		{
			if (pApp->m_NumOfExprGroups != 0
				&& pApp->m_IndexOfGroupBegin[pApp->m_NumOfExprGroups - 1]
				+ pApp->m_NumExpressions[pApp->m_NumOfExprGroups - 1] >= pApp->MaxSavedTotalExpressions)
			{
				AfxMessageBox("Too many saved expressions");
				return;
			}
			for (int n = i + 1; n < pApp->m_NumOfExprGroups; n++)
			{
				pApp->m_IndexOfGroupBegin[n]++;
			}
			pApp->m_NumExpressions[i]++;
			for (int m = pApp->MaxSavedTotalExpressions - 1; m > j + pApp->m_IndexOfGroupBegin[i]; m--)
			{
				pApp->m_Expressions[m] = pApp->m_Expressions[m - 1];
				pApp->m_ExpressionComments[m] = pApp->m_ExpressionComments[m - 1];
				pApp->m_ExpressionNames[m] = pApp->m_ExpressionNames[m - 1];
			}
			pApp->m_Expressions[m] = expr;
			pApp->m_ExpressionComments[m] = dlg.m_Comment;
			pApp->m_ExpressionNames[m] = dlg.m_Name;
		}
		BuildExpressionGroupCombobox(i, j);
	}
}

void CInsertExpressionDialog::OnUpdateDeleteExpression(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetApp()->m_NumOfExprGroups != 0);
}

void CInsertExpressionDialog::OnUpdateInsertExpression(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetApp()->m_NumOfExprGroups != 0);
}

