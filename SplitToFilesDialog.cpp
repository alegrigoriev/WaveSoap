// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// SplitToFilesDialog.cpp : implementation file
//
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "WaveSoapFront.h"
#include "SplitToFilesDialog.h"
#include "WaveFile.h"
#include "TimeToStr.h"
#include "FolderDialog.h"
#include "BladeMP3EncDLL.h"
#include "WaveSoapFrontDoc.h"
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
	, m_FileTypeFlags(0)
	, m_RecentFolders(& m_Profile, _T("RecentOpenDirs"), _T("Dir%d"), 15)
	, m_RecentFilenamePrefixes(& m_Profile, _T("RecentFilenamePrefixes"), _T("Prefix%d"), 15,
								CStringHistory::CaseSensitive)
{
	// fill the files list
	WaveFile.GetSortedFileSegments(m_Files);
	m_Profile.AddItem(_T("Settings"), _T("SplitToFileType"), m_FileType, SoundFileWav, SoundFileWav, SoundFileWma);
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
	DDX_Text(pDX, IDC_COMBO_SAVE_DIR, m_sSaveToFolder);

	DDX_Control(pDX, IDC_COMBO_FILENAME_PREFIX, m_eFilenamePrefix);
	DDX_Text(pDX, IDC_COMBO_FILENAME_PREFIX, m_sFilenamePrefix);

	DDX_Control(pDX, IDC_COMBO_FORMAT_TAG, m_FormatTagCombo);
	DDX_Control(pDX, IDC_COMBO_FORMAT_ATTRIBUTES, m_AttributesCombo);

	DDX_Check(pDX, IDC_CHECK_COMPATIBLE_FORMATS, m_bCompatibleFormatsOnly);


	if (pDX->m_bSaveAndValidate)
	{
		switch (m_FileType)
		{
		case SoundFileMp3:
			m_FileTypeFlags = SaveFile_Mp3File;
			break;
		case SoundFileWma:
			m_FileTypeFlags = SaveFile_WmaFile;
			break;
		default:
			m_FileTypeFlags = SaveFile_WavFile;
			break;
		}

		m_sFilenamePrefix.TrimLeft();
		m_RecentFilenamePrefixes.AddString(m_sFilenamePrefix);


		// validate the folder
		m_sSaveToFolder.TrimLeft();
		m_sSaveToFolder.TrimRight();

		if ( ! VerifyCreateDirectory(m_sSaveToFolder))
		{
			pDX->PrepareEditCtrl(IDC_COMBO_SAVE_DIR);
			pDX->Fail();
		}
		// create valid file names, ask for file replace
		if (! m_sSaveToFolder.IsEmpty())
		{
			m_RecentFolders.AddString(m_sSaveToFolder);

			TCHAR c = m_sSaveToFolder[m_sSaveToFolder.GetLength() - 1];
			if (c != '\\'
				&& c != '/')
			{
				m_sSaveToFolder += '\\';
			}
		}

		// validate the files. Check that the files can be created
		unsigned FileIndex;
		WaveFileSegmentVector::iterator t;

		for (FileIndex = 1, t = m_Files.begin(); t < m_Files.end(); t++, FileIndex++)
		{
			CString Name;
			// add the prefix if any
			// The prefix may contain %d escape sequence
			// if the sequence is %nnd, then the numbering starts from n
			for (int i = 0; i < m_sFilenamePrefix.GetLength(); i++)
			{
				TCHAR c = m_sFilenamePrefix[i];
				switch (c)
				{
				case '%':
					if (m_sFilenamePrefix.GetLength() > i + 1)
					{
						if ('%' == m_sFilenamePrefix[i + 1])
						{
							// double percent escape - translated to a single percent
							i++;
						}
						else if ('d' == m_sFilenamePrefix[i + 1])
						{
							CString s;
							s.Format(_T("%02d"), FileIndex + 1);

							Name += s;
							i++;

							continue;
						}
					}
					break;
				default:
					break;
				}
				Name += c;
			}


			Name += t->Name;
			LPTSTR pName = Name.GetBuffer(0);
			while (*pName != 0)
			{
				TCHAR c= *pName;
				if ('\\' == c
					|| '/' == c
					|| '"' == c
					|| '?' == c
					|| '*' == c
					|| ':' == c
					//|| ';' == c
					//|| ',' == c
					|| '#' == c
					//|| '&' == c
					|| '%' == c)
				{
					*pName = '-';
				}

				pName++;
			}
			Name.ReleaseBuffer();

			switch (m_FileType)
			{
			case SoundFileMp3:
				Name += _T(".mp3");
				break;
			case SoundFileWma:
				Name += _T(".wma");
				break;
			default:
				Name += _T(".wav");
				break;
			}

			t->FullFilename = m_sSaveToFolder + Name;

			// check for existing file, ask for replacement!
			SetLastError(0);
			ULONG AccessMask = GENERIC_READ | GENERIC_WRITE | DELETE;
			HANDLE hFile = CreateFile(t->FullFilename,
									AccessMask, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (NULL == hFile || INVALID_HANDLE_VALUE == hFile)
			{
				DWORD error = GetLastError();
				int id = IDS_UNKNOWN_FILE_CREATION_ERROR;
				switch (error)
				{
				case ERROR_FILE_NOT_FOUND:
					// see if we can create a new file
					hFile = CreateFile(t->FullFilename,
										AccessMask,
										0, NULL, CREATE_NEW,
										FILE_FLAG_DELETE_ON_CLOSE, NULL);

					if (NULL == hFile || INVALID_HANDLE_VALUE == hFile)
					{
						id = IDS_OVERWRITE_ACCESS_DENIED;
						break;
					}
					else
					{
						CloseHandle(hFile);
					}
					continue;

				case ERROR_ACCESS_DENIED:
				case ERROR_FILE_READ_ONLY:
					id = IDS_OVERWRITE_ACCESS_DENIED;
					break;

				case ERROR_SHARING_VIOLATION:
					id = IDS_OVERWRITE_SHARING_VIOLATION;
					break;
				}

				CString s;

				s.Format(id, LPCTSTR(t->FullFilename));
				AfxMessageBox(s, MB_OK | MB_ICONEXCLAMATION);

				pDX->PrepareCtrl(IDC_LIST_FILES);
				pDX->Fail();    // throws exception
			}
			else
			{
				CloseHandle(hFile);
				// file already exists
				CString s;
				s.Format(IDS_REPLACEYESNO, LPCTSTR(t->FullFilename));
				if (IDYES != AfxMessageBox(s, MB_YESNO | MB_ICONEXCLAMATION))
				{
					pDX->PrepareCtrl(IDC_LIST_FILES);
					pDX->Fail();    // throws exception
				}
			}
		}
	}
}

bool CSplitToFilesDialog::GetFileData(unsigned index, CString & FileName, CString & Name,
									SAMPLE_INDEX * pBegin, SAMPLE_INDEX * pEnd) const
{
	ASSERT(pBegin != NULL);
	ASSERT(pEnd != NULL);

	if (index >= m_Files.size())
	{
		return false;
	}

	WaveFileSegmentVector::const_reference f = m_Files[index];

	FileName = f.FullFilename;
	Name = f.Name;
	*pBegin = f.Begin;
	*pEnd = f.End;

	return true;
}

BEGIN_MESSAGE_MAP(CSplitToFilesDialog, BaseClass)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_FOLDER, OnBnClickedButtonBrowseFolder)
	ON_BN_CLICKED(IDC_CHECK_COMPATIBLE_FORMATS, OnCompatibleFormatsClicked)
	ON_CBN_SELCHANGE(IDC_COMBO_FORMAT_TAG, OnComboFormatsChange)
	ON_CBN_SELCHANGE(IDC_COMBO_FORMAT_ATTRIBUTES, OnComboAttributesChange)
	ON_CBN_SELCHANGE(IDC_COMBO_FILE_TYPE, OnComboFileTypeSelChange)
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

void CSplitToFilesDialog::OnComboFileTypeSelChange()
{
	int FileType = m_SaveAsTypesCombo.GetCurSel();
	if (FileType < 0
		|| FileType > SoundFileWma - SoundFileTypeMin)
	{
		return;
	}
	SetFileType(FileType + SoundFileTypeMin);
	//CFileSaveUiSupport::OnComboAttributesChange();
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

BOOL CSplitToFilesDialog::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN
		&& pMsg->hwnd != NULL
		&& pMsg->hwnd == m_FilesList.m_hWnd
		&& pMsg->wParam == VK_F2)
	{
		// find a selected item
		unsigned nSelItem = unsigned(-1);

		if (unsigned(-1) != (nSelItem = m_FilesList.GetNextItem(nSelItem, LVNI_SELECTED)))
		{
			if (nSelItem < m_Files.size())
			{
				m_FilesList.SetItemState(nSelItem, LVIS_FOCUSED, LVIS_FOCUSED);
				TRACE("m_FilesList.EditLabel(%d)\n", nSelItem);
				m_FilesList.EditLabel(nSelItem);
			}
		}
		return TRUE;
	}

	return BaseClass::PreTranslateMessage(pMsg);
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

		if ( ! s.IsEmpty())
		{
			m_Files[pDispInfo->item.iItem].Name = s;
			*pResult = TRUE;
		}
		else
		{
			*pResult = FALSE;
		}
	}
	else
	{
		*pResult = FALSE;
	}
}

void CSplitToFilesDialog::SetFileType(int nType, BOOL Force)
{
	if (! Force && m_FileType == nType)
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
	}
}

BOOL CSplitToFilesDialog::OnInitDialog()
{
	m_RecentFolders.Load();
	m_sSaveToFolder = m_RecentFolders[0];

	m_RecentFilenamePrefixes.Load();
	m_sFilenamePrefix = m_RecentFilenamePrefixes[0];

	BaseClass::OnInitDialog();

	m_eSaveToFolder.SetExtendedUI();
	m_RecentFolders.LoadCombo(& m_eSaveToFolder);

	m_eFilenamePrefix.SetExtendedUI();
	m_RecentFilenamePrefixes.LoadCombo(& m_eFilenamePrefix);

	if (m_SelectedTag.Tag == WAVE_FORMAT_MSAUDIO1
		|| m_SelectedTag.Tag == WAVE_FORMAT_MSAUDIO1 + 1)
	{
		m_SelectedBitrate = m_SelectedWmaBitrate;
	}
	else if (m_SelectedTag.Tag == WAVE_FORMAT_MPEGLAYER3
			|| m_SelectedTag == BladeMp3Encoder::GetTag())
	{
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

	static int const FileTypeIds[] =
	{
		IDR_WAVESOTYPE,
		IDR_MP3TYPE,
		IDR_WMATYPE,
	};

	CString filter;
	for (unsigned i = 0; i < countof(FileTypeIds); i++)
	{
		VERIFY(s.LoadString(FileTypeIds[i]));
		AfxExtractSubString(filter, s, CDocTemplate::filterName);

		m_SaveAsTypesCombo.AddString(filter);
	}

	m_SaveAsTypesCombo.SetCurSel(m_FileType - 1);
	SetFileType(m_FileType, TRUE); // force file type set

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSplitToFilesDialog::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	__super::OnOK();
	// save format selection

	m_RecentFolders.Flush();
	m_RecentFilenamePrefixes.Flush();

	m_SelectedFormat = m_AttributesCombo.GetCurSel();

	CFileSaveUiSupport::m_Profile.FlushAll();
}
