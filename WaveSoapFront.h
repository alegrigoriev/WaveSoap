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

#include "resource.h"       // main symbols

#include "ApplicationProfile.h"
#include "DirectFile.h"
#include "OperationContext2.h"
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
};

enum
{
	OpenDocumentDirectMode = 2,
	OpenDocumentReadOnly = 4,
	OpenDocumentCreateNewWithWaveformat = 8, // NAME is actually WAVEFORMATEX *
	OpenDocumentCreateNewQueryFormat = 0x10,
	OpenNewDocumentZeroLength = 0x20,
	OpenDocumentNonWavFile = 0x100,
};

class CWaveSoapFrontApp : public CWinApp
{
public:
	CWaveSoapFrontApp();
	void QueueOperation(COperationContext * pContext);
	void LoadStdProfileSettings(UINT nMaxMRU);
	void OnActivateDocument(CWaveSoapFrontDoc * pDocument, BOOL bActivate);

	BOOL CanOpenWindowsMedia() const { return NULL != m_hWMVCORE_DLL_Handle; }
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapFrontApp)
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	static OSVERSIONINFO m_VersionInfo;
	static bool SupportsV5FileDialog();
	double GetProfileDouble(LPCTSTR Section, LPCTSTR ValueName,
							double Default, double MinVal, double MaxVal);
	void WriteProfileDouble(LPCTSTR Section, LPCTSTR ValueName, double value);
	CApplicationProfile Profile;
	CString m_CurrentDir;
	int m_OpenFileDialogFilter;
	CString m_sTempDir;             // File proppage
	int m_DefaultOpenMode;
	enum { DefaultOpenReadOnly = 0,
		DefaultOpenDirect = 1,
		DefaultOpenBuffered = 2, };

	bool m_bOpenMaximized;
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

	CDirectFile::CDirectFileCache * m_FileCache;
	CWaveSoapFrontDoc * m_pActiveDocument;
	COperationContext * m_pFirstOp;
	COperationContext * m_pLastOp;
	CString m_CurrentStatusString;
	CWaveSoapFrontDoc * m_pLastStatusDocument;
	CSimpleCriticalSection m_StatusStringLock;

	void GetStatusStringAndDoc(CString & str, CWaveSoapFrontDoc ** ppDoc);
	void SetStatusStringAndDoc(const CString & str, CWaveSoapFrontDoc * pDoc);

	CWaveFile m_ClipboardFile;
	WAVEFORMATEX m_NewFileFormat;
	int m_NewFileChannels;
	long m_NewFileLength;

	CDocTemplate * m_pAllTypesTemplate;
	CDocTemplate * m_pMP3TypeTemplate;
	CDocTemplate * m_pWavTypeTemplate;
	CDocTemplate * m_pWmaTypeTemplate;

	int m_DefaultPlaybackDevice;
	int m_NumPlaybackBuffers;
	ULONG m_SizePlaybackBuffers;

	int m_DefaultRecordDevice;
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
	BOOL m_bUseMemoryFiles;  // File proppage
	int m_MaxMemoryFileSize;    // in Kbytes
	int m_MaxFileCache;

	BOOL m_bUseCountrySpecificNumberAndTime;
	TCHAR m_TimeSeparator;
	TCHAR m_DecimalPoint;
	TCHAR m_ThousandSeparator;

	BOOL m_DontShowMediaPlayerWarning;

	BOOL m_bSnapMouseSelectionToMax;

	int m_SpectrumSectionWidth;
	int m_FftBandsOrder;
	int m_FftWindowType;

	CString m_UserKey;

	CDocument* OpenDocumentFile(LPCTSTR lpszPathName, int flags);
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszPathName);

	CPalette * GetPalette();
	CPalette   m_Palette;
	void CreatePalette();

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
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	CWinThread m_Thread;
	unsigned _ThreadProc();
	bool volatile m_RunThread;
	HANDLE m_hThreadEvent;
	CSimpleCriticalSection m_cs;
	static UINT AFX_CDECL ThreadProc(PVOID arg)
	{
		return ((CWaveSoapFrontApp *) arg)->_ThreadProc();
	}

	HINSTANCE m_hWMVCORE_DLL_Handle;
};

typedef CWaveSoapFrontApp CWaveSoapApp;
typedef CWaveSoapFrontApp CThisApp;
inline CThisApp * GetApp()
{
	return static_cast<CThisApp *>(AfxGetApp());
}

// long to string, thousands separated by commas
CString LtoaCS(long num);
enum
{
	SampleToString_Sample = 0,
	SampleToString_HhMmSs = 1,
	SampleToString_Seconds = 2,
	SampleToString_Mask = 0xF,
	TimeToHhMmSs_NeedsMs = 0x100,
	TimeToHhMmSs_NeedsHhMm = 0x200,
};
CString TimeToHhMmSs(unsigned TimeMs, int Flags = TimeToHhMmSs_NeedsMs);
CString SampleToString(long Sample, long nSamplesPerSec,
						int Flags = SampleToString_HhMmSs
									| TimeToHhMmSs_NeedsHhMm
									| TimeToHhMmSs_NeedsMs);
void SetStatusString(CCmdUI* pCmdUI, const CString & string,
					LPCTSTR MaxString = NULL, BOOL bForceSize = FALSE);
void NotEnoughMemoryMessageBox();
void NotEnoughDiskSpaceMessageBox();
void NotEnoughUndoSpaceMessageBox();
void FileCreationErrorMessageBox(LPCTSTR name);
void AddStringToHistory(const CString & str, CString history[], int NumItems, bool CaseSensitive = false);
BOOL CanExpandWaveFile(const CWaveFile & WaveFile, long NumOfSamplesToAdd);
BOOL CanExpandWaveFileDlg(const CWaveFile & WaveFile, long NumOfSamplesToAdd);
BOOL CanAllocateWaveFileSamples(const WAVEFORMATEX * pWf, LONGLONG NumOfSamples);
BOOL CanAllocateWaveFileSamplesDlg(const WAVEFORMATEX * pWf, LONGLONG NumOfSamples);
/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#endif // !defined(AFX_WAVESOAPFRONT_H__FFA16C44_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
