// EditFadeInOut.cpp : implementation file
//

#include "stdafx.h"
#include "EditFadeInOut.h"

// CEditFadeInOutDlg dialog

IMPLEMENT_DYNAMIC(CEditFadeInOutDlg, CDialog)

CEditFadeInOutDlg::CEditFadeInOutDlg(BOOL bIsFadeOut, int TransitionType,
									CWnd* pParent /*=NULL*/)
	: BaseClass(IDD, pParent)
	, m_IsFadeOut(bIsFadeOut)
	, m_TransitionType(TransitionType - 1)
{
}

CEditFadeInOutDlg::~CEditFadeInOutDlg()
{
}

void CEditFadeInOutDlg::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_FADE_IN, m_IsFadeOut);
	DDX_Radio(pDX, IDC_RADIO_LINEAR, m_TransitionType);
}


BEGIN_MESSAGE_MAP(CEditFadeInOutDlg, BaseClass)
END_MESSAGE_MAP()


// CEditFadeInOutDlg message handlers
