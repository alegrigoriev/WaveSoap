#pragma once
#include "afxwin.h"
#include "resource.h"
#include "ApplicationParameters.h"
#include "afxcmn.h"
#include "UiUpdatedDlg.h"
// CUndoRedoOptionsDlg dialog

class CUndoRedoOptionsDlg : public CUiUpdatedDlg, UndoRedoParameters
{
	typedef CUiUpdatedDlg BaseClass;
	DECLARE_DYNAMIC(CUndoRedoOptionsDlg)

public:
	CUndoRedoOptionsDlg(class CWaveSoapFrontDoc * pDoc, CWnd* pParent = NULL);   // standard constructor
	virtual ~CUndoRedoOptionsDlg();

	UndoRedoParameters const * GetUndoParameters() const
	{
		return this;
	}
// Dialog Data
	enum { IDD = IDD_DIALOG_UNDO_REDO_OPTIONS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
protected:

	class CWaveSoapFrontDoc * m_pDoc;

	CStatic m_UndoStatus;
	CString m_UndoStatusFormat;
	CSpinButtonCtrl m_UndoDepthSpin;

	CStatic m_RedoStatus;
	CString m_RedoStatusFormat;
	CSpinButtonCtrl m_RedoDepthSpin;

	afx_msg void OnBnClickedButtonClearRedo();
	afx_msg void OnBnClickedButtonClearUndo();
	afx_msg void OnBnClickedCheckEnableRedo();
	afx_msg void OnBnClickedCheckEnableUndo();
	afx_msg void OnBnClickedCheckLimitRedoDepth();
	afx_msg void OnBnClickedCheckLimitRedoSize();
	afx_msg void OnBnClickedCheckLimitUndoDepth();
	afx_msg void OnBnClickedCheckLimitUndoSize();

	afx_msg void OnUpdateCheckLimitRedoDepth(CCmdUI * pCmdUI);
	afx_msg void OnUpdateCheckLimitRedoSize(CCmdUI * pCmdUI);
	afx_msg void OnUpdateCheckLimitUndoDepth(CCmdUI * pCmdUI);
	afx_msg void OnUpdateCheckLimitUndoSize(CCmdUI * pCmdUI);
	afx_msg void OnUpdateEditLimitRedoDepth(CCmdUI * pCmdUI);
	afx_msg void OnUpdateEditLimitRedoSize(CCmdUI * pCmdUI);
	afx_msg void OnUpdateEditLimitUndoDepth(CCmdUI * pCmdUI);
	afx_msg void OnUpdateEditLimitUndoSize(CCmdUI * pCmdUI);

	void SetUndoRedoStatusText();

	virtual BOOL OnInitDialog();
};
