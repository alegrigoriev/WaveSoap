// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// SplitToFilesDialog.h : interface file
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "resource.h"
#include "ResizableDialog.h"
#include "WaveSoapFileDialogs.h"
#include <vector>
#include "OperationDialogs.h"

// CSplitToFilesDialog dialog
class CWaveFile;
class CSplitToFilesDialog : public CResizableDialog, public CFileSaveUiSupport, protected CSelectionUiSupport
{
	typedef CResizableDialog BaseClass;
public:
	CSplitToFilesDialog(CWaveFile & WaveFile, int TimeFormat, CWnd* pParent = NULL);   // standard constructor
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

	WaveFileSegmentVector m_Files;
	// List of all files to write
	CListCtrl m_FilesList;
	int m_FileTypeFlags;

	afx_msg void OnBnClickedButtonBrowseFolder();
	afx_msg void OnLvnItemchangedListFiles(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnEndlabeleditListFiles(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCompatibleFormatsClicked();
	afx_msg void OnComboFormatsChange();
	afx_msg void OnComboAttributesChange();
	afx_msg void OnComboFileTypeSelChange();
	// CSelectionSupport handlers:
	afx_msg void OnSelchangeComboTimeFormat();
	afx_msg void OnKillfocusEditEnd();
	afx_msg void OnKillfocusEditLength();
	afx_msg void OnKillfocusEditStart();

	afx_msg void OnSelchangeComboStart();
	afx_msg void OnSelchangeComboEnd();
	afx_msg void OnDeferredSelchangeComboStart();
	afx_msg void OnDeferredSelchangeComboEnd();

	afx_msg void OnBuddyChangeSpinEnd(NMHDR * pNmHdr, LRESULT * pResult);
	afx_msg void OnBuddyChangeSpinLength(NMHDR * pNmHdr, LRESULT * pResult);
	afx_msg void OnBuddyChangeSpinStart(NMHDR * pNmHdr, LRESULT * pResult);
	afx_msg void OnSelchangeComboSelection();
	afx_msg void OnBnClickedButtonNew();
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void EnableIfItemSelected(CCmdUI * pCmdUI);

	virtual void OnOK();
	afx_msg void OnUpdateOK(CCmdUI * pCmdUI);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	virtual void ResetFormatTagCombo();
	virtual void AddFormatTagComboString(int idx, LPCWSTR string);
	virtual int GetFormatTagComboSelection();
	virtual void SetFormatTagComboSelection(int sel);
	virtual void ResetAttributesCombo();
	virtual void AddAttributesComboString(int idx, LPCWSTR string);
	virtual int GetAttributesComboSelection();
	virtual void SetAttributesComboSelection(int sel);

	void SetFileType(unsigned nType, BOOL Force = FALSE);
	void ShowDlgItem(UINT nID, int nCmdShow);

	// sets the data to the array and also to the list view
	void SetFileArrayItem(unsigned index, SAMPLE_INDEX Begin, SAMPLE_INDEX End);
	// sets the data to the list view only
	void SetFileListItem(unsigned index, SAMPLE_INDEX Begin, SAMPLE_INDEX End);
	void InsertFileListItem(unsigned index, LPCTSTR Name, SAMPLE_INDEX Begin, SAMPLE_INDEX End);

	void SaveChangedSelectionRange();

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
