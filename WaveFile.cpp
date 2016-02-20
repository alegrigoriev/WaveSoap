// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WaveFile.cpp
#include "stdafx.h"
#include "DirectFile.h"
#include "WaveFile.h"
#include <atlbase.h>
#include <atlpath.h>
#include "PathEx.h"
#include <algorithm>
#include <atlfile.h>
#include "resource.h"

#define DEBUG_RESCAN_PEAKS 0

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

#ifndef OVERRIDE_MMLIB_FUNCTIONS
	MMIOINFO mmii;
	memzero(mmii);
	mmii.fccIOProc = 0;
	mmii.pIOProc = BufferedIOProc;

	mmii.adwInfo[0] = (DWORD)(DWORD_PTR)this;   // TODO
	m_hmmio = mmioOpen(NULL, & mmii, MMIO_READ //| MMIO_ALLOCBUF
						);
#endif
	Seek(0, FILE_BEGIN);
	return *this;
}

BOOL CMmioFile::Open(LPCTSTR szFileName, long nOpenFlags)
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

#ifndef OVERRIDE_MMLIB_FUNCTIONS
		MMIOINFO mmii;
		memzero(mmii);
		mmii.fccIOProc = 0;
		mmii.pIOProc = BufferedIOProc;

		mmii.adwInfo[0] = (DWORD)(DWORD_PTR)this;  // TODO

		m_hmmio = mmioOpen(NULL, & mmii,
							MMIO_READ //| MMIO_ALLOCBUF
							);

		if (NULL == m_hmmio)
		{
			Close();
			return FALSE;
		}
#endif

		AllocateInstanceData<InstanceDataMm>();

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

#ifndef OVERRIDE_MMLIB_FUNCTIONS
		MMIOINFO mmii;
		memzero(mmii);
		mmii.fccIOProc = 0;
		mmii.pIOProc = BufferedIOProc;

		mmii.adwInfo[0] = (DWORD)(DWORD_PTR)this;  // TODO

		m_hmmio = mmioOpen(NULL, & mmii,
							MMIO_READ | MMIO_WRITE //| MMIO_ALLOCBUF
							);

		if (NULL == m_hmmio)
		{
			Close();
			return FALSE;
		}
#endif
		LPMMCKINFO pRiffck = & AllocateInstanceData<InstanceDataMm>()->riffck;

		if (0 == (nOpenFlags & MmioFileOpenDontCreateRiff))
		{
			if (FOURCC_RIFF != pRiffck->ckid)
			{
				// if RIFF chunk was filled before, it means that it is already read
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
		}
	}
	return TRUE;
}

BOOL CMmioFile::LoadRiffChunk()
{
	LPMMCKINFO pRiffck = & AllocateInstanceData<InstanceDataMm>()->riffck;
	if (FOURCC_RIFF == pRiffck->ckid)
	{
		// RIFF chunk is already read
		return TRUE;
	}

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
	// if RIFF chunk was allocated before, it means that it is already read
	return TRUE;
}

#ifndef OVERRIDE_MMLIB_FUNCTIONS
LRESULT PASCAL CMmioFile::BufferedIOProc(LPSTR lpmmioinfo, UINT wMsg,
										LPARAM lParam1, LPARAM lParam2)
{
	LPMMIOINFO pmmi = (LPMMIOINFO) lpmmioinfo;
	CMmioFile * pFile = (CMmioFile *) (DWORD_PTR)pmmi->adwInfo[0];

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
		DWORD cbRead = pFile->ReadAt((LPVOID) lParam1, (long)lParam2, pmmi->lDiskOffset);
		if (-1 == cbRead)
		{
			return -1;
		}
		pmmi->lDiskOffset += cbRead;
		return cbRead;
	}
		break;
	case MMIOM_WRITEFLUSH:
		TRACE("MMIOM_WRITEFLUSH\n");
	case MMIOM_WRITE:
	{
		//TRACE("MMIOM_WRITE at %08x, %x bytes\n", pmmi->lDiskOffset, lParam2);
		DWORD cbWritten = pFile->WriteAt((LPVOID) lParam1, (long)lParam2, ULONG(pmmi->lDiskOffset));
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
			pmmi->lDiskOffset += (LONG)lParam1;
			//TRACE("MMIOM_SEEK SEEK_CUR pmmi->lDiskOffset=%08x\n", pmmi->lDiskOffset);
			return pmmi->lDiskOffset;
			break;
		case SEEK_END:
			pmmi->lDiskOffset = long(pFile->CDirectFile::GetLength()) + lParam1;
			//TRACE("MMIOM_SEEK SEEK_END pmmi->lDiskOffset=%08x\n", pmmi->lDiskOffset);
			return pmmi->lDiskOffset;
			break;
		case SEEK_SET:
			pmmi->lDiskOffset = (LONG)lParam1;
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
#endif

void CMmioFile::Close( )
{
#ifndef OVERRIDE_MMLIB_FUNCTIONS
	if (m_hmmio != NULL)
	{
		mmioClose(m_hmmio, 0);
		m_hmmio = NULL;
	}
#endif
	BaseClass::Close(0);
}

#ifdef OVERRIDE_MMLIB_FUNCTIONS
BOOL CMmioFile::Descend(MMCKINFO & ck, LPMMCKINFO lpckParent, UINT uFlags)
{
	if ( ! IsOpen()
		|| (NULL != lpckParent && (MEDIA_FILE_POSITION)GetFilePointer() < lpckParent->dwDataOffset))
	{
		return FALSE;
	}

	FOURCC TypeToFind = 0;
	if (uFlags & MMIO_FINDRIFF)
	{
		TypeToFind = FOURCC_RIFF;
	}
	else if (uFlags & MMIO_FINDLIST)
	{
		TypeToFind = FOURCC_LIST;
	}
	else if (uFlags & MMIO_FINDCHUNK)
	{
		TypeToFind = ck.ckid;
	}

	// uFlags can be: MMIO_FINDCHUNK, MMIO_FINDLIST, MMIO_FINDRIFF
	while (NULL == lpckParent
			|| (MEDIA_FILE_POSITION)GetFilePointer() + 8 <= lpckParent->dwDataOffset + lpckParent->cksize)
	{
		DWORD ckhdr[2];
		if (sizeof (ckhdr) != Read(ckhdr, sizeof ckhdr))
		{
			return FALSE;
		}

		if (0 == TypeToFind || TypeToFind == ckhdr[0])
		{

			if (0 == (uFlags & (MMIO_FINDRIFF | MMIO_FINDLIST)))
			{
				ck.cksize = ckhdr[1];
				ck.dwDataOffset = GetFilePointer();
				ck.ckid = ckhdr[0];
				ck.dwFlags = 0;
				ck.fccType = 0;

				return TRUE;
			}
			// RIFF or LIST
			if (ckhdr[0] < sizeof (DWORD))
			{
				return FALSE;
			}

			DWORD fccType;
			if (sizeof (fccType) != Read(& fccType, sizeof fccType))
			{
				return FALSE;
			}

			if (0 == ck.fccType
				|| ck.fccType == fccType)
			{
				ck.cksize = ckhdr[1];
				ck.dwDataOffset = GetFilePointer() - sizeof (fccType);
				ck.ckid = ckhdr[0];
				ck.dwFlags = 0;
				ck.fccType = fccType;
				return TRUE;
			}
			ckhdr[1] -= sizeof (fccType);
		}
		// skip the chunk (align to even boundary)
		Seek((ckhdr[1] + 1) & -2i64, FILE_CURRENT);
	}
	return FALSE;
}

BOOL CMmioFile::Ascend(MMCKINFO & ck)
{
	if (ck.dwFlags & MMIO_DIRTY)
	{
		// pad the chunk, update the length
		LONGLONG Length = GetFilePointer() - ck.dwDataOffset;
		if (Length < 0 || Length >= 0xFFFFFFFFUi64)
		{
			return FALSE;
		}
		DWORD ckhdr[2];

		if (ck.dwDataOffset < sizeof ckhdr)
		{
			return FALSE;
		}
		// update the chunk header
		ckhdr[0] = ck.ckid;
		ckhdr[1] = DWORD(Length & 0xFFFFFFFF);
		if (sizeof ckhdr != WriteAt(ckhdr, sizeof ckhdr, ck.dwDataOffset - sizeof ckhdr))
		{
			return FALSE;
		}
		ck.cksize = ckhdr[1];
		UCHAR pad = 0;
		// pad data, if necessary
		if ((ck.cksize & 1)
			&& 1 != Write( & pad, 1))
		{
			return FALSE;
		}
		ck.dwFlags &= ~MMIO_DIRTY;
		// the current position is already set
		return TRUE;
	}
	Seek((ck.dwDataOffset + ck.cksize + 1) & -2i64);
	return TRUE;
}

BOOL CMmioFile::CreateChunk(MMCKINFO & ck, UINT wFlags)
{
	ck.dwFlags |= MMIO_DIRTY;
	// align to an even boundary?
	if (GetFilePointer() & 1)
	{
		Seek(1, FILE_CURRENT);
	}

	DWORD ckhdr[2] = { ck.ckid, ck.cksize};
	if (wFlags & MMIO_CREATERIFF)
	{
		ck.ckid = FOURCC_RIFF;
	}
	else if (wFlags & MMIO_CREATELIST)
	{
		ck.ckid = FOURCC_LIST;
	}
	if (sizeof ckhdr != Write(ckhdr, sizeof ckhdr))
	{
		return FALSE;
	}
	ck.dwDataOffset = GetFilePointer();
	if (wFlags & (MMIO_CREATERIFF | MMIO_CREATELIST))
	{
		if (sizeof ck.fccType != Write( & ck.fccType, sizeof ck.fccType))
		{
			return FALSE;
		}
	}
	return TRUE;
}
#endif

CWaveFile::CWaveFile()
	: m_FactSamples(~0UL)
{
	m_RiffckType = mmioFOURCC('W', 'A', 'V', 'E');
}

CWaveFile::CWaveFile(CWaveFile & f)
{
	m_RiffckType = mmioFOURCC('W', 'A', 'V', 'E');
	*this = f;
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

BOOL CWaveFile::Open( LPCTSTR lpszFileName, long nOpenFlags)
{
	if ( ! CMmioFile::Open(lpszFileName, nOpenFlags))
	{
		return FALSE;
	}
	AllocateInstanceData<InstanceDataWav>();
	return TRUE;
}

void CWaveFile::Close( )
{
	CMmioFile::Close();
	m_FactSamples = ~0UL;
}

BOOL CWaveFile::LoadWaveformat()
{

	MMCKINFO ck = {mmioFOURCC('f', 'm', 't', ' '), 0, 0, 0, 0};
	if ( ! FindChunk(ck, GetRiffChunk()))
	{
		return FALSE;
	}
	if (ck.cksize > 0xFFF0  // 64K
		|| ck.cksize < sizeof (WAVEFORMAT))
	{
		TRACE("fmt chunk is too small or too big: > 64K\n");
		Ascend(ck);
		return FALSE;
	}

	MMCKINFO * pFmtCk = GetFmtChunk();
	if (NULL == pFmtCk)
	{
		return FALSE;
	}
	// save format chunk in the instance struct
	*pFmtCk = ck;

	// allocate structure
	LPWAVEFORMATEX pWf = AllocateWaveformat(ck.cksize);

	if (NULL != pWf)
	{
		if (ck.cksize == (DWORD) Read(pWf, (LONG)ck.cksize))
		{
			Ascend(ck);
			// try to find 'fact' chunk
			MMCKINFO * pFact = GetFactChunk();
			pFact->ckid = mmioFOURCC('f', 'a', 'c', 't');
			// save current position
			MEDIA_FILE_POSITION CurrPos = Seek(0, SEEK_CUR);
			m_FactSamples = ~0LU;
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
	LPMMCKINFO pDatack = & AllocateInstanceData<InstanceDataWav>()->datack;
	pDatack->ckid = mmioFOURCC('d', 'a', 't', 'a');
	return FindChunk( * pDatack, GetRiffChunk());
}

BOOL CWaveFile::LoadMetadata()
{
	InstanceDataWav * pInstData = AllocateInstanceData<InstanceDataWav>();
	pInstData->ResetMetadata();

	DWORD id;
	MMCKINFO chunk;
	CMmioFile::Seek(pInstData->riffck.dwDataOffset + 4);

	while(Descend(chunk, & pInstData->riffck))
	{
		// scan subchunks and parse for data
		switch (chunk.ckid)
		{
		case FOURCC_LIST:
			if (sizeof chunk.fccType != Read( & chunk.fccType, sizeof chunk.fccType)
				|| ! LoadListMetadata(chunk))
			{
				return FALSE;
			}
			break;
		case mmioFOURCC('c', 'u', 'e', ' '):
			if ( ! ReadCueSheet(chunk))
			{
				return FALSE;
			}
			break;
		case mmioFOURCC('p', 'l', 's', 't'):
			if ( ! ReadPlaylist(chunk))
			{
				return FALSE;
			}
			break;
		case mmioFOURCC('D', 'I', 'S', 'P'):
			if (chunk.cksize < 5
				|| sizeof id != Read( & id, sizeof id))
			{
				return FALSE;
			}
			if (CF_TEXT == id
				|| ! ReadChunkString(chunk.cksize - sizeof id,
									pInstData->m_DisplayTitle))
			{
				return FALSE;
			}
			// TODO: read arbitrary DISP types ( they can be several in one file
			break;
		}
		Ascend(chunk);
	}
	return TRUE;
}

BOOL CMmioFile::ReadChunkString(ULONG Length, CStringA & String)
{
	// the string may be UTF16 or UTF8, or ANSI
	if (0 == Length)
	{
		String.Empty();
		return TRUE;
	}

	CStringA s;
	LPSTR p = s.GetBuffer(Length);

	if (Length != (ULONG)Read(p, Length))
	{
		return FALSE;
	}
	p[Length - 1] = 0;
	s.ReleaseBuffer();
	Length = s.GetLength();

	if (Length >= 3
		&& UCHAR(s[0]) == 0xFE
		&& UCHAR(s[1]) == 0xFF)
	{
		// UNICODE marker
		Length = (Length - 1) & ~1;
		s.SetAt(Length, 0);
		s.SetAt(Length + 1, 0);

		String = PCWSTR(LPCSTR(s) + 2);
	}
	else if (Length >= 4
			&& UCHAR(s[0]) == 0xEF
			&& UCHAR(s[1]) == 0xBB
			&& UCHAR(s[2]) == 0xBF)
	{
		// UTF-8
		CStringW stringW;
		Length -= 3;

		int result = ::MultiByteToWideChar(CP_UTF8, 0, LPCSTR(s) + 3, Length,
											stringW.GetBuffer(Length), Length);

		stringW.ReleaseBuffer(result);
		String = stringW;
	}
	else
	{
		String = s;
	}
	return TRUE;
}

BOOL CMmioFile::ReadChunkString(ULONG Length, CStringW & String)
{
	// the string may be unicode or text
	if (0 == Length)
	{
		String.Empty();
		return TRUE;
	}

	CStringA s;
	LPSTR p = s.GetBuffer(Length);

	if (Length != (ULONG)Read(p, Length))
	{
		return FALSE;
	}
	p[Length - 1] = 0;
	s.ReleaseBuffer();
	Length = s.GetLength();

	if (Length >= 3
		&& UCHAR(s[0]) == 0xFE
		&& UCHAR(s[1]) == 0xFF)
	{
		// UNICODE marker
		Length = (Length - 1) & ~1;
		s.SetAt(Length, 0);
		s.SetAt(Length + 1, 0);

		String = PCWSTR(LPCSTR(s) + 2);
	}
	else if (Length >= 4
			&& UCHAR(s[0]) == 0xEF
			&& UCHAR(s[1]) == 0xBB
			&& UCHAR(s[2]) == 0xBF)
	{
		// UTF-8
		Length -= 3;
		int result = ::MultiByteToWideChar(CP_UTF8, 0, LPCSTR(s) + 3, Length,
											String.GetBuffer(Length), Length);

		String.ReleaseBuffer(result);
	}
	else
	{
		String = s;
	}
	return TRUE;
}

BOOL CMmioFile::ReadChunkStringW(ULONG Length, CStringW & String)
{
	// the string is unicode
	if (0 == Length)
	{
		String.Empty();
		return TRUE;
	}

	CStringW s;
	if (Length != (ULONG)Read(s.GetBuffer(Length / sizeof (WCHAR)), Length))
	{
		return FALSE;
	}

	s.ReleaseBuffer(Length - 1);
	String = s;
	return TRUE;
}

// the function returns number of bytes written. If -1, means failure
int CMmioFile::WriteChunkString(CStringW const & String)    // will save as either ANSI, or UTF8 or UTF-16
{
	int StrLen = String.GetLength();
	switch (m_TextEncodingInFiles)
	{
	case 0:
	default:
		// ANSI, no BOM
		return WriteChunkString(CStringA(String));
		break;
	case 1:
		// UTF-8
	{
		CStringA s("\xEF\xBB\xBF"); // byte order mark
		int length = ::WideCharToMultiByte(CP_UTF8, 0, String, StrLen, NULL, 0, NULL, NULL);

		if (length != 0)
		{
			length = ::WideCharToMultiByte(CP_UTF8, 0, String, StrLen,
											3 + s.GetBuffer(length + 3), length, NULL, NULL);

			length += 3;
			s.ReleaseBuffer(length);
			length++;   // zero-terminator

			if ((length) != Write(s, length))
			{
				return -1;
			}
		}
		return length;
	}
		break;
	case 2:
		// UTF-16, BOM
		if (0 != StrLen)
		{
			WCHAR BOM = 0xFFFE;

			StrLen = (StrLen + 1) * sizeof (WCHAR);
			if (sizeof BOM != Write( & BOM, sizeof BOM)
				|| StrLen != Write(String, StrLen))
			{
				return -1;
			}
			return StrLen;
		}
		else
		{
			return 0;
		}
		break;
	}
}

int CMmioFile::WriteChunkString(CStringA const & String)
{
	// will save as ANSI only
	int Length = String.GetLength();
	if (0 != Length)
	{
		Length++;
		if (Length != Write(String, Length))
		{
			return -1;
		}
	}
	return Length;
}

// the function returns number of bytes written. If -1, means failure
int CMmioFile::ChunkStringLength(CStringW const & String) const   // as either ANSI, or UTF8 or UTF-16
{
	int StrLen = String.GetLength();
	switch (m_TextEncodingInFiles)
	{
	case 0:
	default:
		// ANSI, no BOM
		return ChunkStringLength(CStringA(String));
		break;

	case 1:
		// UTF-8
	{
		int length = ::WideCharToMultiByte(CP_UTF8, 0, String, StrLen, NULL, 0, NULL, NULL);

		if (length != 0)
		{
			length += 4;
		}
		return length;
	}
		break;
	case 2:
		// UTF-16, BOM
		if (0 != StrLen)
		{
			// BOM and zero-terminator included
			return (StrLen + 2) * sizeof (WCHAR);
		}
		else
		{
			return 0;
		}
		break;
	}
}

int CMmioFile::ChunkStringLength(CStringA const & String) const
{
	// will save as ANSI only
	unsigned Length = String.GetLength();
	if (0 != Length)
	{
		Length++;
	}
	return Length;
}

int CMmioFile::WriteChunkStringW(CStringW const & String)
{
	// write UNICODE string, but without BOM
	int Length = String.GetLength() * sizeof (WCHAR);
	if (0 != Length)
	{
		Length += sizeof (WCHAR);
		if (Length != Write(String, Length))
		{
			return -1;
		}
	}
	return Length;
}

BOOL CWaveFile::ReadCueSheet(MMCKINFO & chunk)
{
	InstanceDataWav * pInstData = AllocateInstanceData<InstanceDataWav>();
	DWORD count;

	if (chunk.cksize < sizeof count
		|| sizeof count != Read( & count, sizeof count))
	{
		return FALSE;
	}
	if (count * sizeof (CuePointChunkItem) + sizeof count != chunk.cksize)
	{
		return FALSE;
	}

	pInstData->m_CuePoints.clear();
	pInstData->m_CuePoints.reserve(count);

	for (unsigned i = 0; i < count; i++)
	{
		CuePointChunkItem item;
		if (sizeof item != Read( & item, sizeof item)
			|| item.fccChunk != 'atad')
		{
			return FALSE;
		}

		pInstData->m_CuePoints.push_back(item);
	}

	return TRUE;
}

BOOL CWaveFile::ReadPlaylist(MMCKINFO & chunk)
{
	InstanceDataWav * pInstData = AllocateInstanceData<InstanceDataWav>();
	DWORD count;
	if (chunk.cksize < sizeof count
		|| sizeof count != Read( & count, sizeof count))
	{
		return FALSE;
	}

	if (count * sizeof (PlaylistSegment) + sizeof count != chunk.cksize)
	{
		return FALSE;
	}

	pInstData->m_Playlist.clear();
	pInstData->m_Playlist.reserve(count);

	for (unsigned i = 0; i < count; i++)
	{
		PlaylistSegment item;
		if (sizeof item != Read( & item, sizeof item))
		{
			return FALSE;
		}

		pInstData->m_Playlist.push_back(item);
	}
	return TRUE;
}

CuePointChunkItem * CWaveFile::GetCuePoint(DWORD CueId)
{
	return GetInstanceData()->GetCuePoint(CueId);
}

CuePointChunkItem const * CWaveFile::GetCuePoint(DWORD CueId) const
{
	return GetInstanceData()->GetCuePoint(CueId);
}

CuePointChunkItem * CWaveFile::InstanceDataWav::GetCuePoint(DWORD CueId)
{
	for (CuePointVectorIterator i = m_CuePoints.begin();
		i != m_CuePoints.end(); i++)
	{
		if (CueId == i->CuePointID)
		{
			return i.operator->();
		}
	}

	return NULL;
}

CuePointChunkItem const * CWaveFile::InstanceDataWav::GetCuePoint(DWORD CueId) const
{
	for (ConstCuePointVectorIterator i = m_CuePoints.begin();
		i != m_CuePoints.end(); i++)
	{
		if (CueId == i->CuePointID)
		{
			return i.operator->();
		}
	}

	return NULL;
}

LPCTSTR CWaveFile::GetCueLabel(DWORD CueId) const
{
	return GetInstanceData()->GetCueLabel(CueId);
}

LPCTSTR CWaveFile::InstanceDataWav::GetCueLabel(DWORD CueId) const
{
	for (ConstLabelVectorIterator i = m_Labels.begin();
		i != m_Labels.end(); i++)
	{
		if (CueId == i->CuePointID)
		{
			return i->Text;
		}
	}
	return NULL;
}

LPCTSTR CWaveFile::GetCueComment(DWORD CueId) const
{
	return GetInstanceData()->GetCueComment(CueId);
}

LPCTSTR CWaveFile::InstanceDataWav::GetCueComment(DWORD CueId) const
{
	for (ConstLabelVectorIterator i = m_Notes.begin();
		i != m_Notes.end(); i++)
	{
		if (CueId == i->CuePointID)
		{
			return i->Text;
		}
	}
	return NULL;
}

LPCTSTR CWaveFile::GetCueText(DWORD CueId) const
{
	return GetInstanceData()->GetCueText(CueId);
}

LPCTSTR CWaveFile::InstanceDataWav::GetCueText(DWORD CueId) const
{
	LPCTSTR text = GetCueLabel(CueId);
	if (NULL != text
		&& 0 != text[0])
	{
		return text;
	}

	text = GetCueComment(CueId);
	if (NULL != text
		&& 0 != text[0])
	{
		return text;
	}

	WaveRegionMarker const * pMarker = GetRegionMarker(CueId);
	if (NULL != pMarker
		&& ! pMarker->Name.IsEmpty())
	{
		return pMarker->Name;
	}

	return NULL;
}

LPCTSTR CWaveFile::GetCueTextByIndex(unsigned CueIndex) const
{
	return GetInstanceData()->GetCueTextByIndex(CueIndex);
}

LPCTSTR CWaveFile::InstanceDataWav::GetCueTextByIndex(unsigned CueIndex) const
{
	if (CueIndex < m_CuePoints.size())
	{
		return GetCueText(m_CuePoints[CueIndex].CuePointID);
	}

	return NULL;
}

WaveRegionMarker * CWaveFile::GetRegionMarker(DWORD CueId)
{
	return GetInstanceData()->GetRegionMarker(CueId);
}

WaveRegionMarker const * CWaveFile::GetRegionMarker(DWORD CueId) const
{
	return GetInstanceData()->GetRegionMarker(CueId);
}

WaveRegionMarker * CWaveFile::InstanceDataWav::GetRegionMarker(DWORD CueId)
{
	for (RegionMarkerIterator i = m_RegionMarkers.begin();
		i != m_RegionMarkers.end(); i++)
	{
		if (CueId == i->CuePointID)
		{
			return i.operator->();
		}
	}
	return NULL;
}

WaveRegionMarker const * CWaveFile::InstanceDataWav::GetRegionMarker(DWORD CueId) const
{
	for (ConstRegionMarkerIterator i = m_RegionMarkers.begin();
		i != m_RegionMarkers.end(); i++)
	{
		if (CueId == i->CuePointID)
		{
			return i.operator->();
		}
	}
	return NULL;
}

// this function is called on removal or insertion of data
// Markers inside the replaced area are removed, markers after the area are adjusted
// returns TRUE if the markers has changed
BOOL CWaveFile::InstanceDataWav::MoveMarkers(SAMPLE_INDEX BeginSample, NUMBER_OF_SAMPLES SrcLength, NUMBER_OF_SAMPLES DstLength)
{
	BOOL HasChanged = FALSE;

	for (CuePointVectorIterator i = m_CuePoints.begin();
		i != m_CuePoints.end(); )
	{
		CuePointVectorIterator next = i + 1;

		WaveRegionMarker * pMarker = GetRegionMarker(i->CuePointID);

		if (NULL != pMarker)
		{
			DWORD RegionEnd = i->dwSampleOffset + pMarker->SampleLength;

			if (RegionEnd < unsigned(BeginSample))
			{
				// do nothing
			}
			else if (RegionEnd == unsigned(BeginSample))
			{
				if (SrcLength == 0
					&& DstLength != 0
					&& pMarker->SampleLength != 0)
				{
					pMarker->SampleLength += DstLength;
					HasChanged = TRUE;
				}
			}
			else if (i->dwSampleOffset >= unsigned(BeginSample + SrcLength))
			{
				i->dwSampleOffset += DstLength - SrcLength;
				HasChanged = TRUE;
			}
			else if (i->dwSampleOffset <= unsigned(BeginSample))
			{
				// change length only
				if (RegionEnd < unsigned(BeginSample + SrcLength))
				{
					pMarker->SampleLength = BeginSample - i->dwSampleOffset;
				}
				else
				{
					pMarker->SampleLength += DstLength - SrcLength;
				}
				HasChanged = TRUE;
			}
			else if (RegionEnd >= unsigned(BeginSample + SrcLength))
			{
				// change length only and the start
				// the start of region is moved to end of range
				i->dwSampleOffset = BeginSample + DstLength;
				pMarker->SampleLength = RegionEnd - SrcLength;
				HasChanged = TRUE;
			}
			else
			{
				// delete it and the region
				for (RegionMarkerIterator p = m_RegionMarkers.begin(); p != m_RegionMarkers.end(); )
				{
					if (p->CuePointID == i->CuePointID)
					{
						p = m_RegionMarkers.erase(p);
					}
					else
					{
						p++;
					}
				}
				// delete the cue point
				WAVEREGIONINFO info = { WAVEREGIONINFO::ChangeLabel | WAVEREGIONINFO::ChangeComment };
				info.MarkerCueID = i->CuePointID;
				SetWaveMarker( & info);     // remove label and comment

				next = m_CuePoints.erase(i);

				// delete from playlist
				for (PlaylistVectorIterator p = m_Playlist.begin(); p != m_Playlist.end(); )
				{
					if (p->CuePointID == i->CuePointID)
					{
						p = m_Playlist.erase(p);
					}
					else
					{
						p++;
					}
				}

				HasChanged = TRUE;
			}
		}
		else
		{
			if (i->dwSampleOffset <= unsigned(BeginSample))
			{
				// nothing
			}
			else if (i->dwSampleOffset >= unsigned(BeginSample + SrcLength))
			{
				i->dwSampleOffset += DstLength - SrcLength;
				HasChanged = TRUE;
			}
			else
			{
				// delete the cue point
				WAVEREGIONINFO info = { WAVEREGIONINFO::ChangeLabel | WAVEREGIONINFO::ChangeComment };
				info.MarkerCueID = i->CuePointID;
				SetWaveMarker( & info);     // remove label and comment

				next = m_CuePoints.erase(i);

				// delete from playlist
				for (PlaylistVectorIterator p = m_Playlist.begin(); p != m_Playlist.end(); )
				{
					if (p->CuePointID == i->CuePointID)
					{
						p = m_Playlist.erase(p);
					}
					else
					{
						p++;
					}
				}

				HasChanged = TRUE;
			}
		}
		i = next;
	}

	if (HasChanged)
	{
		m_InfoChanged = true;
	}
	return HasChanged;
}

// copy marker data:
// copy those that fall into SrcBegin...SrcBegin+Length.
// offset their position to DstBegin-SrcBegin
BOOL CWaveFile::InstanceDataWav::CopyMarkers(InstanceDataWav const * pSrc,
											SAMPLE_INDEX SrcBegin, SAMPLE_INDEX DstBegin, NUMBER_OF_SAMPLES Length)
{
	BOOL HasChanged = FALSE;

	for (int i = 0; ; i++)
	{
		WAVEREGIONINFO SrcInfo = { WAVEREGIONINFO::CuePointIndex };
		SrcInfo.MarkerCueID = i;
		if ( ! pSrc->GetWaveMarker( & SrcInfo))
		{
			break;
		}

		// see if we need to copy it
		if (SrcInfo.Sample + SrcInfo.Length > DWORD(SrcBegin + Length)
			|| SrcInfo.Sample < DWORD(SrcBegin))
		{
			// skip it
			continue;
		}

		// adjust sample
		SrcInfo.Sample += DstBegin - SrcBegin;

		WAVEREGIONINFO DstInfo = SrcInfo;
		DstInfo.Flags = DstInfo.FindCue;

		if (GetWaveMarker( & DstInfo)
			&& (SrcInfo.Label == DstInfo.Label
				|| (SrcInfo.Label != NULL && DstInfo.Label != NULL
					&& 0 == _tcscmp(SrcInfo.Label, DstInfo.Label)))
			&& (SrcInfo.Comment == DstInfo.Comment
				|| (SrcInfo.Comment != NULL && DstInfo.Comment != NULL
					&& 0 == _tcscmp(SrcInfo.Comment, DstInfo.Comment)))
			&& (SrcInfo.Ltxt == DstInfo.Ltxt
				|| (SrcInfo.Ltxt != NULL && DstInfo.Ltxt != NULL && 0 == _tcscmp(SrcInfo.Ltxt, DstInfo.Ltxt))))
		{
			continue;
		}

		SrcInfo.Flags = SrcInfo.AddNew;
		SetWaveMarker( & SrcInfo);
		HasChanged = TRUE;
	}

	if (HasChanged)
	{
		m_InfoChanged = true;
	}
	return HasChanged;
}

// reverse markers inside reversed area
BOOL CWaveFile::InstanceDataWav::ReverseMarkers(SAMPLE_INDEX BeginSample, NUMBER_OF_SAMPLES Length)
{
	BOOL HasChanged = FALSE;

	for (CuePointVectorIterator i = m_CuePoints.begin();
		i != m_CuePoints.end(); i++)
	{
		WaveRegionMarker * pMarker = GetRegionMarker(i->CuePointID);

		if (NULL != pMarker)
		{
			DWORD RegionEnd = i->dwSampleOffset + pMarker->SampleLength;

			if (RegionEnd <= unsigned(BeginSample)
				|| i->dwSampleOffset >= unsigned(BeginSample + Length))
			{
				// do nothing
			}
			else if (i->dwSampleOffset >= unsigned(BeginSample)
					&& RegionEnd <= unsigned(BeginSample + Length))
			{
				// the region is all inside the area
				i->dwSampleOffset = BeginSample * 2 + Length - RegionEnd;
				HasChanged = TRUE;
			}
			else if (RegionEnd < unsigned(BeginSample + Length))
			{
				// the region begins before the area
				// exclude the area from it
				pMarker->SampleLength = BeginSample - i->dwSampleOffset;
				HasChanged = TRUE;
			}
			else if (i->dwSampleOffset > unsigned(BeginSample))
			{
				// the region ends after the area
				// exclude the area from it
				pMarker->SampleLength = RegionEnd - (BeginSample + Length);
				i->dwSampleOffset = BeginSample + Length;
				HasChanged = TRUE;
			}
		}
		else
		{
			if (i->dwSampleOffset >= unsigned(BeginSample)
				&& i->dwSampleOffset <= unsigned(BeginSample + Length))
			{
				i->dwSampleOffset = BeginSample * 2 + Length - i->dwSampleOffset;
				HasChanged = TRUE;
			}
		}
	}

	if (HasChanged)
	{
		m_InfoChanged = true;
	}
	return HasChanged;
}

BOOL CWaveFile::InstanceDataWav::RescaleMarkers(long OldSampleRate, long NewSampleRate)
{
	BOOL HasChanged = FALSE;

	if (OldSampleRate == NewSampleRate)
	{
		return FALSE;
	}

	for (CuePointVectorIterator i = m_CuePoints.begin();
		i != m_CuePoints.end(); i++)
	{
		WaveRegionMarker * pMarker = GetRegionMarker(i->CuePointID);

		if (NULL != pMarker)
		{
			DWORD RegionEnd = i->dwSampleOffset + pMarker->SampleLength;

			i->dwSampleOffset = MulDiv(i->dwSampleOffset, NewSampleRate, OldSampleRate);
			RegionEnd = MulDiv(RegionEnd, NewSampleRate, OldSampleRate);

			pMarker->SampleLength = RegionEnd - i->dwSampleOffset;
		}
		else
		{
			i->dwSampleOffset = MulDiv(i->dwSampleOffset, NewSampleRate, OldSampleRate);
		}

		HasChanged = TRUE;
	}

	if (HasChanged)
	{
		m_InfoChanged = true;
	}
	return HasChanged;
}

CWaveFile::InstanceDataWav::InstanceDataWav()
	: m_PeakData(512)
	, m_InfoChanged(false)
	, m_FreeCuePointNumber(0)
{
	memzero(datack);
	memzero(fmtck);
	memzero(factck);
	m_size = sizeof *this;
}

BOOL CWaveFile::LoadListMetadata(MMCKINFO & chunk)
{
	// fccType is already read
	MMCKINFO subchunk;
	InstanceDataWav * pInstData = AllocateInstanceData<InstanceDataWav>();

	switch (chunk.fccType)
	{
	case mmioFOURCC('I', 'N', 'F', 'O'):
		while(Descend(subchunk, & chunk))
		{
			InfoListItemA item;
			item.fccCode = subchunk.ckid;
			BOOL res = ReadChunkString(subchunk.cksize, item.Text);

			if ( ! res)
			{
				return FALSE;
			}

			pInstData->m_InfoList.push_back(item);

			Ascend(subchunk);
		}
		break;

	case mmioFOURCC('U', 'N', 'F', 'O'):    // UNICODE INFO
		while(Descend(subchunk, & chunk))
		{
			InfoListItemW item;
			item.fccCode = subchunk.ckid;
			BOOL res = ReadChunkStringW(subchunk.cksize, item.Text);

			if ( ! res)
			{
				return FALSE;
			}

			pInstData->m_InfoListW.push_back(item);

			Ascend(subchunk);
		}
		break;

	case mmioFOURCC('a', 'd', 't', 'l'):
		while(Descend(subchunk, & chunk))
		{
			BOOL res = TRUE;

			switch (subchunk.ckid)
			{
			case mmioFOURCC('l', 't', 'x', 't'):
			{
				WaveRegionMarker Region;
				if (subchunk.cksize < sizeof (LtxtChunk)
					|| sizeof (LtxtChunk) != Read(static_cast<LtxtChunk *>( & Region),
												sizeof (LtxtChunk)))
				{
					return FALSE;
				}

				if (subchunk.cksize > sizeof (LtxtChunk))
				{
					// read the text
					res = ReadChunkString(subchunk.cksize - sizeof (LtxtChunk), Region.Name);
				}

				pInstData->m_RegionMarkers.push_back(Region);
			}
				break;

			case mmioFOURCC('l', 'a', 'b', 'l'):
				// label for a cue point
			{
				LablNote labl;
				if (subchunk.cksize < 5
					|| sizeof labl.CuePointID != Read( & labl.CuePointID, sizeof labl.CuePointID))
				{
					return FALSE;
				}

				if (ReadChunkString(subchunk.cksize - 4,
									labl.Text))
				{
					pInstData->m_Labels.push_back(labl);
				}
			}
				break;
			case mmioFOURCC('n', 'o', 't', 'e'):
				// comment for a cue point
			{
				LablNote note;
				if (subchunk.cksize < 5
					|| sizeof note.CuePointID != Read( & note.CuePointID, sizeof note.CuePointID))
				{
					return FALSE;
				}

				if (ReadChunkString(subchunk.cksize - 4,
									note.Text))
				{
					pInstData->m_Notes.push_back(note);
				}
			}
				break;
			}

			if ( ! res)
			{
				return FALSE;
			}
			Ascend(subchunk);
		}
		break;
	}
	return TRUE;
}

BOOL CWaveFile::SetWaveMarker(WAVEREGIONINFO * info)
{
	if ( ! GetInstanceData()->SetWaveMarker(info))
	{
		return FALSE;
	}
	if (info->Flags & info->CommitChanges)
	{
		return SaveMetadata();
	}
	return TRUE;
}

BOOL CWaveFile::GetWaveMarker(WAVEREGIONINFO * info) const
{
	return GetInstanceData()->GetWaveMarker(info);
}

BOOL CWaveFile::InstanceDataWav::GetWaveMarker(WAVEREGIONINFO * pInfo) const
{
	BOOL result = FALSE;
	if (pInfo->Flags & pInfo->FindCue)
	{
		// either find an existing cue, or mark it as new
		pInfo->Flags |= pInfo->AddNew;
		pInfo->Flags &= ~pInfo->FindCue;

		for (ConstCuePointVectorIterator i = m_CuePoints.begin();
			i != m_CuePoints.end(); i++)
		{
			if (pInfo->Sample == i->dwSampleOffset)
			{
				pInfo->Flags &= ~(pInfo->AddNew | pInfo->CuePointIndex);
				pInfo->MarkerCueID = i->CuePointID;

				WaveRegionMarker const * pMarker = GetRegionMarker(pInfo->MarkerCueID);
				if (NULL != pMarker)
				{
					if (pMarker->SampleLength == pInfo->Length)
					{
						// found exact match
						break;
					}
				}
				else if (pInfo->Length == 0)
				{
					break;
				}
			}
		}
		if (pInfo->Flags & pInfo->AddNew)
		{
			pInfo->Comment = NULL;
			pInfo->Label = NULL;
			pInfo->Ltxt = NULL;
			return FALSE;
		}
		// continue filling data
	}

	if (pInfo->Flags & pInfo->CuePointIndex)
	{
		if (pInfo->MarkerCueID >= m_CuePoints.size())
		{
			return FALSE;
		}

		pInfo->Sample = m_CuePoints[pInfo->MarkerCueID].dwSampleOffset;
		pInfo->MarkerCueID = m_CuePoints[pInfo->MarkerCueID].CuePointID;
		// now reset the "index" flag
		pInfo->Flags &= ~pInfo->CuePointIndex;

		result = TRUE;
	}
	else
	{
		CuePointChunkItem const * pCue = GetCuePoint(pInfo->MarkerCueID);
		if (NULL != pCue)
		{
			pInfo->Sample = pCue->dwSampleOffset;
			result = TRUE;
		}
		else
		{
			pInfo->Sample = 0;
		}
	}

	pInfo->Label = GetCueLabel(pInfo->MarkerCueID);
	pInfo->Comment = GetCueComment(pInfo->MarkerCueID);

	WaveRegionMarker const * pMarker = GetRegionMarker(pInfo->MarkerCueID);
	if (NULL != pMarker)
	{
		pInfo->Ltxt = pMarker->Name;
		pInfo->Length = pMarker->SampleLength;
	}
	else
	{
		pInfo->Ltxt = NULL;
		pInfo->Length = 0;
	}

	return result;
}

BOOL CWaveFile::MoveWaveMarker(unsigned long MarkerCueID, SAMPLE_INDEX Sample)
{
	return GetInstanceData()->MoveWaveMarker(MarkerCueID, Sample);
}

BOOL CWaveFile::InstanceDataWav::SetWaveMarker(WAVEREGIONINFO * pInfo)
{
	WAVEREGIONINFO info = *pInfo;

	if (info.Flags & info.CuePointIndex)
	{
		ASSERT(info.MarkerCueID < m_CuePoints.size());

		info.MarkerCueID = m_CuePoints[info.MarkerCueID].CuePointID;
	}

	if (info.Flags & info.AddNew)
	{
		// find a CueID not used yet
		int cue;
		for (cue = m_FreeCuePointNumber;
			NULL != GetCuePoint(cue)
			|| NULL != GetCueLabel(cue)
			|| NULL != GetRegionMarker(cue)
			|| NULL != GetCueComment(cue)
			; cue++)
		{
		}
		// found unused index
		info.MarkerCueID = cue;
		pInfo->MarkerCueID = cue;
		m_FreeCuePointNumber = cue + 1;

		// add cue point
		CuePointChunkItem item;
		item.CuePointID = info.MarkerCueID;
		item.dwBlockStart = 0;
		item.dwChunkStart = 0;
		item.dwSampleOffset = info.Sample;
		item.fccChunk = mmioFOURCC('d', 'a', 't', 'a');
		item.SamplePosition = 0;

		m_CuePoints.push_back(item);
		m_InfoChanged = true;

		// add region length
		if (info.Length != 0)
		{
			WaveRegionMarker wrm;
			if (NULL != info.Ltxt)
			{
				wrm.Name = info.Ltxt;
			}

			wrm.Codepage = 0;  // TODO
			wrm.Country = 0;
			wrm.CuePointID = info.MarkerCueID;
			wrm.Dialect = 0;
			wrm.Language = 0; //0x409;
			wrm.Purpose = mmioFOURCC('r', 'g', 'n', ' ');
			wrm.SampleLength = info.Length;

			m_RegionMarkers.push_back(wrm);
		}
		// continue
		// set label, comment, length, etc
		info.Flags |= info.ChangeComment | info.ChangeLabel;
	}
	else if (info.Flags & info.Delete)
	{
		// delete all data for the marker

		for (CuePointVectorIterator i = m_CuePoints.begin(); i != m_CuePoints.end(); i++)
		{
			if (i->CuePointID == info.MarkerCueID)
			{
				// delete text
				i = m_CuePoints.erase(i);
				m_FreeCuePointNumber = info.MarkerCueID;
				m_InfoChanged = true;
				break;
			}
		}

		for (RegionMarkerIterator i = m_RegionMarkers.begin(); i != m_RegionMarkers.end(); i++)
		{
			if (i->CuePointID == info.MarkerCueID)
			{
				// delete text
				i = m_RegionMarkers.erase(i);
				m_InfoChanged = true;
				break;
			}
		}
		// continue deleting labels
		info.Comment = NULL;
		info.Label = NULL;

		info.Flags |= info.ChangeComment | info.ChangeLabel;    // to delete labels
	}

	if (info.Flags & info.ChangeComment)
	{
		LabelVectorIterator i;
		for (i = m_Notes.begin(); i != m_Notes.end(); i++)
		{
			if (i->CuePointID == info.MarkerCueID)
			{
				if (NULL != info.Comment
					&& NULL != info.Comment[0])
				{
					// replace text
					i->Text = info.Comment;
				}
				else
				{
					// delete text
					i = m_Notes.erase(i);
				}
				m_InfoChanged = true;
				break;
			}
		}

		if (i == m_Notes.end()
			&& NULL != info.Comment
			&& NULL != info.Comment[0])
		{
			// not found, add
			LablNote note;
			note.CuePointID = info.MarkerCueID;
			note.Text = info.Comment;

			m_Notes.push_back(note);
			m_InfoChanged = true;
		}
	}

	if (info.Flags & info.ChangeLabel)
	{
		LabelVectorIterator i;
		for (i = m_Labels.begin(); i != m_Labels.end(); i++)
		{
			if (i->CuePointID == info.MarkerCueID)
			{
				if (NULL != info.Label
					&& NULL != info.Label[0])
				{
					// replace text
					i->Text = info.Label;
				}
				else
				{
					// delete text
					i = m_Labels.erase(i);
				}
				m_InfoChanged = true;
				break;
			}
		}

		if (i == m_Labels.end()
			&& NULL != info.Label
			&& NULL != info.Label[0])
		{
			// not found, add
			LablNote note;
			note.CuePointID = info.MarkerCueID;
			note.Text = info.Label;

			m_Labels.push_back(note);
			m_InfoChanged = true;
		}
	}

	if (info.Flags & info.ChangeSample)
	{
		CuePointChunkItem * pCue = GetCuePoint(info.MarkerCueID);
		if (NULL != pCue)
		{
			pCue->dwSampleOffset = info.Sample;
			m_InfoChanged = true;
		}
	}

	if (info.Flags & info.ChangeLtxt)
	{
		RegionMarkerIterator i;
		bool Found = false;

		for (i = m_RegionMarkers.begin(); i != m_RegionMarkers.end(); i++)
		{
			if (i->CuePointID == info.MarkerCueID)
			{
				Found = true;
				if (0 == i->SampleLength
					&& (NULL == info.Ltxt || 0 == info.Ltxt[0]))
				{
					// delete text
					i = m_RegionMarkers.erase(i);
					m_InfoChanged = true;
					break;
				}
			}
		}

		if ( ! Found
			&& NULL != info.Ltxt && 0 != info.Ltxt[0])
		{
			WaveRegionMarker item;

			item.Codepage = 0;  // TODO
			item.Country = 0;
			item.CuePointID = info.MarkerCueID;
			item.Dialect = 0;
			item.Language = 0; //0x409;
			item.Purpose = mmioFOURCC('r', 'g', 'n', ' ');
			item.SampleLength = 0;

			m_RegionMarkers.push_back(item);
			m_InfoChanged = true;
		}
	}

	if (info.Flags & info.ChangeLength)
	{
		RegionMarkerIterator i;
		bool Found = false;

		for (i = m_RegionMarkers.begin(); i != m_RegionMarkers.end(); i++)
		{
			if (i->CuePointID == info.MarkerCueID)
			{
				Found = true;
				i->SampleLength = info.Length;
				// if length becomes zero, and it doesn't have text, convert to a marker (delete region item)
				if (0 == info.Length
					&& i->Name.IsEmpty())
				{
					// delete text
					i = m_RegionMarkers.erase(i);
					m_InfoChanged = true;
					break;
				}
			}
		}

		if (! Found
			&& 0 != info.Length)
		{
			WaveRegionMarker item;

			item.Codepage = 0;  // TODO
			item.Country = 0;
			item.CuePointID = info.MarkerCueID;
			item.Dialect = 0;
			item.Language = 0; //0x409;
			item.Purpose = mmioFOURCC('r', 'g', 'n', ' ');
			item.SampleLength = info.Length;

			m_RegionMarkers.push_back(item);
			m_InfoChanged = true;
		}
	}

	return TRUE;
}

BOOL CWaveFile::InstanceDataWav::MoveWaveMarker(unsigned long MarkerCueID, SAMPLE_INDEX Sample)
{
	WAVEREGIONINFO info;
	info.Flags = info.ChangeSample;
	info.MarkerCueID = MarkerCueID;
	info.Sample = Sample;
	return SetWaveMarker( & info);
}

// creates a file based on template format from pTemplateFile
BOOL CWaveFile::CreateWaveFile(CWaveFile * pTemplateFile, WAVEFORMATEX const * pTemplateFormat,
								CHANNEL_MASK Channels, WAV_FILE_SIZE SizeOrSamples,
								long flags, LPCTSTR FileName)
{
	CString name;
	TCHAR NameBuf[512];
	CThisApp * pApp = GetApp();
	// if the name is empty, create a temp name
	DWORD OpenFlags = MmioFileOpenCreateAlways;
	CWaveFormat wf;

	// create new WAVEFORMATEX
	if (NULL != pTemplateFormat)
	{
		wf = pTemplateFormat;
	}
	else if (NULL != pTemplateFile
			&& pTemplateFile->IsOpen())
	{
		wf = pTemplateFile->GetWaveFormat();
	}
	else
	{
		//ASSERT(NULL != pTemplateFormat || NULL != pTemplateFile);
		wf.InitCdAudioFormat();
	}

	NUMBER_OF_CHANNELS nNumChannels = wf.NumChannelsFromMask(Channels);

	if (flags & CreateWaveFileAllowMemoryFile)
	{
		// check file size
		int nSampleSize =
			sizeof (WAVE_SAMPLE) * nNumChannels;

		LONGLONG size = SizeOrSamples * nSampleSize;
		if (size > pApp->m_MaxMemoryFileSize * 1024)
		{
			flags &= ~CreateWaveFileAllowMemoryFile;
		}
	}

	if (flags & CreateWaveFileAllowMemoryFile)
	{
		OpenFlags |= MmioFileMemoryFile;
	}
	else if (NULL != FileName
			&& FileName[0] != 0
			&& 0 == (flags & CreateWaveFileTemp))
	{
		name = FileName;
	}
	else
	{
		CPathEx dir;

		if (0 == (flags & CreateWaveFileTempDir))
		{
			// get directory name from template file or FileName
			LPCTSTR OriginalName = NULL;

			if ((flags & CreateWaveFileTemp)
				&& NULL != FileName
				&& FileName[0] != 0)
			{
				OriginalName = FileName;
			}
			else if (NULL != pTemplateFile
					&& pTemplateFile->IsOpen())
			{
				OriginalName = pTemplateFile->GetName();
			}

			if (NULL != OriginalName
				&& 0 != OriginalName[0])
			{
				if (dir.MakeFullPath(OriginalName))
				{
					dir.RemoveFileSpec();
				}
			}
		}

		if (dir.IsEmpty())
		{
			dir = GetApp()->m_sTempDir;
		}

		if (dir.IsEmpty())
		{
			dir.GetTempPath();
		}

		if ( ! dir.IsEmpty())
		{
			dir.AddBackslash();
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
		if (NULL == AllocateInstanceData<InstanceDataWav>())
		{
			Close();
			return FALSE;
		}

		if ((flags & CreateWaveFileSizeSpecified)
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

	InstanceDataWav * pInst = AllocateInstanceData<InstanceDataWav>();
	if (NULL == pInst)
	{
		Close();
		return FALSE;
	}

	if (flags & CreateWaveFilePcmFormat)
	{
		// force 16 bits per sample
		pInst->wf.InitFormat(WAVE_FORMAT_PCM, wf.SampleRate(),
							nNumChannels, 16);
	}
	else if (WAVE_FORMAT_PCM == wf.FormatTag())
	{
		// keep given number of bts per sample
		pInst->wf.InitFormat(WAVE_FORMAT_PCM, wf.SampleRate(),
							nNumChannels, wf.BitsPerSample());
	}
	else
	{
		pInst->wf = wf;
	}

	// RIFF created in Open()
	MMCKINFO * pfck = GetFmtChunk();
	pfck->ckid = mmioFOURCC('f', 'm', 't', ' ');
	CreateChunk(* pfck, 0);
	Write(LPWAVEFORMATEX(pInst->wf), pInst->wf.FormatSize());
	Ascend(* pfck);

	// write fact chunk
	MMCKINFO * fact = GetFactChunk();
	if ((flags & CreateWaveFileCreateFact)
		&& NULL != fact)
	{
		memzero(*fact);
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
	memzero(*pDatachunk);

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
			unsigned DataLength = SizeOrSamples * SampleSize();
			SetFileLength(pDatachunk->dwDataOffset + DataLength);
			Seek(LONG(DataLength), SEEK_CUR);
		}
	}
	Ascend( * pDatachunk);
	// and copy INFO
	if (NULL != pTemplateFile
		&& 0 == (flags & CreateWaveFileDontCopyInfo))
	{
		CopyMetadata(*pTemplateFile);
	}
	// then update RIFF
	//Ascend( *GetRiffChunk());
	return TRUE;
}

DWORD CWaveFile::SaveMetadata()
{
	InstanceDataWav * inst = GetInstanceData();
	if (NULL == inst)
	{
		return 0;
	}

	DWORD MetadataBegin = (inst->datack.dwDataOffset + inst->datack.cksize + 1) & -2;
	Seek(MetadataBegin);

	MMCKINFO ck = {0};
	MMCKINFO list;

	if ( ! inst->m_CuePoints.empty())
	{
		// add chunk header and cue count
		ck.ckid = mmioFOURCC('c', 'u', 'e', ' ');
		ck.cksize = 0;
		ck.dwFlags = 0;

		if ( ! CreateChunk(ck))
		{
			return 0;
		}

		DWORD count = (DWORD)inst->m_CuePoints.size();
		if (sizeof (count) != Write( & count, sizeof count))
		{
			return 0;
		}

		for (CuePointVectorIterator i = inst->m_CuePoints.begin();
			i != inst->m_CuePoints.end(); i++)
		{
			if (sizeof (CuePointChunkItem) != Write(i.operator->(), sizeof (CuePointChunkItem)))
			{
				Ascend(ck);
				return 0;
			}
		}
		Ascend(ck);
	}

	if ( ! inst->m_Playlist.empty())
	{
		// add chunk header and playlist count
		ck.ckid = mmioFOURCC('p', 'l', 's', 't');
		ck.cksize = 0;
		ck.dwFlags = 0;

		if ( ! CreateChunk(ck))
		{
			return 0;
		}

		DWORD count = (DWORD)inst->m_Playlist.size();

		if (sizeof (count) != Write( & count, sizeof count))
		{
			return 0;
		}

		for (PlaylistVectorIterator i = inst->m_Playlist.begin();
			i != inst->m_Playlist.end(); i++)
		{
			if (sizeof (PlaylistSegment) != Write(i.operator->(), sizeof (PlaylistSegment)))
			{
				Ascend(ck);
				return 0;
			}
		}
		Ascend(ck);
	}

	if ( ! inst->m_Labels.empty()
		|| ! inst->m_Notes.empty()
		|| ! inst->m_RegionMarkers.empty())
	{
		list.ckid = mmioFOURCC('L', 'I', 'S', 'T');
		list.cksize = 0;
		list.dwFlags = 0;
		list.fccType = mmioFOURCC('a', 'd', 't', 'l');

		// add 'adtl' LIST header size
		if ( ! CreateChunk(list, MMIO_CREATELIST))
		{
			return 0;
		}

		for (RegionMarkerIterator ri = inst->m_RegionMarkers.begin();
			ri != inst->m_RegionMarkers.end();
			ri++)
		{
			ck.ckid = mmioFOURCC('l', 't', 'x', 't');
			ck.cksize = 0;
			ck.dwFlags = 0;

			if ( ! CreateChunk(ck)
				|| sizeof (LtxtChunk) != Write(static_cast<LtxtChunk *>(&*ri),
												sizeof (LtxtChunk)))
			{
				return 0;
			}

			if (-1 == WriteChunkString(ri->Name))
			{
				return 0;
			}
			Ascend(ck);
		}

		LabelVectorIterator i;
		for (i = inst->m_Labels.begin(); i != inst->m_Labels.end(); i++)
		{
			if ( ! i->Text.IsEmpty())
			{
				ck.ckid = mmioFOURCC('l', 'a', 'b', 'l');
				ck.cksize = 0;
				ck.dwFlags = 0;

				if ( ! CreateChunk(ck)
					|| sizeof (DWORD) != Write( & i->CuePointID, sizeof (DWORD)))
				{
					return 0;
				}

				if (-1 == WriteChunkString(i->Text))
				{
					return 0;
				}

				Ascend(ck);
			}
		}

		for (i = inst->m_Notes.begin(); i != inst->m_Notes.end(); i++)
		{
			if ( ! i->Text.IsEmpty())
			{
				ck.ckid = mmioFOURCC('n', 'o', 't', 'e');
				ck.cksize = 0;
				ck.dwFlags = 0;

				if ( ! CreateChunk(ck)
					|| sizeof (DWORD) != Write( & i->CuePointID, sizeof (DWORD)))
				{
					return 0;
				}

				if (-1 == WriteChunkString(i->Text))
				{
					return 0;
				}

				Ascend(ck);
			}
		}

		Ascend(list);
	}

	if ( ! inst->m_InfoList.empty())
	{
		list.fccType = mmioFOURCC('I', 'N', 'F', 'O');
		list.dwFlags = 0;
		list.cksize = 0;

		if ( ! CreateChunk(list, MMIO_CREATELIST))
		{
			return 0;
		}

		for (InfoListItemIterator i = inst->m_InfoList.begin();
			i != inst->m_InfoList.end(); i++)
		{
			ck.ckid = i->fccCode;
			ck.cksize = 0;
			ck.dwFlags = 0;

			if ( ! CreateChunk(ck))
			{
				return 0;
			}

			if (-1 == WriteChunkString(i->Text))
			{
				return 0;
			}

			Ascend(ck);
		}

		Ascend(list);
	}

	if ( ! inst->m_InfoListW.empty())
	{
		list.fccType = mmioFOURCC('U', 'N', 'F', 'O');
		list.dwFlags = 0;
		list.cksize = 0;

		if ( ! CreateChunk(list, MMIO_CREATELIST))
		{
			return 0;
		}

		for (InfoListItemIteratorW i = inst->m_InfoListW.begin();
			i != inst->m_InfoListW.end(); i++)
		{
			ck.ckid = i->fccCode;
			ck.cksize = 0;
			ck.dwFlags = 0;

			if ( ! CreateChunk(ck))
			{
				return 0;
			}

			if (-1 == WriteChunkStringW(i->Text))
			{
				return 0;
			}

			Ascend(ck);
		}

		Ascend(list);
	}

	inst->SetChanged(false);

	return DWORD(GetFilePointer() - MetadataBegin);
}

DWORD CWaveFile::GetMetadataLength() const
{
	InstanceDataWav const * inst = GetInstanceData();
	if (NULL == inst)
	{
		return 0;
	}

	unsigned size = 0;

	if ( ! inst->m_CuePoints.empty())
	{
		// add chunk header and cue count
		C_ASSERT(0 == (1 & sizeof (CuePointChunkItem)));
		size += (unsigned)inst->m_CuePoints.size() * sizeof (CuePointChunkItem) + 3 * sizeof (DWORD);
	}

	if ( ! inst->m_Playlist.empty())
	{
		// add chunk header and playlist count
		C_ASSERT(0 == (1 & sizeof (PlaylistSegment)));
		size += (unsigned)inst->m_Playlist.size() * sizeof (PlaylistSegment) + 3 * sizeof (DWORD);
	}

	if ( ! inst->m_Labels.empty()
		|| ! inst->m_Notes.empty()
		|| ! inst->m_RegionMarkers.empty())
	{
		// add 'adtl' LIST header size
		size += 3 * sizeof (DWORD);

		for (ConstRegionMarkerIterator ri = inst->m_RegionMarkers.begin();
			ri != inst->m_RegionMarkers.end();
			ri++)
		{
			size += (ChunkStringLength(ri->Name) + 1) & ~1UL;
			size += sizeof LtxtChunk + 2 * sizeof (DWORD);
		}

		ConstLabelVectorIterator i;
		for (i = inst->m_Labels.begin(); i != inst->m_Labels.end(); i++)
		{
			if ( ! i->Text.IsEmpty())
			{
				size += 3 * sizeof (DWORD) + ((ChunkStringLength(i->Text) + 1) & ~1UL);
			}
		}

		for (i = inst->m_Notes.begin(); i != inst->m_Notes.end(); i++)
		{
			if ( ! i->Text.IsEmpty())
			{
				size += 3 * sizeof (DWORD) + ((ChunkStringLength(i->Text) + 1) & ~1UL);
			}
		}
	}

	if ( ! inst->m_InfoList.empty())
	{
		size += 3 * sizeof (DWORD); // INFO list header

		for (ConstInfoListItemIterator i = inst->m_InfoList.begin();
			i != inst->m_InfoList.end(); i++)
		{
			size += 3 * sizeof (DWORD) + ((ChunkStringLength(i->Text) + 1) & ~1UL);
		}
	}

	if ( ! inst->m_InfoListW.empty())
	{
		size += 3 * sizeof (DWORD); // INFO list header

		for (ConstInfoListItemIteratorW i = inst->m_InfoListW.begin();
			i != inst->m_InfoListW.end(); i++)
		{
			size += 3 * sizeof (DWORD) + sizeof (WCHAR) * ((i->Text.GetLength() + 1));   // zero-terminated
		}
	}

	return size;
}

BOOL CWaveFile::SetDatachunkLength(DWORD Length)
{
	LPMMCKINFO pck = GetDataChunk();
	if (NULL == pck)
	{
		return FALSE;
	}

	if ( ! SetFileLength(((Length + 1) & ~1UL) + pck->dwDataOffset + ((GetMetadataLength() + 1) & ~1UL)))
	{
		return FALSE;
	}

	if (pck->ckid != 0)
	{
		// update data chunk length
		pck->cksize = Length;
		pck->dwFlags |= MMIO_DIRTY;
	}
	// make it to save metadata
	CommitChanges();
	return TRUE;
}

void CWaveFile::SetFactNumberOfSamples(NUMBER_OF_SAMPLES samples)
{
	GetFactChunk()->dwFlags |= MMIO_DIRTY;
	// save number of samples
	m_FactSamples = samples;
}

BOOL CWaveFile::SetFileLengthSamples(NUMBER_OF_SAMPLES length)
{
	ASSERT(WAVE_FORMAT_PCM == GetWaveFormat()->wFormatTag);

	return SetDatachunkLength(length * SampleSize());
}

int CWaveFile::SampleSize() const
{
	InstanceDataWav * cd = GetInstanceData();
	if (NULL == cd)
	{
		return 0;
	}
	return cd->wf.NumChannels() * cd->wf.BitsPerSample() / 8;
}

NUMBER_OF_SAMPLES CWaveFile::NumberOfSamples() const
{
	InstanceDataWav * cd = GetInstanceData();
	if (NULL == cd
		|| 0 == cd->wf.NumChannels()
		|| 0 == cd->wf.BitsPerSample())
	{
		return 0;
	}
	return MulDiv(cd->datack.cksize, 8, cd->wf.NumChannels() * cd->wf.BitsPerSample());
}

WAVEFORMATEX * CWaveFile::GetWaveFormat() const
{
	InstanceDataWav * pInstData = GetInstanceData();
	if (pInstData)
	{
		return pInstData->wf;
	}
	else
	{
		return NULL;
	}
}

WaveSampleType CWaveFile::GetSampleType() const
{
	InstanceDataWav * pInstData = GetInstanceData();
	if (pInstData)
	{
		return pInstData->wf.GetSampleType();
	}
	return SampleTypeNotSupported;
}

MMCKINFO * CWaveFile::GetFmtChunk() const
{
	InstanceDataWav * tmp = GetInstanceData();
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
	InstanceDataWav * tmp = GetInstanceData();
	if (tmp)
	{
		return & tmp->factck;
	}
	else
	{
		return NULL;
	}
}

CWavePeaks * CWaveFile::GetWavePeaks() const
{
	InstanceDataWav * pInstData = GetInstanceData();
	if (NULL == pInstData)
	{
		return NULL;
	}
	return & pInstData->m_PeakData;
}

NUMBER_OF_CHANNELS CWaveFile::Channels() const
{
	WAVEFORMATEX * pWf = GetWaveFormat();
	if (NULL == pWf)
	{
		return 1;
	}
	return pWf->nChannels;
}

WAVEFORMATEX * CWaveFile::AllocateWaveformat(unsigned FormatSize)
{
	return AllocateInstanceData<InstanceDataWav>()->wf.Allocate(FormatSize);
}

bool CWaveFile::IsCompressed() const
{
	InstanceDataWav * pInstData = GetInstanceData();
	if (NULL == pInstData)
	{
		return false;
	}

	return pInstData->wf.IsCompressed();
}

CHANNEL_MASK CWaveFile::ChannelsMask() const
{
	InstanceDataWav * pInstData = GetInstanceData();
	if (NULL == pInstData)
	{
		return 0;
	}
	return pInstData->wf.ChannelsMask();
}

unsigned CWaveFile::SampleRate() const
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

WORD CWaveFile::BitsPerSample() const
{
	WAVEFORMATEX * pWf = GetWaveFormat();
	if (pWf)
	{
		return pWf->wBitsPerSample;
	}
	else
	{
		return 8;
	}
}

LPMMCKINFO CWaveFile::GetDataChunk() const
{
	InstanceDataWav * pInstData = GetInstanceData();
	if (NULL == pInstData)
	{
		return NULL;
	}
	return & pInstData->datack;
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

	if (NULL != riff && 0 != riff->ckid
		&& ((riff->dwFlags & MMIO_DIRTY) || riff->dwDataOffset + riff->cksize != CurrentLength))
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
		&& 0 != fmtck->dwDataOffset
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
	if (NULL != datack
		&& 0 != datack->dwDataOffset)
	{
		InstanceDataWav * pInst = GetInstanceData();

		if (pInst->IsChanged()
			|| (datack->dwFlags & MMIO_DIRTY))
		{
			DWORD MetadataLength = (SaveMetadata() + 1) & ~1UL;
			if ( ! SetFileLength(datack->dwDataOffset + (~1UL & (datack->cksize + 1)) + MetadataLength))
			{
				return FALSE;
			}

			GetRiffChunk()->dwFlags |= MMIO_DIRTY;
		}

		if (datack->dwFlags & MMIO_DIRTY)
		{
			Seek(datack->dwDataOffset - sizeof datack->cksize);

			Write( & datack->cksize, sizeof datack->cksize);
			datack->dwFlags &= ~MMIO_DIRTY;
			GetRiffChunk()->dwFlags |= MMIO_DIRTY;
		}

	}

	return CMmioFile::CommitChanges();
}

CWavePeaks::CWavePeaks(unsigned granularity)
	: m_pPeaks(NULL)
	, m_WavePeakSize(0)
	, m_AllocatedWavePeakSize(0),
	m_PeakDataGranularity(granularity)
{
}

CWavePeaks::~CWavePeaks()
{
	delete[] m_pPeaks;
}

WavePeak * CWavePeaks::AllocatePeakData(NUMBER_OF_SAMPLES NewNumberOfSamples,
										NUMBER_OF_CHANNELS NumberOfChannels)
{
	// change m_pPeaks size
	// need to synchronize with OnDraw
	unsigned NewWavePeakSize = NumberOfChannels *
								((NewNumberOfSamples + m_PeakDataGranularity - 1) / m_PeakDataGranularity);

	if (NULL == m_pPeaks
		|| NewWavePeakSize > m_AllocatedWavePeakSize)
	{
		unsigned NewAllocatedWavePeakSize = NewWavePeakSize + 1024;  // reserve more
		WavePeak * NewPeaks = new WavePeak[NewAllocatedWavePeakSize];
		if (NULL == NewPeaks)
		{
			return NULL;
		}
		if (NULL != m_pPeaks
			&& 0 != m_WavePeakSize)
		{
			memcpy(NewPeaks, m_pPeaks, m_WavePeakSize * sizeof (WavePeak));
		}
		else
		{
			m_WavePeakSize = 0;
		}
		for (PEAK_INDEX i = m_WavePeakSize; i < NewWavePeakSize; i++)
		{
			NewPeaks[i].high = -0x8000;
			NewPeaks[i].low = 0x7FFF;
		}
		WavePeak * OldPeaks;
		{
			CSimpleCriticalSectionLock lock(m_PeakLock);
			OldPeaks = m_pPeaks;
			m_pPeaks = NewPeaks;
			m_WavePeakSize = NewWavePeakSize;
			m_AllocatedWavePeakSize = NewAllocatedWavePeakSize;
		}
		delete[] OldPeaks;
	}
	else
	{
		for (PEAK_INDEX i = m_WavePeakSize; i < NewWavePeakSize; i++)
		{
			m_pPeaks[i].high = -0x8000;
			m_pPeaks[i].low = 0x7FFF;
		}
		m_WavePeakSize = NewWavePeakSize;
	}
	return m_pPeaks;
}

void CWaveFile::SetPeakData(PEAK_INDEX index, WAVE_PEAK low, WAVE_PEAK high)
{
	CWavePeaks * pPeaks = GetWavePeaks();
	if (NULL != pPeaks)
	{
		pPeaks->SetPeakData(index, low, high);
	}
}

BOOL CWaveFile::AllocatePeakData(NUMBER_OF_SAMPLES NewNumberOfSamples)
{
	CWavePeaks * pPeaks = GetWavePeaks();
	if (NULL != pPeaks)
	{
		return NULL != pPeaks->AllocatePeakData(NewNumberOfSamples, Channels());
	}
	return FALSE;
}

BOOL CWaveFile::SetSourceFile(CWaveFile * const pOriginalFile)
{
	if (BaseClass::SetSourceFile(pOriginalFile))
	{
		// copy all instance info
		CWavePeaks * pPeaks2 = GetWavePeaks();
		CWavePeaks * pPeaks1 = pOriginalFile->GetWavePeaks();
		if (NULL != pPeaks1 && NULL != pPeaks2)
		{
			*pPeaks2 = *pPeaks1;
		}
		return TRUE;
	}
	return FALSE;
}

WavePeak CWaveFile::GetPeakMinMax(PEAK_INDEX from, PEAK_INDEX to, NUMBER_OF_CHANNELS stride) const
{
	CWavePeaks * pPeaks = GetWavePeaks();
	if (NULL != pPeaks)
	{
		return pPeaks->GetPeakMinMax(from, to, stride);
	}
	return WavePeak(0, 0);
}

unsigned CWaveFile::GetPeakGranularity() const
{
	CWavePeaks const * pPeaks = GetWavePeaks();
	if (NULL != pPeaks)
	{
		return pPeaks->GetGranularity();
	}
	return 1024;
}

unsigned CWaveFile::GetPeaksSize() const
{
	CWavePeaks const * pPeaks = GetWavePeaks();
	if (NULL != pPeaks)
	{
		return (unsigned) pPeaks->GetPeaksSize();
	}
	return 0;
}

void CWaveFile::SetPeaks(PEAK_INDEX from, PEAK_INDEX to, NUMBER_OF_CHANNELS stride, WavePeak value)
{
	CWavePeaks * pPeaks = GetWavePeaks();
	if (NULL != pPeaks)
	{
		pPeaks->SetPeaks(from, to, stride, value);
	}
}

void CWaveFile::RescanPeaks(SAMPLE_INDEX begin, SAMPLE_INDEX end)
{
	// if called immediately after data modification, it will get
	// the data directly from the cache
	if (DEBUG_RESCAN_PEAKS) TRACE("RescanPeaks from %d to %d\n", begin, end);

	CWavePeaks * pPeaks = GetWavePeaks();
	unsigned Granularity = pPeaks->GetGranularity();
	unsigned nChannels = Channels();

	float samples[MAX_NUMBER_OF_CHANNELS * 16];	// 512. 2kB

	// Granularity should be divisible by NumSamplesToRead
	unsigned NumSamplesToRead = Granularity;
	while (NumSamplesToRead > countof(samples) / nChannels)
	{
		NumSamplesToRead /= 2;
	}

	if (begin > end)
	{
		SAMPLE_INDEX tmp = begin;
		begin = end;
		end = tmp;
	}

	begin = begin & (0-Granularity);
	end = ((end - 1) | (Granularity - 1)) + 1;

	WavePeak wp[MAX_NUMBER_OF_CHANNELS];
	unsigned PeakIndex = (begin / Granularity) * nChannels;

	while (begin < end)
	{
		int SamplesRead = ReadSamples(ALL_CHANNELS, SampleToPosition(begin), NumSamplesToRead, samples, SampleTypeFloat32);

		if (SamplesRead <= 0)
		{
			break;
		}
		unsigned ch;
		if (0 == (begin & (Granularity - 1)))
		{
			// clear wp
			for (ch = 0; ch < nChannels; ch++)
			{
				wp[ch].low = 10.;
				wp[ch].high = -10.;
			}
		}
		for (unsigned i = 0, j = 0; i < NumSamplesToRead; i++)
		{
			for (ch = 0; ch < nChannels; ch++, j++)
			{
				if (samples[j] < wp[ch].low)
				{
					wp[ch].low = samples[j];
				}
				if (samples[j] > wp[ch].high)
				{
					wp[ch].high = samples[j];
				}
			}
		}

		begin += SamplesRead;
		if (0 == (begin & (Granularity - 1))
			|| begin >= end)
		{
			for (ch = 0; ch < nChannels; ch++, PeakIndex++)
			{
				pPeaks->SetPeakData(PeakIndex, wp[ch].low, wp[ch].high);
			}
		}
	}
}

WavePeak CWavePeaks::GetPeakMinMax(PEAK_INDEX from, PEAK_INDEX to, NUMBER_OF_CHANNELS stride) const
{
	WavePeak peak;
	peak.high = -10.;
	peak.low = 10;

	CSimpleCriticalSectionLock lock(m_PeakLock);

	if (to > m_WavePeakSize)
	{
		to = PEAK_INDEX(m_WavePeakSize);
	}

	for (unsigned j = from; j < to; j += stride)
	{
		if (peak.low > m_pPeaks[j].low)
		{
			peak.low = m_pPeaks[j].low;
		}
		if (peak.high < m_pPeaks[j].high)
		{
			peak.high = m_pPeaks[j].high;
		}
	}
	return peak;
}

void CWavePeaks::SetPeaks(PEAK_INDEX from, PEAK_INDEX to, NUMBER_OF_CHANNELS stride, WavePeak value)
{
	CSimpleCriticalSectionLock lock(m_PeakLock);

	if (to > m_WavePeakSize)
	{
		to = m_WavePeakSize;
	}

	for (unsigned j = from; j < to; j += stride)
	{
		m_pPeaks[j] = value;
	}
}

CWavePeaks & CWavePeaks::operator =(CWavePeaks const & src)
{
	m_PeakDataGranularity = src.GetGranularity();
	WavePeak * pPeaks = AllocatePeakData(src.GetPeaksSize() * m_PeakDataGranularity);
	if (NULL != pPeaks)
	{
		memcpy(pPeaks, src.GetPeakArray(), GetPeaksSize() * sizeof (WavePeak));
	}
	else
	{
		AllocatePeakData(0);
	}
	return *this;
}

bool operator ==(FILETIME const & t1, FILETIME const & t2)
{
	return t1.dwHighDateTime == t2.dwHighDateTime
			&& t1.dwLowDateTime == t2.dwLowDateTime;
}

CPath CWaveFile::MakePeakFileName(LPCTSTR FileName)
{
	CPath path(FileName);
	CPath AppDataPath;


#if _WIN32_WINNT >= 0x0600	// XP
	LPWSTR pFolderPath = NULL;
	if (S_OK == SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, & pFolderPath))
	{
		AppDataPath = pFolderPath;
		CoTaskMemFree(pFolderPath);
	}
#else
	TCHAR FolderPath[MAX_PATH];
	if (S_OK == SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, FolderPath))
	{
		AppDataPath = FolderPath;
	}
#endif
	if ( ! ((CString&)AppDataPath).IsEmpty())
	{
		AppDataPath.Append(L"WaveSoap");
		VerifyCreateDirectory(AppDataPath);

		AppDataPath.Append(LPCTSTR(path) + path.FindFileName());

		path = AppDataPath;
	}

	if (0 == path.GetExtension().CompareNoCase(_T(".WAV")))
	{
		path.RenameExtension(_T(".wspk"));
	}
	else
	{
		static_cast<CString &>(path).Append(_T(".wspk"));
	}
	return path;
}

BOOL CWaveFile::CheckAndLoadPeakFile()
{
	// if peak file exists and the wav file length/date/time matches the stored
	// length/date/time, then use this peak file.
	// otherwise scan the wav file and build the new peak file
	CWavePeaks * pPeakInfo = GetWavePeaks();

	CFile PeakFile;
	PeakFileHeader pfh;

	CPath PeakFilename(MakePeakFileName(GetName()));

	if (PeakFile.Open(PeakFilename,
					CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary))
	{
		if (sizeof (PeakFileHeader)
			== PeakFile.Read( & pfh, sizeof (PeakFileHeader))
			&& PeakFileHeader::pfhSignature == pfh.dwSignature
			&& pfh.dwVersion == PeakFileHeader::pfhMaxVersion
			&& pfh.wSize == sizeof (PeakFileHeader)

			&& pfh.WaveFileTime == GetFileInformation().ftLastWriteTime
			&& pfh.dwWaveFileSize == GetFileInformation().nFileSizeLow

			&& 0 == memcmp(& pfh.wfFormat, GetWaveFormat(), sizeof (PCMWAVEFORMAT))
			&& (WAVE_FORMAT_PCM == pfh.wfFormat.wFormatTag
				|| pfh.wfFormat.cbSize == GetWaveFormat()->cbSize)
			&& pPeakInfo->GetGranularity() == pfh.Granularity
			&& pfh.PeakInfoSize == CalculatePeakInfoSize() * sizeof (WavePeak)
			)
		{
			// allocate data and read it
			WavePeak * pPeaks = pPeakInfo->AllocatePeakData(NumberOfSamples(), Channels());
			if (NULL == pPeaks)
			{
				TRACE("Unable to allocate peak info buffer\n");
				pPeakInfo->AllocatePeakData(0, 1);
				return FALSE;
			}

			if (pfh.PeakInfoSize <= pPeakInfo->GetPeaksSize() * sizeof (WavePeak)
				&& pfh.PeakInfoSize == PeakFile.Read(pPeaks, pfh.PeakInfoSize))
			{
				return TRUE;
			}
			TRACE("Unable to read peak data\n");
			// rebuild the info from the WAV file
		}
		else
		{
			TRACE("Peak Info modification time = 0x%08X%08X, open file time=0x%08X%08X\n",
				pfh.WaveFileTime.dwHighDateTime, pfh.WaveFileTime.dwLowDateTime,
				GetFileInformation().ftLastWriteTime.dwHighDateTime,
				GetFileInformation().ftLastWriteTime.dwLowDateTime);
		}
		PeakFile.Close();
	}
	return FALSE;
}

// the function is called to load peak info for a compressed file
// WaveFile argument - temporary wave file
// OriginalWaveFile - compressed file
BOOL CWaveFile::LoadPeaksForCompressedFile(CWaveFile & OriginalWaveFile,
											ULONG NumberOfSamples)
{

	// don't check peak file data size, just make sure source file parameters match
	// if peak file exists and the wav file length/date/time matches the stored
	// length/date/time, then use this peak file.
	// otherwise don't use it.
	// the peak info will be rebuilt in any case during file load
	CWavePeaks * pPeakInfo = GetWavePeaks();

	AllocatePeakData(NumberOfSamples);

	CFile PeakFile;
	PeakFileHeader pfh = { 0 };
	CPath PeakFilename(MakePeakFileName(OriginalWaveFile.GetName()));

	if ( ! PeakFile.Open(PeakFilename,
						CFile::modeRead | CFile::shareDenyWrite | CFile::typeBinary))
	{
		return FALSE;
	}
	if (offsetof(PeakFileHeader, WaveFileTime)
		== PeakFile.Read( & pfh, offsetof(PeakFileHeader, WaveFileTime))
		&& PeakFileHeader::pfhSignature == pfh.dwSignature
		&& pfh.dwVersion == PeakFileHeader::pfhMaxVersion
		&& pfh.wSize == sizeof (PeakFileHeader)
		// read the rest of the header
		&& pfh.wSize - offsetof(PeakFileHeader, WaveFileTime)
		== PeakFile.Read( & pfh.WaveFileTime, pfh.wSize - offsetof(PeakFileHeader, WaveFileTime))
		// check date and time
		&& pfh.WaveFileTime == OriginalWaveFile.GetFileInformation().ftLastWriteTime
		// check source file size
		&& pfh.dwWaveFileSize == OriginalWaveFile.GetFileInformation().nFileSizeLow
		// check PCM number of channels and sampling rate
		&& 0 == memcmp(& pfh.wfFormat, GetWaveFormat(), sizeof pfh.wfFormat)
		&& pPeakInfo->GetGranularity() == pfh.Granularity
		&& pfh.NumOfSamples == NumberOfSamples
		)
	{
		// allocate data and read it
		WavePeak * pPeaks = pPeakInfo->AllocatePeakData(pfh.NumOfSamples, Channels());
		if (NULL == pPeaks)
		{
			TRACE("Unable to allocate peak info buffer\n");
			pPeakInfo->AllocatePeakData(0, 1);
			return FALSE;
		}

		if (pfh.PeakInfoSize == pPeakInfo->GetPeaksSize() * sizeof (WavePeak)
			&& pfh.PeakInfoSize == PeakFile.Read(pPeaks, pfh.PeakInfoSize))
		{
			return TRUE;
		}
		TRACE("Unable to read peak data\n");
		// rebuild the info from the WAV file
	}
	else
	{
		TRACE("Peak Info modification time = 0x%08X%08X, open file time=0x%08X%08X\n",
			pfh.WaveFileTime.dwHighDateTime, pfh.WaveFileTime.dwLowDateTime,
			GetFileInformation().ftLastWriteTime.dwHighDateTime,
			GetFileInformation().ftLastWriteTime.dwLowDateTime);
	}
	return FALSE;
}

void CWaveFile::SavePeakInfo(CWaveFile & SavedWaveFile)
{
	CPath PeakFilename(MakePeakFileName(SavedWaveFile.GetName()));

	ATL::CAtlFile PeakFile;

	if (S_OK != PeakFile.Create(PeakFilename, GENERIC_WRITE, 0, CREATE_ALWAYS,
								FILE_ATTRIBUTE_HIDDEN))
	{
		return;
	}

	PeakFileHeader pfh;

	pfh.wSize = sizeof PeakFileHeader;
	pfh.dwSignature = pfh.pfhSignature;
	pfh.dwVersion = pfh.pfhMaxVersion;
	pfh.dwWaveFileSize = SavedWaveFile.GetFileSize(NULL);
	pfh.Granularity = GetPeakGranularity();
	pfh.PeakInfoSize = CalculatePeakInfoSize() * sizeof (WavePeak);
	pfh.WaveFileTime = SavedWaveFile.GetFileInformation().ftLastWriteTime;
	pfh.NumOfSamples = NumberOfSamples();
	pfh.wfFormat = * GetWaveFormat();

	if (WAVE_FORMAT_PCM == pfh.wfFormat.wFormatTag)
	{
		pfh.wfFormat.cbSize = 0;
	}

	PeakFile.Write( & pfh, sizeof pfh);
	PeakFile.Write(GetWavePeaks()->GetPeakArray(), pfh.PeakInfoSize);
}

SAMPLE_POSITION CWaveFile::SampleToPosition(SAMPLE_INDEX sample) const
{
	LPMMCKINFO datack = GetDataChunk();
	if (NULL == datack)
	{
		return 0;
	}
	if (LAST_SAMPLE == sample)
	{
		return SAMPLE_POSITION(datack->dwDataOffset + datack->cksize);
	}
	return SAMPLE_POSITION(datack->dwDataOffset + sample * SampleSize());
}

SAMPLE_INDEX CWaveFile::PositionToSample(SAMPLE_POSITION position) const
{
	LPMMCKINFO datack = GetDataChunk();
	if (NULL == datack)
	{
		return 0;
	}
	if (LAST_SAMPLE_POSITION == position)
	{
		return datack->cksize / SampleSize();
	}

	ASSERT(position >= datack->dwDataOffset);
	return SAMPLE_INDEX((position - datack->dwDataOffset) / SampleSize());
}

typedef CWaveFile::InstanceDataWav InstanceDataWav;
void InstanceDataWav::CopyMetadata(InstanceDataWav const * pSrc, unsigned CopyFlags)
{
	if (CopyFlags & MetadataCopyDisp)
	{
		m_DisplayTitle = pSrc->m_DisplayTitle;
	}

	if (CopyFlags & MetadataCopyInfo)
	{
		m_InfoList = pSrc->m_InfoList;
		m_InfoListW = pSrc->m_InfoListW;
	}

	if (CopyFlags & MetadataCopyLtxt)
	{
		m_RegionMarkers = pSrc->m_RegionMarkers;
	}

	if (CopyFlags & MetadataCopyCue)
	{
		m_CuePoints = pSrc->m_CuePoints;
		m_FreeCuePointNumber = pSrc->m_FreeCuePointNumber;
	}

	if (CopyFlags & MetadataCopyPlaylist)
	{
		m_Playlist = pSrc->m_Playlist;
	}

	if (CopyFlags & MetadataCopyLabels)
	{
		m_Labels = pSrc->m_Labels;
		m_Notes = pSrc->m_Notes;
	}

	if (CopyFlags & MetadataCopyPeakData)
	{
		m_PeakData = pSrc->m_PeakData;
	}

	m_InfoChanged = true;
}

void CWaveFile::InstanceDataWav::SwapMetadata(InstanceDataWav * pSrc, unsigned CopyFlags)
{
	if (CopyFlags & MetadataCopyDisp)
	{
		CString tmp(m_DisplayTitle);
		m_DisplayTitle = pSrc->m_DisplayTitle;
		pSrc->m_DisplayTitle = tmp;
	}

	if (CopyFlags & MetadataCopyInfo)
	{
		m_InfoList.swap(pSrc->m_InfoList);
		m_InfoListW.swap(pSrc->m_InfoListW);
	}

	if (CopyFlags & MetadataCopyLtxt)
	{
		m_RegionMarkers.swap(pSrc->m_RegionMarkers);
	}

	if (CopyFlags & MetadataCopyCue)
	{
		m_CuePoints.swap(pSrc->m_CuePoints);
		std::swap(m_FreeCuePointNumber, pSrc->m_FreeCuePointNumber);
	}

	if (CopyFlags & MetadataCopyPlaylist)
	{
		m_Playlist.swap(pSrc->m_Playlist);
	}

	if (CopyFlags & MetadataCopyLabels)
	{
		m_Labels.swap(pSrc->m_Labels);
		m_Notes.swap(pSrc->m_Notes);
	}

	if (CopyFlags & MetadataCopyPeakData)
	{
		m_PeakData = pSrc->m_PeakData;
	}

	m_InfoChanged = true;
}

InstanceDataWav & InstanceDataWav::operator =(InstanceDataWav const & src)
{
	if (this == & src)
	{
		return * this;
	}
	datack = src.datack;
	fmtck = src.fmtck;
	factck = src.factck;
	wf = src.wf;

	CopyMetadata( & src);

	BaseInstanceClass::operator =(src);
	return *this;
}

void InstanceDataWav::ResetMetadata()
{
	m_DisplayTitle.Empty();
#if 0
	Author.Empty();
	Album.Empty();
	Copyright.Empty();
	RecordingEngineer.Empty();

	Title.Empty();
	Date.Empty();
	Genre.Empty();
	Comment.Empty();
	Subject.Empty();
	Keywords.Empty();
	Medium.Empty();
	Source.Empty();
	Digitizer.Empty();
	DigitizationSource.Empty();
#else
	m_InfoList.clear();
	m_InfoListW.clear();
#endif

	m_RegionMarkers.clear();
	m_Playlist.clear();
	m_CuePoints.clear();
	m_Labels.clear();
	m_Notes.clear();

	m_InfoChanged = true;

}

void CWaveFile::SwapMetadata(InstanceDataWav * pSrc, unsigned SwapFlags)
{
	InstanceDataWav * pDst = GetInstanceData();
	if (NULL != pDst)
	{
		pDst->SwapMetadata(pSrc, SwapFlags );
	}
}

void CWaveFile::CopyMetadata(CWaveFile const & src)
{
	InstanceDataWav * pDst = GetInstanceData();
	InstanceDataWav const * pSrc = src.GetInstanceData();
	if (NULL != pDst
		&& NULL != pSrc)
	{
		pDst->CopyMetadata(pSrc);
	}
}

// NOTE: vector 'markers' is NOT cleared in the function!
void CWaveFile::GetSortedMarkers(SAMPLE_INDEX_Vector & markers, BOOL IncludeFileLimits) const
{
	InstanceDataWav * inst = GetInstanceData();
	if (NULL != inst)
	{
		if (IncludeFileLimits)
		{
			markers.push_back(0);
			markers.push_back(NumberOfSamples());
		}
		inst->GetSortedMarkers(markers);
	}
}

void CWaveFile::InstanceDataWav::GetSortedMarkers(SAMPLE_INDEX_Vector & markers) const
{
	markers.reserve(markers.size() + m_CuePoints.size() + m_RegionMarkers.size());

	for (ConstCuePointVectorIterator i = m_CuePoints.begin(); i != m_CuePoints.end(); i++)
	{
		markers.push_back(SAMPLE_INDEX(i->dwSampleOffset));

		WaveRegionMarker const * pMarker = GetRegionMarker(i->CuePointID);
		if (NULL != pMarker)
		{
			markers.push_back(SAMPLE_INDEX(i->dwSampleOffset + pMarker->SampleLength));
		}
	}

	std::sort(markers.begin(), markers.end());
	markers.erase(std::unique(markers.begin(), markers.end()), markers.end());
}

void CWaveFile::GetSortedFileSegments(WaveFileSegmentVector & segments,
									bool IncludeEndMarkerName) const
{
	InstanceDataWav * inst = GetInstanceData();
	if (NULL != inst)
	{
		WaveFileSegment seg;

		seg.Begin = NumberOfSamples();
		seg.End = seg.Begin;
		seg.Name.LoadString(IDS_END_OF_SAMPLE);
		segments.push_back(seg);

		inst->GetSortedFileSegments(segments, IncludeEndMarkerName);
	}
}

void CWaveFile::InstanceDataWav::GetSortedFileSegments(WaveFileSegmentVector & segments,
														bool IncludeEndMarkerName) const
{
	segments.reserve(segments.size() + m_CuePoints.size() + m_RegionMarkers.size());

	WaveFileSegment seg;

	for (ConstCuePointVectorIterator i = m_CuePoints.begin(); i != m_CuePoints.end(); i++)
	{
		seg.Begin = SAMPLE_INDEX(i->dwSampleOffset);
		seg.Name = GetCueText(i->CuePointID);

		WaveRegionMarker const * pMarker = GetRegionMarker(i->CuePointID);
		if (NULL != pMarker)
		{
			seg.End = SAMPLE_INDEX(i->dwSampleOffset + pMarker->SampleLength);
		}
		else
		{
			seg.End = seg.Begin;
		}

		segments.push_back(seg);
	}

	// sort the array
	std::sort(segments.begin(), segments.end());

	WaveFileSegmentVector::iterator pSegGet, pSegPut;

	// set segment length, remove empty segments
	for (pSegGet = segments.begin(), pSegPut = pSegGet; pSegGet < segments.end(); pSegGet++)
	{
		if (pSegGet->Begin < pSegGet->End)
		{
			*pSegPut = *pSegGet;
			++pSegPut;
		}
		else if (! pSegGet->Name.IsEmpty()
				&& pSegGet + 1 < segments.end()
				&& pSegGet->Begin < pSegGet[1].Begin)
		{
			pSegGet->End = pSegGet[1].Begin;

			if (IncludeEndMarkerName)
			{
				pSegGet->Name.Format(IDS_SEGMENT_NAME_FROM_MARKERS,
									// make temporary copy of the begin name, because Format cannot work otherwise
									LPCTSTR(CString(pSegGet->Name)), LPCTSTR(pSegGet[1].Name));
			}

			*pSegPut = *pSegGet;
			++pSegPut;
		}
		else
		{
			// skip this item
		}
	}

	segments.erase(pSegPut, segments.end());
}

NUMBER_OF_CHANNELS CWaveFile::NumChannelsFromMask(CHANNEL_MASK ChannelMask) const
{
	return GetInstanceData()->wf.NumChannelsFromMask(ChannelMask);
}

bool CWaveFile::AllChannels(CHANNEL_MASK Channels) const
{
	CHANNEL_MASK mask = ChannelsMask();
	return mask == (mask & Channels);
}

void CWaveFile::SetWaveFormat(WAVEFORMATEX const * pWf)
{
	InstanceDataWav * pInst = GetInstanceData();
	pInst->wf = pWf;
	pInst->fmtck.dwFlags |= MMIO_DIRTY;
}

static short NumberBitsInMask(CHANNEL_MASK mask)
{
	short n = 0;
	for (unsigned ch = 0; ch < 32; ch++)
	{
		if (mask & (1 << ch))
		{
			n++;
		}
	}
	return n;
}
long CWaveFile::ReadSamples(CHANNEL_MASK SrcChannels,
							SAMPLE_POSITION Pos,    // absolute position in file
							long Samples, void * pBuf, WaveSampleType type)
{
	WaveSampleType FileSampleType = GetSampleType();
	// it is assumed the buffer is big enough to load all samples
	// The buffer contains complete samples to process
	// if Samples > 0, read forward from Pos, Buf points to the buffer begin

	// if Samples < 0, read backward from Pos, Buf points to the buffer end

	// the function returns count how many samples successfully read
	CHANNEL_MASK const FileChannelsMask = ChannelsMask();
	SrcChannels &= FileChannelsMask;
	NUMBER_OF_CHANNELS NumDstChannels = NumberBitsInMask(SrcChannels);
	long TotalSamplesRead = 0;

	ASSERT(0 != SrcChannels);
	int const SrcSampleSize = SampleSize();            // must be signed type
	int DstSampleSize = 2;
	NUMBER_OF_CHANNELS const FileChannels = Channels();
	LONG32   ltmp[MAX_NUMBER_OF_CHANNELS];

	switch (type)
	{
	case SampleType16bit:
		DstSampleSize = 2 * NumDstChannels;
		break;
	case SampleType32bit:
	case SampleTypeFloat32:
		DstSampleSize = 4 * NumDstChannels;
		break;
	default:
		ASSERT(type == SampleType16bit || type == SampleType32bit || type == SampleTypeFloat32);
		return 0;
	}

	if (Samples > 0)
	{
		// read some of channels, and/or possibly with format conversion

		while (Samples > 0)
		{
			long WasRead = 0;
			void * pOriginalSrcBuf;

			int SizeToRead = Samples * SrcSampleSize;
			if (SizeToRead > CacheBufferSize())
			{
				SizeToRead = CacheBufferSize();
			}

			WasRead = GetDataBuffer( & pOriginalSrcBuf,
									SizeToRead, Pos, GetBufferAndPrefetchNext);

			if (0 == WasRead)
			{
				return TotalSamplesRead;
			}

			unsigned SamplesRead = WasRead / SrcSampleSize;

			if (0 == SamplesRead)
			{
				if (SrcSampleSize != ReadAt(ltmp, SrcSampleSize, Pos))
				{
					ReturnDataBuffer(pOriginalSrcBuf, WasRead,
									ReturnBufferDiscard);
					return TotalSamplesRead;
				}
				CopyWaveSamples(pBuf, ALL_CHANNELS, NumDstChannels, ltmp, SrcChannels, FileChannels, 1, type, FileSampleType);
				SamplesRead = 1;
			}
			else
			{
				CopyWaveSamples(pBuf, ALL_CHANNELS, NumDstChannels, pOriginalSrcBuf, SrcChannels, FileChannels, SamplesRead, type, FileSampleType);
			}

			TotalSamplesRead += SamplesRead;
			Pos += SamplesRead * SrcSampleSize;
			pBuf = SamplesRead * DstSampleSize + (PUCHAR)pBuf;
			Samples -= SamplesRead;

			ReturnDataBuffer(pOriginalSrcBuf, WasRead, ReturnBufferDiscard);
		}
		return TotalSamplesRead;
	}
	else if (Samples < 0)               // read backward
	{
		// read some of channels

		while (Samples < 0)
		{
			long WasRead = 0;
			void * pOriginalSrcBuf;

			long SizeToRead = Samples * SrcSampleSize;
			if (SizeToRead < -CacheBufferSize())
			{
				SizeToRead = -CacheBufferSize();
			}

			WasRead = GetDataBuffer( & pOriginalSrcBuf,
									SizeToRead, Pos, GetBufferAndPrefetchNext);

			if (0 == WasRead)
			{
				return TotalSamplesRead;
			}

			unsigned SamplesRead = -WasRead / SrcSampleSize;

			if (0 == SamplesRead)
			{
				if (SrcSampleSize != ReadAt(ltmp, SrcSampleSize, Pos - SrcSampleSize))
				{
					ReturnDataBuffer(pOriginalSrcBuf, WasRead,
									ReturnBufferDiscard);
					return TotalSamplesRead;
				}
				CopyWaveSamples(((PUCHAR)pBuf) - SrcSampleSize, ALL_CHANNELS, NumDstChannels, ltmp, SrcChannels, FileChannels, 1, type, FileSampleType);
				SamplesRead = 1;
			}
			else
			{
				CopyWaveSamples(((PUCHAR)pBuf) - SamplesRead * SrcSampleSize,
								ALL_CHANNELS, NumDstChannels,
								((PUCHAR)pOriginalSrcBuf) - SamplesRead * SrcSampleSize,
								SrcChannels, FileChannels, SamplesRead, type, FileSampleType);
			}

			pBuf = ((PUCHAR)pBuf) - SamplesRead * SrcSampleSize;
			TotalSamplesRead -= SamplesRead;
			Pos -= SamplesRead * SrcSampleSize;
			Samples += SamplesRead;

			ReturnDataBuffer(pOriginalSrcBuf, WasRead,
							CDirectFile::ReturnBufferDiscard);
		}
	}

	return TotalSamplesRead;
}

long CWaveFile::WriteSamples(CHANNEL_MASK DstChannels,
							SAMPLE_POSITION Pos, long Samples,
							void const * pBuf, CHANNEL_MASK SrcChannels,
							NUMBER_OF_CHANNELS NumSrcChannels, WaveSampleType type)
{
	// The buffer contains complete samples to process
	// if Samples > 0, write forward from Pos, Buf points to the buffer begin

	// if Samples < 0, write backward from Pos, Buf points to the buffer end

	// the function returns count how many samples successfully read
	CHANNEL_MASK const FileChannelsMask = ChannelsMask();
	DstChannels &= FileChannelsMask;

	CHANNEL_MASK const SrcChannelsMask = ~((~0UL) << NumSrcChannels);

	SrcChannels &= SrcChannelsMask;

	long TotalSamplesWritten = 0;

	ASSERT(0 != DstChannels);
	ASSERT(0 != SrcChannels);

	int const DstSampleSize = SampleSize();
	NUMBER_OF_CHANNELS const NumFileChannels = Channels();

	if (type == GetSampleType()
		&& FileChannelsMask == DstChannels
		&& SrcChannelsMask == SrcChannels
		&& DstChannels == SrcChannels)
	{
		LONG Written = WriteAt(pBuf, Samples * DstSampleSize, Pos);
		return Written / DstSampleSize;
	}

	WAVE_SAMPLE tmp[MAX_NUMBER_OF_CHANNELS];
	// write some of channels
	if (Samples > 0)
	{
		while (Samples > 0)
		{
			long WasLockedForWrite = 0;
			void * pOriginalDstBuf;

			long SizeToLock = Samples * DstSampleSize;
			if (SizeToLock > CDirectFile::CacheBufferSize())
			{
				SizeToLock = CDirectFile::CacheBufferSize();
			}

			ULONG LockFlags = CDirectFile::GetBufferAndPrefetchNext;
			if (FileChannelsMask == DstChannels)
			{
				LockFlags = CDirectFile::GetBufferWriteOnly;
			}

			WasLockedForWrite = GetDataBuffer( & pOriginalDstBuf,
												SizeToLock, Pos, LockFlags);

			if (0 == WasLockedForWrite)
			{
				return TotalSamplesWritten;
			}

			unsigned SamplesWritten = WasLockedForWrite / DstSampleSize;
			WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) pOriginalDstBuf;

			if (0 == SamplesWritten)
			{
				ReturnDataBuffer(pOriginalDstBuf, WasLockedForWrite, 0);

				if (DstSampleSize != ReadAt(tmp, DstSampleSize, Pos))
				{
					return TotalSamplesWritten;
				}

				pDst = tmp;
				SamplesWritten = 1;
			}

			WAVE_SAMPLE const * pSrc = (WAVE_SAMPLE const *) pBuf;
			pBuf = pSrc + NumSrcChannels * SamplesWritten;

			CopyWaveSamples(pDst, DstChannels, NumFileChannels,
							pSrc, SrcChannels, NumSrcChannels, SamplesWritten,
							GetSampleType(), type);

			if (WasLockedForWrite < DstSampleSize)
			{
				LONG Written = WriteAt(pDst, DstSampleSize, Pos);

				if (Written != DstSampleSize)
				{
					return TotalSamplesWritten;
				}
			}
			else
			{
				ReturnDataBuffer(pOriginalDstBuf, WasLockedForWrite,
								CDirectFile::ReturnBufferDirty);
			}

			TotalSamplesWritten += SamplesWritten;
			Pos += SamplesWritten * DstSampleSize;
			Samples -= SamplesWritten;

		}
		return TotalSamplesWritten;
	}
	else if (Samples < 0)
	{
		while (Samples < 0)
		{
			long WasLockedForWrite = 0;
			void * pOriginalDstBuf;

			long SizeToLock = Samples * DstSampleSize;
			if (SizeToLock < -CDirectFile::CacheBufferSize())
			{
				SizeToLock = -CDirectFile::CacheBufferSize();
			}

			ULONG LockFlags = CDirectFile::GetBufferAndPrefetchNext;
			if (FileChannelsMask == DstChannels)
			{
				LockFlags = CDirectFile::GetBufferWriteOnly;
			}

			WasLockedForWrite = GetDataBuffer( & pOriginalDstBuf,
												SizeToLock, Pos, LockFlags);

			if (0 == WasLockedForWrite)
			{
				return TotalSamplesWritten;
			}

			unsigned SamplesWritten = -WasLockedForWrite / DstSampleSize;
			WAVE_SAMPLE * pDst = (WAVE_SAMPLE *) pOriginalDstBuf;

			if (0 == SamplesWritten)
			{
				ReturnDataBuffer(pOriginalDstBuf, WasLockedForWrite, 0);

				if (DstSampleSize != ReadAt(tmp + NumFileChannels, -DstSampleSize, Pos))
				{
					return TotalSamplesWritten;
				}

				pDst = tmp;
				SamplesWritten = 1;
			}
			else
			{
				pDst -= SamplesWritten * NumFileChannels;
			}

			WAVE_SAMPLE const * pSrc = (WAVE_SAMPLE const *) pBuf;
			pSrc -= SamplesWritten * NumSrcChannels;

			pBuf = pSrc;

			CopyWaveSamples(pDst, DstChannels, NumFileChannels,
							pSrc, SrcChannels, NumSrcChannels, SamplesWritten,
							GetSampleType(), type);

			if (-WasLockedForWrite < DstSampleSize)
			{
				LONG Written = WriteAt(tmp + NumFileChannels, -DstSampleSize, Pos);
				if (Written != -DstSampleSize)
				{
					return TotalSamplesWritten;
				}
			}
			else
			{
				ReturnDataBuffer(pOriginalDstBuf, WasLockedForWrite,
								CDirectFile::ReturnBufferDirty);
			}

			TotalSamplesWritten -= SamplesWritten;
			Pos -= SamplesWritten * DstSampleSize;
			Samples += SamplesWritten;

		}
	}
	return TotalSamplesWritten;
}

int CMmioFile::m_TextEncodingInFiles = 0;  // 0 - default ANSI code page, 1 - UTF-8, 2 - UTF-16
