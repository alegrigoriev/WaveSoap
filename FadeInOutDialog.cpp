// FadeInOutDialog.cpp : implementation file
//

#include "stdafx.h"
#include "WaveSoapFront.h"
#include "FadeInOutDialog.h"


// CFadeInOutDialog dialog

CFadeInOutDialog::CFadeInOutDialog(BOOL EnableFade, int TransitionType,
									unsigned TransitionLengthMs, CWnd* pParent /*=NULL*/)
	: BaseClass(IDD, pParent)
	, m_FadeEnable(EnableFade != FALSE)
	, m_TransitionType(TransitionType - 1)
	, m_TransitionLengthMs(TransitionLengthMs)
{
}

CFadeInOutDialog::~CFadeInOutDialog()
{
}

void CFadeInOutDialog::DoDataExchange(CDataExchange* pDX)
{
	BaseClass::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_CHECK_ENABLE_FADEINOUT, m_CheckFade);
	DDX_Check(pDX, IDC_CHECK_ENABLE_FADEINOUT, m_FadeEnable);

	DDX_Radio(pDX, IDC_RADIO_LINEAR, m_TransitionType);
	DDX_Text(pDX, IDC_EDIT_TRANSITION_LENGTH, m_TransitionLengthMs);
	DDV_MinMaxUInt(pDX, m_TransitionLengthMs, 1, 2000);
}


BEGIN_MESSAGE_MAP(CFadeInOutDialog, BaseClass)
	ON_UPDATE_COMMAND_UI(IDC_STATIC1, OnUpdateFade)
	ON_UPDATE_COMMAND_UI(IDC_STATIC2, OnUpdateFade)
	ON_UPDATE_COMMAND_UI(IDC_STATIC3, OnUpdateFade)
	ON_UPDATE_COMMAND_UI(IDC_RADIO_LINEAR, OnUpdateFade)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_TRANSITION_LENGTH, OnUpdateFade)
	ON_UPDATE_COMMAND_UI(IDC_RADIO_SQUARED_SINE, OnUpdateFade)
	ON_UPDATE_COMMAND_UI(IDC_RADIO_SINE_COSINE, OnUpdateFade)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_FADEINOUT, OnClickedFade)
END_MESSAGE_MAP()


// CFadeInOutDialog message handlers
void CFadeInOutDialog::OnUpdateFade(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(m_FadeEnable);
}

void CFadeInOutDialog::OnClickedFade()
{
	m_FadeEnable = IsDlgButtonChecked(IDC_CHECK_ENABLE_FADEINOUT);

	NeedUpdateControls();
}
