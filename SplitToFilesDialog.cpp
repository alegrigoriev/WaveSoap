// SplitToFilesDialog.cpp : implementation file
//

#include "stdafx.h"
//#include "WaveSoapFront.h"
#include "SplitToFilesDialog.h"
#include "WaveFile.h"
#include "TimeToStr.h"
#include "FolderDialog.h"
#include "BladeMP3EncDLL.h"
#include ".\splittofilesdialog.h"

enum
{
	ListNameColumn,
	ListBeginColumn,
	ListEndColumn,
	ListLengthColumn,
};

// CSplitToFilesDialog dialog

CSplitToFilesDialog::CSplitToFilesDialog(CWaveFile & WaveFile, CWnd* pParent /*=NULL*/)
	: BaseClass(CSplitToFilesDialog::IDD, pParent)
	, CFileSaveUiSupport(WaveFile.GetWaveFormat())
	, m_WaveFile(WaveFile)
{
	// fill the files list
	WaveFile.GetSortedFileSegments(m_Files);
}

CSplitToFilesDialog::~CSplitToFilesDialog()
{
}

void CSplitToFilesDialog::ShowDlgItem(UINT nID, int nCmdShow)
{
	CWnd * pWnd = GetDlgItem(nID);
	if (pWnd)
	{
		pWnd->ShowWindow(nCmdShow);
	}
}

void CSplitToFilesDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST_FILES, m_FilesList);
	DDX_Control(pDX, IDC_COMBO_FILE_TYPE, m_SaveAsTypesCombo);
	DDX_Control(pDX, IDC_COMBO_SAVE_DIR, m_eSaveToFolder);

	DDX_Control(pDX, IDC_COMBO_FORMAT_TAG, m_FormatTagCombo);
	DDX_Control(pDX, IDC_COMBO_FORMAT_ATTRIBUTES, m_AttributesCombo);

	DDX_Check(pDX, IDC_CHECK_COMPATIBLE_FORMATS, m_bCompatibleFormatsOnly);
}


BEGIN_MESSAGE_MAP(CSplitToFilesDialog, BaseClass)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_FOLDER, OnBnClickedButtonBrowseFolder)
	ON_BN_CLICKED(IDC_CHECK_COMPATIBLE_FORMATS, OnCompatibleFormatsClicked)
	ON_CBN_SELCHANGE(IDC_COMBO_FORMAT_TAG, OnComboFormatsChange)
	ON_CBN_SELCHANGE(IDC_COMBO_FORMAT_ATTRIBUTES, OnComboAttributesChange)
//    ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST_FILES, OnLvnBeginlabeleditListFiles)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST_FILES, OnLvnEndlabeleditListFiles)
END_MESSAGE_MAP()

void CSplitToFilesDialog::OnCompatibleFormatsClicked()
{
	m_bCompatibleFormatsOnly = ((CButton*)GetDlgItem(IDC_CHECK_COMPATIBLE_FORMATS))->GetCheck();
	CFileSaveUiSupport::OnCompatibleFormatsClicked();
}

void CSplitToFilesDialog::OnComboFormatsChange()
{
	CFileSaveUiSupport::OnComboFormatsChange();
}

void CSplitToFilesDialog::OnComboAttributesChange()
{
	CFileSaveUiSupport::OnComboAttributesChange();
}

// CSplitToFilesDialog message handlers

void CSplitToFilesDialog::OnBnClickedButtonBrowseFolder()
{
	// TODO: Add your control notification handler code here
	m_eSaveToFolder.GetWindowText(m_sSaveToFolder);

	CFolderDialog dlg(IDS_SELECT_SAVE_TO_FOLDER,
					m_sSaveToFolder, TRUE);

	if (IDOK == dlg.DoModal())
	{
		// TODO: check permissiong in callback
		m_sSaveToFolder = dlg.GetFolderPath();

		m_eSaveToFolder.SetWindowText(m_sSaveToFolder);
		// TODO: check if the folder exists and create if necessary
	}
}

void CSplitToFilesDialog::OnLvnBeginlabeleditListFiles(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CSplitToFilesDialog::OnLvnEndlabeleditListFiles(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	// check that the label is not blank
	if (unsigned(pDispInfo->item.iItem) < m_Files.size())
	{
		CString s = pDispInfo->item.pszText;
		s.Trim();

		// check for invalid characters (0-0x1F, 0x7F, '/', ';', '"', '\\' - when saving it
		m_Files[pDispInfo->item.iItem].Name = s;
	}

	*pResult = 0;
}

void CSplitToFilesDialog::SetFileType(int nType)
{
	if (m_FileType == nType)
	{
		return;
	}
	// Wav, Mp3, wma, raw...
	CString s;
	m_FileType = nType;
	switch (nType)
	{
	default:
	case SoundFileWav:
		// WAV file
		// show Comments, fill formats combo box
		s.LoadString(IDS_FORMAT);
		SetDlgItemText(IDC_STATIC_FORMAT, s);

		ShowDlgItem(IDC_STATIC_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT_TAG, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_STATIC_FORMAT_ATTRIBUTES, SW_SHOWNOACTIVATE);

		FillFormatTagCombo(ExcludeFormats, -1, WaveFormatExcludeFormats);

		m_SelectedFormat = FillFormatCombo(m_FormatTagCombo.GetCurSel());
		break;  // go on
	case SoundFileMp3:
		// MP3 file
		// replace Format with Encoder: (LAME, Fraunhofer
		s.LoadString(IDS_ENCODER);
		SetDlgItemText(IDC_STATIC_FORMAT, s);
		ShowDlgItem(IDC_STATIC_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT_TAG, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_STATIC_FORMAT_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_SHOWNOACTIVATE);
		// Hide Comments, show Artist, Genre, Title
		//
		// set tag to MP3
		// fill formats combo (bitrate, etc)
		FillMp3EncoderCombo();
		break;
	case SoundFileWma:
		// WMA file
		if ( ! GetApp()->CanOpenWindowsMedia())
		{
			int id = AfxMessageBox(IDS_WMA_ENCODER_NOT_AVILABLE, MB_OK | MB_ICONEXCLAMATION | MB_HELP);
			if (IDHELP == id)
			{
				// TODO: show help
			}
			SetFileType(SoundFileWav);
			break;
		}

		// remove Format: combo
		ShowDlgItem(IDC_STATIC_FORMAT, SW_HIDE);
		ShowDlgItem(IDC_COMBO_FORMAT_TAG, SW_HIDE);
		ShowDlgItem(IDC_COMBO_FORMAT_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_STATIC_FORMAT_ATTRIBUTES, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_SHOWNOACTIVATE);
		// fill profiles combo
		FillWmaFormatCombo();
		break;
		//case SoundFileAvi:
		// AVI file
		break;
	case SoundFileRaw:
		// RAW file
		// show formats: 16 bits LSB first, 16 bits MSB first, a-law, u-law, 8 bits, ascii hex, ascii decimal
		// hide attributes
		s.LoadString(IDS_FORMAT);
		SetDlgItemText(IDC_STATIC_FORMAT, s);

		FillRawFormatsCombo();

		ShowDlgItem(IDC_STATIC_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT_TAG, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT_ATTRIBUTES, SW_HIDE);
		ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_HIDE);
		ShowDlgItem(IDC_STATIC_FORMAT_ATTRIBUTES, SW_HIDE);

		break;
	}
}

BOOL CSplitToFilesDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (m_SelectedTag.Tag == WAVE_FORMAT_MSAUDIO1
		|| m_SelectedTag.Tag == WAVE_FORMAT_MSAUDIO1 + 1)
	{
		m_SelectedWmaBitrate = m_Wf.BytesPerSec() * 8;
		m_SelectedBitrate = m_SelectedWmaBitrate;
	}
	else if (m_SelectedTag.Tag == WAVE_FORMAT_MPEGLAYER3
			|| m_SelectedTag == BladeMp3Encoder::GetTag())
	{
		m_SelectedMp3Bitrate = m_Wf.BytesPerSec() * 8;
		m_SelectedBitrate = m_SelectedMp3Bitrate;
	}
	else
	{
		m_SelectedBitrate = 128000;
	}

	m_FilesList.SetExtendedStyle(m_FilesList.GetExtendedStyle()
								| LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP
								| LVS_EX_INFOTIP);
	// add columns
	int ColumnWidth = m_FilesList.GetStringWidth(_T(" 00:00:00.000 "));

	CRect r;
	m_FilesList.GetClientRect(r);

	m_FilesList.InsertColumn(ListNameColumn, LoadCString(IDS_FILE_LIST_NAME_COLUMN),
							ListNameColumn, r.Width() - 3 - 3 * ColumnWidth);

	m_FilesList.InsertColumn(ListBeginColumn, LoadCString(IDS_FILE_LIST_BEGIN_COLUMN),
							LVCFMT_LEFT, ColumnWidth, ListBeginColumn);

	m_FilesList.InsertColumn(ListEndColumn, LoadCString(IDS_FILE_LIST_END_COLUMN),
							LVCFMT_LEFT, ColumnWidth, ListEndColumn);

	m_FilesList.InsertColumn(ListLengthColumn, LoadCString(IDS_FILE_LIST_LENGTH_COLUMN),
							LVCFMT_LEFT, ColumnWidth, ListLengthColumn);

	int ItemIdx = 0;
	CString s;
	for (WaveFileSegmentVector::const_iterator i = m_Files.begin();
		i < m_Files.end(); i++, ItemIdx++)
	{
		LVITEM item;
		memzero(item);

		item.mask = LVIF_TEXT;
		item.iItem = ItemIdx;
		item.iSubItem = ListNameColumn;
		item.pszText = (LPTSTR)LPCTSTR(i->Name);

		m_FilesList.InsertItem( & item);

		item.iSubItem = ListBeginColumn;
		s = SampleToString(i->Begin, m_WaveFile.SampleRate());
		item.pszText = (LPTSTR)LPCTSTR(s);

		m_FilesList.SetItem( & item);

		item.iSubItem = ListEndColumn;
		s = SampleToString(i->End, m_WaveFile.SampleRate());
		item.pszText = (LPTSTR)LPCTSTR(s);

		m_FilesList.SetItem( & item);

		item.iSubItem = ListLengthColumn;
		s = SampleToString(i->End - i->Begin, m_WaveFile.SampleRate());
		item.pszText = (LPTSTR)LPCTSTR(s);

		m_FilesList.SetItem( & item);
		//m_FilesList.SetItem(ItemIdx, ListBeginColumn,  & item);
	}

	SetFileType(SoundFileWav); // TODO: select last type

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
