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

enum
{
	ListNameColumn,
	ListBeginColumn,
	ListEndColumn,
	ListLengthColumn,
};

// CSplitToFilesDialog dialog

CSplitToFilesDialog::CSplitToFilesDialog(CWaveFile & WaveFile, int TimeFormat, CWnd* pParent /*=NULL*/)
	: BaseClass(IDD, pParent)
	, CFileSaveUiSupport(WaveFile.GetWaveFormat())
	, CSelectionUiSupport(0, 0, 0, ALL_CHANNELS, WaveFile, TimeFormat, FALSE, FALSE)
	, m_FileTypeFlags(0)
	, m_RecentFolders(& m_Profile, _T("RecentOpenDirs"), _T("Dir%d"), 15)
	, m_RecentFilenamePrefixes(& m_Profile, _T("RecentFilenamePrefixes"), _T("Prefix%d"), 15,
								CStringHistory::CaseSensitive)
{
	// fill the files list
	WaveFile.GetSortedFileSegments(m_Files);

	m_Profile.AddItem(_T("Settings"), _T("SplitToFileType"),
					m_FileType, SaveFile_WavFile, SaveFile_WavFile, SaveFile_WmaFile);

	// resizable dialog support:
	static const ResizableDlgItem items[] =
	{
		IDC_LIST_FILES, ExpandRight | ExpandDown,
		IDC_COMBO_FILE_TYPE, ExpandRight | MoveDown,
		IDC_COMBO_SAVE_DIR, ExpandRight | MoveDown | ThisIsDropCombobox,
		IDC_COMBO_FILE_TYPE, ExpandRight | MoveDown,
		IDC_COMBO_FILENAME_PREFIX, ExpandRight | MoveDown | ThisIsDropCombobox,
		IDC_COMBO_FORMAT_TAG, ExpandRight | MoveDown,
		IDC_COMBO_FORMAT_ATTRIBUTES, ExpandRight | MoveDown,
		IDC_COMBO_SELECTION, ExpandRight | MoveDown,
		IDC_COMBO_START, ExpandRight | MoveDown,
		IDC_COMBO_END, ExpandRight | MoveDown,
		IDC_EDIT_LENGTH, ExpandRight | MoveDown,
		IDC_COMBO_TIME_FORMAT, MoveDown,

		IDC_BUTTON_BROWSE_FOLDER, MoveRight | MoveDown,
		IDC_CHECK_COMPATIBLE_FORMATS, MoveRight | MoveDown,
		IDC_SPIN_START, MoveRight | MoveDown,
		IDC_SPIN_END, MoveRight | MoveDown,
		IDC_SPIN_LENGTH, MoveRight | MoveDown,

		IDC_STATIC1, MoveDown,
		IDC_STATIC2, MoveDown,
		IDC_STATIC3, MoveDown,
		IDC_STATIC4, MoveDown,
		IDC_STATIC5, MoveDown,
		IDC_STATIC6, MoveDown,
		IDC_STATIC7, MoveDown,
		IDC_STATIC8, MoveDown,
		IDC_STATIC_FORMAT, MoveDown,
		IDC_STATIC_FORMAT_ATTRIBUTES, MoveDown,

		IDOK, MoveRight,
		IDCANCEL, MoveRight,
		IDC_BUTTON_NEW, MoveRight,
		IDC_BUTTON_DELETE, MoveRight,
		IDC_BUTTON_PLAY, MoveRight,
	};

	SetResizeableItems(items, countof(items));
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

void CSplitToFilesDialog::SetFileArrayItem(unsigned index, SAMPLE_INDEX Begin, SAMPLE_INDEX End)
{
	if (index >= m_Files.size())
	{
		return;
	}
	WaveFileSegmentVector::reference f = m_Files[index];

	f.Begin = Begin;
	f.End = End;

	SetFileListItem(index, Begin, End);
}

void CSplitToFilesDialog::InsertFileListItem(unsigned index, LPCTSTR Name, SAMPLE_INDEX Begin, SAMPLE_INDEX End)
{
	m_FilesList.InsertItem(index, Name);

	SetFileListItem(index, Begin, End);
}

void CSplitToFilesDialog::SetFileListItem(unsigned index, SAMPLE_INDEX Begin, SAMPLE_INDEX End)
{
	CString s;
	LVITEM item;
	memzero(item);

	item.mask = LVIF_TEXT;
	item.iItem = index;

	// set "begin" column
	item.iSubItem = ListBeginColumn;
	s = SampleToString(Begin, m_WaveFile.SampleRate(), m_TimeFormat);
	item.pszText = (LPTSTR)LPCTSTR(s);

	m_FilesList.SetItem( & item);

	// set "end" column
	item.iSubItem = ListEndColumn;
	s = SampleToString(End, m_WaveFile.SampleRate(), m_TimeFormat);
	item.pszText = (LPTSTR)LPCTSTR(s);

	m_FilesList.SetItem( & item);

	// set "length" column
	item.iSubItem = ListLengthColumn;
	s = SampleToString(End - Begin, m_WaveFile.SampleRate(), m_TimeFormat);
	item.pszText = (LPTSTR)LPCTSTR(s);

	m_FilesList.SetItem( & item);
}

void CSplitToFilesDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	CSelectionUiSupport::DoDataExchange(pDX, this);

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
		m_FileTypeFlags = m_FileType;

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

		for (FileIndex = 0, t = m_Files.begin(); t < m_Files.end(); t++, FileIndex++)
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
						c = m_sFilenamePrefix[i + 1];
						if ('%' == c)
						{
							// double percent escape - translated to a single percent
							i++;
						}
						else
						{
							// if the sequence is %nnd, then the numbering starts from n
							// if the sequence is %0nd, then the numbering starts from n, with leading zeros
							int FileIndexOffset = 0;
							int NumDigits = 0;
							bool LeadingZero = false;

							CString s;
							while (i < m_sFilenamePrefix.GetLength() - 1)
							{
								c = m_sFilenamePrefix[i + 1];

								if ('d' == c)
								{
									if (0 == NumDigits)
									{
										s.Format(_T("%02d"), FileIndex + 1);
									}
									else
									{
										if (LeadingZero)
										{
											s.Format(_T("%0*d"), NumDigits, FileIndex + FileIndexOffset);
										}
										else
										{
											s.Format(_T("%d"), FileIndex + FileIndexOffset);
										}
									}
									// break from while() loop
									break;
								}
								else if (c >= '0' && c <= '9')
								{
									// digit character
									if (0 == NumDigits)
									{
										if ('0' == c)
										{
											LeadingZero = true;
										}
									}

									FileIndexOffset = (c - '0') + 10 * FileIndexOffset;
									NumDigits++;

									// in case the format spec is incorrect, save the characters in the string
									s += c;
								}
								else
								{
									// not a digit, nor 'd' character encountered. Just copy the saved string
									s += c;
									break;
								}
								i++;
							}
							Name += s;
							i++;
							// go around the loop end, without "Name += c"
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
			case SaveFile_Mp3File:
				Name += _T(".mp3");
				break;
			case SaveFile_WmaFile:
				Name += _T(".wma");
				break;
			default:
				Name += _T(".wav");
				break;
			}

			TRACE(_T("File name =%s\n"), LPCTSTR(Name));

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
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_DELETE, EnableIfItemSelected)
	ON_UPDATE_COMMAND_UI(IDC_COMBO_START, EnableIfItemSelected)
	ON_UPDATE_COMMAND_UI(IDC_COMBO_END, EnableIfItemSelected)
	ON_UPDATE_COMMAND_UI(IDC_COMBO_SELECTION, EnableIfItemSelected)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_LENGTH, EnableIfItemSelected)
	ON_UPDATE_COMMAND_UI(IDC_SPIN_START, EnableIfItemSelected)
	ON_UPDATE_COMMAND_UI(IDC_SPIN_END, EnableIfItemSelected)
	ON_UPDATE_COMMAND_UI(IDC_SPIN_LENGTH, EnableIfItemSelected)
	ON_UPDATE_COMMAND_UI(IDOK, OnUpdateOK)

	ON_BN_CLICKED(IDC_BUTTON_BROWSE_FOLDER, OnBnClickedButtonBrowseFolder)
	ON_BN_CLICKED(IDC_CHECK_COMPATIBLE_FORMATS, OnCompatibleFormatsClicked)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnBnClickedButtonNew)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)

	ON_CBN_SELCHANGE(IDC_COMBO_FORMAT_TAG, OnComboFormatsChange)
	ON_CBN_SELCHANGE(IDC_COMBO_FORMAT_ATTRIBUTES, OnComboAttributesChange)
	ON_CBN_SELCHANGE(IDC_COMBO_FILE_TYPE, OnComboFileTypeSelChange)

	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST_FILES, OnLvnEndlabeleditListFiles)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILES, OnLvnItemchangedListFiles)
// CSelectionSupport
	ON_CBN_SELCHANGE(IDC_COMBO_TIME_FORMAT, OnSelchangeComboTimeFormat)
	ON_CBN_KILLFOCUS(IDC_COMBO_END, OnKillfocusEditEnd)
	ON_NOTIFY(CTimeSpinCtrl::TSC_BUDDY_CHANGE, IDC_SPIN_END, OnBuddyChangeSpinEnd)
	ON_EN_KILLFOCUS(IDC_EDIT_LENGTH, OnKillfocusEditLength)
	ON_NOTIFY(CTimeSpinCtrl::TSC_BUDDY_CHANGE, IDC_SPIN_LENGTH, OnBuddyChangeSpinLength)
	ON_CBN_KILLFOCUS(IDC_COMBO_START, OnKillfocusEditStart)
	ON_NOTIFY(CTimeSpinCtrl::TSC_BUDDY_CHANGE, IDC_SPIN_START, OnBuddyChangeSpinStart)
	ON_CBN_SELCHANGE(IDC_COMBO_SELECTION, OnSelchangeComboSelection)

	ON_CBN_SELCHANGE(IDC_COMBO_END, OnSelchangeComboEnd)
	ON_CBN_SELCHANGE(IDC_COMBO_START, OnSelchangeComboStart)

	ON_CONTROL(20, IDC_COMBO_START, OnDeferredSelchangeComboStart)
	ON_CONTROL(20, IDC_COMBO_END, OnDeferredSelchangeComboEnd)
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
	unsigned FileType = m_SaveAsTypesCombo.GetCurSel();
	if (FileType < m_NumOfFileTypes)
	{
		SetFileType(m_TemplateFlags[FileType]);
		//CFileSaveUiSupport::OnComboAttributesChange();
	}
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

BOOL CSplitToFilesDialog::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN
		&& pMsg->hwnd != NULL
		&& pMsg->hwnd == m_FilesList.m_hWnd)
	{
		if (pMsg->wParam == VK_F2)
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
		else if (pMsg->wParam == VK_DELETE)
		{
			OnBnClickedButtonDelete();
			return TRUE;
		}
		else if (pMsg->wParam == VK_INSERT)
		{
			OnBnClickedButtonNew();
			return TRUE;
		}
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

void CSplitToFilesDialog::SetFileType(unsigned nType, BOOL Force)
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
	case SaveFile_WavFile:
		// WAV file
		// show Comments, fill formats combo box
		s.LoadString(IDS_FORMAT);
		SetDlgItemText(IDC_STATIC_FORMAT, s);

		ShowDlgItem(IDC_STATIC_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT_TAG, SW_SHOWNOACTIVATE);
		//ShowDlgItem(IDC_COMBO_FORMAT_ATTRIBUTES, SW_SHOWNOACTIVATE);
		//ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_SHOWNOACTIVATE);
		//ShowDlgItem(IDC_STATIC_FORMAT_ATTRIBUTES, SW_SHOWNOACTIVATE);

		FillFormatTagCombo(ExcludeFormats, -1, WaveFormatExcludeFormats);

		m_SelectedFormat = FillFormatCombo(m_FormatTagCombo.GetCurSel());
		break;  // go on

	case SaveFile_Mp3File:
		// MP3 file
		// replace Format with Encoder: (LAME, Fraunhofer
		s.LoadString(IDS_ENCODER);
		SetDlgItemText(IDC_STATIC_FORMAT, s);

		ShowDlgItem(IDC_STATIC_FORMAT, SW_SHOWNOACTIVATE);
		ShowDlgItem(IDC_COMBO_FORMAT_TAG, SW_SHOWNOACTIVATE);
		//ShowDlgItem(IDC_COMBO_FORMAT_ATTRIBUTES, SW_SHOWNOACTIVATE);
		//ShowDlgItem(IDC_STATIC_FORMAT_ATTRIBUTES, SW_SHOWNOACTIVATE);
		//ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_SHOWNOACTIVATE);
		// Hide Comments, show Artist, Genre, Title
		//
		// set tag to MP3
		// fill formats combo (bitrate, etc)
		FillMp3EncoderCombo();
		break;

	case SaveFile_WmaFile:
		// WMA file
		if ( ! GetApp()->CanOpenWindowsMedia())
		{
			int id = AfxMessageBox(IDS_WMA_ENCODER_NOT_AVILABLE, MB_OK | MB_ICONEXCLAMATION | MB_HELP);
			if (IDHELP == id)
			{
				// TODO: show help
			}
			SetFileType(SaveFile_WavFile);
			break;
		}

		// remove Format: combo
		ShowDlgItem(IDC_STATIC_FORMAT, SW_HIDE);
		ShowDlgItem(IDC_COMBO_FORMAT_TAG, SW_HIDE);
		//ShowDlgItem(IDC_COMBO_FORMAT_ATTRIBUTES, SW_SHOWNOACTIVATE);
		//ShowDlgItem(IDC_STATIC_FORMAT_ATTRIBUTES, SW_SHOWNOACTIVATE);
		//ShowDlgItem(IDC_CHECK_COMPATIBLE_FORMATS, SW_SHOWNOACTIVATE);
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
	SetWindowIcons(this, IDI_ICON_SPLIT_TO_FILES);

	InitSelectionUi();


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
		InsertFileListItem(ItemIdx, i->Name, i->Begin, i->End);
	}

	if ( ! m_Files.empty())
	{
		m_FilesList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
	}

	static unsigned const FileTypeIds[][2] =
	{
		{IDR_WAVESOTYPE, SaveFile_WavFile},
		{IDR_MP3TYPE, SaveFile_Mp3File},
		{IDR_WMATYPE, SaveFile_WmaFile}
	};

	CString filter;
	unsigned sel = 0;
	for (unsigned i = 0; i < countof(FileTypeIds); i++)
	{
		VERIFY(s.LoadString(FileTypeIds[i][0]));
		AfxExtractSubString(filter, s, CDocTemplate::filterName);

		m_SaveAsTypesCombo.AddString(filter);
		AfxExtractSubString(filter, m_DefExt[i], CDocTemplate::filterExt);

		m_TemplateFlags[i] = FileTypeIds[i][1];
		if (FileTypeIds[i][1] == m_FileType)
		{
			sel = i;
		}
	}

	m_NumOfFileTypes = countof(FileTypeIds);

	m_SaveAsTypesCombo.SetCurSel(sel);
	m_FileType = m_TemplateFlags[sel];

	SetFileType(m_FileType, TRUE); // force file type set

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSplitToFilesDialog::OnUpdateOK(CCmdUI * pCmdUI)
{
	BOOL bEnable = FALSE;
	for (WaveFileSegmentVector::const_iterator i = m_Files.begin();
		i < m_Files.end(); i++)
	{
		if (i->Begin < i->End)
		{
			bEnable = TRUE;
			break;
		}
	}
	pCmdUI->Enable(bEnable);
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

void CSplitToFilesDialog::OnSelchangeComboTimeFormat()
{
	CSelectionUiSupport::OnSelchangeComboTimeFormat();

	// fill the list view
	unsigned ItemIdx = 0;
	for (WaveFileSegmentVector::const_iterator i = m_Files.begin();
		i < m_Files.end(); i++, ItemIdx++)
	{
		SetFileListItem(ItemIdx, i->Begin, i->End);
	}
}

void CSplitToFilesDialog::OnBuddyChangeSpinEnd(NMHDR * pNmHdr, LRESULT * pResult)
{
	CSelectionUiSupport::OnBuddyChangeSpinEnd(pNmHdr, pResult);
	SaveChangedSelectionRange();
}

void CSplitToFilesDialog::OnKillfocusEditEnd()
{
	CSelectionUiSupport::OnKillfocusEditEnd();
	SaveChangedSelectionRange();
}

void CSplitToFilesDialog::OnBuddyChangeSpinLength(NMHDR * pNmHdr, LRESULT * pResult)
{
	CSelectionUiSupport::OnBuddyChangeSpinLength(pNmHdr, pResult);
	SaveChangedSelectionRange();
}

void CSplitToFilesDialog::OnKillfocusEditLength()
{
	CSelectionUiSupport::OnKillfocusEditLength();
	SaveChangedSelectionRange();
}

void CSplitToFilesDialog::OnBuddyChangeSpinStart(NMHDR * pNmHdr, LRESULT * pResult)
{
	CSelectionUiSupport::OnBuddyChangeSpinStart(pNmHdr, pResult);
	SaveChangedSelectionRange();
}

void CSplitToFilesDialog::OnKillfocusEditStart()
{
	CSelectionUiSupport::OnKillfocusEditStart();
	SaveChangedSelectionRange();
}

void CSplitToFilesDialog::OnSelchangeComboSelection()
{
	CSelectionUiSupport::OnSelchangeComboSelection();
	SaveChangedSelectionRange();
}

void CSplitToFilesDialog::OnSelchangeComboStart()
{
	PostMessage(WM_COMMAND, MAKEWPARAM(IDC_COMBO_START, 20), LPARAM(m_eStart.m_hWnd));
}

void CSplitToFilesDialog::OnSelchangeComboEnd()
{
	PostMessage(WM_COMMAND, MAKEWPARAM(IDC_COMBO_END, 20), LPARAM(m_eStart.m_hWnd));
}

void CSplitToFilesDialog::OnDeferredSelchangeComboStart()
{
	CSelectionUiSupport::OnKillfocusEditStart();
}

void CSplitToFilesDialog::OnDeferredSelchangeComboEnd()
{
	CSelectionUiSupport::OnKillfocusEditEnd();
}

void CSplitToFilesDialog::SaveChangedSelectionRange()
{
	// save range from the selection combo-boxes
	unsigned nSelItem = m_FilesList.GetNextItem(-1, LVNI_SELECTED);
	TRACE("Item %d selection saved\n", nSelItem);
	SetFileArrayItem(nSelItem, m_Start, m_End);
	NeedUpdateControls();
}

void CSplitToFilesDialog::OnLvnItemchangedListFiles(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if ((pNMLV->uChanged & LVIF_STATE)
		&& unsigned(pNMLV->iItem) < m_Files.size())
	{
		if (pNMLV->uNewState & LVIS_SELECTED)
		{
			TRACE("List item %d selected\n", pNMLV->iItem);
			// set new range to the selection combo-boxes
			SetSelection(m_Files[pNMLV->iItem].Begin, m_Files[pNMLV->iItem].End);

			UpdateComboSelection();
			NeedUpdateControls();
		}
		if (pNMLV->uOldState & LVIS_SELECTED)
		{
			TRACE("List item %d UNselected\n", pNMLV->iItem);
			// save range from the selection combo-boxes
			UpdateAllSelections();  // KILLFOCUS doesn't come yet, have to just update all
			SetFileArrayItem(pNMLV->iItem, m_Start, m_End);
			NeedUpdateControls();
		}
	}
	*pResult = 0;
}

void CSplitToFilesDialog::OnBnClickedButtonNew()
{
	unsigned nSelItem = m_FilesList.GetNextItem(-1, LVNI_SELECTED);

	WaveFileSegmentVector::iterator i;
	i = m_Files.begin();
	if (nSelItem <= m_Files.size())
	{
		i += nSelItem;
	}
	else
	{
		nSelItem = m_Files.size();
		i = m_Files.end();
	}

	WaveFileSegment seg;
	seg.Begin = 0;
	seg.End = m_WaveFile.NumberOfSamples();

	if (i != m_Files.begin())
	{
		seg.Begin = (i - 1)->End;
	}
	if (i != m_Files.end())
	{
		seg.End = i->Begin;
	}

	seg.Name.Format(IDS_NEW_FILE_SEGMENT_NAME, nSelItem + 1);

	m_Files.insert(i, seg);
	InsertFileListItem(nSelItem, seg.Name, seg.Begin, seg.End);
	m_FilesList.SetFocus();
	m_FilesList.SetItemState(nSelItem, LVIS_SELECTED, LVIS_SELECTED);
	m_FilesList.EditLabel(nSelItem);
	NeedUpdateControls();
}

void CSplitToFilesDialog::EnableIfItemSelected(CCmdUI * pCmdUI)
{
	unsigned nSelItem = m_FilesList.GetNextItem(-1, LVNI_SELECTED);
	pCmdUI->Enable(nSelItem != unsigned(-1));
}

void CSplitToFilesDialog::OnBnClickedButtonDelete()
{
	// find a selected item
	unsigned nSelItem = m_FilesList.GetNextItem(-1, LVNI_SELECTED);

	if (nSelItem < m_Files.size())
	{
		if (m_FilesList.DeleteItem(nSelItem))
		{
			m_Files.erase(m_Files.begin() + nSelItem);
			NeedUpdateControls();

			if ( ! m_Files.empty())
			{
				if (nSelItem >= m_Files.size())
				{
					nSelItem = m_Files.size() - 1;
				}
				m_FilesList.SetItemState(nSelItem, LVIS_SELECTED, LVIS_SELECTED);
			}
		}
	}
}

