// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveSoapFront.h : main header file for the WAVESOAPFRONT application
//

#if !defined(AFX_WAVESOAPFRONT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_WAVESOAPFRONT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "ApplicationProfile.h"
#include "WaveFile.h"
#include "KListEntry.h"
#include "LocaleUtilities.h"
/////////////////////////////////////////////////////////////////////////////
// CWaveSoapFrontApp:
// See WaveSoapFront.cpp for the implementation of this class
//
class CWaveSoapFrontStatusBar : public CStatusBar
{
public:
	CWaveSoapFrontStatusBar() {}
	~CWaveSoapFrontStatusBar() {}
	virtual int OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	//{{AFX_MSG(CWaveSoapFrontStatusBar)
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint pos );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
};

enum
{
	OpenDocumentDirectMode = 2,
	OpenDocumentReadOnly = 4,
	OpenDocumentDefaultMode = OpenDocumentDirectMode | OpenDocumentReadOnly,
	OpenDocumentModeFlagsMask = OpenDocumentDirectMode | OpenDocumentReadOnly,
	OpenDocumentCreateNewWithParameters = 8, // NAME is actually NewFileParameters *
	OpenDocumentCreateNewFromCWaveFile = 0x10,// use m_pFile from NewFileParameters
	OpenDocumentMp3File = 0x1000,
	OpenDocumentWmaFile = 0x2000,
	OpenDocumentRawFile = 0x4000,
	OpenDocumentAviFile = 0x8000,
	OpenRawFileMsbFirst = 0x10000,
	OpenDocumentNonWavFile = OpenDocumentMp3File | OpenRawFileMsbFirst
							| OpenDocumentWmaFile | OpenDocumentRawFile | OpenDocumentAviFile,
};

struct NewFileParameters
{
	CWaveFormat pWf;
	LPCTSTR m_pInitialName;
	NUMBER_OF_SAMPLES m_InitialSamples;
	DWORD m_FileTypeFlags;
	CWaveFile * m_pFile;

	NewFileParameters(WAVEFORMATEX const * pf,
					NUMBER_OF_SAMPLES nSamples = 0)
		: m_pInitialName(NULL)
		, m_InitialSamples(nSamples)
		, m_FileTypeFlags(0)
		, m_pFile(NULL)
		, pWf(pf)
	{
	}
};

class CWaveSoapDocTemplate : public CMultiDocTemplate
{
	typedef CMultiDocTemplate BaseClass;
public:
	CWaveSoapDocTemplate( UINT nIDResource, UINT nIDStringResource,
						CRuntimeClass* pDocClass,
						CRuntimeClass* pFrameClass,
						CRuntimeClass* pViewClass,
						DWORD OpenFlags)
		:BaseClass(nIDResource, pDocClass, pFrameClass, pViewClass),
		m_OpenDocumentFlags(OpenFlags)
	{
		if ( ! m_strDocStrings.LoadString(nIDStringResource))
		{
			m_strDocStrings.LoadString(m_nIDResource);
		}
	}
	~CWaveSoapDocTemplate() {}
	virtual CDocument* OpenDocumentFile( LPCTSTR lpszPathName, int flags = 1
											//BOOL bMakeVisible = TRUE
										);

	virtual void OnIdle();
	void BroadcastUpdate(UINT lHint);
	BOOL IsAnyDocumentModified();
	BOOL CanSaveAnyDocument();
	void SaveAll();
protected:
	Confidence MatchDocType(LPCTSTR lpszPathName, CDocument*& rpDocMatch);
	DWORD m_OpenDocumentFlags;
};

class COperationContext;

class CWaveSoapFrontApp : public CWinApp,
	public DirectFileParameters,
	public LocaleParameters
{
	typedef CWinApp BaseClass;
public:
	CWaveSoapFrontApp();
	void QueueOperation(COperationContext * pContext);
	void LoadStdProfileSettings(UINT nMaxMRU);
	void OnActivateDocument(class CWaveSoapFrontDoc * pDocument, BOOL bActivate);

	BOOL CanOpenWindowsMedia() const { return NULL != m_hWMVCORE_DLL_Handle; }
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapFrontApp)
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	CApplicationProfile Profile;
	CString m_CurrentDir;
	int m_OpenFileDialogFilter;
	int m_DefaultOpenMode;
	enum { DefaultOpenReadOnly = 0,
		DefaultOpenDirect = 1,
		DefaultOpenBuffered = 2, };

	bool m_bOpenChildMaximized;
	bool m_bShowToolbar;
	bool m_bShowStatusBar;
	bool m_bShowNewFormatDialogWhenShiftOnly;
	bool m_bAllow4GbWavFile;      // File proppage
	// display colors:
	union {
		struct {
			DWORD m_WaveBackground;
			DWORD m_SelectedWaveBackground;
			DWORD m_WaveColor;
			DWORD m_6dBLineColor;
			DWORD m_ZeroLineColor;
			DWORD m_ChannelSeparatorColor;
			DWORD m_InterpolatedColor;
			DWORD m_SelectedWaveColor;
			DWORD m_Selected6dBLineColor;
			DWORD m_SelectedZeroLineColor;
			DWORD m_SelectedChannelSeparatorColor;
			DWORD m_SelectedInterpolatedColor;
			DWORD m_MarkerColor;
			DWORD m_SelectedMarkerColor;
			DWORD m_RegionColor;
			DWORD m_SelectedRegionColor;
		};
		DWORD AppColors[16];
	};

	int m_SoundTimeFormat;

	CDirectFileCacheProxy * m_FileCache;
	CWaveSoapFrontDoc * m_pActiveDocument;

	LockedListHead<COperationContext> m_OpList;

	CString m_CurrentStatusString;
	CWaveSoapFrontDoc * m_pLastStatusDocument;
	CSimpleCriticalSection m_StatusStringLock;

	void GetStatusStringAndDoc(CString & str, CWaveSoapFrontDoc ** ppDoc);
	void SetStatusStringAndDoc(const CString & str, CWaveSoapFrontDoc * pDoc);

	CWaveFile m_ClipboardFile;
	WAVEFORMATEX m_NewFileFormat;
	LONG m_NewFileLength;   // seconds

	class CWaveSoapDocTemplate * m_pAllTypesTemplate;
	class CWaveSoapDocTemplate * m_pMP3TypeTemplate;
	class CWaveSoapDocTemplate * m_pWavTypeTemplate;
	class CWaveSoapDocTemplate * m_pWmaTypeTemplate;
	class CWaveSoapDocTemplate * m_pAviTypeTemplate;
	class CWaveSoapDocTemplate * m_pAllWmTypeTemplate;
	class CWaveSoapDocTemplate * m_pRawTypeTemplate;

	INT_PTR m_DefaultPlaybackDevice;
	int m_NumPlaybackBuffers;
	ULONG m_SizePlaybackBuffers;

	INT_PTR m_DefaultRecordDevice;
	int m_NumRecordBuffers;
	ULONG m_SizeRecordBuffers;

	BOOL m_bReadOnly;
	BOOL m_bDirectMode;
	// Undo/Redo parameters
	BOOL m_bUndoEnabled;              // File proppage
	BOOL m_bRedoEnabled;              // File proppage
	int m_MaxUndoDepth;               // File proppage
	int m_MaxRedoDepth;               // File proppage
	DWORD m_MaxUndoSize;              // File proppage
	DWORD m_MaxRedoSize;              // File proppage
	BOOL m_bEnableUndoLimit;          // File proppage
	BOOL m_bEnableRedoLimit;          // File proppage
	BOOL m_bEnableUndoDepthLimit;     // File proppage
	BOOL m_bEnableRedoDepthLimit;     // File proppage
	BOOL m_bRememberSelectionInUndo;  // File proppage

	BOOL m_bUseCountrySpecificNumberAndTime;

	BOOL m_DontShowMediaPlayerWarning;

	BOOL m_bSnapMouseSelectionToMax;
	int m_LastPrefsPropertyPageSelected;

	int m_SpectrumSectionWidth;
	int m_FftBandsOrder;
	int m_FftWindowType;

	int m_PasteResampleMode; // Edit/Paste to different rate: 0 - resample, 1- paste as is
	int m_DefaultPasteMode; // 0 - replace selection, 1 - paste just selection length, 2 - insert

	BOOL m_UseFadeInOut;
	int m_FadeInOutLengthMs;
	int m_FadeInEnvelope;
	int m_FadeOutEnvelope;

	CString m_UserKey;

	CDocument* OpenDocumentFile(LPCTSTR lpszPathName, int flags);
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszPathName);

	CPalette * GetPalette();
	CPalette   m_Palette;
	void CreatePalette();
	BOOL GetMessageString(UINT nID, CString& rMessage);

	void BroadcastUpdate(UINT lHint = 0);
	BOOL IsAnyDocumentModified();
	BOOL CanSaveAnyDocument();

	CString m_NotEnoughMemoryMsg;

	//{{AFX_MSG(CWaveSoapFrontApp)
	afx_msg void OnFileNew();
	afx_msg void OnAppAbout();
	afx_msg void OnEditPasteNew();
	afx_msg void OnUpdateEditPasteNew(CCmdUI* pCmdUI);
	afx_msg BOOL OnOpenRecentFile(UINT nID);
	afx_msg void OnToolsCdgrab();
	afx_msg void OnFileSaveAll();
	afx_msg void OnUpdateFileSaveAll(CCmdUI* pCmdUI);
	afx_msg void OnToolsOptions();
	afx_msg void OnFileBatchconversion();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CWinThread m_Thread;
	unsigned _ThreadProc();
	bool volatile m_RunThread;
	HANDLE m_hThreadEvent;
//    CSimpleCriticalSection m_cs;
	static UINT AFX_CDECL ThreadProc(PVOID arg)
	{
		return ((CWaveSoapFrontApp *) arg)->_ThreadProc();
	}

	HINSTANCE m_hWMVCORE_DLL_Handle;
public:
};

// this class brings a document frame temporary to the top,
// then restores it back
class CDocumentPopup
{
public:
	CDocumentPopup(CDocument * pDoc);
	~CDocumentPopup();
protected:
	CFrameWnd * m_pPopupFrame;
	CFrameWnd * m_pFrameAbove;
};

typedef CWaveSoapFrontApp CWaveSoapApp;
typedef CWaveSoapFrontApp CThisApp;
inline CThisApp * GetApp()
{
	return static_cast<CThisApp *>(AfxGetApp());
}

// long to string, thousands separated by commas
void SetStatusString(CCmdUI* pCmdUI, const CString & string,
					LPCTSTR MaxString = NULL, BOOL bForceSize = FALSE);
void SetStatusString(CCmdUI* pCmdUI, UINT id, // string resource ID
					LPCTSTR MaxString = NULL, BOOL bForceSize = FALSE);

BOOL VerifyCreateDirectory(LPCTSTR pszPath);
void NotEnoughMemoryMessageBox();
void NotEnoughDiskSpaceMessageBox();
void NotEnoughUndoSpaceMessageBox();
void FileCreationErrorMessageBox(LPCTSTR name);

BOOL CanExpandWaveFile(const CWaveFile & WaveFile, NUMBER_OF_SAMPLES NumOfSamplesToAdd);
BOOL CanExpandWaveFileDlg(const CWaveFile & WaveFile, NUMBER_OF_SAMPLES NumOfSamplesToAdd);
BOOL CanAllocateWaveFileSamples(const WAVEFORMATEX * pWf, LONGLONG NumOfSamples);
BOOL CanAllocateWaveFileSamplesDlg(const WAVEFORMATEX * pWf, LONGLONG NumOfSamples);
CString GetSelectionText(SAMPLE_INDEX Start, SAMPLE_INDEX End, CHANNEL_MASK Chan,
						NUMBER_OF_CHANNELS nChannels, BOOL bLockChannels,
						int nSamplesPerSec, int TimeFormat);

CString LoadCString(UINT id);
void SetWindowIcons(CWnd * pWnd, UINT id);

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_WAVESOAPFRONT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
