// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
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
	typedef CFileDialogWithHistory BaseClass;
public:
	CWaveSoapFileOpenDialog(BOOL bOpenFileDialog = TRUE, // TRUE for FileOpen, FALSE for FileSaveAs
							LPCTSTR lpszDefExt = NULL,
							LPCTSTR lpszFileName = NULL,
							DWORD dwFlags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
							LPCTSTR lpszFilter = NULL,
							CWnd* pParentWnd = NULL);

	CWaveFile m_WaveFile;
	//CString GetNextPathName(POSITION& pos) const;

	bool m_bReadOnly;
	bool m_bDirectMode;

	int nChannels;
	int nBitsPerSample;
	int nSamplingRate;
	int nSamples;
	CString WaveFormat;
	unsigned m_MinWmaFilter;
	unsigned m_MaxWmaFilter;
	unsigned m_PrevFilter;

	virtual BOOL OnFileNameOK();

	virtual void OnInitDone();
	virtual void OnFileNameChange();
	virtual void OnTypeChange();
	void ClearFileInfoDisplay();
	void ShowWmaFileInfo(CDirectFile & File);
	//{{AFX_MSG(CWaveSoapFileOpenDialog)
	afx_msg void OnCheckReadOnly();
	afx_msg void OnCheckDirectMode();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
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

class CFileSaveUiSupport
{
public:

	CFileSaveUiSupport(CWaveFormat const & Wf);
	WAVEFORMATEX * GetWaveFormat();
	unsigned GetFileTypeForName(LPCTSTR FileName);

	int GetSelectedRawFormat() const
	{
		return m_SelectedRawFormat;
	}
	int GetFileType() const  // Wav, Mp3, wma, raw...
	{
		return m_FileType;
	}
	int GetFileTypeFlags() const;

protected:

	CApplicationProfile m_Profile;

	//CWaveFile m_WaveFile;
	CComboBox m_FormatTagCombo;
	CComboBox m_AttributesCombo;
	WaveFormatTagEx m_SelectedTag;
	unsigned m_SelectedFormat;
	// m_FileType is one of SaveFile_WavFile, SaveFile_Mp3File, SaveFile_WmaFile
	ULONG m_FileType;// Wav, Mp3, wma, raw...
	int m_SelectedRawFormat;

	unsigned m_SelectedMp3Encoder;
	int m_SelectedMp3Bitrate;
	int m_SelectedWmaBitrate;
	int m_SelectedBitrate;

	BOOL m_bCompatibleFormatsOnly;

	CString m_FormatTagName;
	static unsigned const MaxFileTypes = 10;

	CString m_DefExt[MaxFileTypes];
	CString m_FileTypeStrings[MaxFileTypes];
	CString m_FileTypeFilters[MaxFileTypes];
	ULONG m_TemplateFlags[MaxFileTypes];
	unsigned m_NumOfFileTypes;

	CWaveFormat m_Wf; // original format
	CString WaveFormat;

	CAudioCompressionManager m_Acm;
	int FillFormatCombo(unsigned nSel, int Flags);
	int FillFormatCombo(unsigned nSel)
	{
		return FillFormatCombo(nSel,
								m_bCompatibleFormatsOnly ?
									WaveFormatMatchCompatibleFormats
								: WaveFormatMatchFormatTag);
	}

	void FillFormatTagCombo(WaveFormatTagEx const ListOfTags[] = NULL, int NumTags = 0, DWORD Flags = 0);

	void FillWmaFormatCombo();
	void FillMp3EncoderCombo();
	void FillRawFormatsCombo();
	void FillLameEncoderFormats();
	unsigned GetFileTypeForExt(LPCTSTR lpExt);

	afx_msg void OnCompatibleFormatsClicked();
	afx_msg void OnComboFormatsChange();
	afx_msg void OnComboAttributesChange();

	static WaveFormatTagEx const ExcludeFormats[];
};

class CWaveSoapFileSaveDialog : public CFileDialogWithHistory, public CFileSaveUiSupport
{
	typedef CFileDialogWithHistory BaseClass;
public:
	CWaveSoapFileSaveDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
							CWaveFormat const & Wf,
							CWaveSoapFrontDoc * pDoc,
							LPCTSTR lpszDefExt = NULL,
							LPCTSTR lpszFileName = NULL,
							DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							LPCTSTR lpszFilter = NULL,
							CWnd* pParentWnd = NULL);

	~CWaveSoapFileSaveDialog();

	virtual BOOL OnFileNameOK();
	virtual UINT OnShareViolation( LPCTSTR lpszPathName );

	void ShowDlgItem(UINT nID, int nCmdShow);
	void AddAllTypeFilters(CDocManager * pDocManager);

protected:
	CWaveSoapFrontDoc * m_pDocument;
	CString m_strFilter;
	CString m_strDefaultExt;

	virtual void OnInitDone();
	virtual void OnTypeChange();
	virtual void OnFileNameChange();

	// the function argument is one of SaveFile_WavFile, SaveFile_Mp3File, SaveFile_WmaFile
	void SetFileType(ULONG nType);
	void SetFileType(LPCTSTR lpExt);
	//void ClearFileInfoDisplay();

	//{{AFX_MSG(CWaveSoapFileSaveDialog)
	afx_msg void OnCompatibleFormatsClicked();
	afx_msg void OnComboFormatsChange();
	afx_msg void OnComboAttributesChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif // AFX_WAVESOAPFILE_DIALOGS_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_
