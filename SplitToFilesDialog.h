// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// SplitToFilesDialog.h : interface file
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "resource.h"
#include "WaveSoapFileDialogs.h"
//#include "ResizableDialog.h"
#include <vector>

// CSplitToFilesDialog dialog
class CWaveFile;
class CSplitToFilesDialog : public CDialog, public CFileSaveUiSupport
{
	typedef CDialog BaseClass;
public:
	CSplitToFilesDialog(CWaveFile & WaveFile, CWnd* pParent = NULL);   // standard constructor
	virtual ~CSplitToFilesDialog();

	// return result: Full file name, begin, end
	bool GetFileData(unsigned index, CString & FileName, CString & Name, SAMPLE_INDEX * pBegin, SAMPLE_INDEX * pEnd) const;
	int GetFileTypeFlags() const
	{
		return m_FileTypeFlags;
	}
// Dialog Data
	enum { IDD = IDD_DIALOG_SPLIT_TO_FILES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
protected:

	WaveFileSegmentVector m_Files;
	// List of all files to write
	CListCtrl m_FilesList;
	CWaveFile & m_WaveFile;
	int m_FileTypeFlags;

	afx_msg void OnBnClickedButtonBrowseFolder();
	afx_msg void OnLvnBeginlabeleditListFiles(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnEndlabeleditListFiles(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCompatibleFormatsClicked();
	afx_msg void OnComboFormatsChange();
	afx_msg void OnComboAttributesChange();
	afx_msg void OnComboFileTypeSelChange();

	virtual void OnOK();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void SetFileType(int nType, BOOL Force = FALSE);
	void ShowDlgItem(UINT nID, int nCmdShow);

	virtual BOOL OnInitDialog();

	// Combobox to list supported types
	CComboBox m_SaveAsTypesCombo;
	// Folder to save the files, with MRU history
	CComboBox m_eSaveToFolder;
	CString m_sSaveToFolder;
	CStringHistory m_RecentFolders;

	CComboBox m_eFilenamePrefix;
	CString m_sFilenamePrefix;
	CStringHistory m_RecentFilenamePrefixes;
};
