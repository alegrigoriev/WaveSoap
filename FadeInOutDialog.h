#pragma once
#include "afxwin.h"
#include "UiUpdatedDlg.h"
#include "resource.h"
// CFadeInOutDialog dialog

class CFadeInOutDialog : public CUiUpdatedDlg
{
	typedef CUiUpdatedDlg BaseClass;

public:
	CFadeInOutDialog(BOOL EnableFade, int TransitionType,
					unsigned TransitionLengthMs, CWnd* pParent = NULL);   // standard constructor
	virtual ~CFadeInOutDialog();

// Dialog Data
	enum { IDD = IDD_DIALOG_FADEINOUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL IsFadeEnabled() const
	{
		return m_FadeEnable;
	}
	int GetTransitionType() const
	{
		return m_TransitionType + 1;
	}
	UINT GetTransitionLengthMs() const
	{
		return m_TransitionLengthMs;
	}

protected:
	//CButton m_CheckFade;
	BOOL m_FadeEnable;
	int m_TransitionType;

	void OnUpdateFade(CCmdUI * pCmdUI);
	void OnClickedFade();
	UINT m_TransitionLengthMs;
};
