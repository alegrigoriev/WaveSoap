// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveFile.h
#ifndef WAVE_FILE__H__
#define WAVE_FILE__H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <vector>
/*
Class CWaveFile serves as a WAV file input/output
helper.
*/
#ifndef __AFX_H__
 #include <afx.h>
#endif
#include <Afxmt.h>
/*
  Base class supports:
  Ascend and descend to a chunk
  Automatic descend to RIFF chunk
  Data read/write

*/
#include <mmsystem.h>
#include "DirectFile.h"
#include "WaveSupport.h"
#include <atlbase.h>
#include <atlpath.h>

typedef long SAMPLE_INDEX;
typedef long NUMBER_OF_SAMPLES; // MUST BE SIGNED!
typedef unsigned PEAK_INDEX;

typedef DWORD WAV_FILE_SIZE;
typedef LONGLONG MEDIA_FILE_SIZE;  // to be expanded to 64 bits
typedef ULONGLONG MEDIA_FILE_POSITION;
typedef MEDIA_FILE_POSITION SAMPLE_POSITION;

#define LAST_SAMPLE (NUMBER_OF_SAMPLES(-1))
#define LAST_SAMPLE_POSITION (SAMPLE_POSITION(-1))

enum
{
	MmioFileOpenExisting = CDirectFile::OpenExisting,
	MmioFileOpenCreateNew = CDirectFile::CreateNew,
	MmioFileOpenCreateAlways = CDirectFile::CreateAlways,
	MmioFileOpenReadOnly = CDirectFile::OpenReadOnly,
	// if couldn't be opened for writing, try read-only
	MmioFileAllowReadOnlyFallback = CDirectFile::OpenAllowReadOnlyFallback,
	MmioFileOpenDeleteAfterClose = CDirectFile::OpenDeleteAfterClose,
	MmioFileOpenDontCreateRiff = 0x80000000,
	MmioFileOpenDontLoadRiff = MmioFileOpenDontCreateRiff,
	MmioFileMemoryFile = CDirectFile::CreateMemoryFile,
};

#pragma pack(push, 2)
struct PeakFileHeader
{
	enum { pfhSignature = 'KPSW', pfhMaxVersion = 4};
	DWORD dwSignature;
	WORD wSize;
	WORD dwVersion;
	FILETIME WaveFileTime;
	MEDIA_FILE_SIZE dwWaveFileSize;   // WAV file is less than 4G
	DWORD Granularity;      // number of WAV samples for each PeakFile value
	DWORD PeakInfoSize;
	NUMBER_OF_SAMPLES NumOfSamples;
	WAVEFORMATEX wfFormat;
};

#pragma pack(pop)

typedef struct _MMCKINFO64
{
	FOURCC          ckid;           /* chunk ID */
	DWORD           cksize;         /* chunk size */
	FOURCC          fccType;        /* form type or list type */
	DWORD           dwFlags;        /* flags used by MMIO functions */
	MEDIA_FILE_POSITION  dwDataOffset;   /* offset of data portion of chunk */
} MMCKINFO64, *PMMCKINFO64, NEAR *NPMMCKINFO64, FAR *LPMMCKINFO64;
typedef const MMCKINFO64 *LPCMMCKINFO64;

#ifdef USE_MMCKINFO64
#define _MMCKINFO _MMCKINFO64
#define MMCKINFO MMCKINFO64
#define PMMCKINFO PMMCKINFO64
#define NPMMCKINFO NPMMCKINFO64
#define LPMMCKINFO LPMMCKINFO64
#define LPCMMCKINFO LPCMMCKINFO64

// must use custom mmio
#define OVERRIDE_MMLIB_FUNCTIONS
#endif

class CMmioFile : public CDirectFile
{
	typedef CDirectFile BaseClass;
public:
	// construction
	CMmioFile();
	virtual BOOL Open(LPCTSTR lpszFileName, long nOpenFlags);
	virtual void Close();
	CMmioFile & operator =(CMmioFile &);

#ifndef OVERRIDE_MMLIB_FUNCTIONS
	// read/write:
	LONG Read( void* lpBuf, LONG nCount )
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return mmioRead(m_hmmio, (HPSTR) lpBuf, nCount);
	}

	LONG Write( const void* lpBuf, LONG nCount )
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return mmioWrite(m_hmmio, (const char*)lpBuf, nCount);
	}
	LONG Seek(LONG lOffset, int iOrigin = SEEK_SET)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return mmioSeek(m_hmmio, lOffset, iOrigin);
	}

	void Flush(UINT flag = 0)   // possible value: MMIO_EMPTYBUF
	{
		CSimpleCriticalSectionLock lock(m_cs);
		mmioFlush(m_hmmio, flag);
	}

	BOOL Ascend(MMCKINFO & ck)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return MMSYSERR_NOERROR == mmioAscend(m_hmmio, & ck, 0);
	}

	BOOL Descend(MMCKINFO & ck, LPMMCKINFO lpckParent, UINT uFlags = 0)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return MMSYSERR_NOERROR == mmioDescend(m_hmmio, & ck, lpckParent, uFlags);
	}

	BOOL CreateChunk(MMCKINFO & ck, UINT wFlags = 0)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		ck.cksize = 0;
		return MMSYSERR_NOERROR == mmioCreateChunk(m_hmmio, & ck, wFlags);
	}
	BOOL GetInfo(MMIOINFO & mmioinfo, UINT wFlags = 0)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return MMSYSERR_NOERROR == mmioGetInfo(m_hmmio, & mmioinfo, wFlags);
	}

	BOOL Advance(LPMMIOINFO mmio, UINT uFlags)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return MMSYSERR_NOERROR == mmioAdvance(m_hmmio, mmio, uFlags);
	}

	BOOL IsOpen() const
	{
		return m_hmmio != NULL;
	}

#else
	void Flush(UINT = 0)   // possible value: MMIO_EMPTYBUF
	{
	}

	BOOL Descend(MMCKINFO & ck, LPMMCKINFO lpckParent, UINT uFlags = 0);

	BOOL Ascend(MMCKINFO & ck);

	BOOL CreateChunk(MMCKINFO & ck, UINT wFlags = 0);

#endif

	BOOL FindChunk(MMCKINFO & ck, LPMMCKINFO lpckParent)
	{
		return Descend(ck, lpckParent, MMIO_FINDCHUNK);
	}
	BOOL FindList(MMCKINFO & ck, LPMMCKINFO lpckParent)
	{
		return Descend(ck, lpckParent, MMIO_FINDLIST);
	}
	BOOL FindRiff()
	{
		return Descend( * GetRiffChunk(), NULL, MMIO_FINDRIFF);
	}

	BOOL CreateList(MMCKINFO & ck)
	{
		return CreateChunk(ck, MMIO_CREATELIST);
	}
	BOOL CreateRiff(MMCKINFO & ck)
	{
		return CreateChunk(ck, MMIO_CREATERIFF);
	}

	virtual ~CMmioFile();

	FOURCC m_RiffckType;
	CSimpleCriticalSection m_cs;
	HMMIO m_hmmio;

	struct InstanceDataMm : public BaseClass::InstanceData
	{
	private:
		typedef BaseClass::InstanceData BaseClass;
	public:
		MMCKINFO riffck;
		HMMIO m_hmmio;
		InstanceDataMm()
		{
			m_hmmio = NULL;
			memzero(riffck);
			m_size = sizeof *this;
		}
		InstanceDataMm & operator =(InstanceData const & src)
		{
			if (src.m_size >= sizeof *this)
			{
				InstanceDataMm const* src1 =
					static_cast<InstanceDataMm const*>( & src);
				riffck = src1->riffck;
				m_hmmio = src1->m_hmmio;
			}
			BaseClass::operator=(src);
			return *this;
		}
		InstanceDataMm & operator =(InstanceDataMm const & src)
		{
			return operator =(static_cast<InstanceData const &>(src));
		}
		virtual void MoveDataTo(InstanceData * dst)
		{
			InstanceDataMm * dst1 = static_cast<InstanceDataMm *>(dst);
			dst1->operator=(*this);
		}
	};

	InstanceDataMm * GetInstanceData() const
	{
		return static_cast<InstanceDataMm *>(CDirectFile::GetInstanceData());
	}
	LPMMCKINFO GetRiffChunk() const
	{
		return & GetInstanceData()->riffck;
	}

	BOOL LoadRiffChunk();
	BOOL ReadChunkString(ULONG Length, CStringA & String);
	BOOL ReadChunkString(ULONG Length, CStringW & String);
	BOOL ReadChunkStringW(ULONG Length, CStringW & String);

	// the function returns number of bytes written. If -1, means failure
	int WriteChunkString(CStringW const & String);    // will save as either ANSI, or UTF8 or UTF-16
	int WriteChunkString(CStringA const & String);
	int WriteChunkStringW(CStringW const & String);

	int ChunkStringLength(CStringW const & String) const;
	int ChunkStringLength(CStringA const & String) const;

	BOOL CommitChanges();

	static int m_TextEncodingInFiles;  // 0 - default ANSI code page, 1 - UTF-8, 2 - UTF-16

private:
	// wrong type of constructor
	CMmioFile(const CMmioFile &);

#ifndef OVERRIDE_MMLIB_FUNCTIONS
	static LRESULT PASCAL BufferedIOProc(LPSTR lpmmioinfo, UINT wMsg,
										LPARAM lParam1, LPARAM lParam2);
#endif

};

enum {
	CreateWaveFileDeleteAfterClose = MmioFileOpenDeleteAfterClose,
	CreateWaveFileDontInitStructure = MmioFileOpenDontCreateRiff,
	CreateWaveFileAllowMemoryFile = MmioFileMemoryFile,
	CreateWaveFileTempDir = 0x00100000,
	CreateWaveFileDontCopyInfo = 0x00200000,
	CreateWaveFilePcmFormat = 0x00400000,
	CreateWaveFileTemp = 0x00800000, // create temporary name
	CreateWaveFileAttachTemplateAsSource = 0x01000000,
	CreateWaveFileSizeSpecified = 0x02000000,
	CreateWaveFileCreateFact = 0x04000000,
};

typedef std::vector<SAMPLE_INDEX> SAMPLE_INDEX_Vector;

struct CuePointChunkItem
{
	DWORD CuePointID;       // unique ID
	DWORD SamplePosition;   // ordinal sample position in the playlist order (not used)
	FOURCC fccChunk;        // 'data'
	DWORD dwChunkStart;     // zero for 'data' chunk
	DWORD dwBlockStart;     // position of the sample in 'data' chunk'
	// for a compressed file, dwBlockStart should be a beginning of a compressed block containing that sample
	// but how we figure out the cue sample position?
	DWORD dwSampleOffset;   // sample position of the cue (NOT byte offset) from the block
};

struct WaveFileSegment
{
	SAMPLE_INDEX Begin;
	SAMPLE_INDEX End;
	CString Name;
	CString FullFilename;
};

typedef std::vector<WaveFileSegment> WaveFileSegmentVector;

inline bool operator <(WaveFileSegment const & s1, WaveFileSegment const & s2)
{
	return s1.Begin < s2.Begin;
}

typedef std::vector<CuePointChunkItem> CuePointVector;
typedef CuePointVector::iterator CuePointVectorIterator;
typedef CuePointVector::const_iterator ConstCuePointVectorIterator;

struct PlaylistSegment
{
	DWORD CuePointID;   // CuePoint ID
	DWORD Length;   // in samples
	DWORD NumLoops;    // number of loops
};

typedef std::vector<PlaylistSegment> PlaylistVector;
typedef PlaylistVector::iterator PlaylistVectorIterator;
typedef PlaylistVector::const_iterator ConstPlaylistVectorIterator;

#pragma pack(push, 1)

struct LtxtChunk  // in LIST adtl
{
	DWORD CuePointID;
	DWORD SampleLength; // length in samples. For cue point - 0 or 1
	DWORD Purpose;  //'rgn '
	WORD Country;
	WORD Language;
	WORD Dialect;
	WORD Codepage;
	// then goes zero-terminated text
	//UCHAR text[1];
};

#pragma pack(pop)

struct WaveRegionMarker : public LtxtChunk
{
	CString Name;

	WaveRegionMarker()
	{
		CuePointID = ~0UL;
		SampleLength = 0;
		Purpose = 0;
		Country = 0;
		Language = 0;
		Dialect = 0;
		Codepage = 0;
	}
};

typedef std::vector<WaveRegionMarker> RegionMarkerVector;
typedef RegionMarkerVector::iterator RegionMarkerIterator;
typedef RegionMarkerVector::const_iterator ConstRegionMarkerIterator;

struct LablNote
{
	DWORD CuePointID;
	CString Text;
};

typedef std::vector<LablNote> LabelVector;
typedef LabelVector::iterator LabelVectorIterator;
typedef LabelVector::const_iterator ConstLabelVectorIterator;

// this structure is used to edit region or marker
struct WAVEREGIONINFO
{
	DWORD Flags;
	DWORD MarkerCueID;
	DWORD Sample;
	DWORD Length;
	LPCTSTR Label;
	LPCTSTR Comment;
	LPCTSTR Ltxt;

	enum // flags for WAVEREGIONINFO
	{
		AddNew = 0x00000001,
		ChangeLabel = 0x00000002,
		ChangeComment = 0x00000004,
		ChangeLength = 0x00000008,
		ChangeLtxt = 0x00000010,
		ChangeSample = 0x00000020,
		Delete = 0x00000040,
		CuePointIndex = 0x00000080,     // MarkerCueId is an index in cue point array
		CommitChanges = 0x00000100,     // for Doc class - save UNDO for the file
		FindCue     = 0x00000200,   // find CueID by sample and length

		ChangeAll = ChangeLabel | ChangeComment | ChangeLength | ChangeLtxt | ChangeSample,
	};
};

struct InfoListItemA
{
	FOURCC fccCode;
	CStringA Text;
};

struct InfoListItemW
{
	FOURCC fccCode;
	CStringA Tag;   // tag as in WMA file
	CStringW Text;
};

typedef std::vector<InfoListItemA> InfoListItemVector;
typedef InfoListItemVector::iterator InfoListItemIterator;
typedef InfoListItemVector::const_iterator ConstInfoListItemIterator;

typedef std::vector<InfoListItemW> InfoListItemVectorW;
typedef InfoListItemVectorW::iterator InfoListItemIteratorW;
typedef InfoListItemVectorW::const_iterator ConstInfoListItemIteratorW;

struct WavePeak
{
	WAVE_PEAK low;
	WAVE_PEAK high;
	WavePeak(WAVE_PEAK Low, WAVE_PEAK High)
		: low(Low), high(High) {}
	WavePeak() {}
};

class CWavePeaks
{

public:
	CWavePeaks(unsigned granularity);
	~CWavePeaks();
	unsigned GetGranularity() const
	{
		return m_PeakDataGranularity;
	}
	void SetPeakData(unsigned index, WAVE_PEAK low, WAVE_PEAK high)
	{
		ASSERT(index < m_WavePeakSize);
		m_pPeaks[index].low = low;
		m_pPeaks[index].high = high;
	}

	WAVE_PEAK GetPeakDataLow(PEAK_INDEX index) const
	{
		ASSERT(index < m_WavePeakSize);
		return m_pPeaks[index].low;
	}
	WAVE_PEAK GetPeakDataHigh(PEAK_INDEX index) const
	{
		ASSERT(index < m_WavePeakSize);
		return m_pPeaks[index].high;
	}

	WavePeak * AllocatePeakData(NUMBER_OF_SAMPLES NewNumberOfSamples, NUMBER_OF_CHANNELS NumberOfChannels = 1);
	WavePeak * GetPeakArray()
	{
		return m_pPeaks;
	}
	WavePeak const * GetPeakArray() const
	{
		return m_pPeaks;
	}
	WavePeak GetPeakMinMax(PEAK_INDEX from, PEAK_INDEX to, NUMBER_OF_CHANNELS stride = 1) const;

	PEAK_INDEX GetPeaksSize() const
	{
		return m_WavePeakSize;
	}

	CSimpleCriticalSection & GetLock() const
	{
		return m_PeakLock;
	}

	void SetPeaks(PEAK_INDEX from, PEAK_INDEX to, NUMBER_OF_CHANNELS stride, WavePeak value);
	CWavePeaks & operator =(CWavePeaks const & src);
protected:
	WavePeak * m_pPeaks;
	PEAK_INDEX m_WavePeakSize;
	PEAK_INDEX m_AllocatedWavePeakSize;
	unsigned m_PeakDataGranularity;

	CSimpleCriticalSection mutable m_PeakLock;
};

#pragma warning(push)
#pragma warning(disable : 4521 4522)

class CWaveFile : public CMmioFile
{
	typedef CMmioFile BaseClass;
public:
	CWaveFile();

	CWaveFile(CWaveFile & f);
	~CWaveFile();

	BOOL CreateWaveFile(CWaveFile * pTemplateFile, WAVEFORMATEX const * pTemplateFormat,
						CHANNEL_MASK Channels, WAV_FILE_SIZE SizeOrSamples,
						long flags, LPCTSTR FileName);

	virtual BOOL Open(LPCTSTR lpszFileName, long nOpenFlags);
	virtual void Close();

	WaveSampleType GetSampleType() const
	{
		// todo
		return SampleType16bit;
	}

	long ReadSamples(CHANNEL_MASK Channels,
					SAMPLE_POSITION Pos, long Samples, void * pBuf,
					WaveSampleType type = SampleType16bit);
	long WriteSamples(CHANNEL_MASK DstChannels,
					SAMPLE_POSITION Pos, long Samples,
					void const * pBuf, CHANNEL_MASK SrcChannels,
					NUMBER_OF_CHANNELS NumSrcChannels,
					WaveSampleType type = SampleType16bit);

	int SampleSize() const;
	BOOL SetSourceFile(CWaveFile * const pOriginalFile);
	void SetWaveFormat(WAVEFORMATEX const * pWf);

	void RescanPeaks(SAMPLE_INDEX begin, SAMPLE_INDEX end);
	BOOL AllocatePeakData(NUMBER_OF_SAMPLES NewNumberOfSamples);
	WavePeak GetPeakMinMax(PEAK_INDEX from, PEAK_INDEX to, NUMBER_OF_CHANNELS stride = 1) const;

	unsigned GetPeaksSize() const;
	unsigned GetPeakGranularity() const;

	SAMPLE_POSITION SampleToPosition(SAMPLE_INDEX sample) const;
	SAMPLE_INDEX PositionToSample(SAMPLE_POSITION position) const;

	BOOL SetFileLengthSamples(NUMBER_OF_SAMPLES length);
	BOOL SetDatachunkLength(DWORD Length);
	void SetFactNumberOfSamples(NUMBER_OF_SAMPLES length);

	int CalculatePeakInfoSize() const
	{
		unsigned Granularity = GetPeakGranularity();
		return (NumberOfSamples() + Granularity - 1) / Granularity * Channels();
	}
	void SetPeakData(PEAK_INDEX index, WAVE_PEAK low, WAVE_PEAK high);
	BOOL LoadPeaksForCompressedFile(CWaveFile & OriginalWaveFile, ULONG NumberOfSamples);

	BOOL CheckAndLoadPeakFile();
	void SavePeakInfo(CWaveFile & SavedWaveFile);

	CSimpleCriticalSection & GetPeakLock()
	{
		return GetInstanceData()->m_PeakData.GetLock();
	}

	struct InstanceDataWav : BaseClass::InstanceDataMm
	{
	private:
		typedef CMmioFile::InstanceDataMm BaseInstanceClass;
	public:
		typedef std::auto_ptr<InstanceDataWav> auto_ptr;
		InstanceDataWav();

		MMCKINFO datack;
		MMCKINFO fmtck;
		MMCKINFO factck;

		CWaveFormat wf;
		CWavePeaks m_PeakData;

		CString m_DisplayTitle;   // DISP, WM/Title
#if 0
		CString Author;         // WM/Author
		CString Album;          // WM/AlbumTitle
		CString Copyright;      // ICOP
		CString RecordingEngineer;  // IENG

		CString Title;
		CString OriginalArtist; // IART
		CString Date;       // WM/Year
		CString Genre;      // IGNR, WM/Genre
		CString Comment;    // ICMT
		CString Subject;   // ISBJ
		CString Keywords;  // IKEY
		CString Medium;  // IMED
		CString Source; // ISRC
		CString Digitizer; // ITCH
		CString DigitizationSource; // ISRF
#endif

		RegionMarkerVector m_RegionMarkers;  // markers and regions
		CuePointVector m_CuePoints;
		DWORD m_FreeCuePointNumber;

		PlaylistVector m_Playlist;
		LabelVector m_Labels;   // labels for the cue points
		LabelVector m_Notes;     // comments for the cue points

		InfoListItemVector m_InfoList;
		InfoListItemVectorW m_InfoListW;    // UNFO list

		bool m_InfoChanged;

		// move all data to a derived (bigger) type
		virtual void CopyMetadata(InstanceDataWav const * pSrc, unsigned CopyFlags = MetadataCopyAll);
		virtual void SwapMetadata(InstanceDataWav * pSrc, unsigned SwapFlags = MetadataCopyAll);

		enum {
			MetadataCopyDisp = 0x001,
			MetadataCopyInfo = 0x002,
			MetadataCopyCue = 0x004,
			MetadataCopyLabels = 0x008,
			MetadataCopyPeakData = 0x010,
			MetadataCopyLtxt = 0x020,
			MetadataCopyPlaylist = 0x040,

			MetadataCopyAllCueData = MetadataCopyCue | MetadataCopyLabels | MetadataCopyLtxt,
			MetadataCopyAll = 0xFFFF,
		};

		CuePointChunkItem * GetCuePoint(DWORD CueId);
		WaveRegionMarker * GetRegionMarker(DWORD CueId);
		CuePointChunkItem const * GetCuePoint(DWORD CueId) const;
		WaveRegionMarker const * GetRegionMarker(DWORD CueId) const;

		LPCTSTR GetCueLabel(DWORD CueId) const;   // from 'labl' chunk, if any
		LPCTSTR GetCueComment(DWORD CueId) const; // from 'note' chunk, if any

		LPCTSTR GetCueText(DWORD CueId) const;    // use either label, or comment, or marker text
		LPCTSTR GetCueTextByIndex(unsigned CueIndex) const;   // the same, using cue index

		BOOL SetWaveMarker(WAVEREGIONINFO * info);
		BOOL GetWaveMarker(WAVEREGIONINFO * info) const;

		BOOL MoveWaveMarker(unsigned long MarkerCueID, SAMPLE_INDEX Sample);
		BOOL SetMarkerLabel(unsigned long MarkerCueID, LPCTSTR Label);

		void GetSortedMarkers(SAMPLE_INDEX_Vector & markers) const;
		void GetSortedFileSegments(WaveFileSegmentVector & segments, bool IncludeEndMarkerName) const;

		BOOL MoveMarkers(SAMPLE_INDEX SampleBegin, NUMBER_OF_SAMPLES SrcLength, NUMBER_OF_SAMPLES DstLength);
		BOOL ReverseMarkers(SAMPLE_INDEX SampleBegin, NUMBER_OF_SAMPLES Length);
		BOOL RescaleMarkers(long OldSampleRate, long NewSampleRate);

		BOOL CopyMarkers(InstanceDataWav const * pSrc,
						SAMPLE_INDEX SrcBegin, SAMPLE_INDEX DstBegin, NUMBER_OF_SAMPLES Length);

		bool IsChanged() const
		{
			return m_InfoChanged;
		}

		void SetChanged(bool Changed = true)
		{
			m_InfoChanged = Changed;
		}

		InstanceDataWav & operator =(InstanceDataWav const & src);

		virtual void MoveDataTo(InstanceData * dst)
		{
			InstanceDataWav * dst1 = static_cast<InstanceDataWav *>(dst);
			*dst1 = *this;
		}

		virtual void ResetMetadata();
	};

	void CopyMetadata(CWaveFile const & src);
	void SwapMetadata(InstanceDataWav * pSrc, unsigned SwapFlags = InstanceDataWav::MetadataCopyAll);

	InstanceDataWav * GetInstanceData() const
	{
		return static_cast<InstanceDataWav *>(CDirectFile::GetInstanceData());
	}

	CWavePeaks * GetWavePeaks() const;

	void SetPeaks(PEAK_INDEX from, PEAK_INDEX to, NUMBER_OF_CHANNELS stride, WavePeak value);

	NUMBER_OF_CHANNELS Channels() const;

	NUMBER_OF_CHANNELS NumChannelsFromMask(CHANNEL_MASK Channels) const;
	bool AllChannels(CHANNEL_MASK Channels) const;

	NUMBER_OF_SAMPLES NumberOfSamples() const;
	CWaveFile & operator =(CWaveFile &);

	WAVEFORMATEX * AllocateWaveformat(size_t FormatSize = sizeof (WAVEFORMATEX));

	bool IsCompressed() const;

	CHANNEL_MASK ChannelsMask() const;

	BOOL LoadWaveformat();
	BOOL FindData();
	BOOL LoadMetadata();
	DWORD SaveMetadata();   // returns total saved length
	DWORD GetMetadataLength() const;

	BOOL LoadListMetadata(MMCKINFO & chunk);
	BOOL ReadCueSheet(MMCKINFO & chunk);
	BOOL ReadPlaylist(MMCKINFO & chunk);

	CuePointChunkItem * GetCuePoint(DWORD CueId);
	WaveRegionMarker * GetRegionMarker(DWORD CueId);
	CuePointChunkItem const * GetCuePoint(DWORD CueId) const;
	WaveRegionMarker const * GetRegionMarker(DWORD CueId) const;

	LPCTSTR GetCueLabel(DWORD CueId) const;
	LPCTSTR GetCueComment(DWORD CueId) const;
	LPCTSTR GetCueText(DWORD CueId) const;    // use either label, or comment, or marker text
	LPCTSTR GetCueTextByIndex(unsigned CueIndex) const;   // the same, using cue index

	BOOL SetWaveMarker(WAVEREGIONINFO * info);
	BOOL GetWaveMarker(WAVEREGIONINFO * info) const;
	BOOL MoveWaveMarker(unsigned long MarkerCueID, SAMPLE_INDEX Sample);
	BOOL SetMarkerLabel(unsigned long MarkerCueID, LPCTSTR Label);

	// return sorted array of markers
	void GetSortedMarkers(SAMPLE_INDEX_Vector & markers, BOOL IncludeFileLimits = FALSE) const;
	void GetSortedFileSegments(WaveFileSegmentVector & segments, bool IncludeEndMarkerName = false) const;

	unsigned SampleRate() const;

	WORD BitsPerSample() const;

	LPMMCKINFO GetDataChunk() const;

	WAVEFORMATEX * GetWaveFormat() const;
	// save all changes in wave format and data chunk size
	BOOL CommitChanges();
	MMCKINFO * GetFmtChunk() const;
	MMCKINFO * GetFactChunk() const;

	DWORD m_FactSamples;

private:
	static CPath MakePeakFileName(LPCTSTR FileName);
	// wrong type of constructor
	CWaveFile & operator =(CWaveFile const &);
	CWaveFile(CWaveFile const & f);
};

#pragma warning( pop )

#endif //#ifndef WAVE_FILE__H__
