#if !defined(AFX_WAVESOAPFILE_DIALOGS_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_WAVESOAPFILE_DIALOGS_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "FileDialogWithHistory.h"
#include "ApplicationProfile.h"
#include "DirectFile.h"
#include "WaveSupport.h"

class CWaveSoapFileOpenDialog : public CFileDialogWithHistory
{
public:
	CWaveSoapFileOpenDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
							LPCTSTR lpszDefExt = NULL,
							LPCTSTR lpszFileName = NULL,
							DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							LPCTSTR lpszFilter = NULL,
							CWnd* pParentWnd = NULL)
		: CFileDialogWithHistory(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags,
								lpszFilter, pParentWnd),
		m_bReadOnly(false),
		m_MinWmaFilter(0),
		m_MaxWmaFilter(0),
		m_PrevFilter(-1),
		m_bDirectMode(false)
	{
	}

	CWaveFile m_WaveFile;
	//CString GetNextPathName(POSITION& pos) const;

	bool m_bReadOnly;
	bool m_bDirectMode;

	int nChannels;
	int nBitsPerSample;
	int nSamplingRate;
	int nSamples;
	CString WaveFormat;
	int m_MinWmaFilter;
	int m_MaxWmaFilter;
	int m_PrevFilter;

	virtual BOOL OnFileNameOK();

	virtual void OnInitDone();
	virtual void OnFileNameChange();
	virtual void OnTypeChange();
	void ClearFileInfoDisplay();
	void ShowWmaFileInfo(CDirectFile & File);
	//{{AFX_MSG(CWaveSoapFileOpenDialog)
	afx_msg void OnCheckReadOnly();
	afx_msg void OnCheckDirectMode();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
#ifdef _DEBUG
	//virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
#endif
};

enum {
	SoundFileTypeMin = 1,
	SoundFileWav = 1,
	SoundFileMp3 = 2,
	SoundFileWma = 3,
	SoundFileRaw = 4,
	//SoundFileAvi = 5,
	SoundFileTypeMax = 4,
};
enum
{
	RawSoundFilePcm16Lsb,
	RawSoundFilePcm16Msb,
	RawSoundFilePcm8,
	RawSoundFileALaw8,
	RawSoundFileULaw8,
	RawSoundFileAsciiDecimal,
};

enum
{
	Mp3EncoderNone = 0,
	Mp3EncoderLameencoder = 1,
	Mp3EncoderAcm = 2,
	Mp3EncoderBlade = 3,
};

enum
{
	LameEncBitrate64 = 64,
	LameEncBitrate96 = 96,
	LameEncBitrate128 = 128,
	LameEncBitrate160 = 160,
	LameEncBitrate192 = 192,
	LameEncBitrate256 = 256,
	LameEncBitrate320 = 320,

};

class CWaveSoapFileSaveDialog : public CFileDialogWithHistory
{
public:
	CWaveSoapFileSaveDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
							LPCTSTR lpszDefExt = NULL,
							LPCTSTR lpszFileName = NULL,
							DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							LPCTSTR lpszFilter = NULL,
							CWnd* pParentWnd = NULL)
		: CFileDialogWithHistory(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags,
								lpszFilter, pParentWnd),
		m_SelectedFormat(-1),
		m_SelectedMp3Encoder(0),
		m_NumOfMp3Encoders(0),
		m_SelectedLameMp3Bitrate(LameEncBitrate128),
		m_bCompatibleFormatsOnly(TRUE),
		m_FileType(SoundFileWav),
		m_SelectedRawFormat(RawSoundFilePcm16Lsb),
		m_pDocument(NULL)
	{
		memzero(m_Mp3Encoders);
		if (CThisApp::SupportsV5FileDialog())
		{
			m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG_SAVE_TEMPLATE_V5);
		}
		else
		{
			m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DIALOG_SAVE_TEMPLATE_V4);
		}
		m_DefExt[1] = _T("wav");
		m_DefExt[2] = _T("mp3");
		m_DefExt[3] = _T("wma");
		m_DefExt[4] = _T("raw");
		m_DefExt[5] = _T("avi");
	}
	~CWaveSoapFileSaveDialog() {}

	CWaveFile m_WaveFile;
	CComboBox m_FormatCombo;
	CComboBox m_AttributesCombo;

	WaveFormatTagEx m_SelectedTag;
	int m_SelectedFormat;
	int m_FileType;// Wav, Mp3, wma, raw...
	int m_SelectedRawFormat;

	int m_SelectedMp3Encoder;
	int m_Mp3Encoders[4];
	int m_NumOfMp3Encoders;
	int m_SelectedLameMp3Bitrate;
	int GetLameEncBitrate() const;

	BOOL m_bCompatibleFormatsOnly;

	CString m_FormatTagName;
	CString m_DefExt[10];

	CWaveFormat m_Wf; // original format
	CString WaveFormat;
	CWaveSoapFrontDoc * m_pDocument;

	CAudioCompressionManager m_Acm;

	virtual BOOL OnFileNameOK();
	virtual UINT OnShareViolation( LPCTSTR lpszPathName );

	void ShowDlgItem(UINT nID, int nCmdShow);

	void FillFormatCombo(int nSel, int Flags);
	void FillFormatCombo(int nSel)
	{
		FillFormatCombo(nSel,
						m_bCompatibleFormatsOnly ?
							WaveFormatMatchCompatibleFormats
						: WaveFormatMatchFormatTag);
	}

	void FillFormatTagCombo(WaveFormatTagEx const ListOfTags[] = NULL, int NumTags = 0, DWORD Flags = 0);

	void FillWmaFormatArray();
	void FillMp3EncoderArray();
	void FillMp3FormatArray();
	void FillLameEncoderFormats();

	WAVEFORMATEX * GetWaveFormat();
	virtual void OnInitDone();
	virtual void OnTypeChange();
	virtual void OnFileNameChange();
	void SetFileType(int nType);// Wav, Mp3, wma, raw...
	void SetFileType(LPCTSTR lpExt);
	int GetFileTypeForExt(LPCTSTR lpExt);
	int GetFileTypeForName(LPCTSTR FileName);
	//void ClearFileInfoDisplay();

	//{{AFX_MSG(CWaveSoapFileSaveDialog)
	afx_msg void OnCompatibleFormatsClicked();
	afx_msg void OnComboFormatsChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif // AFX_WAVESOAPFILE_DIALOGS_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_
