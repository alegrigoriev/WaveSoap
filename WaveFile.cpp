// WaveFile.cpp
#include "stdafx.h"
#include "DirectFile.h"
#include "WaveFile.h"

CMmioFile::CMmioFile()
	: m_hmmio(NULL),
	m_RiffckType(0)
{
}

CMmioFile::~CMmioFile()
{
	Close();

}

CMmioFile & CMmioFile::operator=(CMmioFile & SourceFile)
{
	Close();
	Attach( & SourceFile);
	MMIOINFO mmii;
	memset( & mmii, 0, sizeof mmii);
	mmii.fccIOProc = 0;
	mmii.pIOProc = BufferedIOProc;

	mmii.adwInfo[0] = (DWORD)this;
	m_hmmio = mmioOpen(NULL, & mmii, MMIO_READ //| MMIO_ALLOCBUF
						);
	Seek(0);
	return *this;
}

static DWORD GetSectorSize(LPCTSTR szFilename)
{
	CString curr_dir;
	TCHAR c;
	DWORD size = GetCurrentDirectory(0, & c);
	if (0 == size)
	{
		return 0x1000;
	}
	LPTSTR pBuf = curr_dir.GetBuffer(size);
	if (NULL == pBuf)
	{
		return 0x1000;
	}

	if (GetCurrentDirectory(size, pBuf) > size)
	{
		return 0x1000;
	}
	curr_dir.ReleaseBuffer();

	CString file_dir;
	LPTSTR pFilenamePart;
	size = GetFullPathName(szFilename, 0, &c, & pFilenamePart);
	if (0 == size)
	{
		return 0x1000;
	}

	pBuf = file_dir.GetBuffer(size);
	if (NULL == pBuf)
	{
		return 0x1000;
	}

	if (GetFullPathName(szFilename, size, pBuf, & pFilenamePart) > size)
	{
		return 0x1000;
	}

	*pFilenamePart = 0;
	file_dir.ReleaseBuffer();
	if (0 != file_dir.CompareNoCase(curr_dir))
	{
		SetCurrentDirectory(file_dir);
	}

	DWORD sector_size = 0;
	DWORD sectors_per_cluster;
	DWORD free_clusters;
	DWORD total_clusters;
	if (! GetDiskFreeSpace(NULL, & sectors_per_cluster,
							& sector_size, & free_clusters, & total_clusters)
		|| 0 == sector_size)
	{
		sector_size = 0x1000;
	}

	if (0 != file_dir.CompareNoCase(curr_dir))
	{
		SetCurrentDirectory(curr_dir);
	}
	return sector_size;
}

BOOL CMmioFile::Open( LPCTSTR szFileName, UINT nOpenFlags)
{
	Close();
	DWORD DirectFileOpenFlags = 0;
	if (nOpenFlags & MmioFileOpenReadOnly)
	{
		DirectFileOpenFlags |= CDirectFile::OpenReadOnly | CDirectFile::OpenExisting;
	}
	else if (nOpenFlags & MmioFileOpenExisting)
	{
		DirectFileOpenFlags |= CDirectFile::OpenExisting;
		if (nOpenFlags & MmioFileAllowReadOnlyFallback)
		{
			DirectFileOpenFlags |= CDirectFile::OpenAllowReadOnlyFallback;
		}
	}
	else
	{
		if (nOpenFlags & MmioFileOpenCreateAlways)
		{
			DirectFileOpenFlags |= CDirectFile::CreateAlways;
		}
		else if (nOpenFlags & MmioFileOpenCreateNew)
		{
			DirectFileOpenFlags |= CDirectFile::CreateNew;
		}
		if (nOpenFlags & MmioFileOpenDeleteAfterClose)
		{
			DirectFileOpenFlags |= CDirectFile::OpenDeleteAfterClose;
		}
		if (nOpenFlags & MmioFileMemoryFile)
		{
			DirectFileOpenFlags |= CDirectFile::CreateMemoryFile;
		}
	}

	if ( ! CDirectFile::Open(szFileName, DirectFileOpenFlags))
	{
		return FALSE;
	}

	if (0 == (nOpenFlags & (MmioFileOpenCreateNew | MmioFileOpenCreateAlways)))
	{

		MMIOINFO mmii;
		memset( & mmii, 0, sizeof mmii);
		mmii.fccIOProc = 0;
		mmii.pIOProc = BufferedIOProc;

		mmii.adwInfo[0] = (DWORD)this;

		m_hmmio = mmioOpen(NULL, & mmii,
							MMIO_READ //| MMIO_ALLOCBUF
							);

		if (NULL == m_hmmio)
		{
			Close();
			return FALSE;
		}

		if (0 == (nOpenFlags & MmioFileOpenDontLoadRiff)
			&& ! LoadRiffChunk())
		{
			Close();
			return FALSE;
		}
	}
	else
	{
		// new file created
		if (nOpenFlags & MmioFileMemoryFile)
		{
			SetFileLength(0x40);
		}
		MMIOINFO mmii;
		memset( & mmii, 0, sizeof mmii);
		mmii.fccIOProc = 0;
		mmii.pIOProc = BufferedIOProc;

		mmii.adwInfo[0] = (DWORD)this;

		m_hmmio = mmioOpen(NULL, & mmii,
							MMIO_READ | MMIO_WRITE //| MMIO_ALLOCBUF
							);

		if (NULL == m_hmmio)
		{
			Close();
			return FALSE;
		}

		if (0 == (nOpenFlags & MmioFileOpenDontCreateRiff))
		{
			if (NULL == GetRiffChunk())
			{
				AllocateCommonData(0);
				LPMMCKINFO pRiffck = GetRiffChunk();
				pRiffck->ckid = FOURCC_RIFF;
				pRiffck->cksize = 0;
				pRiffck->fccType = m_RiffckType; // derived class can set it
				pRiffck->dwDataOffset = 0;
				pRiffck->dwFlags = 0;
				if ( ! CreateRiff( * pRiffck))
				{
					Close();
					return FALSE;
				}
			}
			// if RIFF chunk was allocated before, it means that it is already read
		}
	}
	return TRUE;
}

BOOL CMmioFile::LoadRiffChunk()
{
	if (NULL == GetRiffChunk())
	{
		AllocateCommonData(0);
		LPMMCKINFO pRiffck = GetRiffChunk();
		pRiffck->ckid = FOURCC_RIFF;
		pRiffck->cksize = 0;
		pRiffck->fccType = m_RiffckType; // derived class can set it
		pRiffck->dwDataOffset = 0;
		pRiffck->dwFlags = 0;
		if ( ! FindRiff())
		{
			Close();
			return FALSE;
		}
	}
	// if RIFF chunk was allocated before, it means that it is already read
	return TRUE;
}
LRESULT PASCAL CMmioFile::BufferedIOProc(LPSTR lpmmioinfo, UINT wMsg,
										LPARAM lParam1, LPARAM lParam2)
{
	LPMMIOINFO pmmi = (LPMMIOINFO) lpmmioinfo;
	CMmioFile * pFile = (CMmioFile *) pmmi->adwInfo[0];

	switch(wMsg)
	{
	case MMIOM_OPEN:
		return MMSYSERR_NOERROR;
		break;
	case MMIOM_CLOSE:
		return MMSYSERR_NOERROR;
		break;
	case MMIOM_READ:
	{
		//TRACE("MMIOM_READ at %08x, %x bytes\n", pmmi->lDiskOffset, lParam2);
		DWORD cbRead = pFile->ReadAt((LPVOID) lParam1, lParam2, pmmi->lDiskOffset);
		if (-1 == cbRead)
			return -1;
		pmmi->lDiskOffset += cbRead;
		return cbRead;
	}
		break;
	case MMIOM_WRITEFLUSH:
		TRACE("MMIOM_WRITEFLUSH\n");
	case MMIOM_WRITE:
	{
		//TRACE("MMIOM_WRITE at %08x, %x bytes\n", pmmi->lDiskOffset, lParam2);
		DWORD cbWritten = pFile->WriteAt((LPVOID) lParam1, lParam2, ULONG(pmmi->lDiskOffset));
		if (-1 == cbWritten)
			return -1;
		pmmi->lDiskOffset += cbWritten;
		return cbWritten;
	}
		break;
	case MMIOM_SEEK:
		switch (lParam2)
		{
		case SEEK_CUR:
			pmmi->lDiskOffset += lParam1;
			//TRACE("MMIOM_SEEK SEEK_CUR pmmi->lDiskOffset=%08x\n", pmmi->lDiskOffset);
			return pmmi->lDiskOffset;
			break;
		case SEEK_END:
			pmmi->lDiskOffset = long(pFile->CDirectFile::GetLength()) + lParam1;
			//TRACE("MMIOM_SEEK SEEK_END pmmi->lDiskOffset=%08x\n", pmmi->lDiskOffset);
			return pmmi->lDiskOffset;
			break;
		case SEEK_SET:
			pmmi->lDiskOffset = lParam1;
			//TRACE("MMIOM_SEEK SEEK_SET pmmi->lDiskOffset=%08x\n", pmmi->lDiskOffset);
			return pmmi->lDiskOffset;
			break;
		default:
			return -1;
		}
		break;
	default:
		return -1;
	}
}


void CMmioFile::Close( )
{
	if (m_hmmio != NULL)
	{
		mmioClose(m_hmmio, 0);
		m_hmmio = NULL;
	}
	CDirectFile::Close(0);
}


CWaveFile::CWaveFile()
{
	m_RiffckType = mmioFOURCC('W', 'A', 'V', 'E');
}

CWaveFile & CWaveFile::operator =(CWaveFile & SourceFile)
{
	CMmioFile::operator=(SourceFile);
	m_FactSamples = SourceFile.m_FactSamples;
	// waveformat will be read as needed
	return *this;
}

CWaveFile::~CWaveFile()
{
}

#if 0
BOOL CWaveFile::Open( LPCTSTR lpszFileName, UINT nOpenFlags)
{
	return CMmioFile::Open(lpszFileName, nOpenFlags);
}

#endif
void CWaveFile::Close( )
{
	CMmioFile::Close();
	m_FactSamples = -1;
}

BOOL CWaveFile::LoadWaveformat()
{

	MMCKINFO ck = {mmioFOURCC('f', 'm', 't', ' '), 0, 0, 0, 0};
	if ( ! FindChunk(ck, GetRiffChunk()))
	{
		return FALSE;
	}
	if (ck.cksize > 0x20000) // 128K
	{
		TRACE("fmt chunk is too big: > 128K\n");
		Ascend(ck);
		return FALSE;
	}
	// allocate structure
	int WaveformatSize = ck.cksize;
	if (WaveformatSize < sizeof (WAVEFORMATEX))
	{
		WaveformatSize = sizeof (WAVEFORMATEX);
	}
	COMMON_DATA * pCd = (COMMON_DATA *)AllocateCommonData(offsetof (COMMON_DATA, wf) + WaveformatSize);
	LPWAVEFORMATEX pWf = & pCd->wf;

	if (NULL != pCd)
	{
		if (ck.cksize == Read(pWf, ck.cksize))
		{
			Ascend(ck);
			// try to find 'fact' chunk
			MMCKINFO * pFact = GetFactChunk();
			pFact->ckid = mmioFOURCC('f', 'a', 'c', 't');
			// save current position
			DWORD CurrPos = Seek(0, SEEK_CUR);
			m_FactSamples = -1;
			if (FindChunk(* pFact, GetRiffChunk()))
			{
				if (pFact->cksize >= sizeof (DWORD))
				{
					// read real number of samples
					Read(& m_FactSamples, sizeof m_FactSamples);
				}
				Ascend(* pFact);
			}
			Seek(CurrPos);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		Ascend(ck);
	}
	return FALSE;
}

BOOL CWaveFile::FindData()
{
	if (GetCommonDataSize() < sizeof (COMMON_DATA))
	{
		if (NULL == AllocateCommonData(sizeof (COMMON_DATA)))
		{
			return FALSE;
		}
	}
	LPMMCKINFO pDatack = (LPMMCKINFO) GetCommonData();
	pDatack->ckid = mmioFOURCC('d', 'a', 't', 'a');
	return FindChunk( * pDatack, GetRiffChunk());
}

// creates a file based on template format from pTemplateFile
BOOL CWaveFile::CreateWaveFile(CWaveFile * pTemplateFile, WAVEFORMATEX * pTemplateFormat,
								int Channels, unsigned long SizeOrSamples, DWORD flags, LPCTSTR FileName)
{
	CString name;
	char NameBuf[512];
	// if the name is empty, create a temp name
	DWORD OpenFlags = MmioFileOpenCreateAlways;
	if ((flags & CreateWaveFileAllowMemoryFile)
		&& SizeOrSamples <= 0x4000)
	{
		OpenFlags |= MmioFileMemoryFile;
	}
	else if (NULL != FileName
			&& FileName[0] != 0)
	{
		name = FileName;
	}
	else
	{
		CString dir;
		if (0 == (flags & CreateWaveFileTempDir))
		{
			// get directory name from template file
			LPTSTR pFilePart = NULL;
			if (NULL != pTemplateFile
				&& pTemplateFile->IsOpen()
				&& 0 != GetFullPathName(pTemplateFile->GetName(),
										sizeof NameBuf, NameBuf, & pFilePart)
				&& pFilePart != NULL)
			{
				*pFilePart = 0;
				dir = NameBuf;
			}
		}
		else
		{
			dir = GetApp()->sTempDir;
			if (dir.IsEmpty())
			{
				if (GetTempPath(sizeof NameBuf, NameBuf))
				{
					dir = NameBuf;
				}
			}
		}

		if ( ! dir.IsEmpty() && dir[dir.GetLength() - 1] != '\\')
		{
			dir += _T("\\");
		}
		if (GetTempFileName(dir, _T("wav"), 0, NameBuf))
		{
			name = NameBuf;
		}
		else
		{
			return FALSE;
		}
	}
	// create a file, RIFF list, fmt chunk, data chunk of specified size
	// temp file with this name may already be created
	if (flags & CreateWaveFileDeleteAfterClose)
	{
		OpenFlags |= MmioFileOpenDeleteAfterClose;
	}
	if (flags & CreateWaveFileDontInitStructure)
	{
		if (FALSE == Open(name, OpenFlags | MmioFileOpenDontCreateRiff))
		{
			return FALSE;
		}
		if (flags & CreateWaveFileSizeSpecified
			&& 0 != SizeOrSamples)
		{
			return SetFileLength(SizeOrSamples);
		}
		return TRUE;
	}
	if (FALSE == Open(name, OpenFlags))
	{
		Close();
		return FALSE;
	}

	if (GetCommonDataSize() < sizeof (COMMON_DATA))
	{
		if (NULL == AllocateCommonData(sizeof (COMMON_DATA)))
		{
			Close();
			return FALSE;
		}
	}
	// create new WAVEFORMATEX
	WAVEFORMATEX * pWF = NULL;
	if (NULL == pTemplateFormat
		&& NULL != pTemplateFile
		&& pTemplateFile->IsOpen())
	{
		pTemplateFormat = pTemplateFile->GetWaveFormat();
	}
	int FormatSize = sizeof PCMWAVEFORMAT;
	if (NULL != pTemplateFormat)
	{
		if ((flags & CreateWaveFilePcmFormat)
			|| WAVE_FORMAT_PCM == pTemplateFormat->wFormatTag)
		{
			pWF = GetWaveFormat();
			if (pWF)
			{
				pWF->nSamplesPerSec = pTemplateFormat->nSamplesPerSec;
				pWF->wFormatTag = WAVE_FORMAT_PCM;
				if (flags & CreateWaveFileTemp)
				{
					pWF->wBitsPerSample = 16;
				}
				else
				{
					pWF->wBitsPerSample = pTemplateFormat->wBitsPerSample;
				}
				if (ALL_CHANNELS == Channels)
				{
					pWF->nChannels = pTemplateFormat->nChannels;
				}
				else
				{
					pWF->nChannels = 1;
				}
				pWF->nBlockAlign = pWF->wBitsPerSample / 8 * pWF->nChannels;
				pWF->nAvgBytesPerSec = pWF->nBlockAlign * pWF->nSamplesPerSec;
			}
			else
			{
				Close();
				return FALSE;
			}
		}
		else
		{
			FormatSize = sizeof WAVEFORMATEX + pTemplateFormat->cbSize;
			AllocateCommonData(FormatSize + offsetof (COMMON_DATA, wf));
			pWF = GetWaveFormat();
			if (pWF)
			{
				memcpy(pWF, pTemplateFormat, FormatSize);
				if (ALL_CHANNELS == Channels)
				{
					pWF->nChannels = pTemplateFormat->nChannels;
				}
				else
				{
					pWF->nChannels = 1;
					if (pTemplateFormat->nChannels != 1)
					{
						// it may not be correct, better query the compressor
						pWF->nBlockAlign /= 2;
						pWF->nAvgBytesPerSec /= 2;
					}
				}
			}
			else
			{
				Close();
				return FALSE;
			}
		}
	}
	else
	{
		// create default PCM descriptor
		pWF = GetWaveFormat();
		if (pWF)
		{
			pWF->nSamplesPerSec = 44100;
			pWF->wFormatTag = WAVE_FORMAT_PCM;
			pWF->wBitsPerSample = 16;
			if (ALL_CHANNELS == Channels)
			{
				pWF->nChannels = 2;
			}
			else
			{
				pWF->nChannels = 1;
			}
			pWF->nBlockAlign = pWF->wBitsPerSample / 8 * pWF->nChannels;
			pWF->nAvgBytesPerSec = pWF->nBlockAlign * pWF->nSamplesPerSec;
		}
		else
		{
			Close();
			return FALSE;
		}
	}
	// RIFF created in Open()
	MMCKINFO * pfck = GetFmtChunk();
	pfck->ckid = mmioFOURCC('f', 'm', 't', ' ');
	CreateChunk(* pfck, 0);
	Write(pWF, FormatSize);
	Ascend(* pfck);

	// write fact chunk
	MMCKINFO * fact = GetFactChunk();
	if ((flags & CreateWaveFileCreateFact)
		&& NULL != fact)
	{
		memset(fact, 0, sizeof *fact);
		fact->ckid = mmioFOURCC('f', 'a', 'c', 't');
		fact->cksize = sizeof (DWORD);
		if (CreateChunk(* fact, 0))
		{
			DWORD tmp = 0;
			Write(& tmp, sizeof tmp);
			Ascend( * fact);
		}
	}

	// create data chunk
	LPMMCKINFO pDatachunk = GetDataChunk();
	memset(pDatachunk, 0, sizeof (MMCKINFO));
	pDatachunk->ckid = mmioFOURCC('d', 'a', 't', 'a');
	CreateChunk( * pDatachunk, 0);
	if (SizeOrSamples)
	{
		if (flags & CreateWaveFileSizeSpecified)
		{
			SetFileLength(SizeOrSamples);
			Seek(SizeOrSamples);
		}
		else
		{
			size_t DataLength = SizeOrSamples * SampleSize();
			SetFileLength(pDatachunk->dwDataOffset + DataLength);
			Seek(DataLength, SEEK_CUR);
		}
	}
	Ascend( * pDatachunk);
	// and copy INFO
	// then update RIFF
	//Ascend( *GetRiffChunk());
	return TRUE;
}

int CWaveFile::SampleSize() const
{
	COMMON_DATA * cd = (COMMON_DATA *)GetCommonData();
	if (NULL == cd
		|| 0 == cd->wf.nChannels
		|| 0 == cd->wf.wBitsPerSample)
	{
		return 0;
	}
	return cd->wf.nChannels * cd->wf.wBitsPerSample / 8;
}

LONG CWaveFile::NumberOfSamples() const
{
	COMMON_DATA * cd = (COMMON_DATA *)GetCommonData();
	if (NULL == cd
		|| 0 == cd->wf.nChannels
		|| 0 == cd->wf.wBitsPerSample)
	{
		return 0;
	}
	return MulDiv(cd->datack.cksize, 8, cd->wf.nChannels * cd->wf.wBitsPerSample);
}

WAVEFORMATEX * CWaveFile::GetWaveFormat() const
{
	COMMON_DATA * tmp = (COMMON_DATA *)GetCommonData();
	if (tmp)
	{
		return & tmp->wf;
	}
	else
	{
		return NULL;
	}
}

MMCKINFO * CWaveFile::GetFmtChunk() const
{
	COMMON_DATA * tmp = (COMMON_DATA *)GetCommonData();
	if (tmp)
	{
		return & tmp->fmtck;
	}
	else
	{
		return NULL;
	}
}

MMCKINFO * CWaveFile::GetFactChunk() const
{
	COMMON_DATA * tmp = (COMMON_DATA *)GetCommonData();
	if (tmp)
	{
		return & tmp->factck;
	}
	else
	{
		return NULL;
	}
}

BOOL CMmioFile::CommitChanges()
{
	if ( ! IsOpen()
		|| IsReadOnly())
	{
		return FALSE;
	}
	// save RIFF header
	DWORD CurrentLength = (DWORD)GetLength();
	LPMMCKINFO riff = GetRiffChunk();
	if (NULL == riff)
	{
		return FALSE;
	}
	if ((riff->dwFlags & MMIO_DIRTY) || riff->dwDataOffset + riff->cksize != CurrentLength)
	{
		Seek(CurrentLength);
		riff->dwFlags |= MMIO_DIRTY;

		Ascend( * riff);
	}
	Flush();
	return TRUE;
}

BOOL CWaveFile::CommitChanges()
{
	if ( ! IsOpen()
		|| IsReadOnly())
	{
		return FALSE;
	}
	// write new fmt chunk
	MMCKINFO * fmtck = GetFmtChunk();
	if (NULL != fmtck
		&& (fmtck->dwFlags & MMIO_DIRTY))
	{
		Seek(fmtck->dwDataOffset);
		Write(GetWaveFormat(), fmtck->cksize);
		fmtck->dwFlags &= ~MMIO_DIRTY;
	}
	MMCKINFO * factck = GetFactChunk();
	if (NULL != factck
		&& factck->dwDataOffset != 0
		&& (factck->dwFlags & MMIO_DIRTY))
	{
		Seek(factck->dwDataOffset);
		Write( & m_FactSamples, sizeof m_FactSamples);
		factck->dwFlags &= ~MMIO_DIRTY;
	}
	// update data chunk
	MMCKINFO * datack = GetDataChunk();
	if (datack->dwFlags & MMIO_DIRTY)
	{
		Seek(datack->dwDataOffset - sizeof datack->cksize);
		Write( & datack->cksize, sizeof datack->cksize);
		datack->dwFlags &= ~MMIO_DIRTY;
		GetRiffChunk()->dwFlags |= MMIO_DIRTY;
	}
	return CMmioFile::CommitChanges();
}
