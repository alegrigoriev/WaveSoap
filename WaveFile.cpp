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
	m_File.Attach( & SourceFile.m_File);
	MMIOINFO mmii;
	SourceFile.GetInfo(mmii);
	m_hmmio = mmioOpen(NULL, & mmii, MMIO_READ | MMIO_ALLOCBUF);
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
	}

	if ( ! m_File.Open(szFileName, DirectFileOpenFlags))
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
							MMIO_READ | MMIO_ALLOCBUF);

		if (NULL == m_hmmio)
		{
			Close();
			return FALSE;
		}

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
	}
	else
	{
		// new file created
		MMIOINFO mmii;
		memset( & mmii, 0, sizeof mmii);
		mmii.fccIOProc = 0;
		mmii.pIOProc = BufferedIOProc;

		mmii.adwInfo[0] = (DWORD)this;

		m_hmmio = mmioOpen(NULL, & mmii,
							MMIO_READ | MMIO_WRITE | MMIO_ALLOCBUF);

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
		DWORD cbRead = pFile->m_File.ReadAt((LPVOID) lParam1, lParam2, pmmi->lDiskOffset);
		if (-1 == cbRead)
			return -1;
		pmmi->lDiskOffset += cbRead;
		return cbRead;
	}
		break;
	case MMIOM_WRITE:
	case MMIOM_WRITEFLUSH:
	{
		DWORD cbWritten = pFile->m_File.WriteAt((LPVOID) lParam1, lParam2, ULONG(pmmi->lDiskOffset));
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
			return pmmi->lDiskOffset;
			break;
		case SEEK_END:
			pmmi->lDiskOffset = long(pFile->m_File.GetLength()) + lParam1;
			return pmmi->lDiskOffset;
			break;
		case SEEK_SET:
			pmmi->lDiskOffset = lParam1;
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

	// read data at the specified position
	// current position won't change after this function
LONG CMmioFile::ReadAt(void * lpBuf, LONG nCount, LONG Position)
{
	//CSimpleCriticalSectionLock(m_cs);
	return m_File.ReadAt(lpBuf, nCount, Position);
}

void CMmioFile::Close( )
{
	if (m_hmmio != NULL)
	{
		mmioClose(m_hmmio, 0);
		m_hmmio = NULL;
	}
	m_File.Close(0);
}


CWaveFile::CWaveFile()
{
	m_RiffckType = mmioFOURCC('W', 'A', 'V', 'E');
}

CWaveFile & CWaveFile::operator =(CWaveFile & SourceFile)
{
	CMmioFile::operator=(SourceFile);
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
	AllocateCommonData(sizeof (MMCKINFO) + WaveformatSize);
	LPWAVEFORMATEX pWf = GetWaveFormat();

	if (NULL != pWf)
	{
		if (ck.cksize == Read(pWf, ck.cksize))
		{
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
	if (GetCommonDataSize() < sizeof (MMCKINFO) + sizeof (WAVEFORMATEX))
	{
		if (NULL == AllocateCommonData(sizeof (MMCKINFO) + sizeof (WAVEFORMATEX)))
		{
			return FALSE;
		}
	}
	LPMMCKINFO pDatack = (LPMMCKINFO) GetCommonData();
	pDatack->ckid = mmioFOURCC('d', 'a', 't', 'a');
	return FindChunk( * pDatack, GetRiffChunk());
}

//DWORD CAviFile::Seek(DWORD position)
	//{
	//return SetFilePointer(m_hFile, position, NULL, FILE_BEGIN);
	//}

// creates a file based on template format from pTemplateFile
BOOL CWaveFile::CreateWaveFile(CWaveFile * pTemplateFile, int Channel,
								int Samples, DWORD flags, LPCTSTR FileName)
{
	CString name;
	char NameBuf[512];
	// if the name is empty, create a temp name
	if (NULL != FileName
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
				&& 0 != GetFullPathName(pTemplateFile->m_File.GetName(),
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
	if (flags & CreateWaveFileDontInitStructure)
	{
		if (FALSE == Open(name, MmioFileOpenCreateNew | MmioFileOpenDontCreateRiff))
		{
			return FALSE;
		}
		return TRUE;
	}
	// create a file, RIFF list, fmt chunk, data chunk of specified size
	// temp file with this name may already be created
	DWORD OpenFlags = MmioFileOpenCreateAlways;
	if (flags & CreateWaveFileDeleteAfterClose)
	{
		OpenFlags |= MmioFileOpenDeleteAfterClose;
	}
	if (FALSE == Open(name, OpenFlags))
	{
		Close();
		return FALSE;
	}

	if (GetCommonDataSize() < sizeof (MMCKINFO) + sizeof (WAVEFORMATEX))
	{
		if (NULL == AllocateCommonData(sizeof (MMCKINFO) + sizeof (WAVEFORMATEX)))
		{
			Close();
			return FALSE;
		}
	}
	// create new WAVEFORMATEX
	WAVEFORMATEX * pWF = NULL;
	WAVEFORMATEX * pTemplateFormat = NULL;
	if (NULL != pTemplateFile)
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
				if (2 == Channel)
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
			AllocateCommonData(FormatSize + sizeof (MMCKINFO));
			pWF = GetWaveFormat();
			if (pWF)
			{
				memcpy(pWF, pTemplateFormat, FormatSize);
				if (2 == Channel)
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
			if (2 == Channel)
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
	MMCKINFO fck = {mmioFOURCC('f', 'm', 't', ' '), 0, 0, 0, 0};
	CreateChunk(fck, 0);
	Write(pWF, FormatSize);
	Ascend(fck);
	// create data chunk
	LPMMCKINFO pDatachunk = GetDataChunk();
	memset(pDatachunk, 0, sizeof (MMCKINFO));
	pDatachunk->ckid = mmioFOURCC('d', 'a', 't', 'a');
	CreateChunk( * pDatachunk, 0);
	if (Samples)
	{
		size_t DataLength = Samples * SampleSize();
		m_File.SetFileLength(pDatachunk->dwDataOffset + DataLength);
		Seek(DataLength, SEEK_CUR);
	}
	Ascend( * pDatachunk);
	// and copy INFO
	// then update RIFF
	//Ascend( *GetRiffChunk());
	return TRUE;
}

