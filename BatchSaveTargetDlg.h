#if !defined(AFX_BATCHSAVETARGETDLG_H__CFACB615_6CD0_466E_BF66_D0C867C94CCE__INCLUDED_)
#define AFX_BATCHSAVETARGETDLG_H__CFACB615_6CD0_466E_BF66_D0C867C94CCE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BatchSaveTargetDlg.h : header file
//
#include "NumEdit.h"
#include "ApplicationProfile.h"

/////////////////////////////////////////////////////////////////////////////
// CBatchSaveTargetDlg dialog

class CBatchSaveTargetDlg : public CDialog
{
// Construction
public:
	CBatchSaveTargetDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CBatchSaveTargetDlg)
	enum { IDD = IDD_DIALOG_BATCH_SAVE_TARGET };
	CButton	m_MakePlaylistOnly;
	CEdit	m_eSaveFolder;
	CEdit	m_ePlaylistFile;
	CNumEdit	m_eNormalizeDb;
	CEdit	m_eHtmlFile;
	CStatic	m_FormatStatic;
	BOOL	m_bMakeHtml;
	BOOL	m_bMakePlaylist;
	BOOL	m_bMakePlaylistOnly;
	BOOL	m_bNormalize;
	CString	m_sHtmlFile;
	CString	m_sPlaylistFile;
	CString	m_sSaveFolder;
	int		m_FileSaveType;
	//}}AFX_DATA

	BOOL m_bNeedUpdateControls;
	// if m_bNeedFolderToSave, then "Generate playlist only" is disabled
	BOOL m_bNeedFolderToSave;
	double m_dNormalizeDb;
	CApplicationProfile m_Profile;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBatchSaveTargetDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	LRESULT OnKickIdle(WPARAM, LPARAM);
	afx_msg void OnUpdateButtonBrowseDstFolder(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonBrowsePlaylist(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonBrowseWebpage(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonFormat(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCheckNormalize(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSaveFolder(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePlaylistFile(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNormalizeDb(CCmdUI* pCmdUI);
	afx_msg void OnUpdateHtmlFile(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFormatStatic(CCmdUI* pCmdUI);
	afx_msg void OnUpdateOk(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRadioFormat(CCmdUI* pCmdUI);

	// Generated message map functions
	//{{AFX_MSG(CBatchSaveTargetDlg)
	afx_msg void OnButtonBrowseDstFolder();
	afx_msg void OnButtonBrowsePlaylist();
	afx_msg void OnButtonBrowseWebpage();
	afx_msg void OnButtonFormat();
	afx_msg void OnCheckMakeHtml();
	afx_msg void OnCheckMakePlaylist();
	afx_msg void OnCheckMakePlaylistOnly();
	afx_msg void OnCheckNormalize();
	afx_msg void OnChangeEditHtml();
	afx_msg void OnChangeEditNormalize();
	afx_msg void OnChangeEditPlaylist();
	afx_msg void OnChangeEditSaveFolder();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BATCHSAVETARGETDLG_H__CFACB615_6CD0_466E_BF66_D0C867C94CCE__INCLUDED_)
