// DirectFile.cpp: implementation of the CDirectFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DirectFile.h"
#include <process.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#pragma function(memcpy)

inline static DWORD InterlockedOr(DWORD * dst, DWORD op2)
{
	DWORD tmp;
	do {
		tmp = *dst;
	} while (tmp != InterlockedCompareExchange((long *) dst, tmp | op2, tmp));
	return tmp | op2;
}
inline static DWORD InterlockedAnd(DWORD * dst, DWORD op2)
{
	DWORD tmp;
	do {
		tmp = *dst;
	} while (tmp != InterlockedCompareExchange((long *) dst, tmp & op2, tmp));
	return tmp & op2;
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDirectFile::CDirectFile()
	: m_FilePointer(0),
	m_pFile(NULL)
{

}

CDirectFile::~CDirectFile()
{
	Close(0);
}

BOOL CDirectFile::Open(LPCTSTR szName, DWORD flags)
{
	Close(0);
	ASSERT(NULL != CDirectFileCache::GetInstance());
	File * pFile = CDirectFileCache::GetInstance()->Open(szName, flags);
	if (NULL == pFile)
	{
		return FALSE;
	}
	m_pFile = pFile;
	m_FilePointer = 0;
	return TRUE;
}

BOOL CDirectFile::Attach(CDirectFile * const pOriginalFile)
{
	if (NULL == pOriginalFile->m_pFile)
	{
		return FALSE;
	}

	InterlockedIncrement( & pOriginalFile->m_pFile->RefCount);
	Close(0);
	m_pFile = pOriginalFile->m_pFile;
	m_FilePointer = 0;
	return TRUE;
}

BOOL CDirectFile::SetSourceFile(CDirectFile * const pOriginalFile)
{
	if (NULL == pOriginalFile->m_pFile
		|| NULL == m_pFile
		|| m_pFile == pOriginalFile->m_pFile
		|| pOriginalFile->m_pFile == m_pFile)
	{
		return FALSE;
	}

	m_FilePointer = 0;
	return m_pFile->SetSourceFile(pOriginalFile->m_pFile);
}

// The function detaches pSourceFile previously set by SetSourceFile
// We need to call it before saving the data back to the main file
BOOL CDirectFile::DetachSourceFile()
{
	if (NULL == m_pFile
		|| ! m_pFile->InitializeTheRestOfFile())
	{
		return FALSE;
	}
	// all data from the source file is copied to the target,
	// we can detach it
	File * pFile = m_pFile->pSourceFile;
	m_pFile->pSourceFile = NULL;
	if (pFile)
	{
		return pFile->Close(0);
	}
	else
		return TRUE;
}

CDirectFile::File * CDirectFile::CDirectFileCache::Open(LPCTSTR szName, DWORD flags)
{
	// find if the file with this name is in the list.
	// if it is, add a reference
	if (flags & CreateMemoryFile)
	{
		// create memory file of zero length
		File * pFile = new File("");
		if (NULL == pFile)
		{
			return NULL;
		}
		pFile->m_Flags |= FileFlagsMemoryFile;
		CSimpleCriticalSectionLock lock(m_cs);

		m_FileList.InsertHead(pFile);

		SetLastError(ERROR_SUCCESS);
		return pFile;
	}
	CString FullName;
	char * pName = FullName.GetBuffer(512);

	LPTSTR pFilePart = NULL;
	GetFullPathName(szName, 511, pName, & pFilePart);
	FullName.ReleaseBuffer();
	TRACE("Full name: %s\n", (LPCTSTR) FullName);

	HANDLE hf;
	BY_HANDLE_FILE_INFORMATION info;
	// open the file to query information
	hf = CreateFile(FullName,
					0,
					FILE_SHARE_WRITE | FILE_SHARE_READ,
					NULL, // lpSecurity
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);
	if (hf != INVALID_HANDLE_VALUE)
	{
		if (FILE_TYPE_DISK != GetFileType(hf))
		{
			CloseHandle(hf);
			SetLastError(ERROR_INVALID_FUNCTION);
			return NULL;
		}
		if (::GetFileInformationByHandle(hf, & info))
		{
			CloseHandle(hf);
			TRACE("Volume ID: %08X, File index: %08X%08X\n",
				info.dwVolumeSerialNumber, info.nFileIndexHigh,
				info.nFileIndexLow);
			// find a File struct with the same parameters
			CSimpleCriticalSectionLock lock(m_cs);
			File * pFile = m_FileList.Next();
			while (m_FileList.Head() != pFile)
			{
				if (info.nFileIndexHigh == pFile->m_FileInfo.nFileIndexHigh
					&& info.nFileIndexLow == pFile->m_FileInfo.nFileIndexLow)
				{
					TRACE("File is in the list\n");
					// read-only file can be reopened only for reading
					if ((pFile->m_Flags & FileFlagsReadOnly)
						&& 0 == (flags & OpenReadOnly))
					{
						SetLastError(ERROR_SHARING_VIOLATION);
						return NULL;
					}

					SetLastError(ERROR_ALREADY_EXISTS);
					pFile->RefCount++;
					return pFile;
				}
				pFile = pFile->Next();
			}
		}
		else
		{
			CloseHandle(hf);
		}
	}
	// otherwise construct a new File structure, open the file
	// and put it to the list.
	File * pFile = new File(FullName);
	if (NULL == pFile)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}

	do {
		DWORD access = GENERIC_READ;
		DWORD ShareMode = FILE_SHARE_READ;
		DWORD Disposition = OPEN_EXISTING;
		if (0 == (flags & OpenReadOnly))
		{
			if (flags & (OpenDirect | OpenExisting))
			{
				ShareMode = 0;
				access = GENERIC_WRITE | GENERIC_READ;
			}
			else if (flags & CreateAlways)
			{
				ShareMode = 0;
				access = GENERIC_WRITE | GENERIC_READ;
				Disposition = CREATE_ALWAYS;
			}
			else if (flags & CreateNew)
			{
				ShareMode = 0;
				access = GENERIC_WRITE | GENERIC_READ;
				Disposition = CREATE_NEW;
			}
		}

		hf = CreateFile(FullName,
						access,
						ShareMode,
						NULL, // lpSecurity
						Disposition,
						FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL,
						NULL);
		if (INVALID_HANDLE_VALUE == hf)
		{
			if ((::GetLastError() == ERROR_SHARING_VIOLATION
					|| ::GetLastError() == ERROR_ACCESS_DENIED)
				&& (flags & OpenExisting)
				&& (flags & OpenAllowReadOnlyFallback))
			{
				flags &= ~(OpenExisting | OpenAllowReadOnlyFallback);
				flags |= OpenReadOnly;
				continue;
			}
			pFile->Close(0); // delete
			return NULL;
		}
	}
	while (hf == INVALID_HANDLE_VALUE);

	if (FILE_TYPE_DISK != GetFileType(hf))
	{
		SetLastError(ERROR_INVALID_FUNCTION);
		CloseHandle(hf);
		pFile->Close(0); // delete
		return NULL;
	}

	::GetFileInformationByHandle(hf, & pFile->m_FileInfo);
	pFile->FileLength = pFile->m_FileInfo.nFileSizeLow
						| (ULONGLONG(pFile->m_FileInfo.nFileSizeHigh << 32));
	pFile->RealFileLength = pFile->FileLength;

	if (0 == (flags & OpenReadOnly))
	{
		int WrittenMaskLength = 512 + int((pFile->FileLength + 0x7FFFF) >> 19);
		pFile->m_pWrittenMask = new char[WrittenMaskLength];
		if (NULL == pFile->m_pWrittenMask)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			CloseHandle(hf);
			pFile->Close(0); // delete
			return NULL;
		}

		pFile->WrittenMaskSize = WrittenMaskLength;
		memset(pFile->m_pWrittenMask, 0, WrittenMaskLength);
		memset(pFile->m_pWrittenMask, 0xFF, (pFile->FileLength +0xFFFF) >> 19);
		pFile->m_pWrittenMask[(pFile->FileLength +0xFFFF) >> 19] =
			0xFF >> (8 - (((pFile->FileLength +0xFFFF) >> 16) & 7));
	}
	else
	{
		pFile->m_pWrittenMask = NULL;
		pFile->WrittenMaskSize = 0;
	}

	if ((flags & OpenDeleteAfterClose)
		&& (flags & (CreateNew | CreateAlways)))
	{
		pFile->m_Flags |= FileFlagsDeleteAfterClose;
	}
	if (flags & OpenReadOnly)
	{
		pFile->m_Flags |= FileFlagsReadOnly;
	}

	pFile->hFile = hf;

	CSimpleCriticalSectionLock lock(m_cs);
	m_FileList.InsertHead(pFile);

	SetLastError(ERROR_SUCCESS);
	return pFile;
}

BOOL CDirectFile::File::SetSourceFile(File * pOriginalFile)
{
	ASSERT(0 == (m_Flags & FileFlagsMemoryFile));
	if (m_Flags & FileFlagsMemoryFile)
	{
		return FALSE;
	}
	if (pOriginalFile->pSourceFile != 0)
	{
		// double indirection is not allowed
		return FALSE;
	}
	InterlockedIncrement( & pOriginalFile->RefCount);
	if (pSourceFile != NULL)
	{
		pSourceFile->Close(0);
	}
	// zero m_pWrittenMask
	if (NULL != m_pWrittenMask)
	{
		memset(m_pWrittenMask, 0, WrittenMaskSize);
	}
	pSourceFile = pOriginalFile;
	if (NULL != pOriginalFile
		&& SetFileLength(pOriginalFile->FileLength))
	{
		UseSourceFileLength = pOriginalFile->FileLength;
	}
	return TRUE;
}

BOOL CDirectFile::File::Commit(DWORD flags)
{
	if (m_Flags & FileFlagsMemoryFile)
	{
		return TRUE;
	}
	if (m_Flags & FileFlagsReadOnly)
	{
		return FALSE;
	}
	// close the file, set length and reopen again if necessary
	if (flags & CommitFileFlushBuffers)
	{
		InitializeTheRestOfFile();
		if (NULL != pSourceFile)
		{
			pSourceFile->Close(0);
			pSourceFile = NULL;
		}
		Flush();

	}
	if (FileLength != RealFileLength)
	{
		CSimpleCriticalSectionLock lock(m_FileLock);
		CloseHandle(hFile);
		hFile = NULL;
		HANDLE hf = CreateFile(sName,
								GENERIC_WRITE,
								0,
								NULL, // lpSecurity
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL);
		long MoveHigh = FileLength >> 32;
		SetLastError(ERROR_SUCCESS);
		if (hf != INVALID_HANDLE_VALUE)
		{
			SetFilePointer(hf, long(FileLength), & MoveHigh, FILE_BEGIN);
			if (::GetLastError() == ERROR_SUCCESS)
			{
				SetEndOfFile(hf);
			}
			CloseHandle(hf);
		}
		if (0 == (flags & CommitFileDontReopen))
		{
			hf = CreateFile(sName,
							GENERIC_WRITE | GENERIC_READ,
							0,
							NULL, // lpSecurity
							OPEN_EXISTING,
							FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL,
							NULL);
			if (INVALID_HANDLE_VALUE == hf)
			{
				// couldnt reopen!!
				return FALSE;
			}
			hFile = hf;
			m_FilePointer = 0;

		}
		if (NULL != hFile)
		{
			TRACE("Refreshing file info after file commit, prev modif. time=0x%08X%08X, ",
				m_FileInfo.ftLastWriteTime.dwHighDateTime, m_FileInfo.ftLastWriteTime.dwLowDateTime);
			::GetFileInformationByHandle(hFile, & m_FileInfo);
			TRACE("new modif. time=0x%08X%08X\n",
				m_FileInfo.ftLastWriteTime.dwHighDateTime, m_FileInfo.ftLastWriteTime.dwLowDateTime);
			RealFileLength = m_FileInfo.nFileSizeLow
							| (ULONGLONG(m_FileInfo.nFileSizeHigh << 32));
		}
	}
	return TRUE;
}

BOOL CDirectFile::File::Rename(LPCTSTR NewName, DWORD flags)
{
	if (m_Flags & FileFlagsMemoryFile)
	{
		return FALSE;
	}
	if (m_Flags & FileFlagsReadOnly)
	{
		return FALSE;
	}
	// close the file, set length and reopen again if necessary
	CSimpleCriticalSectionLock lock(m_FileLock);
	if (FALSE == Commit(flags | CommitFileDontReopen))
	{
		return FALSE;
	}
	if (NULL != hFile)
	{
		CloseHandle(hFile);
		hFile = NULL;
	}
	BOOL result = FALSE;
	if (MoveFile(sName, NewName))
	{
		sName = NewName;
		result = TRUE;
	}
	else
	{
		TRACE("Couldn't rename the file from %s to %s, last error=%X\n",
			LPCTSTR(sName), LPCTSTR(NewName), ::GetLastError());
	}
	if (flags & CommitFileDontReopen)
	{
		return result;
	}

	DWORD access = GENERIC_WRITE | GENERIC_READ;

	if (flags & RenameFileOpenReadOnly)
	{
		access = GENERIC_READ;
		m_Flags |= FileFlagsReadOnly;
	}

	HANDLE hf = CreateFile(sName,
							access,
							0,
							NULL, // lpSecurity
							OPEN_EXISTING,
							FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL,
							NULL);
	if (INVALID_HANDLE_VALUE == hf)
	{
		// couldnt reopen!!
		TRACE("Couldn't reopen the file, last error=%X\n", ::GetLastError());
		return FALSE;
	}
	hFile = hf;
	m_FilePointer = 0;

	if (NULL != hFile)
	{
		::GetFileInformationByHandle(hFile, & m_FileInfo);
		RealFileLength = m_FileInfo.nFileSizeLow
						| (ULONGLONG(m_FileInfo.nFileSizeHigh << 32));
	}
	return result;
}

BOOL CDirectFile::File::Close(DWORD flags)
{
	// dereference File structure.
	CDirectFileCache * pCache = GetCache();
	ASSERT(pCache);
	// stop all background operations on the files

	// if the file in prefetch is ours, then wait for the operation to finish
	// last Close operation is always synchronous, there is no parallel activity
	if (InterlockedIncrement(const_cast<long *>( & pCache->m_ThreadRunState)) < 0)
	{
		while (pCache->m_ThreadRunState < 0)
		{
			WaitForSingleObject(pCache->m_hThreadSuspendedEvent, 2000);
		}
	}

	if (InterlockedDecrement( & RefCount) > 0)
	{
		// allow background thread to run along
		if (InterlockedDecrement(const_cast<long *>( & pCache->m_ThreadRunState)) == 0)
		{
			SetEvent(pCache->m_hEvent);
		}
		return TRUE;
	}
	// if this file was set to prefetch, cancel prefetch
	InterlockedCompareExchangePointer((void **) & pCache->m_pPrefetchFile, NULL, this);

	TRACE("Closing file %s, flags=%X\n", LPCTSTR(sName), m_Flags);
	// If the use count is 0, copy all remaining data
	// from the source file or init the rest and flush all the buffers,
	if (0 == (m_Flags & FileFlagsDeleteAfterClose))
	{
		InitializeTheRestOfFile();
	}
	if (NULL != pSourceFile)
	{
		pSourceFile->Close(0);
		pSourceFile = NULL;
	}
	{
		BufferHeader * pBuf;
		while (NULL != (pBuf = BuffersList.RemoveHead()))
		{
			ASSERT(0 == pBuf->LockCount);
			// something strange: buffer not released
			if (pBuf->LockCount != 0)
			{
				TRACE("CDirectFile::File::Close: Buffer not released!\n");
				// allow background thread to run along
				if (InterlockedDecrement(const_cast<long *>( & pCache->m_ThreadRunState)) == 0)
				{
					SetEvent(pCache->m_hEvent);
				}
				return FALSE;
			}
			if (pBuf->DirtyMask)
			{
				if (0 == (m_Flags & FileFlagsDeleteAfterClose))
				{
					pBuf->FlushDirtyBuffers();
				}
				else
				{
					DirtyBuffersCount--;
					pBuf->DirtyMask = 0;
				}
			}
			ASSERT(0 == pBuf->DirtyMask);

			BuffersCount--;

			CSimpleCriticalSectionLock lock1(pCache->m_cs);

			pCache->m_MruList.RemoveEntry(pBuf);

			pBuf->pFile = NULL;

			pCache->m_FreeBuffers.InsertHead(pBuf);
		}
	}
	// close the handle and delete the structure
	{
		CSimpleCriticalSectionLock lock(pCache->m_cs);
		// remove the structure from the list
		pCache->m_FileList.RemoveEntry(this);
	}

	if (m_Flags & FileFlagsDeleteAfterClose)
	{
		// in Windows NT, the file is filled with zeros before being closed
		// if the file wasn't written, CloseHandle may take a while
		// so we better reset file length
		if (NULL != hFile)
		{
			SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
			if (::GetLastError() == ERROR_SUCCESS)
			{
				SetEndOfFile(hFile);
			}
			CloseHandle(hFile);
			hFile = NULL;
		}
		DeleteFile(sName);
	}
	else
	{
		if (NULL != hFile)
		{
			CloseHandle(hFile);
			hFile = NULL;
		}
		// set file length
		// if the file is open for writing, open it in normal mode,
		// set length, and close. When the file is open in direct mode,
		// we cannot set arbitrary file length
		if (0 == (m_Flags & FileFlagsReadOnly))
		{
			if (0 == (m_Flags & FileFlagsMemoryFile)
				&& FileLength != RealFileLength)
			{
				HANDLE hf = CreateFile(sName,
										GENERIC_WRITE,
										0,
										NULL, // lpSecurity
										OPEN_EXISTING,
										FILE_ATTRIBUTE_NORMAL,
										NULL);
				long MoveHigh = FileLength >> 32;
				SetLastError(ERROR_SUCCESS);
				if (hf != INVALID_HANDLE_VALUE)
				{
					SetFilePointer(hf, long(FileLength), & MoveHigh, FILE_BEGIN);
					if (::GetLastError() == ERROR_SUCCESS)
					{
						SetEndOfFile(hf);
					}
					CloseHandle(hf);
				}
			}
		}
	}

	TRACE("Closed file %s\n", LPCTSTR(sName));
	delete this;

	// allow background thread to run along
	if (InterlockedDecrement(const_cast<long *>( & pCache->m_ThreadRunState)) == 0)
	{
		SetEvent(pCache->m_hEvent);
	}

	return TRUE;
}

BOOL CDirectFile::Close(DWORD flags)
{
	if (NULL == m_pFile)
	{
		return FALSE;
	}
	m_pFile->Close(flags);
	m_pFile = NULL;
	return TRUE;
}

LONGLONG CDirectFile::Seek(LONGLONG position, int origin)
{
	switch (origin)
	{
	case FILE_BEGIN:
		if (position < 0)
		{
			return -1i64;
		}
		m_FilePointer = position;
		break;
	case FILE_CURRENT:
		if (m_FilePointer + position < 0)
		{
			return -1i64;
		}
		m_FilePointer += position;
		break;
	case FILE_END:
		position += GetLength();
		if (position < 0)
		{
			return -1i64;
		}
		m_FilePointer = position;
		break;
	default:
		return -1i64;
	}
	return m_FilePointer;
}

// read data ('count' bytes) from the current position to *buf
long CDirectFile::Read(void *buf, long count)
{
	long TotalRead = 0;
	char * pDstBuf = (char *)buf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}

	if (m_FilePointer >= m_pFile->FileLength)
	{
		// beyond end of file
		return 0;
	}
	if (count > 0 && count + m_FilePointer > m_pFile->FileLength)
	{
		count = m_pFile->FileLength - m_FilePointer;
	}

	while(count > 0)
	{
		void * pSrcBuf;
		long nRead = GetDataBuffer( & pSrcBuf, count, m_FilePointer, GetBufferAndPrefetchNext);
		if (0 == nRead)
		{
			break;
		}
		memmove(pDstBuf, pSrcBuf, nRead);
		ReturnDataBuffer(pSrcBuf, nRead, 0);
		TotalRead += nRead;
		pDstBuf += nRead;
		m_FilePointer += nRead;
		count -= nRead;
	}
	return TotalRead;
}

// read data ('count' bytes) from the given 'position' to *buf
long CDirectFile::ReadAt(void *buf, long count, LONGLONG Position)
{
	long TotalRead = 0;
	char * pDstBuf = (char *)buf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}

	if (Position >= m_pFile->FileLength)
	{
		// beyond end of file
		return 0;
	}
	if (count > 0 && count + Position > m_pFile->FileLength)
	{
		count = m_pFile->FileLength - Position;
	}

	while(count > 0)
	{
		void * pSrcBuf;
		long nRead = GetDataBuffer( & pSrcBuf, count, Position, GetBufferAndPrefetchNext);
		if (0 == nRead)
		{
			break;
		}
		memmove(pDstBuf, pSrcBuf, nRead);
		ReturnDataBuffer(pSrcBuf, nRead, 0);
		Position += nRead;
		TotalRead += nRead;
		pDstBuf += nRead;
		count -= nRead;
	}
	return TotalRead;
}

// write data ('count' bytes) from the current position to *buf
long CDirectFile::Write(const void *pBuf, long count)
{
	long TotalWritten = 0;
	const char * pSrcBuf = (const char *)pBuf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}

	while(count > 0)
	{
		void * pDstBuf;
		long nWritten = GetDataBuffer( & pDstBuf, count, m_FilePointer, GetBufferWriteOnly);
		if (0 == nWritten)
		{
			break;
		}
		memmove(pDstBuf, pSrcBuf, nWritten);
		ReturnDataBuffer(pDstBuf, nWritten, ReturnBufferDirty);
		TotalWritten += nWritten;
		pSrcBuf += nWritten;
		m_FilePointer += nWritten;
		count -= nWritten;
	}
	if (TotalWritten != 0
		&& m_FilePointer > m_pFile->FileLength)
	{
		m_pFile->FileLength = m_FilePointer;
	}
	return TotalWritten;
}

// write data ('count' bytes) at the given 'position' from *buf
long CDirectFile::WriteAt(const void *buf, long count, LONGLONG Position)
{
	long TotalWritten = 0;
	char * pSrcBuf = (char *)buf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}
	while(count > 0)
	{
		void * pDstBuf;
		long nWritten = GetDataBuffer( & pDstBuf, count, Position, GetBufferWriteOnly);
		if (0 == nWritten)
		{
			break;
		}
		memmove(pDstBuf, pSrcBuf, nWritten);
		ReturnDataBuffer(pDstBuf, nWritten, ReturnBufferDirty);
		Position += nWritten;
		TotalWritten += nWritten;
		pSrcBuf += nWritten;
		count -= nWritten;
	}
	if (TotalWritten != 0
		&& Position > m_pFile->FileLength)
	{
		m_pFile->FileLength = Position;
	}
	return TotalWritten;
}

CDirectFile::CDirectFileCache *
	CDirectFile::CDirectFileCache::SingleInstance = NULL;

CDirectFile::CDirectFileCache::CDirectFileCache(size_t MaxCacheSize)
	: m_hThread(NULL),
	m_hEvent(NULL),
	m_pHeaders(NULL),
	m_pBuffersArray(NULL),
	m_MRU_Count(1),
	m_Flags(0),
	m_NumberOfBuffers(0),
	m_pPrefetchFile(NULL),
	m_PrefetchPosition(0),
	m_PrefetchLength(0),
	m_MinPrefetchMRU(0),
	m_FlushRequest(0),
	m_ThreadRunState(0)
{
	if (SingleInstance != NULL)
	{
		return;
	}
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	// this is manual reset event
	m_hThreadSuspendedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	// round up to 64K
	size_t CacheSize;
	MEMORYSTATUS st;
	GlobalMemoryStatus( & st);
	CacheSize = st.dwTotalPhys / 8;
	if (MaxCacheSize != 0
		&& CacheSize > MaxCacheSize)
	{
		CacheSize = MaxCacheSize;
	}

	TRACE("Direct file CacheSize = %d MB\n", CacheSize / 0x100000);
	CacheSize = (CacheSize + 0xFFFF) & ~0xFFFF;
	m_pBuffersArray = VirtualAlloc(NULL, CacheSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	m_NumberOfBuffers = CacheSize / 0x10000;

	m_pHeaders = new BufferHeader[m_NumberOfBuffers];

	for (int i = 0; i < m_NumberOfBuffers; i++)
	{
		m_FreeBuffers.InsertHead( & m_pHeaders[i]);

		m_pHeaders[i].DirtyMask = 0;
		m_pHeaders[i].ReadMask = 0;
		m_pHeaders[i].m_Flags = 0;
		m_pHeaders[i].LockCount = 0;
		m_pHeaders[i].MRU_Count = 0;
		m_pHeaders[i].pBuf = i * 0x10000 + (char *) m_pBuffersArray;
		m_pHeaders[i].pFile = NULL;
		m_pHeaders[i].PositionKey = 0;
	}

	unsigned ThreadID = NULL;

	m_hThread = (HANDLE) _beginthreadex(NULL, 0x10000, ThreadProc,
										this, CREATE_SUSPENDED, & ThreadID);
	if (m_hThread != NULL)
	{
		if (m_hEvent != NULL
			&& m_hThreadSuspendedEvent != NULL
			&& m_pBuffersArray != NULL
			&& m_pHeaders != NULL)
		{
			SingleInstance = this;
			SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);
			ResumeThread(m_hThread);
		}
		else
		{
			TerminateThread(m_hThread, -1);
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}
}

CDirectFile::CDirectFileCache::~CDirectFileCache()
{
	// stop the thread
	if (NULL != m_hThread)
	{
		m_ThreadRunState = ~0;
#ifdef _DEBUG
		DWORD Time = timeGetTime();
#endif
		SetEvent(m_hEvent);
		if (WAIT_TIMEOUT == WaitForSingleObject(m_hThread, 5000))
		{
			TerminateThread(m_hThread, -1);
		}
#ifdef _DEBUG
		TRACE("File Cache Thread finished in %d ms\n",
			timeGetTime() - Time);
#endif
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	// close the files
	// free the buffers
	if (NULL != m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
	if (NULL != m_hThreadSuspendedEvent)
	{
		CloseHandle(m_hThreadSuspendedEvent);
		m_hThreadSuspendedEvent = NULL;
	}
	if (NULL != m_pBuffersArray)
	{
		VirtualFree(m_pBuffersArray, 0, MEM_DECOMMIT | MEM_RELEASE);
		m_pBuffersArray = NULL;
		m_NumberOfBuffers = 0;
	}
	if (NULL != m_pHeaders)
	{
		delete[] m_pHeaders;
		m_pHeaders = NULL;
	}

	if (this == GetInstance())
	{
		SingleInstance = NULL;
	}
}

// Insert a buffer to the list with the specified key
// m_FileLock should be locked around this function call
void CDirectFile::File::InsertBuffer(BufferHeader * pBuf)
{
	unsigned long const key = pBuf->PositionKey;
	BuffersCount++;
	if (BuffersList.IsEmpty()
		|| BuffersList.Next()->PositionKey > key)
	{
		BuffersList.InsertHead(pBuf);
		// insert to the empty list
		return;
	}
	else if (BuffersList.Prev()->PositionKey < key)
	{
		// insert to the tail
		BuffersList.InsertTail(pBuf);
		return;
	}

	// find which side may be closer to the key
	BufferHeader * pBufAfter;
	if (key - BuffersList.Next()->PositionKey > BuffersList.Prev()->PositionKey - key)
	{
		// search from tail
		pBufAfter = BuffersList.Prev();
		while (pBufAfter->PositionKey > key)
		{
			pBufAfter = pBufAfter->KListEntry<BufferHeader>::Prev();
		}
	}
	else
	{
		// search from head
		pBufAfter = BuffersList.Next();
		while (pBufAfter->PositionKey < key)
		{
			pBufAfter = pBufAfter->KListEntry<BufferHeader>::Next();
			ASSERT(pBufAfter != NULL);
		}
		pBufAfter = pBufAfter->KListEntry<BufferHeader>::Prev();
		ASSERT(pBufAfter != NULL);
	}

	// pBufAfter->PositionKey < pBuf->PositionKey
	pBufAfter->KListEntry<BufferHeader>::InsertHead(pBuf);
}

// find a buffer in the list with the specified key
// m_FileLock should be locked around this function call
CDirectFile::BufferHeader * CDirectFile::File::FindBuffer(unsigned long key) const
{
	// the buffers are ordered: BuffersListHead has lowest key, BuffersListTail has the highest key
	BufferHeader * pBuf;
#ifdef _DEBUG
	ValidateList();
#endif
	if (BuffersList.IsEmpty()
		|| BuffersList.Next()->PositionKey > key
		|| BuffersList.Prev()->PositionKey < key)
	{
		return NULL;
	}
	// find which side may be closer to the key
	if (key - BuffersList.Next()->PositionKey > BuffersList.Prev()->PositionKey - key)
	{
		// search from tail
		pBuf = BuffersList.Prev();
		while (pBuf != BuffersList.Head() && pBuf->PositionKey >= key)
		{
			if (pBuf->PositionKey == key)
			{
				return pBuf;
			}
			pBuf = pBuf->KListEntry<BufferHeader>::Prev();
		}
	}
	else
	{
		// search from head
		pBuf = BuffersList.Next();
		while (pBuf != BuffersList.Head() && pBuf->PositionKey <= key)
		{
			if (pBuf->PositionKey == key)
			{
				return pBuf;
			}
			pBuf = pBuf->KListEntry<BufferHeader>::Next();
		}
	}
	return NULL;    // buffer not found
}

long CDirectFile::CDirectFileCache::GetDataBuffer(File * pFile,
	void * * ppBuf, LONGLONG length, LONGLONG position, DWORD flags, unsigned MaxMRU)
{
	ASSERT(pFile);
	ASSERT(ppBuf);
	if (NULL == pFile)
	{
		return 0;
	}
	long BytesRequested = 0;
	long BytesReturned = 0;
	int OffsetInBuffer = 0;
	if (pFile->m_Flags & FileFlagsMemoryFile)
	{
		if (NULL == pFile->m_pMemoryFileBuffer)
		{
			return 0;
		}
		*ppBuf = NULL;
		if (length > 0)
		{
			if (position >= pFile->m_MemoryFileBufferSize)
			{
				return 0;
			}
			long MaxLength = pFile->m_MemoryFileBufferSize - long(position);
			if (length > MaxLength)
			{
				length = MaxLength;
			}
		}
		else if (length < 0)
		{
			if (position > pFile->m_MemoryFileBufferSize)
			{
				return 0;
			}
			if (-length > position)
			{
				length = -position;
			}
		}
		else
		{
			return 0;
		}
		*ppBuf = pFile->m_pMemoryFileBuffer + DWORD(position);
		pFile->m_MemoryBufferRefCount++;
		return length;
	}
	unsigned long BufferPosition;
	if (length > 0)
	{
		BufferPosition = long(position >> 16);
		OffsetInBuffer = int(position) & 0xFFFF;
		if (OffsetInBuffer + length > 0x10000)
		{
			BytesRequested = 0x10000 - OffsetInBuffer;
			BytesReturned = BytesRequested;
		}
		else
		{
			BytesRequested = long(length);
			BytesReturned = BytesRequested;
			if (flags & GetBufferAndPrefetchNext)
			{
				// read the rest of the buffer
				BytesRequested = 0x10000 - OffsetInBuffer;
			}
		}
	}
	else if (length < 0)
	{
		if (-length > position)
		{
			length = -position;
			if (0 == length)
			{
				*ppBuf = NULL;
				return 0;
			}
		}
		OffsetInBuffer = 0x10000 - ((-long(position)) & 0xFFFF);
		if (OffsetInBuffer + length <= 0)    // length < 0
		{
			BytesRequested = OffsetInBuffer;
			BytesReturned = BytesRequested;
		}
		else
		{
			BytesRequested = -long(length);
			BytesReturned = BytesRequested;
			if (flags & GetBufferAndPrefetchNext)
			{
				// read the rest of the buffer
				BytesRequested = OffsetInBuffer;
			}
		}
		BufferPosition = long((position - BytesRequested) >> 16);
		OffsetInBuffer = (long(position) - BytesRequested) & 0xFFFF;
	}
	else
	{
		*ppBuf = NULL;
		return 0;
	}

	DWORD RequestedMask = (0xFFFFFFFFu << (OffsetInBuffer >> 11));
	if (OffsetInBuffer + BytesRequested <= 0xF800)
	{
		RequestedMask &= ~(0xFFFFFFFFu << ((OffsetInBuffer + BytesRequested + 0x7FF) >> 11));
	}
	DWORD MaskToRead = RequestedMask;
	if (flags & GetBufferWriteOnly)
	{
		MaskToRead = ~RequestedMask;
		if (OffsetInBuffer & 0x7FF)
		{
			MaskToRead |= (1L << (OffsetInBuffer >> 11));
		}
		if ((OffsetInBuffer + BytesRequested) & 0x7FF)
		{
			MaskToRead |= (1L << ((OffsetInBuffer + BytesRequested) >> 11));
		}
	}

	BufferHeader * pBuf = NULL;
	BufferHeader * pFreeBuf = NULL;

	{
		if (0 == InterlockedIncrement( (PLONG) & m_MRU_Count))
		{
			CSimpleCriticalSectionLock lock(CDirectFileCache::GetInstance()->m_cs);
			// change it to 0x80000000, subtract 0x80000000 from all counters
			for (int i = 0; i < m_NumberOfBuffers; i++)
			{
				if (m_pHeaders[i].MRU_Count > 0x80000000u)
				{
					m_pHeaders[i].MRU_Count -= 0x80000000u;
				}
				else
				{
					m_pHeaders[i].MRU_Count = 0;
				}
			}
			m_MRU_Count = 0x80000000u;
		}
	}

	{
		CSimpleCriticalSectionLock lock(pFile->BuffersList);
		pBuf = pFile->FindBuffer(BufferPosition);
		if (NULL != pBuf)
		{
			InterlockedIncrement( & pBuf->LockCount);
			pBuf->MRU_Count = m_MRU_Count;
		}
	}

	if (NULL == pBuf)
	{
		// the buffer is not found, get another free one
		pFreeBuf = GetFreeBuffer(MaxMRU);
		if (NULL == pFreeBuf)
		{
			// unable to find an available  buffer
			*ppBuf = NULL;
			return 0;
		}
		CSimpleCriticalSectionLock lock(pFile->BuffersList);
		// make sure the buffer with the same position was not added
		// while we might flush the buffers
		pBuf = pFile->FindBuffer(BufferPosition);

		if (NULL != pBuf)
		{
			InterlockedIncrement( & pBuf->LockCount);
		}
		else
		{
			pBuf = pFreeBuf;
			pFreeBuf = NULL;
			pBuf->BufferMruEntry::Init();

			pBuf->LockCount = 1;
			pBuf->ReadMask = 0;
			pBuf->DirtyMask = 0;
			pBuf->pFile = pFile;
			pBuf->PositionKey = BufferPosition;
			// insert the buffer to the list for file,
			// to the correct position
			pFile->InsertBuffer(pBuf);
		}
		pBuf->MRU_Count = m_MRU_Count;
	}
	{
		// extract the buffer from MRU list
		CSimpleCriticalSectionLock lock(m_cs);

		m_MruList.RemoveEntry(pBuf);
		// since we already have the required buffer,
		// return the free buffer back
		if (NULL != pFreeBuf)
		{
			m_FreeBuffers.InsertHead(pFreeBuf);
		}
		// insert the buffer to the list head
		m_MruList.InsertHead(pBuf);
	}

	if (MaskToRead != (pBuf->ReadMask & MaskToRead)
		|| (flags & GetBufferWriteOnly))
	{
		pFile->ReadDataBuffer(pBuf, MaskToRead);
	}
	// start prefetch for the rest of the data, if GetBufferWriteOnly is not set
	// return number of bytes available

	if (length >= 0)
	{
		*ppBuf = OffsetInBuffer + (char *)pBuf->pBuf;
		if (0 == (flags & (GetBufferNoPrefetch | GetBufferWriteOnly))
			&& (BytesRequested < length || (flags & GetBufferAndPrefetchNext)))
		{
			if (flags & GetBufferAndPrefetchNext)
			{
				if (0 || length <= BytesRequested)
				{
					// prefetch the next buffer
					length = (long(position) & ~0xFFFFL) + 0x20000 - long(position);
				}
				else
				{
					// round the length to make it prefetch the whole buffer
					length += 0xFFFF & -(long(position) + long(length));
				}
			}
			RequestPrefetch(pFile, position + BytesRequested, length - BytesRequested, m_MRU_Count);
		}
		ASSERT(BytesReturned <= length);
		return BytesReturned;
	}
	else
	{
		* ppBuf = OffsetInBuffer + BytesRequested + (char *)pBuf->pBuf;
		if (0 == (flags & (GetBufferNoPrefetch | GetBufferWriteOnly))
			&& (BytesRequested < -length || (flags & GetBufferAndPrefetchNext)))
		{
			if (flags & GetBufferAndPrefetchNext)
			{
				if ( 0 || -length <= BytesRequested)
				{
					// prefetch the next buffer
					length = (position | 0xFFFF) + 1 - 0x20000 - position;
				}
				else
				{
					// round the length to make it prefetch the whole buffer
					length -= 0xFFFF & (long(position) - length);
				}
			}
			RequestPrefetch(pFile, position - BytesRequested, length + BytesRequested, m_MRU_Count);
		}
		ASSERT(BytesReturned <= -length);
		return -BytesReturned;
	}
}

void CDirectFile::CDirectFileCache::RequestPrefetch(File * pFile,
													LONGLONG PrefetchPosition,
													LONGLONG PrefetchLength, unsigned MaxMRU)
{
	if (PrefetchLength > 0)
	{
		if (PrefetchPosition + PrefetchLength > pFile->FileLength)
		{
			PrefetchLength = pFile->FileLength - PrefetchPosition;
			if (PrefetchLength <= 0)
			{
				return;
			}
		}
	}
	if (0) TRACE("RequestPrefetch: 0x%X bytes at 0x%X\n", long(PrefetchLength),
				long(PrefetchPosition));
	{
		CSimpleCriticalSectionLock lock(m_cs);
		if (m_pPrefetchFile != pFile
			&& PrefetchPosition + PrefetchLength !=
			m_PrefetchPosition + m_PrefetchLength)
		{
			m_pPrefetchFile = pFile;
			m_PrefetchPosition = PrefetchPosition;
			m_PrefetchLength = PrefetchLength;
		}
		m_MinPrefetchMRU = MaxMRU;
	}
	SetEvent(m_hEvent);
}

void CDirectFile::CDirectFileCache::RequestFlush(File * pFile, LONGLONG FlushPosition,
												LONG FlushLength)
{
	// flush only whole buffers
	if (0 == pFile->m_FlushLength)
	{
		pFile->m_FlushBegin = FlushPosition;
		pFile->m_FlushLength = FlushLength;
	}
	else
	{
		if (FlushPosition == pFile->m_FlushBegin + pFile->m_FlushLength)
		{
			pFile->m_FlushLength += FlushLength;
		}
		else
		{
			if (pFile->m_FlushBegin > FlushPosition)
			{
				pFile->m_FlushLength += LONG(pFile->m_FlushBegin) - LONG(FlushPosition);
				pFile->m_FlushBegin = FlushPosition;
			}
			if (pFile->m_FlushBegin + pFile->m_FlushLength < FlushPosition + FlushLength)
			{
				pFile->m_FlushLength = LONG(FlushPosition) - LONG(pFile->m_FlushBegin) + FlushLength;
			}
		}
	}
	// if flush boundary ends or crosses 64 boundary, request flush
	if ((LONG(pFile->m_FlushBegin) & 0xFFFF) + pFile->m_FlushLength >= 0x10000)
	{
		m_FlushRequest = TRUE;
		SetEvent(m_hEvent);
	}
}

void CDirectFile::CDirectFileCache::FlushRequestedFiles()
{
	// file cannot be removed during this call

	for (File * pFile = m_FileList.Next(); m_FileList.Head() != pFile; pFile = pFile->Next())
	{
		pFile->FlushRequestedRange();
	}
}

void CDirectFile::CDirectFileCache::ReturnDataBuffer(File * pFile,
													void * pBuffer, long count, DWORD flags)
{
	if (NULL == pBuffer || 0 == count)
	{
		return;
	}
	if (pFile->m_Flags & FileFlagsMemoryFile)
	{
		pFile->m_MemoryBufferRefCount--;
		return;
	}
	// find the buffer descriptor
	DWORD dwBuffer = DWORD(pBuffer);
	if (count < 0)
	{
		dwBuffer += count;
		count = -count;
	}
	int BufferNumber = (dwBuffer - DWORD(m_pBuffersArray)) / 0x10000;
	BufferHeader * pBuf = & m_pHeaders[BufferNumber];
	ASSERT(BufferNumber >= 0 && BufferNumber < m_NumberOfBuffers);
	ASSERT(pBuf->pFile == pFile);
	ASSERT(pBuf->LockCount > 0);
	ASSERT((DWORD)pBuf->pBuf == (dwBuffer & ~0xFFFF));
	// mark the buffer dirty, if necessary
	int OffsetInBuffer = dwBuffer & 0xFFFF;
	LONGLONG FileOffset = OffsetInBuffer + (LONGLONG(pBuf->PositionKey) << 16);

	ASSERT(OffsetInBuffer + count <= 0x10000);

	{
		CSimpleCriticalSectionLock lock(pFile->BuffersList);
		if (flags & CDirectFile::ReturnBufferDirty)
		{
			DWORD RequestedMask = (0xFFFFFFFFu << (OffsetInBuffer >> 11));
			if (OffsetInBuffer + count <= 0xF800)
			{
				RequestedMask &= ~(0xFFFFFFFFu << ((OffsetInBuffer + count + 0x7FF) >> 11));
			}
			DWORD OldDirtyMask = pBuf->DirtyMask;
			InterlockedOr( & pBuf->DirtyMask, RequestedMask);
			InterlockedOr( & pBuf->ReadMask, RequestedMask);
			if (0 == OldDirtyMask && 0 != RequestedMask)
			{
				InterlockedIncrement((PLONG) & pFile->DirtyBuffersCount);
			}
		}
	}
	BufferMruEntry * pMru = pBuf;
	if (0 == ((count + dwBuffer) & 0xFFFF)
		&& (flags & CDirectFile::ReturnBufferDiscard))
	{
		// move the buffer to MRU tail
		// extract the buffer from MRU list
		CSimpleCriticalSectionLock lock(m_cs);

		m_MruList.RemoveEntry(pMru);
		// insert the buffer to the list tail
		// to do: insert the buffer before the first with MRU_Count==1
		pMru->MRU_Count = 1;
		BufferMruEntry * pBufAfter = m_MruList.Prev();
		while (m_MruList.Head() != pBufAfter
				&& 1 == pBufAfter->MRU_Count)
		{
			pBufAfter = pBufAfter->Prev();
		}

		pBufAfter->InsertHead(pMru);

	}
	// decrement lock count
	if (0 == InterlockedDecrement( & pBuf->LockCount))
	{
		// set a request for writing
		if (flags & CDirectFile::ReturnBufferFlush)
		{
			if (count >= 0)
			{
				RequestFlush(pFile, FileOffset, count);
			}
			else
			{
				RequestFlush(pFile, FileOffset + count, -count);
			}
		}
	}
}

CDirectFile::BufferHeader
	* CDirectFile::CDirectFileCache::GetFreeBuffer(unsigned MaxMRU)
{
	while (1) {
		CSimpleCriticalSectionLock lock(m_cs);
		{
			BufferHeader * pBuf = NULL;
			// try to find an empty buffer
			if ( ! m_FreeBuffers.IsEmpty())
			{
				pBuf = m_FreeBuffers.RemoveHead();
				pBuf->ReadMask = 0;
				pBuf->DirtyMask = 0;
				return pBuf;
			}
		}

		// find an unlocked buffer with oldest MRU stamp
		if (0 == MaxMRU)
		{
			MaxMRU = m_MRU_Count;
		}
		BufferMruEntry * pMru = m_MruList.Prev();
		// find least recent unlocked and non-dirty buffer (with lowest MRU)
		// whose MRU is at most MaxMRU.
		while ( & m_MruList != pMru
				&& pMru->MRU_Count < MaxMRU
				&& (pMru->LockCount > 0
					|| 0 != pMru->DirtyMask))
		{
			pMru = pMru->Prev();
		}

		// unlocked buffer not found or buffer was too recent
		if ( & m_MruList == pMru || pMru->MRU_Count >= MaxMRU)
		{
			// try to find a dirty buffer
			pMru = m_MruList.Prev();
			while ( & m_MruList != pMru
					&& pMru->MRU_Count < MaxMRU
					&& pMru->LockCount > 0)
			{
				pMru = pMru->Prev();
			}
			if ( & m_MruList != pMru
				&& 0 == pMru->LockCount
				&& pMru->MRU_Count < MaxMRU)
			{
				// only "dirty" buffer available
				static_cast<BufferHeader *>(pMru)->FlushDirtyBuffers();
				// try the loop again
				continue;
			}
			return NULL;  // unable to find a buffer
		}

		File * pFile = pMru->pFile;
		CSimpleCriticalSectionLock lock1(pFile->BuffersList);
		// check if the buffer changed
		if (0 != pMru->LockCount
			|| 0 != pMru->DirtyMask)
		{
			continue;   // try again
		}

		// get the buffer from the MRU list
		m_MruList.RemoveEntry(pMru);

		BufferHeader * pBuf = static_cast<BufferHeader *>(pMru);
		pBuf->KListEntry<BufferHeader>::RemoveFromList();

		pFile->BuffersCount--;
		ASSERT(0 == pBuf->DirtyMask);
		pBuf->ReadMask = 0;
		pBuf->DirtyMask = 0;
		pBuf->pFile = NULL;
		return pBuf;
	}
	return NULL;
}

// read the data by 32-bit MaskToRead (each bit corresponds to 2K)
// to the buffer defined by BufferHeader *pBuf
void CDirectFile::File::ReadDataBuffer(BufferHeader * pBuf, DWORD MaskToRead)
{
	// check if the required data is already in the buffer
	if (0 != MaskToRead
		&& (pBuf->ReadMask & MaskToRead) == MaskToRead)
	{
		return;
	}
	int OldPriority;
	{
		CSimpleCriticalSectionLock lock(m_FileLock);
		// the operation in progress might change the mask
		if (0 != MaskToRead
			&& (pBuf->ReadMask & MaskToRead) == MaskToRead)
		{
			return;
		}
		MaskToRead &= ~ pBuf->ReadMask;
		// read the required data
		LONGLONG StartFilePtr = ULONGLONG(pBuf->PositionKey) << 16;
		char * buf = (char *) pBuf->pBuf;
		// check if the buffer is in the initialized part of the file
		if (0 == (m_Flags & FileFlagsReadOnly))
		{
			int WrittenMaskOffset = pBuf->PositionKey >> 3;
			if (WrittenMaskOffset >= WrittenMaskSize)
			{
				int NewWrittenMaskSize = WrittenMaskOffset + 512;   // 256 more megs
				char * NewWrittenMask = new char[NewWrittenMaskSize];
				if (NULL == NewWrittenMask)
				{
					if (0) TRACE("NewWrittenMask == NULL\n");
					memset(pBuf->pBuf, 0, 0x10000);
					pBuf->ReadMask = 0xFFFFFFFF;
					return;
				}
				if (0) TRACE("Allocated NewWrittenMask, size=%d\n", NewWrittenMaskSize);
				memset(NewWrittenMask, 0, NewWrittenMaskSize);
				memcpy(NewWrittenMask, m_pWrittenMask, WrittenMaskSize);
				delete[] m_pWrittenMask;
				m_pWrittenMask = NewWrittenMask;
				WrittenMaskSize = NewWrittenMaskSize;
			}
			ASSERT(m_pWrittenMask);
			if (0 == (m_pWrittenMask[WrittenMaskOffset]
					& (1 << (pBuf->PositionKey & 7))))
			{
				m_pWrittenMask[WrittenMaskOffset] |= 1 << (pBuf->PositionKey & 7);

				DWORD BytesRead = 0;
				int ToZero = 0x10000;
				if (NULL != pSourceFile
					&& StartFilePtr < UseSourceFileLength
					&& MaskToRead != 0)
				{
					// read the data from the source file
					int ToRead = 0x10000;
					if (UseSourceFileLength - StartFilePtr < 0x10000)
					{
						ToRead = int(UseSourceFileLength) - int(StartFilePtr);
					}

					CSimpleCriticalSectionLock lock(pSourceFile->m_FileLock);
					if (StartFilePtr != pSourceFile->m_FilePointer)
					{
						LONG FilePtrH = LONG(StartFilePtr >> 32);
						SetFilePointer(pSourceFile->hFile, (LONG)StartFilePtr, & FilePtrH, FILE_BEGIN);
						pSourceFile->m_FilePointer = StartFilePtr;
					}
					// round to sector size! otherwize ReadFile would fail
					if (0) TRACE("ReadSourceFile(%08x, pos=0x%08X, bytes=%X)\n", hFile, long(StartFilePtr), ToRead);
					ReadFile(pSourceFile->hFile, buf, (ToRead + 0x1FF) & ~0x1FF, & BytesRead, NULL);
#ifdef _DEBUG
					if (BytesRead < ToRead)
					{
						if (0) TRACE("ToRead=%x, BytesRead=%x\n", ToRead, BytesRead);
					}
#endif
					if (0 == m_LastError)
					{
						m_LastError = ::GetLastError();
					}

					pSourceFile->m_FilePointer += BytesRead;
					ToZero -= BytesRead;
				}

				if (ToZero > 0)
				{
					memset(buf + BytesRead, 0, ToZero);
				}

				pBuf->ReadMask = 0xFFFFFFFF;
				if (0 == InterlockedExchange( & reinterpret_cast<long &>(pBuf->DirtyMask), 0xFFFFFFFF))
				{
					InterlockedIncrement((PLONG) & DirtyBuffersCount);
				}
				return;
			}
		}

		DWORD mask = MaskToRead;
		while (mask != 0)
		{
			size_t ToRead;
			if (mask == 0xFFFFFFFF)
			{
				mask = 0;
				ToRead = 0x10000;
			}
			else
			{
				while (0 == (mask & 1))
				{
					mask >>= 1;
					StartFilePtr += 0x800;
					buf += 0x800;
				}
				ToRead = 0;
				while (mask & 1)
				{
					mask >>= 1;
					ToRead += 0x800;
				}
			}
			DWORD BytesRead = 0;

			if (0) TRACE("Stored file pointer: %X, actual: %X\n",
						long(m_FilePointer), SetFilePointer(hFile, 0, NULL, FILE_CURRENT));

			if (StartFilePtr != m_FilePointer)
			{
				LONG FilePtrH = LONG(StartFilePtr >> 32);
				SetFilePointer(hFile, (LONG)StartFilePtr, & FilePtrH, FILE_BEGIN);
				m_FilePointer = StartFilePtr;
			}
			if (0) TRACE("ReadFile(%08x, pos=0x%08X, bytes=%X)\n", hFile, long(StartFilePtr), ToRead);
			ReadFile(hFile, buf, ToRead, & BytesRead, NULL);
			if (0 == m_LastError)
			{
				m_LastError = ::GetLastError();
			}
#ifdef _DEBUG
			if (BytesRead < ToRead)
			{
				if (0) TRACE("ToRead=%x, BytesRead=%x\n", ToRead, BytesRead);
			}
#endif
			m_FilePointer += BytesRead;
			buf += BytesRead;
			StartFilePtr += BytesRead;
			ToRead -= BytesRead;
			if (ToRead)
			{
				memset(buf, 0, ToRead);
			}
			buf += ToRead;
			StartFilePtr += ToRead;
		}
		InterlockedOr(& pBuf->ReadMask, MaskToRead);
		DWORD OldDirtyMask = pBuf->DirtyMask;
		InterlockedAnd(& pBuf->DirtyMask, ~MaskToRead);
		if (0 != OldDirtyMask && 0 == pBuf->DirtyMask)
		{
			InterlockedDecrement((PLONG) & DirtyBuffersCount);
		}
		OldPriority = ::GetThreadPriority(GetCurrentThread());
		// yield execution to the waiting thread by lowering priority
		::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	}
	::SetThreadPriority(GetCurrentThread(), OldPriority);
}

void CDirectFile::BufferHeader::FlushDirtyBuffers(unsigned long MaxKey)
{
	// flush all unlocked dirty buffers in sequence with pBuf;
	BufferHeader * pDirtyBuf = this;
	BufferHeader * pBuf = KListEntry<BufferHeader>::Prev();
	unsigned key = PositionKey;
	ASSERT(NULL != pDirtyBuf);
	ASSERT(pFile);

	CSimpleCriticalSectionLock lock(pFile->BuffersList);

	while (pBuf != pFile->BuffersList.Head())
	{
		if (key - pBuf->PositionKey <= 2)
		{
			if (0 != pBuf->DirtyMask)
			{
				key = pBuf->PositionKey;
				pDirtyBuf = pBuf;
			}
		}
		else
		{
			break;
		}
		pBuf = pBuf->KListEntry<BufferHeader>::Prev();
	}
	pBuf = pDirtyBuf;
	key = pBuf->PositionKey;
	// 55ms resolution is OK
	DWORD StartTickCount = GetTickCount();
	do {
		if (pBuf->PositionKey > MaxKey)
		{
			break;
		}
		if (pBuf->DirtyMask)
		{
			key = pBuf->PositionKey;
			CSimpleCriticalSectionLock lock(pFile->m_FileLock);
			LONGLONG StartFilePtr = ULONGLONG(pBuf->PositionKey) << 16;
			unsigned char * buf = (unsigned char *) pBuf->pBuf;
			int WrittenMaskOffset = pBuf->PositionKey >> 3;

			ASSERT(WrittenMaskOffset < pFile->WrittenMaskSize);
			ASSERT(pFile->m_pWrittenMask);

			DWORD mask = InterlockedExchange( (PLONG)& pBuf->DirtyMask, 0);
			if (mask) InterlockedDecrement((PLONG) & pFile->DirtyBuffersCount);
			while (mask != 0)
			{
				size_t ToWrite;
				if (mask == 0xFFFFFFFF)
				{
					mask = 0;
					ToWrite = 0x10000;
				}
				else
				{
					while (0 == (mask & 1))
					{
						mask >>= 1;
						StartFilePtr += 0x800;
						buf += 0x800;
					}
					ToWrite = 0;
					while (mask & 1)
					{
						mask >>= 1;
						ToWrite += 0x800;
					}
				}
				DWORD BytesWritten = 0;
				if (0) TRACE("Stored file pointer: %X, actual: %X\n",
							long(pFile->m_FilePointer), SetFilePointer(pFile->hFile, 0, NULL, FILE_CURRENT));

				if (StartFilePtr != pFile->m_FilePointer)
				{
					LONG FilePtrH = LONG(StartFilePtr >> 32);
					SetFilePointer(pFile->hFile, (LONG)StartFilePtr, & FilePtrH, FILE_BEGIN);
					pFile->m_FilePointer = StartFilePtr;
				}
				if (0) TRACE("Writing h=%X, pos=%X, len=%X, "
							"data=%02X %02X %02X %02X %02X %02X %02X %02X\n", pFile->hFile,
							long(StartFilePtr), ToWrite,
							buf[0], buf[1], buf[2], buf[3], buf[4],
							buf[5], buf[6], buf[7]);
				if (0) TRACE("WriteFile(%08x, pos=0x%08X, bytes=%X)\n", pFile->hFile, long(StartFilePtr), ToWrite);
				BOOL result = WriteFile(pFile->hFile, buf, ToWrite, & BytesWritten, NULL);
				if (0 == pFile->m_LastError)
				{
					pFile->m_LastError = ::GetLastError();
				}
				if (BytesWritten != ToWrite)
				{
					// run a message box in the main thread
					// if it is a secondary thread, post a command
				}
				pFile->m_FilePointer += BytesWritten;
				buf += ToWrite;
				StartFilePtr += ToWrite;
				if (StartFilePtr > pFile->RealFileLength)
				{
					pFile->RealFileLength = StartFilePtr;
				}
				if (BytesWritten != ToWrite)
				{
					break;
				}
			}
		}
		pBuf = pBuf->KListEntry<BufferHeader>::Next();
	}
	while (pFile->BuffersList.Head() != pBuf
			&& pBuf->PositionKey - key <= 2
			// limit the operation to 500 ms
			&& GetTickCount() - StartTickCount < 300);
}

#ifdef _DEBUG
#define VL_ASSERT(expr) if ( ! (expr)) \
	{ TRACE("FALSE ==(" #expr ")\n"); \
	__asm int 3 \
		}
void CDirectFile::File::ValidateList() const
{
	if (m_Flags & FileFlagsMemoryFile)
	{
		return;
	}
	CSimpleCriticalSectionLock lock(BuffersList);
	BufferHeader * pBuf = BuffersList.Next();

	int BufCount = 0;
	int DirtyBufCount = 0;
	while (BuffersList.Head() != pBuf)
	{
		VL_ASSERT(pBuf->pFile == this);

		//VL_ASSERT(pBuf);
		BufCount++;
		if (pBuf->DirtyMask)
		{
			DirtyBufCount++;
		}
		pBuf = pBuf->KListEntry<BufferHeader>::Next();
	}
	VL_ASSERT(BufCount == BuffersCount && DirtyBufCount == DirtyBuffersCount);
}
#endif

unsigned CDirectFile::CDirectFileCache::_ThreadProc()
{
#ifdef _DEBUG
	FILETIME UserTime, EndTime, tmp;
	GetThreadTimes(GetCurrentThread(), & tmp, & tmp, & tmp, & UserTime);
#endif
	while (m_ThreadRunState != ~0)
	{
		int WaitStatus = WaitForSingleObject(m_hEvent, 2000);
		if (0 != InterlockedCompareExchange(const_cast<long *>( & m_ThreadRunState), 0x80000000, 0))
		{
			// signal that the thread is inactive
			SetEvent(m_hThreadSuspendedEvent);
			while (m_ThreadRunState != ~0 && 0 != m_ThreadRunState)
			{
				WaitStatus = WaitForSingleObject(m_hEvent, 2000);
			}
			ResetEvent(m_hThreadSuspendedEvent);
			continue;
		}
		if (WAIT_TIMEOUT == WaitStatus)
		{
			// flush the files
			for (int nFile = 0 ; NULL == m_pPrefetchFile && 0x80000000 == m_ThreadRunState
				; nFile++)
			{
				File * pFile = 0;
				{
					CSimpleCriticalSectionLock lock(m_cs);
					pFile = m_FileList.Next();
					for (int i = 0; pFile != m_FileList.Head()
						&& (i < nFile
							|| 0 == pFile->DirtyBuffersCount
							|| (pFile->m_Flags & FileFlagsMemoryFile)); i++,
						pFile = pFile->Next())
					{
						// empty
					}
					nFile = i;
					if (pFile == m_FileList.Head())
					{
						break;
					}
					if (0 == pFile->DirtyBuffersCount)
					{
						continue;
					}
					// there is no need to add a reference to file
					// Close() will wait for the loop to suspend
				}
				pFile->Flush();
			}
		}
		while(0x80000000 == m_ThreadRunState)
		{
			// flush the files
			while (m_FlushRequest)
			{
				m_FlushRequest = FALSE;
				FlushRequestedFiles();
			}
			LONGLONG PrefetchPosition;
			LONGLONG PrefetchLength;
			unsigned PrefetchMRU;
			File * pPrefetchFile;
			if (NULL == m_pPrefetchFile)
			{
				break;
			}
			{
				CSimpleCriticalSectionLock lock(m_cs);
				pPrefetchFile = m_pPrefetchFile;
				PrefetchPosition = m_PrefetchPosition;
				PrefetchLength = m_PrefetchLength;
				PrefetchMRU = m_MinPrefetchMRU;
			}
			if (NULL == pPrefetchFile)
			{
				break;
			}

			// check for prefetch range expiration
			DWORD CurrentTick = GetTickCount();
			LONGLONG OriginalPrefetchPosition = PrefetchPosition;
			LONGLONG OriginalPrefetchLength = PrefetchLength;

			if (CurrentTick - pPrefetchFile->m_LastPrefetchTick
				> pPrefetchFile->PrefetchRangeExpirationTimeout)
			{
				pPrefetchFile->m_PrefetchedBeginBlock = -1;
				pPrefetchFile->m_PrefetchedEndBlock = -1;
			}
			pPrefetchFile->m_LastPrefetchTick = CurrentTick;

			// limit prefetch length
			if (PrefetchLength >= 0)
			{
				if (DWORD(PrefetchLength >> 16) > pPrefetchFile->m_MaxBlocksToPrefetch)
				{
					PrefetchLength = LONGLONG(pPrefetchFile->m_MaxBlocksToPrefetch) << 16;
				}
			}
			else
			{
				if (DWORD(( - PrefetchLength) >> 16) > pPrefetchFile->m_MaxBlocksToPrefetch)
				{
					PrefetchLength = -LONGLONG(pPrefetchFile->m_MaxBlocksToPrefetch) << 16;
				}
			}

			// if prefetch area is active
			// check already prefetched ranges
			if (pPrefetchFile->m_PrefetchedEndBlock != -1)
			{
				if (PrefetchLength >= 0)
				{
					if (DWORD(PrefetchPosition >> 16) < pPrefetchFile->m_PrefetchedEndBlock)
					{
						PrefetchLength -= (LONGLONG(pPrefetchFile->m_PrefetchedEndBlock) << 16)
										- PrefetchPosition;
						PrefetchPosition = LONGLONG(pPrefetchFile->m_PrefetchedEndBlock) << 16;

					}
					pPrefetchFile->m_PrefetchedBeginBlock = DWORD(PrefetchPosition >> 16);
					if (PrefetchLength < 0)
					{
						PrefetchLength = 0;
					}
				}
				else
				{
					if (DWORD(PrefetchPosition >> 16) > pPrefetchFile->m_PrefetchedBeginBlock)
					{
						PrefetchLength -= (LONGLONG(pPrefetchFile->m_PrefetchedBeginBlock) << 16)
										- PrefetchPosition;
						PrefetchPosition = LONGLONG(pPrefetchFile->m_PrefetchedBeginBlock) << 16;

					}
					pPrefetchFile->m_PrefetchedEndBlock = DWORD(PrefetchPosition >> 16);
					if (PrefetchLength >= 0)
					{
						PrefetchLength = 0;
					}
				}
			}
			else
			{
				// set prefetch area
				pPrefetchFile->m_PrefetchedBeginBlock = PrefetchPosition >> 16;
				pPrefetchFile->m_PrefetchedEndBlock = pPrefetchFile->m_PrefetchedBeginBlock;
			}

			if (0 == PrefetchLength)
			{
				break;
			}
			void * pBuf = NULL;
			long ReadLength = GetDataBuffer(pPrefetchFile, & pBuf, PrefetchLength,
											PrefetchPosition, GetBufferNoPrefetch, PrefetchMRU);
			//TRACE("Prefetched buffer at 0x%X\n", long(PrefetchPosition));
			if (pBuf)
			{
				ReturnDataBuffer(pPrefetchFile, pBuf, ReadLength, 0);
			}
			// prefetch was canceled for this file
			{
				CSimpleCriticalSectionLock lock(m_cs);
				if (m_pPrefetchFile != pPrefetchFile
					|| OriginalPrefetchPosition != m_PrefetchPosition
					|| OriginalPrefetchLength != m_PrefetchLength
					)
				{
					// prefetch assignment changed
					continue;
				}

				m_PrefetchLength -= ReadLength;
				if (0 != ReadLength
					&& 0 != m_PrefetchLength)
				{
					m_PrefetchPosition = PrefetchPosition + ReadLength;
					if (ReadLength >= 0)
					{
						pPrefetchFile->m_PrefetchedEndBlock = DWORD(PrefetchPosition >> 16);
					}
					else
					{
						pPrefetchFile->m_PrefetchedBeginBlock = DWORD(PrefetchPosition >> 16);
					}
					continue;
				}
				else
				{
					m_pPrefetchFile = NULL;
					m_PrefetchLength = 0;
					break;
				}
			}
		}
		// reset bit 0x80000000, but only if not equal ~0
		long tmp;
		while ((tmp = m_ThreadRunState) != ~0
				&& tmp != InterlockedCompareExchange(const_cast<long *>( & m_ThreadRunState),
													tmp & ~ 0x80000000, tmp));
	}
#ifdef _DEBUG
	GetThreadTimes(GetCurrentThread(), & tmp, & tmp, & tmp, & EndTime);
	TRACE("File cache thread used time=%d ms\n",
		(EndTime.dwLowDateTime - UserTime.dwLowDateTime) / 10000);
#endif
	return 0;
}

BOOL CDirectFile::File::InitializeTheRestOfFile(int timeout, int * pPercentCompleted)
{
	if (NULL == m_pWrittenMask
		|| (m_Flags & FileFlagsReadOnly)
		|| (m_Flags & FileFlagsMemoryFile))
	{
		if (pPercentCompleted)
		{
			*pPercentCompleted = 100;
		}
		return TRUE;
	}
	// find uninitialized buffer
	DWORD BeginTime = timeGetTime();
	for (unsigned i = 0; i < unsigned((FileLength + 0xFFFF) >> 16); i++)
	{
		if (0 == (m_pWrittenMask[i / 8] & (1 << (i & 7))))
		{
			void * ptr;
			LONG count = GetDataBuffer( & ptr, 0x10000, LONGLONG(i) << 16, 0);
			if (count)
			{
				ReturnDataBuffer(ptr, 0x10000, ReturnBufferDiscard);
			}
		}
		if (timeout != 0
			&& timeGetTime() - BeginTime > timeout)
		{
			if (pPercentCompleted)
			{
				*pPercentCompleted = MulDiv(100, i, (FileLength + 0xFFFF) >> 16);
			}
			return FALSE;   // not finished yet
		}
	}
	Flush();
	if (pPercentCompleted)
	{
		*pPercentCompleted = 100;
	}
	return TRUE;
}

BOOL CDirectFile::File::SetFileLength(LONGLONG NewLength)
{
	if (m_Flags & FileFlagsMemoryFile)
	{
		// allocate or reallocate memory buffer
		if (NewLength > 0x100000
			|| NewLength < 0
			|| 0 != m_MemoryBufferRefCount)
		{
			// limit 1 M
			return FALSE;
		}
		if (long(NewLength) <= m_MemoryFileBufferSize)
		{
			if (NewLength > FileLength)
			{
				memset(m_pMemoryFileBuffer + FileLength, 0, NewLength - FileLength);
			}
			FileLength = NewLength;
			return TRUE;
		}
		char * NewBuf = new char[long(NewLength)];
		if (NULL == NewBuf)
		{
			return FALSE;
		}
		if (m_pMemoryFileBuffer)
		{
			memcpy(NewBuf, m_pMemoryFileBuffer, FileLength);
			delete m_pMemoryFileBuffer;
		}
		m_MemoryFileBufferSize = NewLength;
		m_pMemoryFileBuffer = NewBuf;
		memset(m_pMemoryFileBuffer + FileLength, 0, NewLength - FileLength);
		FileLength = NewLength;
		return TRUE;
	}
	if (m_Flags & FileFlagsReadOnly)
	{
		SetLastError(ERROR_FILE_READ_ONLY);
		return FALSE;
	}
	// if the file becomes shorter, discard all buffers after the trunk point
	if (NewLength < FileLength)
	{
		Flush();
		UseSourceFileLength = NewLength;
		// todo: clear written mask
	}
	else if (NewLength > FileLength)
	{
	}
	if (NewLength == RealFileLength)
	{
		return TRUE;
	}
	// TODO:
	CSimpleCriticalSectionLock lock(m_FileLock);
	FileLength = NewLength;
	// set the length to the multiple of 2K
	NewLength = (NewLength + 0x7FF) & ~0x7FF;
	LONG MoveHigh = LONG(NewLength>> 32);
	DWORD SizeLow, SizeHigh;
	SetLastError(0);
	m_FilePointer = -1i64;  // invalidate file pointer
	if ((0xFFFFFFFF != SetFilePointer(hFile, LONG(NewLength), & MoveHigh, FILE_BEGIN)
			|| ::GetLastError() == NO_ERROR)
		&& SetEndOfFile(hFile)
		&& FlushFileBuffers(hFile))
	{
		SizeLow = ::GetFileSize(hFile, & SizeHigh);
		if (::GetLastError() == NO_ERROR)
		{
			RealFileLength = SizeLow | (LONGLONG(SizeHigh) << 32);
			return TRUE;
		}
		else
		{
			SetLastError(ERROR_DISK_FULL);
			return FALSE;
		}
	}
	else
	{
		SizeLow = ::GetFileSize(hFile, & SizeHigh);
		if (::GetLastError() == NO_ERROR)
		{
			RealFileLength = SizeLow | (LONGLONG(SizeHigh) << 32);
		}
		SetLastError(ERROR_DISK_FULL);
		return FALSE;
	}

}

BOOL CDirectFile::File::Flush()
{
	// flush all the buffers
	m_FlushLength = 0;

	while(1)
	{
		CSimpleCriticalSectionLock lock(m_FileLock);
		BufferHeader * pBuf = BuffersList.Next();
		while (1)
		{
			if (BuffersList.Head() == pBuf)
			{
				return TRUE;
			}
			if (0 != pBuf->DirtyMask)
			{
				if (0 != pBuf->LockCount)
				{
					return FALSE;
				}
				pBuf->FlushDirtyBuffers();
				break;
			}
			pBuf = pBuf->KListEntry<BufferHeader>::Next();
		}
	}
}

void CDirectFile::File::FlushRequestedRange()
{
	// flush all the buffers
	while((LONG(m_FlushBegin) & 0xFFFF) + m_FlushLength >= 0x10000)
	{
		CSimpleCriticalSectionLock lock(m_FileLock);
		ULONG key = ULONG(m_FlushBegin >> 16);
		BufferHeader * pBuf = FindBuffer(key);

		if (NULL != pBuf)
		{
			if (0 != pBuf->LockCount)
			{
				break;
			}
			if (0 != pBuf->DirtyMask)
			{
				if (0) TRACE("Block %d flushed\n", key);
				pBuf->FlushDirtyBuffers(key);
			}
		}
		LONG Flushed = 0x10000 - (LONG(m_FlushBegin) & 0xFFFF);
		m_FlushBegin += Flushed;
		m_FlushLength -= Flushed;
	}
}

void * CDirectFile::File::AllocateCommonData(size_t size)
{
	if (0 == size)
	{
		return m_pCommonData;
	}
	if (size <= m_CommonDataSize)
	{
		memset((char*)m_pCommonData + size, 0, m_CommonDataSize - size);
		return m_pCommonData;
	}

	char * tmp = new char[size];
	if (NULL == tmp)
	{
		return NULL;
	}

	if (NULL != m_pCommonData)
	{
		memcpy(tmp, m_pCommonData, m_CommonDataSize);
		memset(tmp + m_CommonDataSize, 0, size - m_CommonDataSize);
		delete[] (char*) m_pCommonData;
	}
	else
	{
		memset(tmp, 0, size);
	}
	m_CommonDataSize = size;
	m_pCommonData = tmp;
	return tmp;
}

CDirectFile const & CDirectFile::operator=(CDirectFile & file)
{
	Close(0);
	m_FilePointer = 0;
	Attach( & file);
	return *this;
}
