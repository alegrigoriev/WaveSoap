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

// Dialog Data
	enum { IDD = IDD_DIALOG_SPLIT_TO_FILES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
protected:

	WaveFileSegmentVector m_Files;
	// List of all files to write
	CListCtrl m_FilesList;
	CWaveFile & m_WaveFile;

	afx_msg void OnBnClickedButtonBrowseFolder();
	afx_msg void OnLvnBeginlabeleditListFiles(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnEndlabeleditListFiles(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCompatibleFormatsClicked();
	afx_msg void OnComboFormatsChange();
	afx_msg void OnComboAttributesChange();

	void SetFileType(int nType);
	void ShowDlgItem(UINT nID, int nCmdShow);

	virtual BOOL OnInitDialog();

public:
	// Combobox to list supported types
	CComboBox m_SaveAsTypesCombo;
	// Folder to save the files, with MRU history
	CComboBox m_eSaveToFolder;
	CString m_sSaveToFolder;
};
