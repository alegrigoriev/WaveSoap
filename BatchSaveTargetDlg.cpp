// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// BatchSaveTargetDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "WaveSoapFront.h"
#include "BatchSaveTargetDlg.h"
#include "FolderDialog.h"
#include "FileDialogWithHistory.h"
#include <afxpriv.h>
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBatchSaveTargetDlg dialog


CBatchSaveTargetDlg::CBatchSaveTargetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBatchSaveTargetDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBatchSaveTargetDlg)
	m_bMakeHtml = FALSE;
	m_bMakePlaylist = FALSE;
	m_bMakePlaylistOnly = FALSE;
	m_bNormalize = FALSE;
	m_sHtmlFile = _T("");
	m_sPlaylistFile = _T("");
	m_sSaveFolder = _T("");
	m_FileSaveType = -1;
	//}}AFX_DATA_INIT
	m_dNormalizeDb = 0;
	m_bNeedFolderToSave = FALSE;
	m_bNeedUpdateControls = TRUE;
}


void CBatchSaveTargetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBatchSaveTargetDlg)
	DDX_Control(pDX, IDC_CHECK_MAKE_PLAYLIST_ONLY, m_MakePlaylistOnly);
	DDX_Control(pDX, IDC_EDIT_SAVE_FOLDER, m_eSaveFolder);
	DDX_Control(pDX, IDC_EDIT_PLAYLIST, m_ePlaylistFile);
	DDX_Control(pDX, IDC_EDIT_NORMALIZE, m_eNormalizeDb);
	DDX_Control(pDX, IDC_EDIT_HTML, m_eHtmlFile);
	DDX_Control(pDX, IDC_STATIC_FORMAT_DETAILS, m_FormatStatic);
	DDX_Check(pDX, IDC_CHECK_MAKE_HTML, m_bMakeHtml);
	DDX_Check(pDX, IDC_CHECK_MAKE_PLAYLIST, m_bMakePlaylist);
	DDX_Check(pDX, IDC_CHECK_MAKE_PLAYLIST_ONLY, m_bMakePlaylistOnly);
	DDX_Check(pDX, IDC_CHECK_NORMALIZE, m_bNormalize);
	DDX_Text(pDX, IDC_EDIT_HTML, m_sHtmlFile);
	DDX_Text(pDX, IDC_EDIT_PLAYLIST, m_sPlaylistFile);
	DDX_Text(pDX, IDC_EDIT_SAVE_FOLDER, m_sSaveFolder);
	DDX_Radio(pDX, IDC_RADIO_SAVE_TYPE, m_FileSaveType);
	//}}AFX_DATA_MAP
	m_eNormalizeDb.ExchangeData(pDX, m_dNormalizeDb,
								IDS_INPUT_NAME_NORMALIZE_LEVEL, IDS_DECIBEL, -20., 0.);
	if (pDX->m_bSaveAndValidate)
	{
		// check that folder exists and accessible
		m_Profile.UnloadAll();
	}
}


BEGIN_MESSAGE_MAP(CBatchSaveTargetDlg, CDialog)
	//{{AFX_MSG_MAP(CBatchSaveTargetDlg)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_DST_FOLDER, OnButtonBrowseDstFolder)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_PLAYLIST, OnButtonBrowsePlaylist)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_WEBPAGE, OnButtonBrowseWebpage)
	ON_BN_CLICKED(IDC_BUTTON_FORMAT, OnButtonFormat)
	ON_BN_CLICKED(IDC_CHECK_MAKE_HTML, OnCheckMakeHtml)
	ON_BN_CLICKED(IDC_CHECK_MAKE_PLAYLIST, OnCheckMakePlaylist)
	ON_BN_CLICKED(IDC_CHECK_MAKE_PLAYLIST_ONLY, OnCheckMakePlaylistOnly)
	ON_BN_CLICKED(IDC_CHECK_NORMALIZE, OnCheckNormalize)
	ON_EN_CHANGE(IDC_EDIT_HTML, OnChangeEditHtml)
	ON_EN_CHANGE(IDC_EDIT_NORMALIZE, OnChangeEditNormalize)
	ON_EN_CHANGE(IDC_EDIT_PLAYLIST, OnChangeEditPlaylist)
	ON_EN_CHANGE(IDC_EDIT_SAVE_FOLDER, OnChangeEditSaveFolder)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_BROWSE_DST_FOLDER, OnUpdateButtonBrowseDstFolder)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOk)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_BROWSE_PLAYLIST, OnUpdateButtonBrowsePlaylist)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_BROWSE_WEBPAGE, OnUpdateButtonBrowseWebpage)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_FORMAT, OnUpdateButtonFormat)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_NORMALIZE, OnUpdateCheckNormalize)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_SAVE_FOLDER, OnUpdateSaveFolder)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_PLAYLIST, OnUpdatePlaylistFile)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_NORMALIZE, OnUpdateNormalizeDb)
	ON_UPDATE_COMMAND_UI(IDC_STATIC_NORMALIZE_DB, OnUpdateNormalizeDb)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_HTML, OnUpdateHtmlFile)
	ON_UPDATE_COMMAND_UI(IDC_STATIC_SAVE_TYPE, OnUpdateRadioFormat)
	ON_UPDATE_COMMAND_UI(IDC_RADIO_SAVE_TYPE, OnUpdateRadioFormat)
	ON_UPDATE_COMMAND_UI(IDC_RADIO_MP3, OnUpdateRadioFormat)
	ON_UPDATE_COMMAND_UI(IDC_RADIO_WMA, OnUpdateRadioFormat)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBatchSaveTargetDlg message handlers
LRESULT CBatchSaveTargetDlg::OnKickIdle(WPARAM, LPARAM)
{
	if (m_bNeedUpdateControls)
	{
		UpdateDialogControls(this, FALSE);
	}
	m_bNeedUpdateControls = FALSE;
	return 0;
}


void CBatchSaveTargetDlg::OnButtonBrowseDstFolder()
{
	m_eSaveFolder.GetWindowText(m_sSaveFolder);
	CFolderDialog dlg(IDS_SELECT_SAVE_TO_FOLDER,
					m_sSaveFolder, TRUE);
	if (IDOK == dlg.DoModal())
	{
		// TODO: check permissiong in callback
		m_sSaveFolder = dlg.GetFolderPath();
		m_eSaveFolder.SetWindowText(m_sSaveFolder);
		// TODO: check if the folder exists and create if necessary
	}
}

void CBatchSaveTargetDlg::OnButtonBrowsePlaylist()
{
	CString FileName;
	CString Filter;
	Filter.LoadString(IDS_PLAYLIST_FILE_FILTER);

	CString Title;
	Title.LoadString(IDS_PLAYLIST_SAVE_TITLE);

	m_ePlaylistFile.GetWindowText(m_sPlaylistFile);
	if (m_sPlaylistFile.IsEmpty())
	{
		m_sPlaylistFile = _T("album.m3u");
	}

	CFileDialogWithHistory dlg(FALSE,
								_T("m3u"), m_sPlaylistFile,
								OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	m_eSaveFolder.GetWindowText(m_sSaveFolder);
	dlg.m_ofn.lpstrInitialDir = m_sSaveFolder;

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	m_sPlaylistFile = dlg.GetPathName();
	// if folder is the same as save folder, remove it from the file name
	if (0 == _tcsnicmp(m_sSaveFolder, m_sPlaylistFile, m_sSaveFolder.GetLength()))
	{
		int pos = m_sSaveFolder.GetLength();
		if (m_sPlaylistFile.GetLength() > pos
			&& ('\\' == m_sPlaylistFile[pos]
				|| '/' == m_sPlaylistFile[pos]))
		{
			pos++;
		}
		m_sPlaylistFile = CString(pos + LPCTSTR(m_sPlaylistFile));
	}
	m_ePlaylistFile.SetWindowText(m_sPlaylistFile);
}

void CBatchSaveTargetDlg::OnButtonBrowseWebpage()
{
	CString FileName;
	CString Filter;
	Filter.LoadString(IDS_HTML_FILE_FILTER);

	CString Title;
	Title.LoadString(IDS_HTML_SAVE_TITLE);

	m_eHtmlFile.GetWindowText(m_sHtmlFile);
	if (m_sHtmlFile.IsEmpty())
	{
		m_sHtmlFile = _T("album.htm");
	}

	CFileDialogWithHistory dlg(FALSE,
								_T("htm"), m_sPlaylistFile,
								OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT
								| OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST,
								Filter);
	dlg.m_ofn.lpstrTitle = Title;

	m_eSaveFolder.GetWindowText(m_sSaveFolder);
	dlg.m_ofn.lpstrInitialDir = m_sSaveFolder;

	if (IDOK != dlg.DoModal())
	{
		return;
	}

	m_sHtmlFile = dlg.GetPathName();
	// if folder is the same as save folder, remove it from the file name
	if (0 == _tcsnicmp(m_sSaveFolder, m_sHtmlFile, m_sSaveFolder.GetLength()))
	{
		int pos = m_sSaveFolder.GetLength();
		if (m_sHtmlFile.GetLength() > pos
			&& ('\\' == m_sHtmlFile[pos]
				|| '/' == m_sHtmlFile[pos]))
		{
			pos++;
		}
		m_sHtmlFile = CString(pos + LPCTSTR(m_sHtmlFile));
	}
	m_eHtmlFile.SetWindowText(m_sHtmlFile);
}

void CBatchSaveTargetDlg::OnButtonFormat()
{
	// TODO: Add your control notification handler code here

}

void CBatchSaveTargetDlg::OnCheckMakeHtml()
{
	m_bMakeHtml = IsDlgButtonChecked(IDC_CHECK_MAKE_HTML);
	m_bNeedUpdateControls = TRUE;
}

void CBatchSaveTargetDlg::OnCheckMakePlaylist()
{
	m_bMakePlaylist = IsDlgButtonChecked(IDC_CHECK_MAKE_PLAYLIST);
	m_bNeedUpdateControls = TRUE;
}

void CBatchSaveTargetDlg::OnCheckMakePlaylistOnly()
{
	m_bMakePlaylistOnly = 1 == IsDlgButtonChecked(IDC_CHECK_MAKE_PLAYLIST_ONLY);
	m_bNeedUpdateControls = TRUE;
}

void CBatchSaveTargetDlg::OnCheckNormalize()
{
	m_bNormalize = 1 == IsDlgButtonChecked(IDC_CHECK_NORMALIZE);
	m_bNeedUpdateControls = TRUE;
}

void CBatchSaveTargetDlg::OnUpdateSaveFolder(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bMakePlaylistOnly);
}
void CBatchSaveTargetDlg::OnUpdateButtonBrowseDstFolder(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bMakePlaylistOnly);
}

void CBatchSaveTargetDlg::OnUpdatePlaylistFile(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bMakePlaylist);
}
void CBatchSaveTargetDlg::OnUpdateButtonBrowsePlaylist(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bMakePlaylist);
}

void CBatchSaveTargetDlg::OnUpdateHtmlFile(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bMakeHtml);
}
void CBatchSaveTargetDlg::OnUpdateButtonBrowseWebpage(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_bMakeHtml);
}

void CBatchSaveTargetDlg::OnUpdateButtonFormat(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bMakePlaylistOnly);
}
void CBatchSaveTargetDlg::OnUpdateCheckNormalize(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bMakePlaylistOnly);
}

void CBatchSaveTargetDlg::OnUpdateNormalizeDb(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bMakePlaylistOnly
					&& m_bNormalize);
}

void CBatchSaveTargetDlg::OnUpdateFormatStatic(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bMakePlaylistOnly);
}

void CBatchSaveTargetDlg::OnUpdateRadioFormat(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( ! m_bMakePlaylistOnly);
}

void CBatchSaveTargetDlg::OnUpdateOk(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(
					(! m_bMakeHtml || 0 != m_eHtmlFile.GetWindowTextLength())
					&& (! m_bMakePlaylist || 0 != m_ePlaylistFile.GetWindowTextLength())
					&& (m_bMakePlaylistOnly
						|| (0 != m_eSaveFolder.GetWindowTextLength()
							&& (! m_bNormalize || 0 != m_eNormalizeDb.GetWindowTextLength()))));
}


void CBatchSaveTargetDlg::OnChangeEditHtml()
{
	m_bNeedUpdateControls = TRUE;
}

void CBatchSaveTargetDlg::OnChangeEditNormalize()
{
	m_bNeedUpdateControls = TRUE;
}

void CBatchSaveTargetDlg::OnChangeEditPlaylist()
{
	m_bNeedUpdateControls = TRUE;
}

void CBatchSaveTargetDlg::OnChangeEditSaveFolder()
{
	m_bNeedUpdateControls = TRUE;
}


BOOL CBatchSaveTargetDlg::OnInitDialog()
{
	m_Profile.AddItem(_T("Settings"), _T("BatchSaveFolder"), m_sSaveFolder);
	m_Profile.AddItem(_T("Settings"), _T("BatchPlaylistFile"), m_sPlaylistFile);
	m_Profile.AddItem(_T("Settings"), _T("BatchHtmlFile"), m_sHtmlFile);

	m_Profile.AddBoolItem(_T("Settings"), _T("BatchMakeHtml"), m_bMakeHtml);
	m_Profile.AddBoolItem(_T("Settings"), _T("BatchMakePlaylist"), m_bMakePlaylist);
	m_Profile.AddBoolItem(_T("Settings"), _T("BatchNormalize"), m_bNormalize);
	m_Profile.AddItem(_T("Settings"), _T("BatchNormalizeLevel"),
					m_dNormalizeDb, 0., -20., 0.);
	if ( ! m_bNeedFolderToSave)
	{
		m_Profile.AddBoolItem(_T("Settings"), _T("BatchMakePlaylistOnly"), m_bMakePlaylistOnly);
	}
	else
	{
		m_bMakePlaylistOnly = FALSE;
	}

	CDialog::OnInitDialog();

	if (m_bNeedFolderToSave)
	{
		m_MakePlaylistOnly.EnableWindow(FALSE);
		m_MakePlaylistOnly.ShowWindow(SW_HIDE);
	}

	UpdateDialogControls(this, FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
