// UndoRedoOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "UndoRedoOptionsDlg.h"
#include ".\undoredooptionsdlg.h"
#include "WaveSoapFrontDoc.h"

// CUndoRedoOptionsDlg dialog

IMPLEMENT_DYNAMIC(CUndoRedoOptionsDlg, CDialog)

CUndoRedoOptionsDlg::CUndoRedoOptionsDlg(CWaveSoapFrontDoc * pDoc, CWnd* pParent /*=NULL*/)
	: CDialog(IDD, pParent)
	, UndoRedoParameters(*pDoc->GetUndoParameters())
	, m_pDoc(pDoc)
{
}

CUndoRedoOptionsDlg::~CUndoRedoOptionsDlg()
{
}

void CUndoRedoOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_UNDO_STATUS, m_UndoStatus);
	DDX_Control(pDX, IDC_STATIC_REDO_STATUS, m_RedoStatus);

	DDX_Check(pDX, IDC_CHECK_ENABLE_UNDO, m_UndoEnabled);
	DDX_Check(pDX, IDC_CHECK_ENABLE_REDO, m_RedoEnabled);
	DDX_Check(pDX, IDC_CHECK_LIMIT_UNDO_DEPTH, m_LimitUndoDepth);
	DDX_Check(pDX, IDC_CHECK_LIMIT_UNDO_SIZE, m_LimitUndoSize);
	DDX_Text(pDX, IDC_EDIT_UNDO_SIZE_LIMIT, m_UndoSizeLimit);
	DDV_MinMaxUInt(pDX, m_UndoSizeLimit, 0, 4096);
	DDX_Text(pDX, IDC_EDIT_UNDO_DEPTH_LIMIT, m_UndoDepthLimit);
	DDV_MinMaxUInt(pDX, m_UndoDepthLimit, 0, 1000);

	DDX_Check(pDX, IDC_CHECK_LIMIT_REDO_DEPTH, m_LimitRedoDepth);
	DDX_Check(pDX, IDC_CHECK_LIMIT_REDO_SIZE, m_LimitRedoSize);
	DDX_Text(pDX, IDC_EDIT_REDO_SIZE_LIMIT, m_RedoSizeLimit);
	DDV_MinMaxUInt(pDX, m_RedoSizeLimit, 0, 4096);
	DDX_Text(pDX, IDC_EDIT_REDO_DEPTH_LIMIT, m_RedoDepthLimit);
	DDV_MinMaxUInt(pDX, m_RedoDepthLimit, 0, 1000);

	DDX_Check(pDX, IDC_CHECK_REMEMBER_SELECTION_IN_UNDO, m_RememberSelectionInUndo);

	DDX_Control(pDX, IDC_SPIN_UNDO_DEPTH, m_UndoDepthSpin);
	DDX_Control(pDX, IDC_SPIN_REDO_DEPTH, m_RedoDepthSpin);
}


BEGIN_MESSAGE_MAP(CUndoRedoOptionsDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR_REDO, OnBnClickedButtonClearRedo)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR_UNDO, OnBnClickedButtonClearUndo)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_REDO, OnBnClickedCheckEnableRedo)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_UNDO, OnBnClickedCheckEnableUndo)
	ON_BN_CLICKED(IDC_CHECK_LIMIT_REDO_DEPTH, OnBnClickedCheckLimitRedoDepth)
	ON_BN_CLICKED(IDC_CHECK_LIMIT_REDO_SIZE, OnBnClickedCheckLimitRedoSize)
	ON_BN_CLICKED(IDC_CHECK_LIMIT_UNDO_DEPTH, OnBnClickedCheckLimitUndoDepth)
	ON_BN_CLICKED(IDC_CHECK_LIMIT_UNDO_SIZE, OnBnClickedCheckLimitUndoSize)
END_MESSAGE_MAP()


// CUndoRedoOptionsDlg message handlers

void CUndoRedoOptionsDlg::OnBnClickedButtonClearUndo()
{
	m_pDoc->ClearUndo();
	SetUndoRedoStatusText();
}

void CUndoRedoOptionsDlg::OnBnClickedButtonClearRedo()
{
	m_pDoc->ClearRedo();
	SetUndoRedoStatusText();
}

void CUndoRedoOptionsDlg::OnBnClickedCheckEnableRedo()
{
	// TODO: Add your control notification handler code here
}

void CUndoRedoOptionsDlg::OnBnClickedCheckEnableUndo()
{
	// TODO: Add your control notification handler code here
}

void CUndoRedoOptionsDlg::OnBnClickedCheckLimitRedoDepth()
{
	// TODO: Add your control notification handler code here
}

void CUndoRedoOptionsDlg::OnBnClickedCheckLimitRedoSize()
{
	// TODO: Add your control notification handler code here
}

void CUndoRedoOptionsDlg::OnBnClickedCheckLimitUndoDepth()
{
	// TODO: Add your control notification handler code here
}

void CUndoRedoOptionsDlg::OnBnClickedCheckLimitUndoSize()
{
	// TODO: Add your control notification handler code here
}

BOOL CUndoRedoOptionsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_UndoDepthSpin.SetRange(0, 1000);
	m_RedoDepthSpin.SetRange(0, 1000);
	m_UndoStatus.GetWindowText(m_UndoStatusFormat);
	m_RedoStatus.GetWindowText(m_RedoStatusFormat);

	SetUndoRedoStatusText();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CUndoRedoOptionsDlg::SetUndoRedoStatusText()
{
	CString s;
	s.Format(m_UndoStatusFormat,
			m_pDoc->GetUndoDepth(), ULONG(m_pDoc->GetUndoSize() / 0x100000U));
	m_UndoStatus.SetWindowText(s);

	s.Format(m_RedoStatusFormat,
			m_pDoc->GetRedoDepth(), ULONG(m_pDoc->GetRedoSize() / 0x100000U));
	m_RedoStatus.SetWindowText(s);
}
