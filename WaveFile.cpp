// WaveFile.cpp
#include "stdafx.h"
#include "DirectFile.h"
#include "WaveFile.h"

CMmioFile::CMmioFile()
	: m_hmmio(NULL),
	m_dwSize(0)
{
	memset( & m_riffck, 0, sizeof m_riffck);
}

CMmioFile::~CMmioFile()
{
	Close();

}

CMmioFile & CMmioFile::operator=(CMmioFile & SourceFile)
{
	Close();
	m_File.Attach( & SourceFile.m_File);
	m_dwSize = SourceFile.m_dwSize;
	MMIOINFO mmii;
	SourceFile.GetInfo(mmii);
	m_hmmio = mmioOpen(NULL, & mmii, MMIO_READ | MMIO_ALLOCBUF);
	m_riffck.ckid = FOURCC_RIFF;
	m_riffck.cksize = 0;
	m_riffck.dwDataOffset = 0;
	m_riffck.dwFlags = 0;
	Seek(0);
	FindRiff();
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

		DWORD dwSizeHigh = 0;
		m_dwSize = m_File.GetFileSize(& dwSizeHigh);

		if (dwSizeHigh != 0 ||
			(0xFFFFFFFF == m_dwSize && GetLastError() != NO_ERROR))
		{
			Close();
			return FALSE;
		}

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

		m_riffck.ckid = FOURCC_RIFF;
		m_riffck.cksize = 0;
//        m_riffck.fccType = 0; // derived class can set it
		m_riffck.dwDataOffset = 0;
		m_riffck.dwFlags = 0;
		if ( ! FindRiff())
		{
			Close();
			return FALSE;
		}
	}
	else
	{
		// new file created
		m_dwSize = 0;
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

		m_riffck.ckid = FOURCC_RIFF;
		m_riffck.cksize = 0;
//        m_riffck.fccType = 0; // derived class can set it
		m_riffck.dwDataOffset = 0;
		m_riffck.dwFlags = 0;
		if (0 == (nOpenFlags & MmioFileOpenDontCreateRiff))
		{
			CreateRiff( m_riffck);
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
	: m_pWf(NULL)
{
	m_riffck.fccType = mmioFOURCC('W', 'A', 'V', 'E');
}

CWaveFile::CWaveFile( LPCTSTR lpszFileName, UINT nOpenFlags )
	: CMmioFile(), m_pWf(NULL)
{
	Open(lpszFileName, nOpenFlags);
}

CWaveFile & CWaveFile::operator =(CWaveFile & SourceFile)
{
	CMmioFile::operator=(SourceFile);
	// waveformat will be read as needed
	m_datack.cksize = 0;
	m_datack.dwFlags = 0;
	return *this;
}

CWaveFile::~CWaveFile()
{
	if (NULL != m_pWf)
	{
		delete [] (char*) m_pWf;
		m_pWf = NULL;
	}
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
	if (NULL != m_pWf)
	{
		delete [] (char*) m_pWf;
		m_pWf = NULL;
	}

	MMCKINFO ck = {mmioFOURCC('f', 'm', 't', ' '), 0, 0, 0, 0};
	if ( ! FindChunk(ck, & m_riffck))
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
	LPWAVEFORMATEX pWf = (LPWAVEFORMATEX) new char[WaveformatSize];
	memset(pWf, 0, WaveformatSize);
	if (NULL != pWf)
	{
		if (ck.cksize == Read(pWf, ck.cksize))
		{
			m_pWf = pWf;
			return TRUE;
		}
		else
		{
			delete [] (char*) pWf;
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
	m_datack.ckid = mmioFOURCC('d', 'a', 't', 'a');
	return FindChunk(m_datack, & m_riffck);
}

unsigned CWaveFile::SampleRate() const
{
	if (m_pWf)
	{
		return m_pWf->nSamplesPerSec;
	}
	else
	{
		return 0;
	}
}

int CWaveFile::Channels() const
{
	if (m_pWf)
	{
		return m_pWf->nChannels;
	}
	else
	{
		return 0;
	}
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
	// create new WAVEFORMATEX
	WAVEFORMATEX * pWF = NULL;
	int FormatSize = sizeof PCMWAVEFORMAT;
	if (NULL != pTemplateFile
		&& NULL != pTemplateFile->m_pWf)
	{
		if ((flags & CreateWaveFilePcmFormat)
			|| WAVE_FORMAT_PCM == pTemplateFile->m_pWf->wFormatTag)
		{
			pWF = (LPWAVEFORMATEX) new char[sizeof PCMWAVEFORMAT];
			if (pWF)
			{
				pWF->nSamplesPerSec = pTemplateFile->m_pWf->nSamplesPerSec;
				pWF->wFormatTag = WAVE_FORMAT_PCM;
				if (flags & CreateWaveFileTemp)
				{
					pWF->wBitsPerSample = 16;
				}
				else
				{
					pWF->wBitsPerSample = pTemplateFile->m_pWf->wBitsPerSample;
				}
				if (2 == Channel)
				{
					pWF->nChannels = pTemplateFile->m_pWf->nChannels;
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
			FormatSize = sizeof WAVEFORMATEX + pTemplateFile->m_pWf->cbSize;
			pWF = (LPWAVEFORMATEX) new char[FormatSize];
			if (pWF)
			{
				memcpy(pWF, pTemplateFile->m_pWf, FormatSize);
				if (2 == Channel)
				{
					pWF->nChannels = pTemplateFile->m_pWf->nChannels;
				}
				else
				{
					pWF->nChannels = 1;
					if (pTemplateFile->m_pWf->nChannels != 1)
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
		pWF = (LPWAVEFORMATEX) new char[sizeof PCMWAVEFORMAT];
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
	m_pWf = pWF;
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
	// RIFF created in Open()
	MMCKINFO fck = {mmioFOURCC('f', 'm', 't', ' '), 0, 0, 0, 0};
	CreateChunk(fck, 0);
	Write(m_pWf, FormatSize);
	Ascend(fck);
	// create data chunk
	memset( & m_datack, 0, sizeof m_datack);
	m_datack.ckid = mmioFOURCC('d', 'a', 't', 'a');
	CreateChunk(m_datack, 0);
	if (Samples)
	{
		size_t DataLength = Samples * (m_pWf->nChannels * m_pWf->wBitsPerSample / 8);
		m_File.SetFileLength(m_datack.dwDataOffset + DataLength);
		Seek(DataLength, SEEK_CUR);
	}
	Ascend(m_datack);
	// and copy INFO
	// then update RIFF
	Ascend(m_riffck);
	return TRUE;
}

