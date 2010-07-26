// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// ChildDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "ChildDialog.h"
#include "MainFrm.h"
#include "SaveExpressionDialog.h"
#include "FileDialogWithHistory.h"
#include "PathEx.h"
#include "resource.h"

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
	m_dFrequency1 = 0.;
	m_dFrequency2 = 0.;
	m_dFrequency3 = 0.;

	m_eFrequency.SetPrecision(2);
	m_eFrequency1.SetPrecision(2);
	m_eFrequency2.SetPrecision(2);
	m_eFrequency3.SetPrecision(2);
}


void COperandsDialog::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COperandsDialog)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_EDIT_FREQUENCY, m_eFrequency);
	DDX_Control(pDX, IDC_EDIT_FREQUENCY1, m_eFrequency1);
	DDX_Control(pDX, IDC_EDIT_FREQUENCY2, m_eFrequency2);
	DDX_Control(pDX, IDC_EDIT_FREQUENCY3, m_eFrequency3);
	m_eFrequency.ExchangeData(pDX, m_dFrequency,
							IDS_INPUT_NAME_FREQUENCY_ARGUMENT, 0, 0., 1000000.);
	m_eFrequency1.ExchangeData(pDX, m_dFrequency1,
								IDS_INPUT_NAME_FREQUENCY_ARGUMENT1, 0, 0., 1000000.);
	m_eFrequency2.ExchangeData(pDX, m_dFrequency2,
								IDS_INPUT_NAME_FREQUENCY_ARGUMENT2, 0, 0., 1000000.);
	m_eFrequency3.ExchangeData(pDX, m_dFrequency3,
								IDS_INPUT_NAME_FREQUENCY_ARGUMENT3, 0, 0., 1000000.);
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
	m_ExpressionsChanged = false;
}


void CInsertExpressionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInsertExpressionDialog)
	DDX_Control(pDX, IDC_COMBO_SAVED_EXPRESSION_GROUP, m_ExpressionGroupCombo);
	DDX_Control(pDX, IDC_STATIC_DESCRIPTION, m_Description);
	DDX_Control(pDX, IDC_COMBO_SAVED_EXPRESSIONS, m_SavedExpressionCombo);
	//}}AFX_DATA_MAP
	if (pDX->m_bSaveAndValidate
		&& m_ExpressionsChanged)
	{
		UnloadExpressions();
	}
}


BEGIN_MESSAGE_MAP(CInsertExpressionDialog, CDialog)
	//{{AFX_MSG_MAP(CInsertExpressionDialog)
	ON_CBN_SELCHANGE(IDC_COMBO_SAVED_EXPRESSION_GROUP, OnSelchangeComboSavedExpressionGroup)
	ON_CBN_SELCHANGE(IDC_COMBO_SAVED_EXPRESSIONS, OnSelchangeComboSavedExpressions)
	ON_BN_CLICKED(IDC_BUTTON_INSERT_EXPRESSION, OnButtonInsertExpression)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_EXPRESSION, OnButtonDeleteExpression)
	ON_BN_CLICKED(IDC_BUTTON_EXPORT_EXPRESSIONS, OnButtonExportExpressions)
	ON_BN_CLICKED(IDC_BUTTON_IMPORT_EXPRESSIONS, OnButtonImportExpressions)
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_DELETE_EXPRESSION, OnUpdateDeleteExpression)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_INSERT_EXPRESSION, OnUpdateInsertExpression)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInsertExpressionDialog message handlers

BOOL CInsertExpressionDialog::OnInitDialog()
{
	BaseClass::OnInitDialog();
	// load expressions
	LoadExpressions(m_Expressions);
	if (m_Expressions.size() <= 1)
	{
		// load default expressions
		CPathEx ProfileName;

		if (ProfileName.GetModuleFileName(NULL)
			&& ProfileName.RemoveFileSpec()
			&& ProfileName.Append(_T("Expressions.ini")))
		{
			LoadExpressions(m_Expressions, ProfileName);
			m_ExpressionsChanged = true;
		}
	}

	BuildExpressionGroupCombobox(m_ExpressionGroupSelected, m_ExpressionSelected);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInsertExpressionDialog::BuildExpressionGroupCombobox(unsigned nGroupSelected, unsigned nExprSelected)
{
	m_ExpressionGroupCombo.ResetContent();
	for (ExprGroupIterator ii = m_Expressions.begin()
		; ii < m_Expressions.end(); ii++)
	{
		m_ExpressionGroupCombo.AddString(ii->name);
	}

	if (nGroupSelected >= m_Expressions.size())
	{
		nGroupSelected = 0;
	}
	m_ExpressionGroupSelected = nGroupSelected;
	m_CurrExpressionGroupSelected = nGroupSelected;

	m_ExpressionGroupCombo.SetCurSel(m_ExpressionGroupSelected);
	LoadExpressionCombobox(nGroupSelected, nExprSelected);
}

void CInsertExpressionDialog::LoadExpressionCombobox(unsigned nGroupSelected, unsigned nExprSelected)
{
	m_ExpressionGroupSelected = nGroupSelected;
	m_CurrExpressionGroupSelected = nGroupSelected;

	m_SavedExpressionCombo.ResetContent();
	if (nGroupSelected >= m_Expressions.size())
	{
		m_CurrExpressionGroupSelected = -1;
		m_SavedExpressionCombo.SetCurSel(-1);
		OnSelchangeComboSavedExpressions();
		return;
	}
	for (ExprIterator jj = m_Expressions[nGroupSelected].exprs.begin()
		; jj < m_Expressions[nGroupSelected].exprs.end(); jj++)
	{
		m_SavedExpressionCombo.AddString(jj->name);
	}
	unsigned NumExpressions =  m_Expressions[nGroupSelected].exprs.size();
	if (nExprSelected >= NumExpressions)
	{
		nExprSelected = NumExpressions - 1;
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
	unsigned sel = m_SavedExpressionCombo.GetCurSel();

	ExprGroupIterator ii = m_Expressions.begin() + m_ExpressionGroupSelected;

	if (sel < ii->exprs.size())
	{
		CString s;

		s.Format(_T("%s - %s"), LPCTSTR(ii->exprs[sel].expr),
				LPCTSTR(ii->exprs[sel].comment));
		m_Description.SetWindowText(s);
	}
	else
	{
		m_Description.SetWindowText(_T(""));
	}
}

void CInsertExpressionDialog::OnButtonInsertExpression()
{
	CEdit * pEdit = (CEdit *) GetParent()->GetDlgItem(IDC_EDIT_EXPRESSION);

	if (NULL != pEdit)
	{
		unsigned sel = m_SavedExpressionCombo.GetCurSel();

		ExprGroupVector::iterator ii = m_Expressions.begin() + m_ExpressionGroupSelected;

		if (sel < ii->exprs.size())
		{
			pEdit->ReplaceSel(ii->exprs[sel].expr, TRUE);
			pEdit->SetFocus();
		}
	}
}

void CInsertExpressionDialog::OnButtonDeleteExpression()
{
	CString s;

	unsigned ExprSel = m_SavedExpressionCombo.GetCurSel();
	unsigned nGroup = m_ExpressionGroupSelected;

	if (nGroup >= m_Expressions.size()
		|| ExprSel >= m_Expressions[nGroup].exprs.size())
	{
		return;
	}

	ExprGroupIterator ii = m_Expressions.begin() + nGroup;
	if (0 == nGroup)
	{
		// All Expressions selected, find one in the group
		ii ++;
		for ( ; ii < m_Expressions.end(); ii++)
		{
			nGroup++;
			if (ExprSel < ii->exprs.size())
			{
				break;
			}
			ExprSel -= ii->exprs.size();
		}
		if (ii >= m_Expressions.end())
		{
			return;
		}
	}

	ExprIterator jj = ii->exprs.begin() + ExprSel;
	s.Format(IDS_DELETE_EXPRESSION, LPCTSTR(jj->name),
			LPCTSTR(ii->name));

	if (IDYES == AfxMessageBox(s, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2))
	{
		ii->exprs.erase(jj);
		if (ii->exprs.empty())
		{
			m_Expressions.erase(ii);
			RebuildAllExpressionsList();
			// rebuild combobox
			BuildExpressionGroupCombobox(m_CurrExpressionGroupSelected, 0);
		}
		else
		{
			RebuildAllExpressionsList();
			// rebuild combobox
			BuildExpressionGroupCombobox(m_CurrExpressionGroupSelected, ExprSel);
		}
	}
}

void CInsertExpressionDialog::RebuildAllExpressionsList()
{
	m_Expressions[0].exprs.clear();

	for (ExprGroupIterator ii = m_Expressions.begin() + 1
		; ii < m_Expressions.end(); ii++)
	{
		m_Expressions[0].exprs.insert(m_Expressions[0].exprs.end(),
									ii->exprs.begin(), ii->exprs.end());
	}
}

void CInsertExpressionDialog::SaveExpressionAs(const CString & expr)
{
	// query expression name and comment
	CSaveExpressionDialog dlg(m_Expressions);

	if (IDOK == dlg.DoModal()
		&& SaveExpression(expr, dlg.GetName(), dlg.GetGroupName(), dlg.GetComment(), false))
	{
		RebuildAllExpressionsList();
		int nGroup;
		int nExpr;
		if (FindExpression(dlg.GetGroupName(), dlg.GetName(), & nGroup, & nExpr))
		{
			BuildExpressionGroupCombobox(nGroup, nExpr);
		}
		UnloadExpressions();
	}
}

BOOL CInsertExpressionDialog::FindExpression(LPCTSTR Group, LPCTSTR Name, int * nGroup, int * nExpr)
{
	// find if there is such group
	for (ExprGroupIterator ii = m_Expressions.begin()
		; ii < m_Expressions.end(); ii++)
	{
		if (0 == ii->name.CompareNoCase(Group))
		{
			// check if there is already an expression with the same name
			for (ExprIterator jj = ii->exprs.begin()
				; jj < ii->exprs.end(); jj++)
			{
				if (0 == jj->name.CompareNoCase(Name))
				{
					*nGroup = ii - m_Expressions.begin();
					*nExpr = jj - ii->exprs.begin();
					return TRUE;
				}
			}
			return FALSE;
		}
	}
	return FALSE;
}

BOOL CInsertExpressionDialog::SaveExpression(const CString & expr,
											const CString & Name,
											const CString & Group, const CString & Comment, bool bEnableCancel)
{
	// find if there is such group
	ExprGroupIterator ii;
	for (ii = m_Expressions.begin()
		; ii < m_Expressions.end(); ii++)
	{
		if (0 == ii->name.CompareNoCase(Group))
		{
			break;
		}
	}
	if (ii == m_Expressions.end())
	{
		// add a new group
		ii = m_Expressions.insert(ii, ExprGroup()); // one item
		ii->name = Group;
	}
	// check if there is already an expression with the same name
	ExprIterator jj;
	for (jj = ii->exprs.begin(); jj < ii->exprs.end(); jj++)
	{
		if (0 == jj->name.CompareNoCase(Name))
		{
			int result = IDYES;
			// only query if the expression or comment changed
			if (0 != expr.Compare(jj->expr)
				|| 0 != Comment.Compare(jj->comment))
			{
				CString s;
				s.Format(IDS_REPLACE_EXPRESSION, LPCTSTR(jj->name),
						LPCTSTR(jj->expr), LPCTSTR(ii->name), LPCTSTR(expr));
				int flags;
				if (bEnableCancel)
				{
					flags = MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2;
				}
				else
				{
					flags = MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2;
				}
				result = AfxMessageBox(s, flags);
			}
			if (IDYES == result)
			{
				jj->expr = expr;
				jj->comment = Comment;
				return TRUE;
			}
			if (IDCANCEL == result)
			{
				return FALSE;
			}
			if (bEnableCancel)
			{
				// expression not saved, but go forward with others
				return TRUE;
			}
			return FALSE;
		}
	}
	jj = ii->exprs.insert(jj, Expr());
	jj->name = Name;
	jj->expr = expr;
	jj->comment = Comment;

	return TRUE;
}

void CInsertExpressionDialog::OnUpdateDeleteExpression(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_Expressions.size() > 1);
}

void CInsertExpressionDialog::OnUpdateInsertExpression(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_Expressions.size() > 1);
}

void CInsertExpressionDialog::LoadExpressions(ExprGroupVector & Expressions, LPCTSTR ProfileName)
{
	CApplicationProfile profile;
	if (NULL != ProfileName)
	{
		profile.m_pszProfileName = ProfileName;
	}
	int NumGroups = profile.GetProfileInt(_T("Expressions"), _T("NumOfGroups"), 0);
	Expressions.reserve(NumGroups + 1);
	Expressions.resize(1);
	Expressions[0].name.LoadString(IDS_ALL_EXPRESSIONS_GROUP);

	for (int i = 1; i < NumGroups + 1; i++)
	{
		ExprGroup egroup;
		CString s;
		s.Format(_T("ExprsInGroup%d"), i);

		int GroupSize = profile.GetProfileInt(_T("Expressions"), s, 0);
		s.Format(_T("GroupName%d"), i);
		egroup.name = profile.GetProfileString(_T("Expressions"), s, _T(""));

		if (egroup.name.IsEmpty())
		{
			continue;
		}
		for (int j = 0; j < GroupSize; j++)
		{
			Expr expr;
			s.Format(_T("Name%d.%d"), i, j + 1);
			expr.name = profile.GetProfileString(_T("Expressions"), s, _T(""));

			s.Format(_T("Expr%d.%d"), i, j + 1);
			expr.expr = profile.GetProfileString(_T("Expressions"), s, _T(""));

			s.Format(_T("Comment%d.%d"), i, j + 1);
			expr.comment = profile.GetProfileString(_T("Expressions"), s, _T(""));

			if ( ! expr.name.IsEmpty()
				&& ! expr.comment.IsEmpty())
			{
				egroup.exprs.push_back(expr);
			}
		}
		if ( ! egroup.exprs.empty())
		{
			Expressions.push_back(egroup);
			Expressions[0].exprs.insert(Expressions[0].exprs.end(),
										egroup.exprs.begin(), egroup.exprs.end());
		}
	}
}

void CInsertExpressionDialog::UnloadExpressions(LPCTSTR ProfileName)
{
	CString s;
	CApplicationProfile profile;
	if (NULL != ProfileName)
	{
		profile.m_pszProfileName = ProfileName;
	}
	profile.WriteProfileString(_T("Expressions"), NULL, NULL);    // delete key or section
	profile.WriteProfileInt(_T("Expressions"), _T("NumOfGroups"), m_Expressions.size() - 1);

	int i = 1;
	for (ExprGroupIterator ii = m_Expressions.begin() + 1
		; ii < m_Expressions.end(); ii++, i++)
	{
		s.Format(_T("ExprsInGroup%d"), i);
		profile.WriteProfileInt(_T("Expressions"), s, ii->exprs.size());

		s.Format(_T("GroupName%d"), i);
		profile.WriteProfileString(_T("Expressions"), s, ii->name);

		int j = 1;
		for (ExprIterator jj = ii->exprs.begin(); jj < ii->exprs.end(); j++, jj++)
		{
			s.Format(_T("Name%d.%d"), i, j);
			profile.WriteProfileString(_T("Expressions"), s, jj->name);

			s.Format(_T("Expr%d.%d"), i, j);
			profile.WriteProfileString(_T("Expressions"), s, jj->expr);

			s.Format(_T("Comment%d.%d"), i, j);
			profile.WriteProfileString(_T("Expressions"), s, jj->comment);
		}
	}
}

void CInsertExpressionDialog::OnButtonExportExpressions()
{
	CString FileName;
	CString Filter;
	Filter.LoadString(IDS_EXPRESSIONS_FILE_FILTER);

	CString Title;
	Title.LoadString(IDS_EXPRESSIONS_SAVE_TITLE);

	CFileDialogWithHistory dlg(FALSE,
								_T("Expr"), NULL,
								OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	FileName = dlg.GetPathName();
	UnloadExpressions(FileName);
}

void CInsertExpressionDialog::OnButtonImportExpressions()
{
	CString FileName;
	CString Filter;
	Filter.LoadString(IDS_EXPRESSIONS_FILE_FILTER);

	CString Title;
	Title.LoadString(IDS_EXPRESSIONS_LOAD_TITLE);

	CFileDialogWithHistory dlg(TRUE,
								_T("Expr"), NULL,
								OFN_HIDEREADONLY
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_FILEMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	if (IDOK != dlg.DoModal())
	{
		return;
	}
	FileName = dlg.GetPathName();
	ExprGroupVector vExpressions;
	LoadExpressions(vExpressions, FileName);
	// merge with m_Expressions
	for (ExprGroupIterator gr = vExpressions.begin() + 1
		; gr < vExpressions.end(); gr++)
	{
		for (ExprIterator jj = gr->exprs.begin(); jj < gr->exprs.end(); jj++)
		{
			if ( ! SaveExpression(jj->expr, jj->name, gr->name, jj->comment, true))
			{
				break;
			}
		}
	}
	RebuildAllExpressionsList();
	BuildExpressionGroupCombobox(0, 0);
	UnloadExpressions();
}
