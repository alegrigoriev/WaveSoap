// WaveFile.h
#ifndef WAVE_FILE__H__
#define WAVE_FILE__H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
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

class CMmioFile : public CDirectFile
{
public:
	// construction
	CMmioFile();
	virtual BOOL Open( LPCTSTR lpszFileName, UINT nOpenFlags);
	virtual void Close( );
	CMmioFile & operator =(CMmioFile &);

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
		mmioFlush(m_hmmio, 0);
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

	BOOL Advance(LPMMIOINFO mmio, UINT uFlags)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return MMSYSERR_NOERROR == mmioAdvance(m_hmmio, mmio, uFlags);
	}

	BOOL CreateChunk(MMCKINFO & ck, UINT wFlags)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		ck.cksize = 0;
		return MMSYSERR_NOERROR == mmioCreateChunk(m_hmmio, & ck, wFlags);
	}
	BOOL CreateList(MMCKINFO & ck)
	{
		return CreateChunk(ck, MMIO_CREATELIST);
	}
	BOOL CreateRiff(MMCKINFO & ck)
	{
		return CreateChunk(ck, MMIO_CREATERIFF);
	}

	BOOL GetInfo(MMIOINFO & mmioinfo, UINT wFlags = 0)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		return MMSYSERR_NOERROR == mmioGetInfo(m_hmmio, & mmioinfo, wFlags);
	}

	virtual ~CMmioFile();

	HMMIO m_hmmio;
	FOURCC m_RiffckType;
	CSimpleCriticalSection m_cs;

	struct InstanceDataMm : public CDirectFile::InstanceData
	{
		MMCKINFO riffck;
		HMMIO m_hmmio;
		InstanceDataMm()
		{
			m_hmmio = NULL;
			memzero(riffck);
			m_size = sizeof *this;
		}
		virtual void CopyDataTo(InstanceData * dst)
		{
			InstanceDataMm * dst1 = static_cast<InstanceDataMm *>(dst);
			dst1->riffck = riffck;
			dst1->m_hmmio = m_hmmio;
			InstanceData::CopyDataTo(dst);
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

	BOOL IsOpen() const
	{
		return m_hmmio != NULL;
	}

	BOOL CommitChanges();

private:
	// wrong type of constructor
	CMmioFile(const CMmioFile &)
	{
		ASSERT(FALSE);
	}

	static LRESULT PASCAL BufferedIOProc(LPSTR lpmmioinfo, UINT wMsg,
										LPARAM lParam1, LPARAM lParam2);

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

class CWaveFile : public CMmioFile
{
public:
	CWaveFile();
	~CWaveFile();
	BOOL CreateWaveFile(CWaveFile * pTemplateFile, WAVEFORMATEX * pTemplateFormat,
						int Channels, unsigned long SizeOrSamples, DWORD flags, LPCTSTR FileName);
#if 0
	virtual BOOL Open( LPCTSTR lpszFileName, UINT nOpenFlags);
#endif
	virtual void Close( );
	int SampleSize() const;

	struct InstanceDataWav : CMmioFile::InstanceDataMm
	{
		MMCKINFO datack;
		MMCKINFO fmtck;
		MMCKINFO factck;
		MMCKINFO infock;    // INFO chunk
		CWaveFormat wf;

		InstanceDataWav()
		{
			memzero(datack);
			memzero(fmtck);
			memzero(factck);
			memzero(infock);
			m_size = sizeof *this;
		}
		virtual void CopyDataTo(InstanceData * dst)
		{
			InstanceDataWav * dst1 = static_cast<InstanceDataWav *>(dst);
			dst1->datack = datack;
			dst1->fmtck = fmtck;
			dst1->factck = factck;
			dst1->infock = infock;
			dst1->wf = wf;
			InstanceDataMm::CopyDataTo(dst);
		}
	};
	InstanceDataWav * GetInstanceData() const
	{
		return static_cast<InstanceDataWav *>(CDirectFile::GetInstanceData());
	}
	int Channels() const
	{
		WAVEFORMATEX * pWf = GetWaveFormat();
		if (NULL == pWf)
		{
			return 0;
		}
		return pWf->nChannels;
	}
	LONG NumberOfSamples() const;
	CWaveFile & operator =(CWaveFile &);

	//MMCKINFO m_datack;

	WAVEFORMATEX * AllocateWaveformat(size_t FormatSize = sizeof (WAVEFORMATEX))
	{
		return GetInstanceData()->wf.Allocate(FormatSize - sizeof (WAVEFORMATEX));
	}

	BOOL LoadWaveformat();
	BOOL FindData();

	unsigned SampleRate() const
	{
		WAVEFORMATEX * pWf = GetWaveFormat();
		if (pWf)
		{
			return pWf->nSamplesPerSec;
		}
		else
		{
			return 0;
		}
	}

	LPMMCKINFO GetDataChunk() const
	{
		return & GetInstanceData()->datack;
	}
	WAVEFORMATEX * GetWaveFormat() const;
	// save all changes in wave format and data chunk size
	BOOL CommitChanges();
	MMCKINFO * GetFmtChunk() const;
	MMCKINFO * GetFactChunk() const;

	DWORD m_FactSamples;
private:
	// wrong type of constructor
	CWaveFile(const CWaveFile &)
	{
		ASSERT(FALSE);
	}
};

#endif //#ifndef WAVE_FILE__H__
