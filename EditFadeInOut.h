#pragma once
#include "resource.h"

// CEditFadeInOutDlg dialog

class CEditFadeInOutDlg : public CDialog
{
	typedef CDialog BaseClass;
	DECLARE_DYNAMIC(CEditFadeInOutDlg)

public:
	CEditFadeInOutDlg(BOOL bIsFadeOut, int TransitionType,
					CWnd* pParent = NULL);   // standard constructor
	virtual ~CEditFadeInOutDlg();

	int GetTransitionType() const
	{
		return m_TransitionType + 1;
	}
	BOOL IsFadeOut() const
	{
		return m_IsFadeOut != 0;
	}
// Dialog Data
	enum { IDD = IDD_DIALOG_EDIT_FADE_IN_OUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	// 0 - fade in, 1 - fade out
	int m_IsFadeOut;
	int m_TransitionType; // 0 - linear, 1 - squared sine, 2 - sine/cosine
};
