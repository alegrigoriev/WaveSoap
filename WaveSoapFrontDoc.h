// WaveSoapFrontDoc.h : interface of the CWaveSoapFrontDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAVESOAPFRONTDOC_H__FFA16C4C_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_WAVESOAPFRONTDOC_H__FFA16C4C_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "WmaFile.h"
struct WavePeak
{
	__int16 low;
	__int16 high;
};

#define ALL_CHANNELS (-1)
class CSelectionUpdateInfo : public CObject
{
public:
	CSelectionUpdateInfo() {}
	~CSelectionUpdateInfo() {}
	int SelBegin;
	int SelEnd;
	int SelChannel;
	int CaretPos;
	int Flags;
};

class CSoundUpdateInfo : public CObject
{
public:
	CSoundUpdateInfo()
		: pNext(NULL)
	{}
	~CSoundUpdateInfo() {}
	CSoundUpdateInfo * pNext;
	DWORD FileID;
	int UpdateCode;
	int Begin;
	int End;
	int Length;
};

// Active document have highest priority for disk-intensive operations.
// while it is executing a disk-intensive command,
// such operations with non-active documents are suspended.
// To implement it, App object starts a thread for such operations.
//
enum {
	UpdateSoundDontRescanPeaks = 1,
	SetSelection_MakeCaretVisible = 1,
	SetSelection_MoveCaretToCenter = 2,
	SetSelection_SnapToMaximum = 4,
	SaveFile_SameName = 4,
	SaveFile_CloseAfterSave = 8,
	SaveFile_SaveCopy = 0x10,
	SaveFile_Mp3File = OpenDocumentMp3File,
	SaveFile_WmaFile = OpenDocumentWmaFile,
	SaveFile_RawFile = OpenDocumentRawFile,
	SaveFile_AviFile = OpenDocumentAviFile,
	SaveFile_NonWavFile = OpenDocumentNonWavFile,
};
enum
{
	MaxInterpolatedLength = 128,
};
class CWaveSoapFrontDoc : public CDocument
{
protected: // create from serialization only
	CWaveSoapFrontDoc();
	DECLARE_DYNCREATE(CWaveSoapFrontDoc)

// Attributes
public:
	BOOL UndoEnabled() const
	{
		return m_bUndoEnabled;
	}
	BOOL RedoEnabled() const
	{
		return m_bRedoEnabled;
	}
	BOOL ChannelsLocked() const
	{
		return m_bChannelsLocked;
	}

	int WaveFileSamples() const
	{
		return m_WavFile.NumberOfSamples();
	}
	LPMMCKINFO WaveDataChunk() const
	{
		return m_WavFile.GetDataChunk();
	}
	LPWAVEFORMATEX WaveFormat() const
	{
		return m_WavFile.GetWaveFormat();
	}
	int WaveChannels() const
	{
		return m_WavFile.Channels();
	}
	int WaveSampleSize() const
	{
		return m_WavFile.SampleSize();
	}
	DWORD WaveFileID() const
	{
		return m_WavFile.GetFileID();
	}
	virtual BOOL IsModified();
	BOOL IsBusy() const
	{
		return m_OperationInProgress != 0;
	}
// Operations
public:
	void IncrementModified(BOOL DeleteRedo = TRUE, int KeepPreviousUndo = -1);
	void DecrementModified();   // called at UNDO
	virtual void SetModifiedFlag(BOOL bModified = TRUE, int KeepPreviousUndo = -1);
	void AddUndoRedo(class CUndoRedoContext * pUndo);
	void EnableUndo(BOOL bEnable = TRUE);
	void EnableRedo(BOOL bEnable = TRUE);
	void DeletePermanentUndoRedo();
	BOOL InitUndoRedo(class CUndoRedoContext * pContext);
	BOOL OnSaveDocument(LPCTSTR lpszPathName, DWORD flags, WAVEFORMATEX * pWf);

	// flags OpenDocumentReadOnly - ReadOnly, 2 - DirectMode
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName, int flags);
// Overrides
	virtual BOOL OnNewDocument(WAVEFORMATEX * pWfx, long InitialLengthSeconds);
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaveSoapFrontDoc)
public:
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);
protected:
	virtual BOOL SaveModified();
	//}}AFX_VIRTUAL
	virtual void OnIdle();
	virtual BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace = TRUE);
	BOOL OpenWmaFileDocument(LPCTSTR lpszPathName);
	BOOL OpenRawFileDocument(LPCTSTR lpszPathName);
	BOOL OpenMp3FileDocument(LPCTSTR lpszPathName)
	{
		return OpenWmaFileDocument(lpszPathName);
	}
	BOOL OpenAviFileDocument(LPCTSTR lpszPathName);
	BOOL OpenNonWavFileDocument(LPCTSTR lpszPathName, int flags);
public:
	CString m_szWaveFilename;
	CString szWaveTitle;
	// wave file currently containing the data (it could be the temp file)
	CWaveFile m_WavFile;
	// original file with the name which was open
	// for new file, it isn't open
	CWaveFile m_OriginalWavFile;
	int m_ModificationSequenceNumber;

	//WAVEFORMATEX WavFileFormat;

	WavePeak * m_pPeaks;
	size_t m_WavePeakSize;
	size_t m_AllocatedWavePeakSize;
	int m_PeakDataGranularity;
	CSimpleCriticalSection m_PeakLock;
	void RescanPeaks(long begin, long end);
	BOOL AllocatePeakData(long NewNumberOfSamples);

	LONG m_CaretPosition;
	LONG m_SelectionStart;
	LONG m_SelectionEnd;
	int m_SelectedChannel; // 0, 1, 2
	bool m_TimeSelectionMode;
	void SetSelection(long begin, long end, int channel, int caret, int flags = 0);
	void SoundChanged(DWORD FileID, long begin, long end, int length = -1, DWORD flags = 0);
	void PlaybackPositionNotify(long position, int channel);

	enum {UpdateSelectionChanged = 1,
		UpdateSelectionModeChanged,
		UpdateSoundChanged,
		UpdateSampleRateChanged,
		UpdateWholeFileChanged,
		UpdatePlaybackPositionChanged,
	};

// Implementation
public:
	virtual ~CWaveSoapFrontDoc();
	void UpdateDocumentTitle();
	void LoadPeaksForCompressedFile(CWaveFile & WaveFile, CWaveFile & OriginalWaveFile);
	void CheckAndLoadPeakFile(CWaveFile & WaveFile);
	void BuildPeakInfo(BOOL bSavePeakFile);
	void GetSoundMinMax(int & MinL, int & MaxL, int & MinR, int & MaxR, long begin, long end);
	int CalculatePeakInfoSize() const
	{
		return (WaveFileSamples() + m_PeakDataGranularity - 1)
			/ m_PeakDataGranularity * WaveChannels();
	}
	BOOL SetThreadPriority(int priority)
	{
		return m_Thread.SetThreadPriority(priority);
	}

	void SavePeakInfo(CWaveFile & WaveFile, CWaveFile & SavedWaveFile);
	BOOL OpenWaveFile(CWaveFile & WaveFile, LPCTSTR szName, DWORD flags);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	long m_OperationInProgress;
	bool volatile m_StopOperation;
	bool m_OperationNonCritical;
	bool m_PlayingSound;
	bool m_bInOnIdle;
	bool m_bUndoEnabled;
	bool m_bRedoEnabled;
	bool m_bChannelsLocked;
	int m_PrevChannelToCopy;
	int m_DefaultPasteMode;

	CString m_CurrentStatusString;
	CSimpleCriticalSection m_StatusStringLock;
	void GetCurrentStatusString(CString & str);
	void SetCurrentStatusString(const CString & str);

	bool m_bReadOnly;
	bool m_bDirectMode;
	bool m_bClosing;    // tells OnSaveDocument that it is called in close context
	bool m_bClosePending;   // Save is in progress, request close afterwards
	bool m_bCloseThisDocumentNow;   // CDocTemplate should close it in OnIdle
	CSimpleCriticalSection m_cs;
	COperationContext * m_pCurrentContext;
	COperationContext * m_pUndoList;
	COperationContext * m_pRedoList;
	COperationContext * m_pRetiredList;
	CSoundUpdateInfo * m_pUpdateList;

protected:
	HANDLE m_hThreadEvent;
	bool volatile m_bRunThread;
	CWinThread m_Thread;
	UINT _ThreadProc();
	static UINT AFX_CDECL ThreadProc(LPVOID arg)
	{
		CWaveSoapFrontDoc * pDoc = DYNAMIC_DOWNCAST(CWaveSoapFrontDoc, (CObject *)arg);
		if (NULL == pDoc)
		{
			return -1;
		}
		return pDoc->_ThreadProc();
	}


// Generated message map functions
public:
	void OnActivateDocument(BOOL bActivate);
	void DoEditPaste()
	{
		OnEditPaste();
	}
	void QueueOperation(COperationContext * pContext);
	void UpdateFrameTitles()
	{
		UpdateFrameCounts();
	}
	void PostFileSave(CFileSaveContext * pContext);
	BOOL PostCommitFileSave(int flags, LPCTSTR FullTargetName);
protected:
	// save the selected area to the permanent or temporary file
	BOOL CanCloseFrame(CFrameWnd* pFrameArg);
	void DoCopy(LONG Start, LONG End, LONG Channel, LPCTSTR FileName);
	void DoPaste(LONG Start, LONG End, LONG Channel, LPCTSTR FileName);
	void DoCut(LONG Start, LONG End, LONG Channel);
	void DoDelete(LONG Start, LONG End, LONG Channel);
	void DeleteUndo();
	void DeleteRedo();

	void OnUpdateSampleRate(CCmdUI* pCmdUI, unsigned SampleRate);
	void SetSampleRate(unsigned SampleRate);
	void ChangeChannels(int nChannels);

	//{{AFX_MSG(CWaveSoapFrontDoc)
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditStop();
	afx_msg void OnUpdateEditStop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditSelection();
	afx_msg void OnUpdateEditSelection(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSoundPlay(CCmdUI* pCmdUI);
	afx_msg void OnSoundPlay();
	afx_msg void OnUpdateSoundStop(CCmdUI* pCmdUI);
	afx_msg void OnSoundStop();
	afx_msg void OnStopAll();
	afx_msg void OnSoundPause();
	afx_msg void OnPlayAndStop();
	afx_msg void OnUpdateSoundPause(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorFileSize(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorCurrentPos(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorSelectionLength(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorSampleRate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorSampleSize(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorChannels(CCmdUI* pCmdUI);
	afx_msg void OnEditDelete();
	afx_msg void OnUpdateEditDelete(CCmdUI* pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnEditChannelsLock();
	afx_msg void OnUpdateEditChannelsLock(CCmdUI* pCmdUI);
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	afx_msg void OnSample16bit();
	afx_msg void OnUpdateSample16bit(CCmdUI* pCmdUI);
	afx_msg void OnSample8bit();
	afx_msg void OnUpdateSample8bit(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate11025();
	afx_msg void OnUpdateSamplerate11025(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate16000();
	afx_msg void OnUpdateSamplerate16000(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate22050();
	afx_msg void OnUpdateSamplerate22050(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate32000();
	afx_msg void OnUpdateSamplerate32000(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate44100();
	afx_msg void OnUpdateSamplerate44100(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate48k();
	afx_msg void OnUpdateSamplerate48k(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate7200();
	afx_msg void OnUpdateSamplerate7200(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate8000();
	afx_msg void OnUpdateSamplerate8000(CCmdUI* pCmdUI);
	afx_msg void OnSamplerate96k();
	afx_msg void OnUpdateSamplerate96k(CCmdUI* pCmdUI);
	afx_msg void OnChannelsMono();
	afx_msg void OnUpdateChannelsMono(CCmdUI* pCmdUI);
	afx_msg void OnChannelsStereo();
	afx_msg void OnUpdateChannelsStereo(CCmdUI* pCmdUI);
	afx_msg void OnProcessChangevolume();
	afx_msg void OnUpdateProcessChangevolume(CCmdUI* pCmdUI);
	afx_msg void OnViewStatusHhmmss();
	afx_msg void OnViewStatusSamples();
	afx_msg void OnViewStatusSeconds();
	afx_msg void OnUpdateProcessDcoffset(CCmdUI* pCmdUI);
	afx_msg void OnProcessDcoffset();
	afx_msg void OnUpdateProcessInsertsilence(CCmdUI* pCmdUI);
	afx_msg void OnProcessInsertsilence();
	afx_msg void OnUpdateProcessMute(CCmdUI* pCmdUI);
	afx_msg void OnProcessMute();
	afx_msg void OnUpdateProcessNormalize(CCmdUI* pCmdUI);
	afx_msg void OnProcessNormalize();
	afx_msg void OnUpdateProcessResample(CCmdUI* pCmdUI);
	afx_msg void OnProcessResample();
	afx_msg void OnFileStatistics();
	afx_msg void OnUpdateFileStatistics(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditGoto(CCmdUI* pCmdUI);
	afx_msg void OnEditGoto();
	afx_msg void OnUpdateProcessInvert(CCmdUI* pCmdUI);
	afx_msg void OnProcessInvert();
	afx_msg void OnUpdateViewRescanPeaks(CCmdUI* pCmdUI);
	afx_msg void OnViewRescanPeaks();
	afx_msg void OnUpdateProcessSynthesisExpressionEvaluation(CCmdUI* pCmdUI);
	afx_msg void OnProcessSynthesisExpressionEvaluation();
	afx_msg void OnUpdateViewStatusHhmmss(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewStatusSamples(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewStatusSeconds(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSaveCopyAs(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveCopyAs();
	afx_msg void OnUpdateToolsInterpolate(CCmdUI* pCmdUI);
	afx_msg void OnToolsInterpolate();
	afx_msg void OnUpdateProcessDoUlf(CCmdUI* pCmdUI);
	afx_msg void OnProcessDoUlf();
	afx_msg void OnUpdateProcessDoDeclicking(CCmdUI* pCmdUI);
	afx_msg void OnProcessDoDeclicking();
	afx_msg void OnUpdateProcessNoiseReduction(CCmdUI* pCmdUI);
	afx_msg void OnProcessNoiseReduction();
	afx_msg void OnFileClose();
	afx_msg void OnProcessChannels();
	afx_msg void OnUpdateProcessChannels(CCmdUI* pCmdUI);
	afx_msg void OnSamplerateCustom();
	afx_msg void OnUpdateSamplerateCustom(CCmdUI* pCmdUI);
	afx_msg void OnEditEnableUndo();
	afx_msg void OnUpdateEditEnableUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditEnableRedo();
	afx_msg void OnUpdateEditEnableRedo(CCmdUI* pCmdUI);
	afx_msg void OnEditClearUndo();
	afx_msg void OnUpdateEditClearUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditClearRedo();
	afx_msg void OnUpdateEditClearRedo(CCmdUI* pCmdUI);
	afx_msg void OnProcessEqualizer();
	afx_msg void OnUpdateProcessEqualizer(CCmdUI* pCmdUI);
	afx_msg void OnProcessSwapchannels();
	afx_msg void OnUpdateProcessSwapchannels(CCmdUI* pCmdUI);
	afx_msg void OnProcessFilter();
	afx_msg void OnUpdateProcessFilter(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

#pragma pack(push, 2)
struct PeakFileHeader
{
	DWORD dwSignature;
	WORD wSize;
	enum { pfhSignature = 'KPSW', pfhMaxVersion=2};
	WORD dwVersion;
	FILETIME WaveFileTime;
	DWORD dwWaveFileSize;   // WAV file is less than 4G
	DWORD Granularity;      // number of WAV samples for each PeakFile value
	DWORD PeakInfoSize;
	DWORD NumOfSamples;
	WAVEFORMATEX wfFormat;
};

#pragma pack(1)
struct MP3_IDTAG
{
	char Tag[3];    // "TAG"
	char Title[30];
	char Artist[30];
	char Album[30];
	char Year[4];
	union {
		char CommentV0[30];
		struct {
			char CommentV1[28];
			char Track[2];
		};
	};
	char Genre;
};
#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVESOAPFRONTDOC_H__FFA16C4C_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
